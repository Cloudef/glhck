#include "internal.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

#define PERFORM_ON_CHILDS(parent, function, ...)         \
   { unsigned int _cbc_;                                 \
   for (_cbc_ = 0; _cbc_ != parent->numChilds; ++_cbc_)  \
      function(parent->childs[_cbc_], ##__VA_ARGS__); }

/* \brief assign object to draw list */
inline void _glhckObjectInsertToQueue(glhckObject *object)
{
   __GLHCKobjectQueue *objects;
   glhckObject **queue;
   unsigned int i;

   objects = &GLHCKRD()->objects;

   /* check duplicate */
   for (i = 0; i != objects->count; ++i)
      if (objects->queue[i] == object) return;

   /* need alloc dynamically more? */
   if (objects->allocated <= objects->count+1) {
      queue = _glhckRealloc(objects->queue,
            objects->allocated,
            objects->allocated + GLHCK_QUEUE_ALLOC_STEP,
            sizeof(glhckObject*));

      /* epic fail here */
      if (!queue) return;
      objects->queue = queue;
      objects->allocated += GLHCK_QUEUE_ALLOC_STEP;
   }

   /* assign the object to list */
   objects->queue[objects->count] = glhckObjectRef(object);
   objects->count++;
}

/* update target from rotation */
static inline void _glhckObjectUpdateTargetFromRotation(glhckObject *object)
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
static inline void _glhckObjectUpdateRotationFromTarget(glhckObject *object)
{
   kmVec3 toTarget;
   CALL(2, "%p", object);
   assert(object);

   /* update rotation */
   kmVec3Subtract(&toTarget, &object->view.target, &object->view.translation);
   kmVec3GetHorizontalAngle(&object->view.rotation, &toTarget);
}

/* stub draw function */
static inline void _glhckObjectStubDraw(const glhckObject *object)
{
   if (object->geometry && object->geometry->vertexType != GLHCK_VERTEX_NONE) {
      DEBUG(GLHCK_DBG_WARNING, "Stub draw function called for object which actually has vertexdata!");
      DEBUG(GLHCK_DBG_WARNING, "Did you forget to call glhckObjectUpdate() after inserting vertex data?");
   }
}

/* \brief build translation matrix for object */
static void _glhckObjectBuildTranslation(glhckObject *object, kmMat4 *translation)
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
static void _glhckObjectBuildRotation(glhckObject *object, kmMat4 *rotation)
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
static void _glhckObjectBuildScaling(glhckObject *object, kmMat4 *scaling)
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

/* \brief build parent affection matrix for object */
static void _glhckObjectBuildParent(glhckObject *object, kmMat4 *parentMatrix)
{
   kmMat4 matrix, tmp;
   glhckObject *parent, *child;

   kmMat4Identity(parentMatrix);
   for (parent = object->parent, child = object; parent;
        parent = parent->parent, child = child->parent) {
      kmMat4Identity(&matrix);

      if (child->affectionFlags & GLHCK_AFFECT_ROTATION) {
         _glhckObjectBuildRotation(parent, &tmp);
         kmMat4Multiply(&matrix, &tmp, &matrix);
      }

      if (child->affectionFlags & GLHCK_AFFECT_SCALING) {
         _glhckObjectBuildScaling(parent, &tmp);
         kmMat4Multiply(&matrix, &tmp, &matrix);
      }

      if (child->affectionFlags & GLHCK_AFFECT_TRANSLATION) {
         _glhckObjectBuildTranslation(parent, &tmp);
         kmMat4Multiply(&matrix, &tmp, &matrix);
      }

      kmMat4Multiply(parentMatrix, &matrix, parentMatrix);
   }
}

/* \brief update bounding boxes of object */
inline void _glhckObjectUpdateBoxes(glhckObject *object)
{
   kmVec3 min, max;
   kmVec3 mixxyz, mixyyz, mixyzz;
   kmVec3 maxxyz, maxyyz, maxyzz;

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
}

/* \brief update view matrix of object */
static inline void _glhckObjectUpdateMatrix(glhckObject *object)
{
   unsigned int i;
   kmMat4 translation, rotation, scaling, parentMatrix;
   CALL(2, "%p", object);

   /* build affection matrices */
   _glhckObjectBuildParent(object, &parentMatrix);
   _glhckObjectBuildTranslation(object, &translation);
   _glhckObjectBuildRotation(object, &rotation);
   _glhckObjectBuildScaling(object, &scaling);

   /* affection of parent matrix */
   kmMat4Multiply(&translation, &parentMatrix, &translation);

   /* build model matrix */
   kmMat4Multiply(&scaling, &rotation, &scaling);
   kmMat4Multiply(&object->view.matrix, &translation, &scaling);

   /* update bounding boxes */
   _glhckObjectUpdateBoxes(object);

   /* update childs on next draw */
   for (i = 0; i != object->numChilds; ++i)
      object->childs[i]->view.update = 1;

   /* done */
   object->view.update = 0;
}

/* set object's filename */
void _glhckObjectFile(glhckObject *object, const char *file)
{
   char *fileCopy = NULL;
   CALL(1, "%p, %s", object, file);
   assert(object);

   if (file && !(fileCopy = _glhckStrdup(file)))
      return;

   IFDO(_glhckFree, object->file);
   object->file = fileCopy;
}

/***
 * public api
 ***/

/* \brief new object */
GLHCKAPI glhckObject *glhckObjectNew(void)
{
   glhckObject *object;
   TRACE(0);

   if (!(object = _glhckCalloc(1, sizeof(glhckObject))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* default view matrix */
   glhckObjectScalef(object, 1.0f, 1.0f, 1.0f);
   _glhckObjectUpdateMatrix(object);

   /* default object settings */
   glhckObjectCull(object, 1);
   glhckObjectDepth(object, 1);

   /* default affection flags */
   glhckObjectParentAffection(object, GLHCK_AFFECT_TRANSLATION |
                                      GLHCK_AFFECT_ROTATION);

   /* set stub draw function */
   object->drawFunc = _glhckObjectStubDraw;

   /* insert to world */
   _glhckWorldInsert(object, object, glhckObject*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief copy object */
GLHCKAPI glhckObject* glhckObjectCopy(const glhckObject *src)
{
   glhckObject *object;
   CALL(0, "%p", src);
   assert(src);

   if (!(object = _glhckCalloc(1, sizeof(glhckObject))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* copy metadata */
   _glhckObjectFile(object, src->file);

   /* copy geometry */
   if (src->geometry)
      object->geometry = _glhckGeometryCopy(src->geometry);

   /* reference material */
   if (src->material)
      glhckObjectMaterial(object, glhckMaterialRef(src->material));

   /* copy properties */
   memcpy(&object->view, &src->view, sizeof(__GLHCKobjectView));
   object->affectionFlags = src->affectionFlags;
   object->flags = src->flags;

   /* update object */
   glhckObjectUpdate(object);

   /* insert to world */
   _glhckWorldInsert(object, object, glhckObject*);

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
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free object */
GLHCKAPI unsigned int glhckObjectFree(glhckObject *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* remove itself from parent, reference self to avoid double free */
   if (object->parent)
      glhckObjectRemoveFromParent(glhckObjectRef(object));

   /* remove children objects */
   glhckObjectRemoveChildren(object);

   /* free metadata */
   _glhckObjectFile(object, NULL);

   /* free animations */
   glhckObjectInsertAnimations(object, NULL, 0);

   /* free bones */
   glhckObjectInsertBones(object, NULL, 0);

   /* free material */
   glhckObjectMaterial(object, NULL);

   /* free geometry */
   IFDO(_glhckGeometryFree, object->bindGeometry);
   IFDO(_glhckGeometryFree, object->geometry);

   /* remove from world */
   _glhckWorldRemove(object, object, glhckObject*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief is object treated as root? */
GLHCKAPI int glhckObjectIsRoot(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_ROOT);
   return object->flags & GLHCK_OBJECT_ROOT;
}

/* \brief make object as root, or demote it */
GLHCKAPI void glhckObjectMakeRoot(glhckObject *object, int root)
{
   CALL(2, "%p", object);
   assert(object);
   if (root) object->flags |= GLHCK_OBJECT_ROOT;
   else object->flags &= ~GLHCK_OBJECT_ROOT;
}

/* \brief get object's affection flags */
GLHCKAPI unsigned char glhckObjectGetParentAffection(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%u", object->affectionFlags);
   return object->affectionFlags;
}

/* \brief set object's affection flags */
GLHCKAPI void glhckObjectParentAffection(glhckObject *object, unsigned char affectionFlags)
{
   CALL(2, "%p", object);
   assert(object);

   /* we need to update matrix on next draw */
   if (object->affectionFlags != affectionFlags)
      object->view.update = 1;

   object->affectionFlags = affectionFlags;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectParentAffection, affectionFlags);
   }
}

/* \brief get object's parent */
GLHCKAPI glhckObject* glhckObjectParent(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->parent);
   return object->parent;
}

/* \brief get object's children */
GLHCKAPI glhckObject** glhckObjectChildren(glhckObject *object, unsigned int *num_children)
{
   CALL(2, "%p, %p", object, num_children);
   assert(object);
   if (num_children) *num_children = object->numChilds;
   RET(2, "%p", object->childs);
   return object->childs;
}

/* \brief add child object */
GLHCKAPI void glhckObjectAddChild(glhckObject *object, glhckObject *child)
{
   size_t i;
   glhckObject **newChilds = NULL;
   CALL(0, "%p, %p", object, child);
   assert(object && child && object != child);

   /* already added? */
   if (child->parent == object) return;
   if (child->parent) glhckObjectRemoveChild(child->parent, child);

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

/* \brief remove child object */
GLHCKAPI void glhckObjectRemoveChild(glhckObject *object, glhckObject *child)
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

/* \brief remove children objects */
GLHCKAPI void glhckObjectRemoveChildren(glhckObject *object)
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
   glhckObjectRemoveChild(object->parent, object);
}

/* \brief set material to object */
GLHCKAPI void glhckObjectMaterial(glhckObject *object, glhckMaterial *material)
{
   CALL(2, "%p, %p", object, material);
   assert(object);
   IFDO(glhckMaterialFree, object->material);
   object->material = (material?glhckMaterialRef(material):NULL);

   /* sanity warning */
   if (material && material->texture && object->geometry && object->geometry->textureRange > 1) {
      if ((material->texture->width  > object->geometry->textureRange) ||
          (material->texture->height > object->geometry->textureRange)) {
         DEBUG(GLHCK_DBG_WARNING, "Texture dimensions are above the maximum precision of object's vertexdata (%s:%u)",
               glhckVertexTypeString(object->geometry->vertexType), object->geometry->textureRange);
      }
   }

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectMaterial, material);
   }
}

/* \brief get material from object */
GLHCKAPI glhckMaterial* glhckObjectGetMaterial(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->material);
   return object->material;
}

/* \brief add object to draw queue */
GLHCKAPI void glhckObjectDraw(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* insert to draw queue, referenced until glhckRender */
   _glhckObjectInsertToQueue(object);

   /* insert texture to drawing queue? */
   if (object->material && object->material->texture) {
      /* insert object's texture to textures queue, referenced until glhckRender */
      _glhckTextureInsertToQueue(object->material->texture);
   }

   /* draw childs as well */
   PERFORM_ON_CHILDS(object, glhckObjectDraw);
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
   assert(object->drawFunc);
   object->drawFunc(object);
}

/* \brief set whether object should be culled */
GLHCKAPI void glhckObjectCull(glhckObject *object, int cull)
{
   CALL(2, "%p, %d", object, cull);
   assert(object);
   if (cull) object->flags |= GLHCK_OBJECT_CULL;
   else object->flags &= ~GLHCK_OBJECT_CULL;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectCull, cull);
   }
}

/* \brief get whether object is culled */
GLHCKAPI int glhckObjectGetCull(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_CULL);
   return object->flags & GLHCK_OBJECT_CULL;
}

/* \brief set wether object should be depth tested */
GLHCKAPI void glhckObjectDepth(glhckObject *object, int depth)
{
   CALL(2, "%p, %d", object, depth);
   assert(object);
   if (depth) object->flags |= GLHCK_OBJECT_DEPTH;
   else object->flags &= ~GLHCK_OBJECT_DEPTH;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectDepth, depth);
   }
}

