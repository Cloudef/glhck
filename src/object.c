#include "internal.h"
#include <assert.h>  /* for assert */
#include <float.h>   /* for FLT_MAX */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

/* update target from rotation */
static void _glhckObjectUpdateTargetFromRotation(glhckObject *object)
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
static void _glhckObjectUpdateRotationFromTarget(glhckObject *object)
{
   kmVec3 toTarget;
   CALL(2, "%p", object);
   assert(object);

   /* update rotation */
   kmVec3Subtract(&toTarget, &object->view.target, &object->view.translation);
   kmVec3GetHorizontalAngle(&object->view.rotation, &toTarget);
}

/* stub draw function */
static void _glhckObjectStubDraw(const glhckObject *object)
{
   if (object->geometry && object->geometry->vertexCount) {
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
   glhckObject *parent, *child;

   kmMat4Identity(parentMatrix);
   for (parent = object->parent, child = object; parent;
        parent = parent->parent, child = child->parent) {
      kmMat4 matrix, tmp;
      kmMat4Identity(&matrix);

      if (child->affectionFlags & GLHCK_AFFECT_SCALING) {
         /* don't use _glhckObjectBuildScaling, as we don't want parent->geometry->scale affection
          * XXX: Should we do same for the translation below? */
         kmMat4Scaling(&tmp, parent->view.scaling.x, parent->view.scaling.y, parent->view.scaling.z);
         kmMat4Multiply(&matrix, &tmp, &matrix);
      }

      if (child->affectionFlags & GLHCK_AFFECT_ROTATION) {
         _glhckObjectBuildRotation(parent, &tmp);
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
void _glhckObjectUpdateBoxes(glhckObject *object)
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
   memcpy(&object->view.aabbFull, &object->view.aabb, sizeof(kmAABB));
   memcpy(&object->view.obbFull, &object->view.obb, sizeof(kmAABB));

   /* update full aabb && obb */
   if (object->childs && chckArrayCount(object->childs)) {
      glhckObject *child;
      const kmAABB *caabb, *cobb;
      kmAABB *aabb = &object->view.aabbFull, *obb = &object->view.obbFull;

      for (chckPoolIndex iter = 0; (child = chckArrayIter(object->childs, &iter));) {
         caabb = glhckObjectGetAABBWithChildren(child);
         cobb = glhckObjectGetOBBWithChildren(child);
         glhckMinV3(&aabb->min, &caabb->min);
         glhckMaxV3(&aabb->max, &caabb->max);
         glhckMinV3(&obb->min, &cobb->min);
         glhckMaxV3(&obb->max, &cobb->max);
      }
   }
}

/* \brief update view matrix of object */
static void _glhckObjectUpdateMatrix(glhckObject *object)
{
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

   /* we need to flip the matrix for 2D drawing */
   if (GLHCKRD()->view.flippedProjection)
      kmMat4Multiply(&object->view.matrix, &object->view.matrix, _glhckRenderGetFlipMatrix());

   /* update bounding boxes */
   _glhckObjectUpdateBoxes(object);

   /* update childs on next draw */
   if (object->childs) {
      glhckObject *child;
      for (chckPoolIndex iter = 0; (child = chckArrayIter(object->childs, &iter));)
         child->view.update = 1;
   }

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
GLHCKAPI glhckObject* glhckObjectNew(void)
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

   /* something that isn't zero */
   object->transformedGeometryTime = FLT_MAX;

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->objects, object);

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
   _glhckWorldAdd(&GLHCKW()->objects, object);

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

   /* free skin bones */
   glhckObjectInsertSkinBones(object, NULL, 0);

   /* free material */
   glhckObjectMaterial(object, NULL);

   /* free geometry */
   IFDO(_glhckGeometryFree, object->geometry);

   /* remove from world */
   _glhckWorldRemove(&GLHCKW()->objects, object);

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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectParentAffection, affectionFlags);
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
GLHCKAPI glhckObject** glhckObjectChildren(glhckObject *object, unsigned int *numChildren)
{
   CALL(2, "%p, %p", object, numChildren);
   assert(object);

   size_t memb = 0;
   glhckObject **childs = (object->childs ? chckArrayToCArray(object->childs, &memb) : NULL);

   if (numChildren)
      *numChildren = memb;

   RET(2, "%p", childs);
   return childs;
}

/* \brief add child object */
GLHCKAPI void glhckObjectAddChild(glhckObject *object, glhckObject *child)
{
   CALL(0, "%p, %p", object, child);
   assert(object && child && object != child);

   /* already added? */
   if (child->parent == object)
      return;

   /* keep child alive */
   glhckObjectRef(child);

   if (child->parent)
      glhckObjectRemoveChild(child->parent, child);

   if (!object->childs && !(object->childs = chckArrayNew(32, 1))) {
      glhckObjectFree(child);
      return;
   }

   if (!chckArrayAdd(object->childs, child)) {
      glhckObjectFree(child);
      return;
   }

   child->parent = object;
}

/* \brief remove child object */
GLHCKAPI void glhckObjectRemoveChild(glhckObject *object, glhckObject *child)
{
   CALL(0, "%p, %p", object, child);
   assert(object && child && object != child);

   /* do we have the child? */
   if (child->parent != object)
      return;

   chckArrayRemove(object->childs, child);

   child->parent = NULL;
   glhckObjectFree(child);

   if (chckArrayCount(object->childs) <= 0) {
      chckArrayFree(object->childs);
      object->childs = NULL;
   }
}

/* \brief remove children objects */
GLHCKAPI void glhckObjectRemoveChildren(glhckObject *object)
{
   CALL(0, "%p", object);
   assert(object);

   if (!object->childs)
      return;

   glhckObject *child;
   for (chckPoolIndex iter = 0; (child = chckArrayIter(object->childs, &iter));) {
      child->parent = NULL;
      glhckObjectFree(child);
   }

   chckArrayFree(object->childs);
   object->childs = NULL;
}

/* \brief remove object from parent */
GLHCKAPI void glhckObjectRemoveFromParent(glhckObject *object)
{
   CALL(0, "%p", object);
   assert(object);

   if (!object->parent)
      return;

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
         DEBUG(GLHCK_DBG_WARNING, "Texture dimensions are above the maximum precision of object's vertexdata (%zu:%u)",
               GLHCKVT(object->geometry->vertexType)->max[2], object->geometry->textureRange);
      }
   }

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectMaterial, material);
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
   if (chckArrayAdd(GLHCKRD()->objects, object))
      glhckObjectRef(object);

   /* insert texture to drawing queue? */
   if (object->material && object->material->texture) {
      /* insert object's texture to textures queue, referenced until glhckRender */
      if (chckArrayAdd(GLHCKRD()->textures, object->material->texture))
         glhckTextureRef(object->material->texture);
   }

   /* draw childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDraw);
}

/* \brief render object */
GLHCKAPI void glhckObjectRender(glhckObject *object)
{
   glhckObject *parent;
   CALL(2, "%p", object);
   assert(object);

   /* does parents view matrices need updating? */
   for (parent = object->parent; parent; parent = parent->parent) {
      if (parent->view.update || parent->view.wasFlipped != GLHCKRD()->view.flippedProjection) {
         _glhckObjectUpdateMatrix(parent);
         parent->view.wasFlipped = GLHCKRD()->view.flippedProjection;
      }
   }

   /* does our view matrix need update? */
   if (object->view.update || object->view.wasFlipped != GLHCKRD()->view.flippedProjection) {
      _glhckObjectUpdateMatrix(object);
      object->view.wasFlipped = GLHCKRD()->view.flippedProjection;
   }

   /* render */
   assert(object->drawFunc);
   object->drawFunc(object);
}

