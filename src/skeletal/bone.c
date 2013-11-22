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

   /* identity matrices */
   kmMat4Identity(&object->transformedMatrix);
   kmMat4Identity(&object->transformationMatrix);

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
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free bone object */
GLHCKAPI unsigned int glhckBoneFree(glhckBone *object)
{
   glhckBone *b;
   glhckSkinBone *sb;

   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free the name */
   IFDO(_glhckFree, object->name);

   /* get rid of uncounted references */
   for (b = GLHCKW()->bone; b; b = b->next)
      if (b->parent == object) b->parent = NULL;
   for (sb = GLHCKW()->skinBone; sb; sb = sb->next)
      if (sb->bone == object) sb->bone = NULL;

   /* remove from world */
   _glhckWorldRemove(bone, object, glhckBone*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
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

/* \brief set parent bone to this bone */
GLHCKAPI void glhckBoneParentBone(glhckBone *object, glhckBone *parentBone)
{
   CALL(0, "%p, %p", object, parentBone);
   assert(object);
   object->parent = parentBone;
}

/* \brief return parent bone index of this bone */
GLHCKAPI glhckBone* glhckBoneGetParentBone(glhckBone *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->parent);
   return object->parent;
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

/* \brief get relative position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionRelativeOnObject(glhckBone *object, glhckObject *gobject, kmVec3 *outPosition)
{
   CALL(2, "%p, %p, %p", object, gobject, outPosition);
   assert(object && gobject && outPosition);
   outPosition->x = object->transformedMatrix.mat[12] * gobject->view.scaling.x;
   outPosition->y = object->transformedMatrix.mat[13] * gobject->view.scaling.y;
   outPosition->z = object->transformedMatrix.mat[14] * gobject->view.scaling.z;
}

/* \brief get absolute position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionAbsoluteOnObject(glhckBone *object, glhckObject *gobject, kmVec3 *outPosition)
{
   kmMat4 matrix;
   CALL(2, "%p, %p, %p", object, gobject, outPosition);
   assert(object && gobject && outPosition);
   kmMat4Multiply(&matrix, &gobject->view.matrix, &object->transformedMatrix);
   outPosition->x = matrix.mat[12];
   outPosition->y = matrix.mat[13];
   outPosition->z = matrix.mat[14];
}

/* vim: set ts=8 sw=3 tw=0 :*/
