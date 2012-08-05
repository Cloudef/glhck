#include "internal.h"
#include "helper/vertexdata.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

/* \brief update view matrix of object */
static void _glhckObjectUpdateMatrix(_glhckObject *object)
{
   kmVec3 min, max;
   kmVec3 mixxyz, mixyyz, mixyzz;
   kmVec3 maxxyz, maxyyz, maxyzz;
   kmMat4 translation, rotation, scaling, temp;
   CALL(2, "%p", object);

   /* translation */
   kmMat4Translation(&translation,
         object->view.translation.x + object->geometry.bias.x,
         object->view.translation.y + object->geometry.bias.y,
         object->view.translation.z + object->geometry.bias.z);

   /* rotation */
   kmMat4RotationX(&rotation, kmDegreesToRadians(object->view.rotation.x));
   kmMat4Multiply(&rotation, &rotation,
         kmMat4RotationY(&temp, kmDegreesToRadians(object->view.rotation.y)));
   kmMat4Multiply(&rotation, &rotation,
         kmMat4RotationZ(&temp, kmDegreesToRadians(object->view.rotation.z)));

   /* scaling */
   kmMat4Scaling(&scaling,
         object->view.scaling.x + object->geometry.scale.x,
         object->view.scaling.y + object->geometry.scale.y,
         object->view.scaling.z + object->geometry.scale.z);

   /* build matrix */
   kmMat4Multiply(&translation, &translation, &rotation);
   kmMat4Multiply(&object->view.matrix, &translation, &scaling);

   /* update transformed obb */
   kmVec3Transform(&object->view.obb.min, &object->view.bounding.min, &object->view.matrix);
   kmVec3Transform(&object->view.obb.max, &object->view.bounding.max, &object->view.matrix);

   /* update transformed aabb */
   maxxyz.x = object->view.bounding.max.x;
   maxxyz.y = object->view.bounding.min.y;
   maxxyz.z = object->view.bounding.min.z;

   maxyyz.x = object->view.bounding.min.x;
   maxyyz.y = object->view.bounding.max.y;
   maxyyz.z = object->view.bounding.min.z;

   maxyzz.x = object->view.bounding.min.x;
   maxyzz.y = object->view.bounding.min.y;
   maxyzz.z = object->view.bounding.max.z;

   mixxyz.x = object->view.bounding.min.x;
   mixxyz.y = object->view.bounding.max.y;
   mixxyz.z = object->view.bounding.max.z;

   mixyyz.x = object->view.bounding.max.x;
   mixyyz.y = object->view.bounding.min.y;
   mixyyz.z = object->view.bounding.max.z;

   mixyzz.x = object->view.bounding.max.x;
   mixyzz.y = object->view.bounding.max.y;
   mixyzz.z = object->view.bounding.min.z;

   /* transform edges */
   kmVec3Transform(&mixxyz, &mixxyz, &object->view.matrix);
   kmVec3Transform(&maxxyz, &maxxyz, &object->view.matrix);
   kmVec3Transform(&mixyyz, &mixyyz, &object->view.matrix);
   kmVec3Transform(&maxyyz, &maxyyz, &object->view.matrix);
   kmVec3Transform(&mixyzz, &mixyzz, &object->view.matrix);
   kmVec3Transform(&maxyzz, &maxyzz, &object->view.matrix);

   /* find max edges */
   set3d(max, object->view.obb.max);
   max3d(max, maxxyz);
   max3d(max, maxyyz);
   max3d(max, maxyzz);
   max3d(max, mixxyz);
   max3d(max, mixyyz);
   max3d(max, mixyzz);

   /* find min edges */
   set3d(min, object->view.obb.min);
   min3d(min, mixxyz);
   min3d(min, mixyyz);
   min3d(min, mixyzz);
   min3d(min, maxxyz);
   min3d(min, maxyyz);
   min3d(min, maxyzz);

   /* set edges */
   set3d(object->view.aabb.max, max);
   set3d(object->view.aabb.min, min);

   /* done */
   object->view.update = 0;
}

