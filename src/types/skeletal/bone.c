#include <stdlib.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_BONE

static const kmMat4 identity = {
   .mat = {
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f,  1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 1.0f, 0.0f,
      0.0f,  0.0f, 0.0f, 1.0f,
   }
};

enum pool {
   $transformationMatrix, // kmMat4
   $transformedMatrix, // kmMat4
   $offsetMatrix, // kmMat4
   $poseMatrix, // kmMat4
   $parent, // bone handle
   $name, // string handle
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(kmMat4), // transformationMatrix
   sizeof(kmMat4), // transformedMatrix
   sizeof(kmMat4), // offsetMatrix
   sizeof(kmMat4), // poseMatrix
   sizeof(glhckHandle), // parent
   sizeof(glhckHandle), // name
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckBoneName(handle, NULL);
   glhckBoneParentBone(handle, 0);
}

/* \brief allocate new bone object */
GLHCKAPI glhckHandle glhckBoneNew(void)
{
   TRACE(0);

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_BONE, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   set($transformedMatrix, handle, &identity);
   set($transformationMatrix, handle, &identity);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

/* \brief set bone name */
GLHCKAPI void glhckBoneName(const glhckHandle handle, const char *name)
{
   setCStr($name, handle, name);
}

/* \brief get bone name */
GLHCKAPI const char* glhckBoneGetName(const glhckHandle handle)
{
   return getCStr($name, handle);
}

/* \brief set parent bone to this bone */
GLHCKAPI void glhckBoneParentBone(const glhckHandle handle, const glhckHandle parent)
{
   assert(handle != parent);
   setHandle($parent, handle, parent);
}

/* \brief return parent bone index of this bone */
GLHCKAPI glhckHandle glhckBoneGetParentBone(const glhckHandle handle)
{
   return *(glhckHandle*)get($parent, handle);
}

/* \brief set transformation matrix to bone */
GLHCKAPI void glhckBoneTransformationMatrix(const glhckHandle handle, const kmMat4 *transformationMatrix)
{
   set($transformationMatrix, handle, transformationMatrix);
}

/* \brief get transformation matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(const glhckHandle handle)
{
   return get($transformationMatrix, handle);
}

GLHCKAPI void glhckBoneOffsetMatrix(const glhckHandle handle, const kmMat4 *offsetMatrix)
{
   set($offsetMatrix, handle, offsetMatrix);
}

GLHCKAPI const kmMat4* glhckBoneGetOffsetMatrix(const glhckHandle handle)
{
   return get($offsetMatrix, handle);
}

/* \brief get transformed matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(const glhckHandle handle)
{
   return get($transformedMatrix, handle);
}

GLHCKAPI const kmMat4* glhckBoneGetPoseMatrix(const glhckHandle handle)
{
   return get($poseMatrix, handle);
}

/* \brief get relative position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionRelativeOnObject(const glhckHandle handle, const glhckHandle object, kmVec3 *outPosition)
{
   CALL(2, "%s, %s, %p", glhckHandleRepr(handle), glhckHandleRepr(object), outPosition);
   assert(handle > 0 && object > 0 && outPosition);

   const kmMat4 *transformed = glhckBoneGetTransformedMatrix(handle);
   const kmVec3 *scaling = glhckObjectGetScale(object);
   outPosition->x = transformed->mat[12] * scaling->x;
   outPosition->y = transformed->mat[13] * scaling->y;
   outPosition->z = transformed->mat[14] * scaling->z;
}

/* \brief get absolute position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionAbsoluteOnObject(const glhckHandle handle, const glhckHandle object, kmVec3 *outPosition)
{
   CALL(2, "%s, %s, %p", glhckHandleRepr(handle), glhckHandleRepr(object), outPosition);
   assert(handle > 0 && object > 0 && outPosition);

   kmMat4 matrix;
   const kmMat4 *modelMatrix = glhckObjectGetMatrix(object);
   kmMat4Multiply(&matrix, modelMatrix, glhckBoneGetTransformedMatrix(handle));
   outPosition->x = matrix.mat[12];
   outPosition->y = matrix.mat[13];
   outPosition->z = matrix.mat[14];
}

/* vim: set ts=8 sw=3 tw=0 :*/