/* \brief render object and its children */
GLHCKAPI void glhckObjectRenderAll(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   glhckObjectRender(object);

   if (object->childs)
      chckArrayIterCall(object->childs, glhckObjectRenderAll);
}

/* \brief set whether object should use vertex colors */
GLHCKAPI void glhckObjectVertexColors(glhckObject *object, int vertexColors)
{
   CALL(2, "%p, %d", object, vertexColors);
   assert(object);
   if (vertexColors) object->flags |= GLHCK_OBJECT_VERTEX_COLOR;
   else object->flags &= ~GLHCK_OBJECT_VERTEX_COLOR;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectVertexColors, vertexColors);
}

/* \brief get whether object should use vertex colors */
GLHCKAPI int glhckObjectGetVertexColors(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_VERTEX_COLOR);
   return object->flags & GLHCK_OBJECT_VERTEX_COLOR;
}

/* \brief set whether object should be culled */
GLHCKAPI void glhckObjectCull(glhckObject *object, int cull)
{
   CALL(2, "%p, %d", object, cull);
   assert(object);
   if (cull) object->flags |= GLHCK_OBJECT_CULL;
   else object->flags &= ~GLHCK_OBJECT_CULL;

   /* perform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectCull, cull);
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDepth, depth);
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDrawAABB, drawAABB);
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDrawOBB, drawOBB);
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDrawSkeleton, drawSkeleton);
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectDrawWireframe, drawWireframe);
}

/* \brief get object's wireframe drawing */
GLHCKAPI int glhckObjectGetDrawWireframe(const glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->flags & GLHCK_OBJECT_DRAW_WIREFRAME);
   return object->flags & GLHCK_OBJECT_DRAW_WIREFRAME;
}