/* \brief convert vertex data to internal format */
static void _glhckConvertVertexData(_glhckObject *object, void *internal,
      const glhckImportVertexData *import, size_t memb)
{
   size_t i;
   char no_vconvert, no_nconvert, is3d;
   kmVec3 vmin, vmax,
          nmin, nmax;
   __GLHCKvertexData2d *internal2d;
   __GLHCKvertexData3d *internal3d;
   CALL(0, "%p, %p, %zu", internal, import, memb);

   is3d = object->geometry.flags & GEOMETRY_3D;
   if (is3d) internal3d = (__GLHCKvertexData3d*)internal;
   else      internal2d = (__GLHCKvertexData2d*)internal;

   if (is3d) {
      set3d(vmax, import[0].vertex);
      set3d(vmin, import[0].vertex);
      set3d(nmax, import[0].normal);
      set3d(nmin, import[0].normal);
   } else {
      set2d(vmax, import[0].vertex);
      set2d(vmin, import[0].vertex);
      set2d(nmax, import[0].normal);
      set2d(nmin, import[0].normal);
      vmax.z = 0;
      vmin.z = 0;
      nmax.z = 0;
      nmin.z = 0;
   }

   /* find max && min first */
   for (i = 1; i != memb; ++i) {
      if (is3d) {
         max3d(vmax, import[i].vertex);
         min3d(vmin, import[i].vertex);
         max3d(nmax, import[i].normal);
         min3d(nmin, import[i].normal);
      } else {
         max2d(vmax, import[i].vertex);
         min2d(vmin, import[i].vertex);
         max2d(nmax, import[i].normal);
         min2d(nmin, import[i].normal);
      }
   }

   /* do we need conversion? */
   no_vconvert = 0;
   if ((vmax.x + vmin.x == 1 &&
        vmax.y + vmin.y == 1 &&
        vmax.z + vmin.z == 1) ||
       (vmax.x + vmin.x == 0 &&
        vmax.y + vmin.y == 0 &&
        vmax.z + vmin.z == 0))
      no_vconvert = 1;

   no_nconvert = 0;
   if ((nmax.x + nmin.x == 1 &&
        nmax.y + nmin.y == 1 &&
        nmax.z + nmin.z == 1) ||
       (nmax.x + nmin.x == 0 &&
        nmax.y + nmin.y == 0 &&
        nmax.z + nmin.z == 0))
      no_nconvert = 1;

   /* lie about bounds by 1 point so,
    * we don't get artifacts */
   vmax.x++; vmax.y++; vmax.z++;
   vmin.x--; vmin.y--; vmin.z--;

   /* do conversion */
   for (i = 0; i != memb; ++i) {
      /* vertex && normal conversion */
#if GLHCK_PRECISION_VERTEX == GLHCK_FLOAT
      if (is3d) {
         memcpy(&internal3d[i].vertex, &import[i].vertex,
               sizeof(_glhckVertex3d));
         memcpy(&internal3d[i].normal, &import[i].normal,
               sizeof(_glhckVertex3d));
      } else {
         memcpy(&internal2d[i].vertex, &import[i].vertex,
               sizeof(_glhckVertex2d));
         memcpy(&internal2d[i].normal, &import[i].normal,
               sizeof(_glhckVertex3d));
      }
#else
      if (no_vconvert) {
         if (is3d) {
            set3d(internal3d[i].vertex, import[i].vertex);
         } else {
            set2d(internal2d[i].vertex, import[i].vertex);
         }
      } else {
         if (is3d) {
            convert3d(internal3d[i].vertex, import[i].vertex,
                  vmax, vmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
         } else {
            convert2d(internal2d[i].vertex, import[i].vertex,
                  vmax, vmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
         }
      }
      if (no_nconvert) {
         if (is3d) {
            set3d(internal3d[i].normal, import[i].normal);
         } else {
            set2d(internal2d[i].normal, import[i].normal);
         }
      } else {
         if (is3d) {
            convert3d(internal3d[i].normal, import[i].normal,
                  nmax, nmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
         } else {
            convert2d(internal2d[i].normal, import[i].normal,
                  nmax, nmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
         }
      }
#endif

      /* texture coord conversion */
#if GLHCK_PRECISION_COORD == GLHCK_FLOAT
      if (is3d) {
         memcpy(&internal3d[i].coord, &import[i].coord,
               sizeof(_glhckCoord2d));
      } else {
         memcpy(&internal2d[i].coord, &import[i].coord,
               sizeof(_glhckCoord2d));
      }
#else
      if (is3d) {
         internal3d[i].coord.x = import[i].coord.x * GLHCK_RANGE_COORD;
         internal3d[i].coord.y = import[i].coord.y * GLHCK_RANGE_COORD;
      } else {
         internal2d[i].coord.x = import[i].coord.x * GLHCK_RANGE_COORD;
         internal2d[i].coord.y = import[i].coord.y * GLHCK_RANGE_COORD;
      }
#endif

      /* color is always unsigned char, memcpy it */
#if GLHCK_VERTEXDATA_COLOR
      if (is3d) {
         memcpy(&internal3d[i].color, import[i].color,
               sizeof(glhckColor));
      } else {
         memcpy(&internal2d[i].color, import[i].color,
               sizeof(glhckColor));
      }
#endif
   }

   /* fix geometry bias && scale after conversion */
#if GLHCK_PRECISION_VERTEX != GLHCK_FLOAT
   if (no_vconvert) return;
   object->geometry.bias.x = (GLHCK_BIAS_OFFSET) *
      (vmax.x - vmin.x) + vmin.x;
   object->geometry.bias.y = (GLHCK_BIAS_OFFSET) *
      (vmax.y - vmin.y) + vmin.y;

   if (is3d) {
      object->geometry.bias.z = (GLHCK_BIAS_OFFSET) *
         (vmax.z - vmin.z) + vmin.z;
   } else {
      object->geometry.bias.z = 1;
   }

   object->geometry.scale.x =
      (vmax.x - vmin.x) / GLHCK_SCALE_OFFSET;
   object->geometry.scale.y =
      (vmax.y - vmin.y) / GLHCK_SCALE_OFFSET;

   if (is3d) {
      object->geometry.scale.z =
         (vmax.z - vmin.z) / GLHCK_SCALE_OFFSET;
   } else {
      object->geometry.scale.z = 1;
   }

   /* update matrix */
   _glhckObjectUpdateMatrix(object);
#endif
}

/* \brief convert index data to internal format */
static void _glhckConvertIndexData(GLHCK_CAST_INDEX *internal,
      const unsigned int *import, size_t memb)
{
   size_t i;
   CALL(0, "%p, %p, %zu", internal, import, memb);

   /* TODO: Handle unsigned short indices or just use unsigned int for sake of simplicity? */
   for (i = 0; i != memb; ++i)
      internal[i] = import[i];
}

/* \brief calculate object's bounding box */
static void _glhckObjectCalculateAABB(_glhckObject *object)
{
   size_t v;
   char is3d;
   kmVec3 min, max;
   kmAABB aabb_box;
   __GLHCKvertexData2d *internal2d;
   __GLHCKvertexData3d *internal3d;
   CALL(2, "%p", object);

   is3d = object->geometry.flags & GEOMETRY_3D;
   if (is3d) {
      internal3d = (__GLHCKvertexData3d*)object->geometry.vertexData;
      set3d(max, internal3d[0].vertex);
      set3d(min, internal3d[0].vertex);
   } else {
      internal2d = (__GLHCKvertexData2d*)object->geometry.vertexData;
      set2d(max, internal2d[0].vertex);
      set2d(min, internal2d[0].vertex);
      max.z = 0;
      min.z = 0;
   }

   /* find min and max corners */
   for(v = 1; v != object->geometry.vertexCount; ++v) {
      if (is3d) {
         max3d(max, internal3d[v].vertex);
         min3d(min, internal3d[v].vertex);
      } else {
         max2d(max, internal2d[v].vertex);
         min2d(min, internal2d[v].vertex);
      }
   }

   /* assign aabb to object */
   aabb_box.min = min;
   aabb_box.max = max;
   object->view.bounding = aabb_box;
}

/* set object's filename */
void _glhckObjectSetFile(_glhckObject *object, const char *file)
{
   CALL(1, "%p, %s", object, file);
   assert(object);
   IFDO(_glhckFree, object->file);
   if (file) object->file = _glhckStrdup(file);
}

/* set object's data */
void _glhckObjectSetData(_glhckObject *object, const char *data)
{
   CALL(1, "%p, %s", object, data);
   assert(object);
   IFDO(_glhckFree, object->data);
   if (data) object->data = _glhckStrdup(data);
}

/* public api */

/* \brief new object */
GLHCKAPI glhckObject *glhckObjectNew(void)
{
   _glhckObject *object;
   TRACE(0);

   if (!(object = _glhckMalloc(sizeof(_glhckObject))))
      goto fail;

   /* init object */
   memset(object, 0, sizeof(_glhckObject));
   memset(&object->geometry, 0, sizeof(__GLHCKobjectGeometry));
   memset(&object->view, 0, sizeof(__GLHCKobjectView));
   memset(&object->material, 0, sizeof(__GLHCKobjectMaterial));

   /* default geometry type is tristrip */
   object->geometry.type = GLHCK_TRIANGLE_STRIP;

   /* default view matrix */
   glhckObjectScalef(object, 1.0f, 1.0f, 1.0f);
   _glhckObjectUpdateMatrix(object);

   /* default material flags */
   glhckObjectSetMaterialFlags(object, GLHCK_MATERIAL_DEPTH |
                                       GLHCK_MATERIAL_CULL);

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(olist, object, _glhckObject*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief copy object */
GLHCKAPI glhckObject* glhckObjectCopy(const glhckObject *src)
{
   _glhckObject *object;
   CALL(0, "%p", src);
   assert(src);

   if (!(object = _glhckMalloc(sizeof(_glhckObject))))
      goto fail;

   /* copy static data */
   memset(object, 0, sizeof(_glhckObject));
   memcpy(&object->geometry, &src->geometry, sizeof(__GLHCKobjectGeometry));
   memcpy(&object->view, &src->view, sizeof(__GLHCKobjectView));
   memcpy(&object->material, &src->material, sizeof(__GLHCKobjectMaterial));

   /* copy metadata */
   if (src->file)
      object->file = _glhckStrdup(src->file);
   if (src->data)
      object->data = _glhckStrdup(src->data);

   /* copy vertex data */
   if (object->geometry.flags & GEOMETRY_3D) {
      if (!(object->geometry.vertexData =
               _glhckCopy(src->geometry.vertexData,
                  src->geometry.vertexCount * sizeof(__GLHCKvertexData3d))))
         goto fail;
   } else {
      if (!(object->geometry.vertexData =
               _glhckCopy(src->geometry.vertexData,
                  src->geometry.vertexCount * sizeof(__GLHCKvertexData2d))))
         goto fail;
   }

   /* copy index data */
   if (!(object->geometry.indices =
            _glhckCopy(src->geometry.indices,
               src->geometry.indicesCount * sizeof(GLHCK_CAST_INDEX))))
      goto fail;

   /* copy texture */
   glhckObjectSetTexture(object, src->material.texture);

   /* set ref counter to 1 */
   object->refCounter = 1;

   /* insert to world */
   _glhckWorldInsert(olist, object, _glhckObject*);

   RET(0, "%p", object);
   return object;

fail:
   if (object) {
      IFDO(_glhckFree, object->file);
      IFDO(_glhckFree, object->data);
      IFDO(_glhckFree, object->geometry.vertexData);
      IFDO(_glhckFree, object->geometry.indices);
   }
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference object */
GLHCKAPI glhckObject* glhckObjectRef(glhckObject *object)
{
   CALL(3, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free object */
GLHCKAPI short glhckObjectFree(glhckObject *object)
{
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free metadata */
   IFDO(_glhckFree, object->file);
   IFDO(_glhckFree, object->data);

   /* free geometry */
   IFDO(_glhckFree, object->geometry.vertexData);
   IFDO(_glhckFree, object->geometry.indices);
   IFDO(_glhckFree, object->geometry.transformedCoordinates);

   /* free material */
   glhckObjectSetTexture(object, NULL);

   /* remove from world */
   _glhckWorldRemove(olist, object, _glhckObject*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set object's texture */
GLHCKAPI void glhckObjectSetTexture(glhckObject *object, glhckTexture *texture)
{
   CALL(1, "%p, %p", object, texture);
   assert(object);

   IFDO(glhckTextureFree, object->material.texture);
   if (texture) object->material.texture = glhckTextureRef(texture);
}

/* \brief get object's texture */
GLHCKAPI glhckTexture* glhckObjectGetTexture(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%p", object->material.texture);
   return object->material.texture;
}

/* \brief add object to draw queue */
GLHCKAPI void glhckObjectDraw(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   if (_GLHCKlibrary.render.draw.oqueue[GLHCK_MAX_DRAW-1]) {
      DEBUG(GLHCK_DBG_WARNING, "Maximum draw queue limit reached!");
      return;
   }

   /* insert object to drawing queue */
   _glhckQueueInsert(_GLHCKlibrary.render.draw.oqueue, object);
   glhckObjectRef(object); /* free after render */

   /* insert texture to drawing queue */
   if (object->material.texture) {
      _glhckQueueInsert(_GLHCKlibrary.render.draw.tqueue,
            object->material.texture);
      glhckTextureRef(object->material.texture); /* free after render */
   }
}

/* \brief render object */
GLHCKAPI void glhckObjectRender(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* does view matrix need update? */
   if (object->view.update)
      _glhckObjectUpdateMatrix(object);

   /* render */
   if (object->geometry.drawFunc)
      object->geometry.drawFunc(object);
}

/* \brief set's object material flags */
GLHCKAPI void glhckObjectSetMaterialFlags(_glhckObject *object, unsigned int flags)
{
   CALL(1, "%p, %u", object, flags);
   assert(object);

   object->material.flags = flags;
}

/* \brief get obb bounding box of the object */
GLHCKAPI const kmAABB*  glhckObjectGetOBB(const _glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%p", object->view.obb);
   return &object->view.obb;
}

/* \brief get aabb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetAABB(const _glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%p", object->view.aabb);
   return &object->view.aabb;
}

/* \brief get object position */
GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, VEC3S, VEC3(&object->view.translation));
   return &object->view.translation;
}

/* \brief position object */
GLHCKAPI void glhckObjectPosition(glhckObject *object, const kmVec3 *position)
{
   CALL(2, "%p, "VEC3S, object, VEC3(position));
   assert(object && position);

   if (object->view.translation.x == position->x &&
       object->view.translation.y == position->y &&
       object->view.translation.z == position->z)
      return;

   kmVec3Assign(&object->view.translation, position);
   object->view.update = 1;
}

/* \brief position object (with kmScalar) */
GLHCKAPI void glhckObjectPositionf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 position = { x, y, z };
   glhckObjectPosition(object, &position);
}

/* \brief move object */
GLHCKAPI void glhckObjectMove(glhckObject *object, const kmVec3 *move)
{
   CALL(2, "%p, "VEC3S, object, VEC3(move));
   assert(object && move);

   kmVec3Add(&object->view.translation,
         &object->view.translation, move);
   object->view.update = 1;
}

/* \brief move object (with kmScalar) */
GLHCKAPI void glhckObjectMovef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 move = { x, y, z };
   glhckObjectMove(object, &move);
}

/* \brief get object rotation */
GLHCKAPI const kmVec3* glhckObjectGetRotation(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, VEC3S, VEC3(&object->view.rotation));
   return &object->view.rotation;
}

/* \brief set object rotation */
GLHCKAPI void glhckObjectRotation(glhckObject *object, const kmVec3 *rotation)
{
   CALL(2, "%p, "VEC3S, object, VEC3(rotation));
   assert(object && rotation);

   if (object->view.rotation.x == rotation->x &&
       object->view.rotation.y == rotation->y &&
       object->view.rotation.z == rotation->z)
      return;

   kmVec3Assign(&object->view.rotation, rotation);
   object->view.update = 1;
}

/* \brief set object rotation (with kmScalar) */
GLHCKAPI void glhckObjectRotationf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotation = { x, y, z };
   glhckObjectRotation(object, &rotation);
}

/* \brief rotate object */
GLHCKAPI void glhckObjectRotate(glhckObject *object, const kmVec3 *rotate)
{
   CALL(2, "%p, "VEC3S, object, VEC3(rotate));
   assert(object && rotate);

   kmVec3Add(&object->view.rotation, &object->view.rotation, rotate);
   object->view.update = 1;
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckObjectRotatef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotate = { x, y, z };
   glhckObjectRotate(object, &rotate);
}

/* \brief get object scale */
GLHCKAPI const kmVec3* glhckObjectGetScale(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, VEC3S, VEC3(&object->view.scaling));
   return &object->view.scaling;
}

/* \brief scale object */
GLHCKAPI void glhckObjectScale(glhckObject *object, const kmVec3 *scale)
{
   CALL(2, "%p, "VEC3S, object, VEC3(scale));
   assert(object && scale);

   if (object->view.scaling.x == scale->x &&
       object->view.scaling.y == scale->y &&
       object->view.scaling.z == scale->z)
      return;

   kmVec3Assign(&object->view.scaling, scale);
   object->view.update = 1;
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckObjectScalef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 scaling = { x, y, z };
   glhckObjectScale(object, &scaling);
}

/* \brief transform coordinates with vec4 (off x, off y, width, height) and rotation */
GLHCKAPI void glhckObjectTransformCoordinates(
      _glhckObject *object, const kmVec4 *transformed, short degrees)
{
   size_t i;
   char is3d;
   kmVec2 out;
   _glhckCoord2d *coord;
   __GLHCKvertexData2d *v2d;
   __GLHCKvertexData3d *v3d;
   __GLHCKcoordTransform *newCoords, *oldCoords;
   kmVec2 center = { 0.5f, 0.5f };
   CALL(2, "%p, "VEC4S", %d", object, VEC4(transformed), degrees);

   if (transformed->z == 0.f || transformed->w == 0.f)
      return;

   if (!(newCoords = _glhckMalloc(sizeof(__GLHCKcoordTransform))))
      return;

   /* old coordinates */
   oldCoords = object->geometry.transformedCoordinates;

   is3d = object->geometry.flags & GEOMETRY_3D;
   if (is3d) v3d = (__GLHCKvertexData3d*)object->geometry.vertexData;
   else v2d = (__GLHCKvertexData2d*)object->geometry.vertexData;

   /* out */
   for (i = 0; i != object->geometry.vertexCount; ++i) {
      if (is3d) coord = &v3d[i].coord;
      else coord = &v2d[i].coord;

      out.x = (kmScalar)coord->x/GLHCK_RANGE_COORD;
      out.y = (kmScalar)coord->y/GLHCK_RANGE_COORD;

      if (oldCoords) {
         if (oldCoords->degrees != 0)
            kmVec2RotateBy(&out, &out, -oldCoords->degrees, &center);

         out.x -= oldCoords->transform.x;
         out.x /= oldCoords->transform.z;
         out.y -= oldCoords->transform.y;
         out.y /= oldCoords->transform.w;
      }

      if (degrees != 0)
         kmVec2RotateBy(&out, &out, degrees, &center);

      out.x *= transformed->z;
      out.x += transformed->x;
      out.y *= transformed->w;
      out.y += transformed->y;

      coord->x = out.x * GLHCK_RANGE_COORD;
      coord->y = out.y * GLHCK_RANGE_COORD;
   }

   newCoords->degrees   = degrees;
   newCoords->transform = *transformed;
   IFDO(_glhckFree, object->geometry.transformedCoordinates);
   object->geometry.transformedCoordinates = newCoords;
}

/* \brief insert vertex data into object */
GLHCKAPI int glhckObjectInsertVertexData3d(
      _glhckObject *object,
      size_t memb,
      const glhckImportVertexData *vertexData)
{
   __GLHCKvertexData3d *new;
   CALL(0, "%p, %zu, %p", object, memb, vertexData);
   assert(object);

   /* allocate new buffer */
   if (!(new = (__GLHCKvertexData3d*)_glhckMalloc(memb *
               sizeof(__GLHCKvertexData3d))))
      goto fail;

   /* free old buffer */
   IFDO(_glhckFree, object->geometry.vertexData);
   object->geometry.flags |= GEOMETRY_3D; /* new vertexdata is 3d data */

   /* assign new buffer */
   _glhckConvertVertexData(object, new, vertexData, memb);
   object->geometry.vertexData = new;
   object->geometry.vertexCount = memb;

   /* calculate AABB from vertices */
   _glhckObjectCalculateAABB(object);

   /* insert correct draw function */
   object->geometry.drawFunc = _GLHCKlibrary.render.api.objectDraw3d;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert 2d vertex data into object */
GLHCKAPI int glhckObjectInsertVertexData2d(
      _glhckObject *object,
      size_t memb,
      const glhckImportVertexData *vertexData)
{
   __GLHCKvertexData2d *new;
   CALL(0, "%p, %zu, %p", object, memb, vertexData);
   assert(object);

   /* allocate new buffer */
   if (!(new = (__GLHCKvertexData2d*)_glhckMalloc(memb *
               sizeof(__GLHCKvertexData2d))))
      goto fail;

   /* free old buffer */
   IFDO(_glhckFree, object->geometry.vertexData);
   object->geometry.flags &= ~GEOMETRY_3D; /* new vertexdata is 2d data */

   /* assign new buffer */
   _glhckConvertVertexData(object, new, vertexData, memb);
   object->geometry.vertexData = new;
   object->geometry.vertexCount = memb;

   /* calculate AABB from vertices */
   _glhckObjectCalculateAABB(object);

   /* insert correct draw function */
   object->geometry.drawFunc = _GLHCKlibrary.render.api.objectDraw2d;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert index data into object */
GLHCKAPI int glhckObjectInsertIndices(
      _glhckObject *object,
      size_t memb,
      const unsigned int *indices)
{
   GLHCK_CAST_INDEX *new;
   CALL(0, "%p, %zu, %p", object, memb, indices);
   assert(object);

   /* allocate new buffer */
   if (!(new = (GLHCK_CAST_INDEX*)_glhckMalloc(memb *
               sizeof(GLHCK_CAST_INDEX))))
      goto fail;

   /* free old buffer */
   IFDO(_glhckFree, object->geometry.indices);

   /* assign new buffer */
#if GLHCK_NATIVE_IMPORT_INDEXDATA
   memcpy(new, indices, memb *
         sizeof(GLHCK_CAST_INDEX));
#else
   _glhckConvertIndexData(new, indices, memb);
#endif
   object->geometry.indices = new;
   object->geometry.indicesCount = memb;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
