#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_BONE

/* \brief allocate new bone object */
GLHCKAPI glhckBone* glhckBoneNew(void)
{
   glhckBone *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckBone))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(bone, object, glhckBone*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference bone object */
GLHCKAPI glhckBone* glhckBoneRef(glhckBone *object)
{
   CALL(3, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free bone object */
GLHCKAPI unsigned int glhckBoneFree(glhckBone *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free the name */
   IFDO(_glhckFree, object->name);

   /* free the weights */
   glhckBoneInsertWeights(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(bone, object, glhckBone*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set bone name */
GLHCKAPI void glhckBoneName(glhckBone *object, const char *name)
{
   char *nameCopy = NULL;
   CALL(2, "%p, %s", object, name);

   if (name && !(nameCopy = _glhckStrdup(name)))
      return;

   IFDO(_glhckFree, object->name);
   object->name = nameCopy;
}

/* \brief get bone name */
GLHCKAPI const char* glhckBoneGetName(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%s", object->name);
   return object->name;
}

/* \brief set weights to bone */
GLHCKAPI int glhckBoneInsertWeights(glhckBone *object, const glhckVertexWeight *weights, unsigned int memb)
{
   glhckVertexWeight *weightsCopy = NULL;
   CALL(0, "%p, %p, %zu", object, weights, memb);
   assert(object);

   /* copy weights, if they exist */
   if (weights && !(weightsCopy = _glhckCopy(weights, memb)))
      goto fail;

   IFDO(_glhckFree, object->weights);
   object->weights = weightsCopy;
   object->numWeights = (weightsCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief get weights from bone */
GLHCKAPI const glhckVertexWeight* glhckBoneWeights(glhckBone *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numWeights;
   RET(2, "%p", object->weights);
   return object->weights;
}

/* \brief add children to this bone */
GLHCKAPI void glhckBoneAddChildren(glhckBone *object, glhckBone *child)
{
   CALL(0, "%p, %p", object, child);
   assert(object && child);
   assert(object != child);
   child->parent = object;
}

/* \brief return parent of this bone */
GLHCKAPI glhckBone* glhckBoneParent(glhckBone *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->parent);
   return object->parent;
}

/* \brief set offset matrix to bone */
GLHCKAPI void glhckBoneOffsetMatrix(glhckBone *object, const kmMat4 *offsetMatrix)
{
   CALL(2, "%p, %p", object, offsetMatrix);
   assert(object && offsetMatrix);
   memcpy(&object->offsetMatrix, offsetMatrix, sizeof(kmMat4));
   memcpy(&object->transformedMatrix, offsetMatrix, sizeof(kmMat4));
}

/* \brief get offset matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetOffsetMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->offsetMatrix);
   return &object->offsetMatrix;
}

/* \brief set transformation matrix to bone */
GLHCKAPI void glhckBoneTransformationMatrix(glhckBone *object, const kmMat4 *transformationMatrix)
{
   CALL(2, "%p, %p", object, transformationMatrix);
   assert(object && transformationMatrix);
   memcpy(&object->transformationMatrix, transformationMatrix, sizeof(kmMat4));
}

/* \brief get transformation matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->transformationMatrix);
   return &object->transformationMatrix;
}

/* \brief get transformed matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->transformedMatrix);
   return &object->transformedMatrix;
}

/* vim: set ts=8 sw=3 tw=0 :*/