/* \brief get obb bounding box of the object that includes all the children */
GLHCKAPI const kmAABB* glhckObjectGetOBBWithChildren(glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   /* update matrix first, if needed */
   if (object->view.update) _glhckObjectUpdateMatrix(object);

   RET(2, "%p", &object->view.obbFull);
   return &object->view.obbFull;
}

/* \brief get obb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetOBB(glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   /* if performed on root, return the full obb instead */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      return glhckObjectGetOBBWithChildren(object);

   /* update matrix first, if needed */
   if (object->view.update) _glhckObjectUpdateMatrix(object);

   RET(1, "%p", &object->view.obb);
   return &object->view.obb;
}

/* \brief get aabb bounding box of the object that includes all the children */
GLHCKAPI const kmAABB* glhckObjectGetAABBWithChildren(glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   /* update matrix first, if needed */
   if (object->view.update) _glhckObjectUpdateMatrix(object);

   RET(2, "%p", &object->view.aabbFull);
   return &object->view.aabbFull;
}

/* \brief get aabb bounding box of the object */
GLHCKAPI const kmAABB* glhckObjectGetAABB(glhckObject *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* if performed on root, return the full aabb instead */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      return glhckObjectGetAABBWithChildren(object);

   /* update matrix first, if needed */
   if (object->view.update) _glhckObjectUpdateMatrix(object);

   RET(2, "%p", &object->view.aabb);
   return &object->view.aabb;
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
GLHCKAPI void glhckObjectPositionf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 position = { x, y, z };
   glhckObjectPosition(object, &position);
}

/* \brief move object */
GLHCKAPI void glhckObjectMove(glhckObject *object, const kmVec3 *move)
{
   CALL(2, "%p, "VEC3S, object, VEC3(move));
   assert(object && move);

   kmVec3Add(&object->view.translation, &object->view.translation, move);
   object->view.update = 1;
}

/* \brief move object (with kmScalar) */
GLHCKAPI void glhckObjectMovef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
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
GLHCKAPI void glhckObjectRotationf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
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
GLHCKAPI void glhckObjectRotatef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
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
GLHCKAPI void glhckObjectTargetf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
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
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectScale, scale);
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckObjectScalef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 scaling = { x, y, z };
   glhckObjectScale(object, &scaling);
}

