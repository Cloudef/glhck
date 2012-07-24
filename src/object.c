#include "internal.h"
#include "helper/vertexdata.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

/* \brief convert vertex data to internal format */
static void _glhckConvertVertexData(_glhckObject *object, __GLHCKvertexData *internal,
      const glhckImportVertexData *import, size_t memb)
{
   size_t i;
   char no_vconvert, no_nconvert;
   kmVec3 vmin, vmax,
          nmin, nmax;
   CALL(0, "%p, %p, %zu", internal, import, memb);

   set3d(vmax, import[0].vertex);
   set3d(vmin, import[0].vertex);
   set3d(nmax, import[0].normal);
   set3d(nmin, import[0].normal);

   /* find max && min first */
   for (i = 1; i != memb; ++i) {
      max3d(vmax, import[i].vertex);
      min3d(vmin, import[i].vertex);
      max3d(nmax, import[i].normal);
      min3d(nmin, import[i].normal);
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
      memcpy(&internal[i].vertex, &import[i].vertex,
            sizeof(_glhckVertex3d));
      memcpy(&internal[i].normal, &import[i].normal,
            sizeof(_glhckVertex3d));
#else
      if (no_vconvert) {
         set3d(internal[i].vertex, import[i].vertex)
      } else {
         convert3d(internal[i].vertex, import[i].vertex,
               vmax, vmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
      }
      if (no_nconvert) {
         set3d(internal[i].normal, import[i].normal)
      } else {
         convert3d(internal[i].normal, import[i].normal,
               nmax, nmin, GLHCK_VERTEX_MAGIC, GLHCK_CAST_VERTEX);
      }
#endif

      /* texture coord conversion */
#if GLHCK_PRECISION_COORD == GLHCK_FLOAT
      memcpy(&internal[i].coord, &import[i].coord,
            sizeof(_glhckCoord2d));
#else
      internal[i].coord.x = import[i].coord.x * GLHCK_RANGE_COORD;
      internal[i].coord.y = import[i].coord.y * GLHCK_RANGE_COORD;
#endif

      /* color is always unsigned char, memcpy it */
#if GLHCK_VERTEXDATA_COLOR
      memcpy(&internal[i].color, import[i].color,
            sizeof(glhckColor));
#endif
   }

   /* fix geometry bias && scale after conversion */
#if GLHCK_PRECISION_VERTEX != GLHCK_FLOAT
   if (no_vconvert) return;
   object->geometry.bias.x = (GLHCK_BIAS_OFFSET) *
      (vmax.x - vmin.x) + vmin.x;
   object->geometry.bias.y = (GLHCK_BIAS_OFFSET) *
      (vmax.y - vmin.y) + vmin.y;
   object->geometry.bias.z = (GLHCK_BIAS_OFFSET) *
      (vmax.z - vmin.z) + vmin.z;

   object->geometry.scale.x =
      (vmax.x - vmin.x) / GLHCK_SCALE_OFFSET;
   object->geometry.scale.y =
      (vmax.y - vmin.y) / GLHCK_SCALE_OFFSET;
   object->geometry.scale.z =
      (vmax.z - vmin.z) / GLHCK_SCALE_OFFSET;
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
   kmVec3 min, max;
   kmAABB aabb_box;
   __GLHCKobjectGeometry *g;
   CALL(2, "%p", object);

   g = &object->geometry;
   set3d(max, g->vertexData[0].vertex);
   set3d(min, g->vertexData[0].vertex);

   /* find min and max corners */
   for(v = 1; v != g->vertexCount; ++v) {
      max3d(max, g->vertexData[v].vertex);
      min3d(min, g->vertexData[v].vertex);
   }

   /* assign aabb to object */
   aabb_box.min = min;
   aabb_box.max = max;
   object->view.bounding = aabb_box;
}

/* \brief update view matrix of object */
static void _glhckObjectUpdateMatrix(_glhckObject *object)
{
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

   /* done */
   object->view.update = 0;
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
GLHCKAPI glhckObject *glhckObjectCopy(glhckObject *src)
{
   _glhckObject *object;
   CALL(0, "%p", src);
   assert(src);

   if (!(object = _glhckMalloc(sizeof(_glhckObject))))
      goto fail;

   /* copy static data */
   memcpy(object, src, sizeof(_glhckObject));
   memcpy(&object->geometry, &src->geometry, sizeof(__GLHCKobjectGeometry));
   memcpy(&object->view, &src->view, sizeof(__GLHCKobjectView));

   /* copy metadata */
   if (src->file)
      object->file = _glhckStrdup(src->file);
   if (src->data)
      object->data = _glhckStrdup(src->data);

   /* copy vertex data */
   if (!(object->geometry.vertexData =
            _glhckCopy(src->geometry.vertexData,
               src->geometry.vertexCount * sizeof(__GLHCKvertexData))))
      goto fail;

   /* copy index data */
   if (!(object->geometry.indices =
            _glhckCopy(src->geometry.vertexData,
               src->geometry.indicesCount * sizeof(GLHCK_CAST_INDEX))))
      goto fail;

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
   CALL(0, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(0, "%p", object);
   return object;
}

/* \brief free object */
GLHCKAPI short glhckObjectFree(glhckObject *object)
{
   CALL(0, "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free metadata */
   IFDO(_glhckFree, object->file);
   IFDO(_glhckFree, object->data);

   /* free geometry */
   IFDO(_glhckFree, object->geometry.vertexData);
   IFDO(_glhckFree, object->geometry.indices);

   /* free material */
   glhckObjectSetTexture(object, NULL);

   /* remove from world */
   _glhckWorldRemove(olist, object, _glhckObject*);

   /* free */
   _glhckFree(object);
   object = NULL;

success:
   RET(0, "%d", object?object->refCounter:0);
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

/* \brief add object to draw queue */
GLHCKAPI void glhckObjectDraw(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* does view matrix need update? */
   if (object->view.update)
      _glhckObjectUpdateMatrix(object);

   /* insert object to drawing queue */
   _glhckQueueInsert(_GLHCKlibrary.render.draw.oqueue, object);

   /* insert texture to drawing queue */
   if (object->material.texture) {
      _glhckQueueInsert(_GLHCKlibrary.render.draw.tqueue,
            object->material.texture);
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
   _GLHCKlibrary.render.api.objectDraw(object);
}

/* \brief set's object material flags */
GLHCKAPI void glhckObjectSetMaterialFlags(_glhckObject *object, unsigned int flags)
{
   object->material.flags = flags;
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

/* \brief rotate object */
GLHCKAPI void glhckObjectRotate(glhckObject *object, const kmVec3 *rotate)
{
   CALL(2, "%p, "VEC3S, object, VEC3(rotate));
   assert(object && rotate);

   if (object->view.rotation.x == rotate->x &&
       object->view.rotation.y == rotate->y &&
       object->view.rotation.z == rotate->z)
      return;

   kmVec3Assign(&object->view.rotation, rotate);
   object->view.update = 1;
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckObjectRotatef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotation = { x, y, z };
   glhckObjectRotate(object, &rotation);
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

/* \brief insert vertex data into object */
GLHCKAPI int glhckObjectInsertVertexData(
      _glhckObject *object,
      size_t memb,
      const glhckImportVertexData *vertexData)
{
   __GLHCKvertexData *new;
   CALL(0, "%p, %zu, %p", object, memb, vertexData);
   assert(object);

   /* allocate new buffer */
   if (!(new = (__GLHCKvertexData*)_glhckMalloc(memb *
               sizeof(__GLHCKvertexData))))
      goto fail;

   /* free old buffer */
   IFDO(_glhckFree, object->geometry.vertexData);

   /* assign new buffer */
   _glhckConvertVertexData(object, new, vertexData, memb);
   object->geometry.vertexData = new;
   object->geometry.vertexCount = memb;

   /* calculate AABB from vertices */
   _glhckObjectCalculateAABB(object);

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