/* \brief get wether object should be depth tested */
GLHCKAPI int glhckObjectGetDepth(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DEPTH);
   return object->flags & GLHCK_OBJECT_DEPTH;
}

/* \brief set object's AABB drawing */
GLHCKAPI void glhckObjectDrawAABB(glhckObject *object, int drawAABB)
{
   CALL(2, "%p, %d", object, drawAABB);
   assert(object);
   if (drawAABB) object->flags |= GLHCK_OBJECT_DRAW_AABB;
   else object->flags &= ~GLHCK_OBJECT_DRAW_AABB;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectDrawAABB, drawAABB);
   }
}

/* \brief get object's AABB drawing */
GLHCKAPI int glhckObjectGetDrawAABB(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DRAW_AABB);
   return object->flags & GLHCK_OBJECT_DRAW_AABB;
}

/* \brief set object's OBB drawing */
GLHCKAPI void glhckObjectDrawOBB(glhckObject *object, int drawOBB)
{
   CALL(2, "%p, %d", object, drawOBB);
   assert(object);
   if (drawOBB) object->flags |= GLHCK_OBJECT_DRAW_OBB;
   else object->flags &= ~GLHCK_OBJECT_DRAW_OBB;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectDrawOBB, drawOBB);
   }
}

/* \brief get object's OBB drawing */
GLHCKAPI int glhckObjectGetDrawOBB(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DRAW_OBB);
   return object->flags & GLHCK_OBJECT_DRAW_OBB;
}