/* \brief insert bones to object */
GLHCKAPI int glhckObjectInsertBones(glhckObject *object, glhckBone **bones, unsigned int memb)
{
   CALL(0, "%p, %p, %u", object, bones, memb);
   assert(object);

   /* free old bones */
   if (object->bones) {
      chckArrayIterCall(object->bones, glhckBoneFree);
      chckArrayFree(object->bones);
      object->bones = NULL;
   }

   if (bones && memb > 0) {
      if ((object->bones = chckArrayNewFromCArray(bones, memb, 32))) {
         chckArrayIterCall(object->bones, glhckBoneRef);
      } else {
         goto fail;
      }
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

   size_t nmemb = 0;
   glhckBone **bones = (object->bones ? chckArrayToCArray(object->bones, &nmemb) : NULL);

   if (memb)
      *memb = nmemb;

   RET(2, "%p", object->bones);
   return bones;
}

/* \brief get bone by name */
GLHCKAPI glhckBone* glhckObjectGetBone(glhckObject *object, const char *name)
{
   assert(object && name);
   CALL(2, "%p, %s", object, name);

   glhckBone *bone;
   for (chckPoolIndex iter = 0; (bone = chckArrayIter(object->bones, &iter));) {
      const char *bname = glhckBoneGetName(bone);
      if (bname && !strcmp(bname, name)) {
         RET(2, "%p", bone);
         return bone;
      }
   }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief insert skin bones to object */
GLHCKAPI int glhckObjectInsertSkinBones(glhckObject *object, glhckSkinBone **skinBones, unsigned int memb)
{
   CALL(0, "%p, %p, %u", object, skinBones, memb);
   assert(object);

   /* free old skin bones */
   if (object->skinBones) {
      chckArrayIterCall(object->skinBones, glhckSkinBoneFree);
      chckArrayFree(object->skinBones);
      object->skinBones = NULL;
   }

   if (skinBones && memb > 0) {
      if ((object->skinBones = chckArrayNewFromCArray(skinBones, memb, 32))) {
         chckArrayIterCall(object->skinBones, glhckSkinBoneRef);
      } else {
         goto fail;
      }
   }

   if (object->skinBones) {
      __GLHCKvertexType *type = GLHCKVT(object->geometry->vertexType);
      if (!object->bind && !(object->bind = _glhckCopy(object->geometry->vertices, object->geometry->vertexCount * type->size)))
         goto fail;
      if (!object->zero && !(object->zero = _glhckCopy(object->geometry->vertices, object->geometry->vertexCount * type->size)))
         goto fail;

      for (int v = 0; v < object->geometry->vertexCount; ++v)
         memset(object->zero + type->size * v + type->offset[0], 0, type->msize[0] * type->memb[0]);
      _glhckSkinBoneTransformObject(object, 1);
   } else {
      IFDO(_glhckFree, object->bind);
      IFDO(_glhckFree, object->zero);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   glhckObjectInsertSkinBones(object, NULL, 0);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get skin bones assigned to this object */
GLHCKAPI glhckSkinBone** glhckObjectSkinBones(glhckObject *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);

   size_t nmemb = 0;
   glhckSkinBone **skinBones = (object->skinBones ? chckArrayToCArray(object->skinBones, &nmemb) : NULL);

   if (memb)
      *memb = nmemb;

   RET(2, "%p", object->skinBones);
   return skinBones;
}

/* \brief get skin bone by name */
GLHCKAPI glhckSkinBone* glhckObjectGetSkinBone(glhckObject *object, const char *name)
{
   glhckSkinBone *skinBone;
   assert(object && name);
   CALL(2, "%p, %s", object, name);

   for (chckPoolIndex iter = 0; (skinBone = chckArrayIter(object->skinBones, &iter));) {
      const char *sbname = glhckSkinBoneGetName(skinBone);
      if (sbname && !strcmp(sbname, name)) {
         RET(2, "%p", skinBone);
         return skinBone;
      }
   }

   RET(2, "%p", NULL);
   return NULL;
}

/* \brief insert animations to object */
GLHCKAPI int glhckObjectInsertAnimations(glhckObject *object, glhckAnimation **animations, unsigned int memb)
{
   CALL(0, "%p, %p, %u", object, animations, memb);
   assert(object);

   /* free old animations */
   if (object->animations) {
      chckArrayIterCall(object->animations, glhckAnimationFree);
      chckArrayFree(object->animations);
      object->animations = NULL;
   }

   if (animations && memb > 0) {
      if ((object->animations = chckArrayNewFromCArray(animations, memb, 32))) {
         chckArrayIterCall(object->animations, glhckAnimationRef);
      } else {
         goto fail;
      }
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

   size_t nmemb = 0;
   glhckAnimation **animations = (object->animations ? chckArrayToCArray(object->animations, &nmemb) : NULL);

   if (memb)
      *memb = nmemb;

   RET(2, "%p", object->animations);
   return animations;
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

/* \brief get geometry from object */
GLHCKAPI glhckGeometry* glhckObjectGetGeometry(const glhckObject *object)
{
   CALL(1, "%p", object);
   assert(object);

   RET(1, "%p", object->geometry);
   return object->geometry;
}

/* \brief insert indices to object */
GLHCKAPI int glhckObjectInsertIndices(glhckObject *object, unsigned char type, const glhckImportIndexData *indices, int memb)
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
      if (!object->geometry->vertices) {
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
GLHCKAPI int glhckObjectInsertVertices(glhckObject *object, unsigned char type, const glhckImportVertexData *vertices, int memb)
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
      if (!object->geometry->indices) {
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

   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, glhckObjectUpdate);
}

/* \brief return intersecting texture coordinate from object's geometry using ray */
GLHCKAPI int glhckObjectPickTextureCoordinatesWithRay(const glhckObject *object, const kmRay3 *ray, kmVec2 *outCoords)
{
   CALL(0, "%p, %p, %p", object, ray, outCoords);
   assert(object && ray && outCoords);

   /* FIXME: move to geometry.c
    * Can we use collision mesh for this instead somehow?
    * It would simplify a lot since collisionMesh is optimized more for intersections. */
   assert(object->geometry->vertexType == GLHCK_VTX_V3F && "Only GLHCK_VTX_V3F supported for now");
   assert((object->geometry->type == GLHCK_TRIANGLES || object->geometry->type == GLHCK_TRIANGLE_STRIP) && "Only TRIANGLES and STRIPS supported for now");

   /* Transform the ray to model coordinates */
   kmMat4 inverseView;
   kmMat4Inverse(&inverseView, &object->view.matrix);

   kmRay3 r;
   kmVec3Transform(&r.start, &ray->start, &inverseView);
   kmVec3Transform(&r.dir, &ray->dir, &inverseView);
   kmVec3 origin = {0, 0, 0};
   kmVec3Transform(&origin, &origin, &inverseView);
   kmVec3Subtract(&r.dir, &r.dir, &origin);

   /* Find closest intersecting primitive */
   int intersectionIndex, intersectionFound = 0;
   kmScalar closestDistanceSq, s, t;

   int i, fix[3];
   int skipCount = (object->geometry->type == GLHCK_TRIANGLES ? 3 : 1);
   glhckVertexData3f *vertices = (glhckVertexData3f*) object->geometry->vertices;
   for (i = 0; i + 2 < object->geometry->vertexCount; i += skipCount) {
      int ix[3] = {0, 1, 2};
      if (skipCount == 1 && (i % 2)) {
         ix[0] = 1;
         ix[1] = 0;
         ix[2] = 2;
      }

      const kmTriangle tri = {
         {vertices[i+ix[0]].vertex.x, vertices[i+ix[0]].vertex.y, vertices[i+ix[0]].vertex.z},
         {vertices[i+ix[1]].vertex.x, vertices[i+ix[1]].vertex.y, vertices[i+ix[1]].vertex.z},
         {vertices[i+ix[2]].vertex.x, vertices[i+ix[2]].vertex.y, vertices[i+ix[2]].vertex.z}
      };

      kmVec3 intersection;
      kmScalar ss, tt;
      if (kmRay3IntersectTriangle(&r, &tri, &intersection, &ss, &tt)) {
         kmVec3 delta;
         kmVec3Subtract(&delta, &ray->start, &intersection);
         kmScalar distanceSq = kmVec3LengthSq(&delta);
         if (!intersectionFound || distanceSq < closestDistanceSq) {
            intersectionFound = 1;
            closestDistanceSq = distanceSq;
            intersectionIndex = i;
            s = ss;
            t = tt;
            memcpy(fix, ix, sizeof(fix));
         }
      }
   }

   /* determine texture coordinates of the intersection point */
   if (intersectionFound) {
      const kmVec2 texCoods[] = {
         {vertices[intersectionIndex+fix[0]].coord.x, vertices[intersectionIndex+fix[0]].coord.y},
         {vertices[intersectionIndex+fix[1]].coord.x, vertices[intersectionIndex+fix[1]].coord.y},
         {vertices[intersectionIndex+fix[2]].coord.x, vertices[intersectionIndex+fix[2]].coord.y},
      };

      kmVec2 u, v;
      kmVec2Subtract(&u, &texCoods[1], &texCoods[0]);
      kmVec2Subtract(&v, &texCoods[2], &texCoods[0]);
      kmVec2Scale(&u, &u, s);
      kmVec2Scale(&v, &v, t);
      memcpy(outCoords, &texCoods[0], sizeof(kmVec2));
      kmVec2Add(outCoords, outCoords, &u);
      kmVec2Add(outCoords, outCoords, &v);
   }

   return intersectionFound;
}

/* vim: set ts=8 sw=3 tw=0 :*/
