#include "internal.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

#define PERFORM_ON_CHILDS(parent, function, ...)         \
   { unsigned int _cbc_;                                 \
   for (_cbc_ = 0; _cbc_ != parent->numChilds; ++_cbc_)  \
      function(parent->childs[_cbc_], ##__VA_ARGS__); }

/* \brief assign object to draw list */
inline void _glhckObjectInsertToQueue(_glhckObject *object)
{
   __GLHCKobjectQueue *objects;
   unsigned int i;

   objects = &_GLHCKlibrary.render.draw.objects;

   /* check duplicate */
   for (i = 0; i != objects->count; ++i)
      if (objects->queue[i] == object) return;

   /* need alloc dynamically more? */
   if (objects->allocated <= objects->count+1) {
      objects->queue = _glhckRealloc(objects->queue,
            objects->allocated,
            objects->allocated + GLHCK_QUEUE_ALLOC_STEP,
            sizeof(_glhckObject*));

      /* epic fail here */
      if (!objects->queue) return;
      objects->allocated += GLHCK_QUEUE_ALLOC_STEP;
   }

   /* assign the object to list */
   objects->queue[objects->count] = glhckObjectRef(object);
   objects->count++;
}

/* update target from rotation */
static inline void _glhckObjectUpdateTargetFromRotation(_glhckObject *object)
{
   kmVec3 rotToDir;
   const kmVec3 forwards = { 0, 0, 1 };
   CALL(2, "%p", object);
   assert(object);

   /* update target */
   kmVec3RotationToDirection(&rotToDir, &object->view.rotation, &forwards);
   kmVec3Add(&object->view.target, &object->view.translation, &rotToDir);
}

/* update rotation from target */
static inline void _glhckObjectUpdateRotationFromTarget(_glhckObject *object)
{
   kmVec3 toTarget;
   CALL(2, "%p", object);
   assert(object);

   /* update rotation */
   kmVec3Subtract(&toTarget, &object->view.target, &object->view.translation);
   kmVec3GetHorizontalAngle(&object->view.rotation, &toTarget);
}

/* stub draw function */
static inline void _glhckObjectStubDraw(const struct _glhckObject *object)
{
   if (object->geometry && object->geometry->vertexType != GLHCK_VERTEX_NONE) {
      DEBUG(GLHCK_DBG_WARNING, "Stub draw function called for object which actually has vertexdata!");
      DEBUG(GLHCK_DBG_WARNING, "Did you forget to call glhckObjectUpdate() after inserting vertex data?");
   }
}

/* \brief build translation matrix for object */
static void _glhckObjectBuildTranslation(_glhckObject *object, kmMat4 *translation)
{
   kmVec3 bias;
   bias.x = bias.y = bias.z = 0.0f;

   if (object->geometry) {
      bias.x = object->geometry->bias.x;
      bias.y = object->geometry->bias.y;
      bias.z = object->geometry->bias.z;
   }

   /* translation */
   kmMat4Translation(translation,
         object->view.translation.x + (bias.x * object->view.scaling.x),
         object->view.translation.y + (bias.y * object->view.scaling.y),
         object->view.translation.z + (bias.z * object->view.scaling.z));
}

/* \brief build rotation matrix for object */
static void _glhckObjectBuildRotation(_glhckObject *object, kmMat4 *rotation)
{
   kmMat4 tmp;

   /* rotation */
   kmMat4RotationAxisAngle(rotation, &(kmVec3){0,0,1},
         kmDegreesToRadians(object->view.rotation.z));
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){0,1,0},
         kmDegreesToRadians(object->view.rotation.y));
   kmMat4Multiply(rotation, rotation, &tmp);
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){1,0,0},
         kmDegreesToRadians(object->view.rotation.x));
   kmMat4Multiply(rotation, rotation, &tmp);
}