/* \brief set object's skeleton drawing */
GLHCKAPI void glhckObjectDrawSkeleton(glhckObject *object, int drawSkeleton)
{
   CALL(2, "%p, %d", object, drawSkeleton);
   assert(object);
   if (drawSkeleton) object->flags |= GLHCK_OBJECT_DRAW_SKELETON;
   else object->flags &= ~GLHCK_OBJECT_DRAW_SKELETON;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectDrawSkeleton, drawSkeleton);
   }
}

/* \brief get object's skeleton drawing */
GLHCKAPI int glhckObjectGetDrawSkeleton(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DRAW_SKELETON);
   return object->flags & GLHCK_OBJECT_DRAW_SKELETON;
}

/* \brief set object's wireframe drawing */
GLHCKAPI void glhckObjectDrawWireframe(glhckObject *object, int drawWireframe)
{
   CALL(2, "%p, %d", object, drawWireframe);
   assert(object);
   if (drawWireframe) object->flags |= GLHCK_OBJECT_DRAW_WIREFRAME;
   else object->flags &= ~GLHCK_OBJECT_DRAW_WIREFRAME;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectDrawWireframe, drawWireframe);
   }
}

/* \brief get object's wireframe drawing */
GLHCKAPI int glhckObjectGetDrawWireframe(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DRAW_WIREFRAME);
   return object->flags & GLHCK_OBJECT_DRAW_WIREFRAME;
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
   CALL(2, "%p", object);
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

   RET(2, "%p", aabb);
   return aabb;
}

