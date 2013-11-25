#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_SKINBONE

#define PERFORM_ON_CHILDS(parent, function, ...) \
   { unsigned int _cbc_;                                   \
   for (_cbc_ = 0; _cbc_ != parent->numChilds; ++_cbc_)    \
      function(parent->childs[_cbc_], ##__VA_ARGS__); }

/* \brief update bone structure's transformed matrices */
static void _glhckSkinBoneUpdateBones(glhckSkinBone **skinBones, unsigned int memb)
{
   glhckBone *bone, *parent;
   unsigned int n;
   assert(skinBones);

   /* 1. Transform bone to 0,0,0 using offset matrix so we can transform it locally
    * 2. Apply all transformations from parent bones
    * 3. We'll end up back to bone space with the transformed matrix */

   for (n = 0; n != memb; ++n) {
      if (!(bone = skinBones[n]->bone)) continue;
      memcpy(&bone->transformedMatrix, &bone->transformationMatrix, sizeof(kmMat4));
      for (parent = bone->parent; parent; parent = parent->parent)
         kmMat4Multiply(&bone->transformedMatrix, &parent->transformationMatrix, &bone->transformedMatrix);
   }
}

/* \brief transform object with its skin bones */
void _glhckSkinBoneTransformObject(glhckObject *object, int updateBones)
{
   /* we are root, perform this transform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, _glhckSkinBoneTransformObject, updateBones);
   }

   /* ah, we can't do this ;_; */
   if (!object->geometry || !object->skinBones) return;

   /* update bones, if requested */
   if (updateBones) _glhckSkinBoneUpdateBones(object->skinBones, object->numSkinBones);

   assert(object->zero && object->bind);
   __GLHCKvertexType *type = GLHCKVT(object->geometry->vertexType);
   memcpy(object->geometry->vertices, object->zero, object->geometry->vertexCount * type->size);
   type->api.transform(object->geometry, object->bind, object->skinBones, object->numSkinBones);

   /* update bounding box for object */
   glhckGeometryCalculateBB(object->geometry, &object->view.bounding);
   _glhckObjectUpdateBoxes(object);
}

/* \brief allocate new skin bone object */
GLHCKAPI glhckSkinBone* glhckSkinBoneNew(void)
{
   glhckSkinBone *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckSkinBone))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* identity matrices */
   kmMat4Identity(&object->offsetMatrix);

   /* insert to world */
   _glhckWorldInsert(skinBone, object, glhckSkinBone*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference skin bone object */
GLHCKAPI glhckSkinBone* glhckSkinBoneRef(glhckSkinBone *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free skin bone object */
GLHCKAPI unsigned int glhckSkinBoneFree(glhckSkinBone *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free the weights */
   glhckSkinBoneInsertWeights(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(skinBone, object, glhckSkinBone*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set pointer to real bone from skinned bone */
GLHCKAPI void glhckSkinBoneBone(glhckSkinBone *object, glhckBone *bone)
{
   CALL(0, "%p, %p", object, bone);
   assert(object);
   object->bone = bone;
}

/* \brief return pointer to real bone */
GLHCKAPI glhckBone* glhckSkinBoneGetBone(glhckSkinBone *object)
{
   CALL(0, "%p", object);
   assert(object);
   RET(0, "%p", object->bone);
   return object->bone;
}

/* \brief get name of skin bone (needs real bone) */
GLHCKAPI const char* glhckSkinBoneGetName(glhckSkinBone *object)
{
   if (object->bone) return glhckBoneGetName(object->bone);
   return NULL;
}

/* \brief set weights to skin bone */
GLHCKAPI int glhckSkinBoneInsertWeights(glhckSkinBone *object, const glhckVertexWeight *weights, unsigned int memb)
{
   glhckVertexWeight *weightsCopy = NULL;
   CALL(0, "%p, %p, %u", object, weights, memb);
   assert(object);

   /* copy weights, if they exist */
   if (weights && !(weightsCopy = _glhckCopy(weights, memb * sizeof(glhckVertexWeight))))
      goto fail;

   IFDO(_glhckFree, object->weights);
   object->weights = weightsCopy;
   object->numWeights = (weightsCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief get weights from skin bone */
GLHCKAPI const glhckVertexWeight* glhckSkinBoneWeights(glhckSkinBone *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numWeights;
   RET(2, "%p", object->weights);
   return object->weights;
}

/* \brief set offset matrix to skin bone */
GLHCKAPI void glhckSkinBoneOffsetMatrix(glhckSkinBone *object, const kmMat4 *offsetMatrix)
{
   CALL(2, "%p, %p", object, offsetMatrix);
   assert(object && offsetMatrix);
   memcpy(&object->offsetMatrix, offsetMatrix, sizeof(kmMat4));
}

/* \brief get offset matrix from skin bone */
GLHCKAPI const kmMat4* glhckSkinBoneGetOffsetMatrix(glhckSkinBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->offsetMatrix);
   return &object->offsetMatrix;
}

/* vim: set ts=8 sw=3 tw=0 :*/