/* \brief build scaling matrix for object */
static void _glhckObjectBuildScaling(_glhckObject *object, kmMat4 *scaling)
{
   kmVec3 scale;
   scale.x = scale.y = scale.z = 1.0f;

   if (object->geometry) {
      scale.x = object->geometry->scale.x;
      scale.y = object->geometry->scale.y;
      scale.z = object->geometry->scale.z;
   }

   /* scaling */
   kmMat4Scaling(scaling,
         scale.x * object->view.scaling.x,
         scale.y * object->view.scaling.y,
         scale.z * object->view.scaling.z);
}

/* \brief update view matrix of object */
void _glhckObjectUpdateMatrix(_glhckObject *object)
{
   unsigned int i;
   kmVec3 min, max;
   kmVec3 mixxyz, mixyyz, mixyzz;
   kmVec3 maxxyz, maxyyz, maxyzz;
   kmMat4 translation, rotation, scaling;
   CALL(2, "%p", object);

   /* build affection matrices */
   _glhckObjectBuildTranslation(object, &translation);
   _glhckObjectBuildRotation(object, &rotation);
   _glhckObjectBuildScaling(object, &scaling);

   /* build view matrix */
   kmMat4Multiply(&translation, &translation, &rotation);
   kmMat4Multiply(&object->view.matrix, &translation, &scaling);

   /* parent matrix affects us! */
   if (object->parent) {
      if (object->affectionFlags & GLHCK_AFFECT_SCALING) {
         _glhckObjectBuildScaling(object->parent, &scaling);
         kmMat4Multiply(&object->view.matrix, &scaling, &object->view.matrix);
      }

      if (object->affectionFlags & GLHCK_AFFECT_ROTATION) {
         _glhckObjectBuildRotation(object->parent, &rotation);
         kmMat4Multiply(&object->view.matrix, &rotation, &object->view.matrix);
      }

      if (object->affectionFlags & GLHCK_AFFECT_TRANSLATION) {
         _glhckObjectBuildTranslation(object->parent, &translation);
         kmMat4Multiply(&object->view.matrix, &translation, &object->view.matrix);
      }
   }

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
   glhckSetV3(&max, &object->view.obb.max);
   glhckMaxV3(&max, &maxxyz);
   glhckMaxV3(&max, &maxyyz);
   glhckMaxV3(&max, &maxyzz);
   glhckMaxV3(&max, &mixxyz);
   glhckMaxV3(&max, &mixyyz);
   glhckMaxV3(&max, &mixyzz);

   /* find min edges */
   glhckSetV3(&min, &object->view.obb.min);
   glhckMinV3(&min, &mixxyz);
   glhckMinV3(&min, &mixyyz);
   glhckMinV3(&min, &mixyzz);
   glhckMinV3(&min, &maxxyz);
   glhckMinV3(&min, &maxyyz);
   glhckMinV3(&min, &maxyzz);

   /* set edges */
   glhckSetV3(&object->view.aabb.max, &max);
   glhckSetV3(&object->view.aabb.min, &min);

   /* update childs on next draw */
   for (i = 0; i != object->numChilds; ++i)
      object->childs[i]->view.update = 1;

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

/***
 * public api
 ***/

/* \brief new object */
GLHCKAPI glhckObject *glhckObjectNew(void)
{
   _glhckObject *object;
   TRACE(0);

   if (!(object = _glhckCalloc(1, sizeof(_glhckObject))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* default view matrix */
   glhckObjectScalef(object, 1.0f, 1.0f, 1.0f);
   _glhckObjectUpdateMatrix(object);

   /* default material flags */
   glhckObjectMaterialFlags(object, GLHCK_MATERIAL_DEPTH |
                                    GLHCK_MATERIAL_CULL);

   /* default affection flags */
   glhckObjectParentAffection(object, GLHCK_AFFECT_TRANSLATION |
                                      GLHCK_AFFECT_ROTATION);

   /* default color */
   glhckObjectColorb(object, 255, 255, 255, 255);

   /* set stub draw function */
   object->drawFunc = _glhckObjectStubDraw;

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

   if (!(object = _glhckCalloc(1, sizeof(_glhckObject))))
      goto fail;

   /* copy static data */
   memcpy(&object->view, &src->view, sizeof(__GLHCKobjectView));
   memcpy(&object->material, &src->material, sizeof(__GLHCKobjectMaterial));

   /* copy metadata */
   if (src->file) {
      if (!(object->file = _glhckStrdup(src->file)))
         goto fail;
   }

   /* copy geometry */
   // object->geometry = _glhckGeometryCopy(src->geometry);
   object->geometry = NULL;

   /* copy texture */
   object->material.texture = NULL; /* make sure we don't free reference */
   glhckObjectTexture(object, src->material.texture);

   /* assign stub draw to copy */
   object->drawFunc = _glhckObjectStubDraw;

   /* set ref counter to 1 */
   object->refCounter = 1;

   /* insert to world */
   _glhckWorldInsert(olist, object, _glhckObject*);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
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
GLHCKAPI size_t glhckObjectFree(glhckObject *object)
{
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* not initialized */
   if (!_glhckInitialized) return 0;

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* remove itself from parent, reference self to avoid double free */
   if (object->parent)
      glhckObjectRemoveFromParent(glhckObjectRef(object));

   /* remove all children objects */
   glhckObjectRemoveAllChildren(object);

   /* free metadata */
   IFDO(_glhckFree, object->file);

   /* free geometry */
   IFDO(_glhckGeometryFree, object->geometry);

   /* free material */
   glhckObjectTexture(object, NULL);

   /* remove from world */
   _glhckWorldRemove(olist, object, _glhckObject*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief is object treated as root? */
GLHCKAPI int glhckObjectIsRoot(const glhckObject *object)
{
   CALL(3, "%p", object);
   assert(object);
   RET(3, "%d", object->flags & GLHCK_OBJECT_ROOT);
   return object->flags & GLHCK_OBJECT_ROOT;
}

/* \brief make object as root, or demote it */
GLHCKAPI void glhckObjectMakeRoot(glhckObject *object, int root)
{
   CALL(3, "%p", object);
   assert(object);
   if (root) object->flags |= GLHCK_OBJECT_ROOT;
   else object->flags &= ~GLHCK_OBJECT_ROOT;
}

/* \brief get object's affection flags */
GLHCKAPI unsigned int glhckObjectGetParentAffection(const glhckObject *object)
{
   CALL(3, "%p", object);
   assert(object);
   RET(3, "%u", object->affectionFlags);
   return object->affectionFlags;
}

/* \brief set object's affection flags */
GLHCKAPI void glhckObjectParentAffection(glhckObject *object, unsigned char affectionFlags)
{
   CALL(3, "%p", object);
   assert(object);

   /* we need to update matrix on next draw */
   if (object->affectionFlags != affectionFlags)
      object->view.update = 1;

   object->affectionFlags = affectionFlags;
}

/* \brief get object's parent */
GLHCKAPI glhckObject* glhckObjectParent(glhckObject *object)
{
   CALL(3, "%p", object);
   assert(object);
   RET(3, "%p", object->parent);
   return object->parent;
}

/* \brief get object's children */
GLHCKAPI glhckObject** glhckObjectChildren(glhckObject *object, size_t *num_children)
{
   CALL(3, "%p, %p", object, num_children);
   assert(object);
   if (num_children) *num_children = object->numChilds;
   RET(3, "%p", object->childs);
   return object->childs;
}

/* \brief add children object */
GLHCKAPI void glhckObjectAddChildren(glhckObject *object, glhckObject *child)
{
   size_t i;
   glhckObject **newChilds = NULL;
   CALL(0, "%p, %p", object, child);
   assert(object && child && object != child);

   /* already added? */
   if (child->parent == object) return;
   if (child->parent) glhckObjectRemoveChildren(child->parent, child);

   /* add child */
   if (!(newChilds = _glhckMalloc((object->numChilds+1) * sizeof(glhckObject*))))
      return;

   for (i = 0; i != object->numChilds; ++i) newChilds[i] = object->childs[i];
   newChilds[object->numChilds] = glhckObjectRef(child);
   child->parent = object;

   IFDO(_glhckFree, object->childs);
   object->childs = newChilds;
   object->numChilds++;
}

/* \brief remove children object */
GLHCKAPI void glhckObjectRemoveChildren(glhckObject *object, glhckObject *child)
{
   size_t i, newCount;
   glhckObject **newChilds = NULL;
   CALL(0, "%p, %p", object, child);
   assert(object && child && object != child);

   /* do we have the child? */
   if (child->parent != object) return;

   /* remove child */
   if ((object->numChilds-1) != 0 &&
      !(newChilds = _glhckMalloc((object->numChilds-1) * sizeof(glhckObject*))))
      return;

   child->parent = NULL;
   glhckObjectFree(child);
   for (i = 0, newCount = 0; i != object->numChilds && newChilds; ++i) {
      if (object->childs[i] == child) continue;
      newChilds[newCount++] = object->childs[i];
   }

   IFDO(_glhckFree, object->childs);
   object->childs = newChilds;
   object->numChilds = newCount;
}

/* \brief remove all children objects */
GLHCKAPI void glhckObjectRemoveAllChildren(glhckObject *object)
{
   size_t i;
   CALL(0, "%p", object);
   assert(object);

   if (!object->childs) return;
   for (i = 0; i != object->numChilds; ++i) {
      object->childs[i]->parent = NULL;
      glhckObjectFree(object->childs[i]);
   }

   NULLDO(_glhckFree, object->childs);
   object->numChilds = 0;
}

/* \brief remove object from parent */
GLHCKAPI void glhckObjectRemoveFromParent(glhckObject *object)
{
   CALL(0, "%p", object);
   assert(object);
   if (!object->parent) return;
   glhckObjectRemoveChildren(object->parent, object);
}

/* \brief set object's texture */
GLHCKAPI void glhckObjectTexture(glhckObject *object, glhckTexture *texture)
{
   CALL(1, "%p, %p", object, texture);
   assert(object);

   /* already applied */
   if (object->material.texture == texture)
      return;

   /* sanity warning */
   if (object->geometry && object->geometry->textureRange > 1) {
      if ((texture->width  > object->geometry->textureRange) ||
          (texture->height > object->geometry->textureRange)) {
         DEBUG(GLHCK_DBG_WARNING, "Texture dimensions are above the maximum precision of object's vertexdata (%s:%u)",
               glhckVertexTypeString(object->geometry->vertexType), object->geometry->textureRange);
      }
   }

   /* free old texture */
   IFDO(glhckTextureFree, object->material.texture);

   /* assign new texture */
   if (texture) {
      object->material.texture = glhckTextureRef(texture);
      if (texture->importFlags & GLHCK_TEXTURE_IMPORT_TEXT) {
         glhckObjectMaterialBlend(object, GLHCK_ONE, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else if (texture->importFlags & GLHCK_TEXTURE_IMPORT_ALPHA) {
         glhckObjectMaterialBlend(object, GLHCK_SRC_ALPHA, GLHCK_ONE_MINUS_SRC_ALPHA);
      } else {
         glhckObjectMaterialBlend(object, 0, 0);
      }
   }

   /* set texture on all the childs */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectTexture, texture);
   }
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

   /* insert to draw queue, referenced until glhckRender */
   _glhckObjectInsertToQueue(object);

   /* insert texture to drawing queue? */
   if (object->material.texture) {
      /* insert object's texture to textures queue, referenced until glhckRender */
      _glhckTextureInsertToQueue(object->material.texture);
   }

   /* draw childs as well */
   PERFORM_ON_CHILDS(object, glhckObjectDraw);
}

/* \brief render object */
GLHCKAPI void glhckObjectRender(glhckObject *object)
{
   glhckObject *parent;
   CALL(2, "%p", object);
   assert(object);

   /* lets make sure parents are updated first */
   for (parent = object->parent; parent; parent = parent->parent)
      if (parent->view.update) _glhckObjectUpdateMatrix(parent);

   /* does view matrix need update? */
   if (object->view.update)
      _glhckObjectUpdateMatrix(object);

   /* render */
   assert(object->drawFunc);
   object->drawFunc(object);
}

/* \brief get object's material flag s*/
GLHCKAPI unsigned int glhckObjectGetMaterialFlags(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%u", object->material.flags);
   return object->material.flags;
}

/* \brief set object's material flags */
GLHCKAPI void glhckObjectMaterialFlags(glhckObject *object, unsigned int flags)
{
   CALL(1, "%p, %u", object, flags);
   assert(object);

   object->material.flags = flags;

   /* set material flags on all the childs */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectMaterialFlags, flags);
   }
}

/* \brief set object's material blending */
GLHCKAPI void glhckObjectMaterialBlend(glhckObject *object, unsigned int blenda, unsigned int blendb)
{
   if (blenda == GLHCK_ZERO && blendb == GLHCK_ZERO) {
      object->material.blenda = GLHCK_ZERO;
      object->material.blendb = GLHCK_ZERO;
      return;
   }

   object->material.blenda = blenda;
   object->material.blendb = blendb;
}

/* \brief get object's color */
GLHCKAPI const glhckColorb* glhckObjectGetColor(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   RET(2, "%p", &object->material.color);
   return &object->material.color;
}

/* \brief set object's color */
GLHCKAPI void glhckObjectColor(glhckObject *object, const glhckColorb *color)
{
   CALL(2, "%p, %p", object, color);
   assert(object && color);

   object->material.color.r = color->r;
   object->material.color.g = color->g;
   object->material.color.b = color->b;
   object->material.color.a = color->a;

   /* set color on all the childs */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectColor, color);
   }
}

/* \brief set object's color (with unsigned char) */
GLHCKAPI void glhckObjectColorb(glhckObject *object,
      unsigned char r, unsigned char g, unsigned char b, unsigned char a)
{
   glhckColorb color = { r, g, b, a };
   glhckObjectColor(object, &color);
}

/* \brief get obb bounding box of the object */
GLHCKAPI const kmAABB*  glhckObjectGetOBB(const glhckObject *object)
{
   const kmAABB *obb;
   unsigned int i;
   CALL(1, "%p", object);
   assert(object);

   /* if performed on root, get the largest obb */
   obb = &object->view.obb;
   if (object->flags & GLHCK_OBJECT_ROOT) {
      for (i = 0; i != object->numChilds; ++i) {
         if (obb->max.x < object->childs[i]->view.obb.max.x &&
             obb->max.y < object->childs[i]->view.obb.max.y)
            obb = & object->childs[i]->view.obb;
      }
   }

   RET(1, "%p", obb);
   return obb;
}

/* \brief get aabb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetAABB(const glhckObject *object)
{
   const kmAABB *aabb;
   unsigned int i;
   CALL(1, "%p", object);
   assert(object);

   /* if performed on root, get the largest aabb */
   aabb = &object->view.aabb;
   if (object->flags & GLHCK_OBJECT_ROOT) {
      for (i = 0; i != object->numChilds; ++i) {
         if (aabb->max.x < object->childs[i]->view.aabb.max.x &&
             aabb->max.y < object->childs[i]->view.aabb.max.y)
            aabb = & object->childs[i]->view.aabb;
      }
   }

   RET(1, "%p", aabb);
   return aabb;
}

/* \brief get object's matrix */
GLHCKAPI const kmMat4* glhckObjectGetMatrix(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%p", &object->view.matrix);
   return &object->view.matrix;
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
   CALL(2, "%p", object);
   assert(object);

   RET(2, VEC3S, VEC3(&object->view.rotation));
   return &object->view.rotation;
}

/* \brief set object rotation */
GLHCKAPI void glhckObjectRotation(glhckObject *object, const kmVec3 *rotation)
{
   CALL(2, "%p, "VEC3S, object, VEC3(rotation));
   assert(object && rotation);

   kmVec3Assign(&object->view.rotation, rotation);
   _glhckObjectUpdateTargetFromRotation(object);
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
   _glhckObjectUpdateTargetFromRotation(object);
   object->view.update = 1;
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckObjectRotatef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotate = { x, y, z };
   glhckObjectRotate(object, &rotate);
}

/* \brief get object target */
GLHCKAPI const kmVec3* glhckObjectGetTarget(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   RET(2, VEC3S, VEC3(&object->view.target));
   return &object->view.target;
}

/* \brief set object target */
GLHCKAPI void glhckObjectTarget(glhckObject *object, const kmVec3 *target)
{
   CALL(2, "%p, "VEC3S, object, VEC3(target));
   assert(object && target);

   kmVec3Assign(&object->view.target, target);
   _glhckObjectUpdateRotationFromTarget(object);
   object->view.update = 1;
}

/* \brief set object target (with kmScalar) */
GLHCKAPI void glhckObjectTargetf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 target = { x, y, z };
   glhckObjectTarget(object, &target);
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

   kmVec3Assign(&object->view.scaling, scale);
   object->view.update = 1;

   /* perform scale on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectScale, scale);
   }
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckObjectScalef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 scaling = { x, y, z };
   glhckObjectScale(object, &scaling);
}

/* \brief create new geometry for object, replacing existing one. */
GLHCKAPI glhckGeometry* glhckObjectNewGeometry(glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   IFDO(_glhckGeometryFree, object->geometry);
   object->geometry = _glhckGeometryNew();

   RET(1, "%p", object->geometry);
   return object->geometry;
}

/* \brief get geometry from object */
GLHCKAPI glhckGeometry* glhckObjectGetGeometry(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%p", object->geometry);
   return object->geometry;
}

/* \brief insert indices to object */
GLHCKAPI int glhckObjectInsertIndices(
      glhckObject *object, glhckGeometryIndexType type,
      const glhckImportIndexData *indices, size_t memb)
{
   CALL(0, "%p, %zu, %d, %p", object, memb, type, indices);
   assert(object);

   /* create new geometry for object, if not one already */
   if (!object->geometry) {
      if (!(object->geometry = _glhckGeometryNew()))
         goto fail;
   }

   /* do the work (tm) */
   if (!_glhckGeometryInsertIndices(object->geometry, memb, type, indices))
      goto fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert vertices to object */
GLHCKAPI int glhckObjectInsertVertices(
      glhckObject *object, glhckGeometryVertexType type,
      const glhckImportVertexData *vertices, size_t memb)
{
   CALL(0, "%p, %zu, %d, %p", object, memb, type, vertices);
   assert(object);

   /* create new geometry for object, if not one already */
   if (!object->geometry) {
      if (!(object->geometry = _glhckGeometryNew()))
         goto fail;
   }

   /* do the work (tm) */
   if (!_glhckGeometryInsertVertices(object->geometry, memb, type, vertices))
      goto fail;

   glhckObjectUpdate(object);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief update/recalculate object's internal state */
GLHCKAPI void glhckObjectUpdate(glhckObject *object)
{
   TRACE(1);
   assert(object);

   /* no geometry, useless to proceed further */
   if (!object->geometry)
      return;

   glhckGeometryCalculateBB(object->geometry, &object->view.bounding);
   _glhckObjectUpdateMatrix(object);
   object->drawFunc = _GLHCKlibrary.render.api.objectDraw;

   /* perform update on all the childs */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectUpdate);
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