/* \brief get object's matrix */
GLHCKAPI const kmMat4* glhckObjectGetMatrix(glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   /* update matrix first, if needed */
   if (object->view.update)
      _glhckObjectUpdateMatrix(object);

   RET(1, "%p", &object->view.matrix);
   return &object->view.matrix;
}

/* \brief get object position */
GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   RET(2, VEC3S, VEC3(&object->view.translation));
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

/* \brief insert bones to object */
GLHCKAPI int glhckObjectInsertBones(glhckObject *object, glhckBone **bones, unsigned int memb)
{
   unsigned int i;
   glhckBone **bonesCopy = NULL;
   CALL(0, "%p, %p, %u", object, bones, memb);
   assert(object);

   /* copy bones, if they exist */
   if (bones && !(bonesCopy = _glhckCopy(bones, memb * sizeof(glhckBone*))))
      goto fail;

   /* free old bones */
   if (object->bones) {
      for (i = 0; i != object->numBones; ++i)
         glhckBoneFree(object->bones[i]);
      _glhckFree(object->bones);
   }

   object->bones = bonesCopy;
   object->numBones = (bonesCopy?memb:0);

   /* reference new bones */
   if (object->bones) {
      for (i = 0; i != object->numBones; ++i)
         glhckBoneRef(object->bones[i]);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get bones assigned to this object */
GLHCKAPI glhckBone** glhckObjectBones(glhckObject *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numBones;
   RET(2, "%p", object->bones);
   return object->bones;
}

/* \brief insert animations to object */
GLHCKAPI int glhckObjectInsertAnimations(glhckObject *object, glhckAnimation **animations, unsigned int memb)
{
   unsigned int i;
   glhckAnimation **animationsCopy = NULL;
   CALL(0, "%p, %p, %u", object, animations, memb);
   assert(object);

   /* copy animations, if they exist */
   if (animations && !(animationsCopy = _glhckCopy(animations, memb * sizeof(glhckAnimation*))))
      goto fail;

   /* free old animations */
   if (object->animations) {
      for (i = 0; i != object->numAnimations; ++i)
         glhckAnimationFree(object->animations[i]);
      _glhckFree(object->animations);
   }

   object->animations = animationsCopy;
   object->numAnimations = (animationsCopy?memb:0);

   /* reference new animations */
   if (object->animations) {
      for (i = 0; i != object->numAnimations; ++i)
         glhckAnimationRef(object->animations[i]);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get animations assigned to this object */
GLHCKAPI glhckAnimation** glhckObjectAnimations(glhckObject *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numAnimations;
   RET(2, "%p", object->animations);
   return object->animations;
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
      const glhckImportIndexData *indices, int memb)
{
   CALL(0, "%p, %d, %d, %p", object, memb, type, indices);
   assert(object);

   if (indices) {
      /* create new geometry for object, if not one already */
      if (!object->geometry) {
         if (!(object->geometry = _glhckGeometryNew()))
            goto fail;
      }

      /* do the work (tm) */
      if (!_glhckGeometryInsertIndices(object->geometry, memb, type, indices))
         goto fail;
   } else {
      glhckGeometryInsertIndices(object->geometry, 0, NULL, 0);
      if (!object->geometry->vertices.any) {
         IFDO(_glhckGeometryFree, object->geometry);
      }
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert vertices to object */
GLHCKAPI int glhckObjectInsertVertices(
      glhckObject *object, glhckGeometryVertexType type,
      const glhckImportVertexData *vertices, int memb)
{
   CALL(0, "%p, %d, %d, %p", object, memb, type, vertices);
   assert(object);

   if (vertices) {
      /* create new geometry for object, if not one already */
      if (!object->geometry) {
         if (!(object->geometry = _glhckGeometryNew()))
            goto fail;
      }

      /* do the work (tm) */
      if (!_glhckGeometryInsertVertices(object->geometry, memb, type, vertices))
         goto fail;
   } else {
      glhckGeometryInsertVertices(object->geometry, 0, NULL, 0);
      if (!object->geometry->indices.any) {
         IFDO(_glhckGeometryFree, object->geometry);
      }
   }

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
   _glhckObjectUpdateBoxes(object);
   object->drawFunc = GLHCKRA()->objectRender;

   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, glhckObjectUpdate);
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
