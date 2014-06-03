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
   $parent, // bone handle
   $name, // string handle
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(kmMat4), // transformationMatrix
   sizeof(kmMat4), // transformedMatrix
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
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_BONE, pools, pool_sizes, POOL_LAST, destructor, NULL)))
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
   set($transformedMatrix, handle, transformationMatrix);
}

/* \brief get transformation matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(const glhckHandle handle)
{
   return get($transformationMatrix, handle);
}

/* \brief get transformed matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(const glhckHandle handle)
{
   return get($transformedMatrix, handle);
}

/* \brief get relative position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionRelativeOnObject(const glhckHandle handle, const glhckHandle object, kmVec3 *outPosition)
{
   CALL(2, "%s, %s, %p", glhckHandleRepr(handle), glhckHandleRepr(object), outPosition);
   assert(handle > 0 && object > 0 && outPosition);
#if 0
   outPosition->x = object->transformedMatrix.mat[12] * gobject->view.scaling.x;
   outPosition->y = object->transformedMatrix.mat[13] * gobject->view.scaling.y;
   outPosition->z = object->transformedMatrix.mat[14] * gobject->view.scaling.z;
#endif
}

/* \brief get absolute position of transformed bone in object */
GLHCKAPI void glhckBoneGetPositionAbsoluteOnObject(const glhckHandle handle, const glhckHandle object, kmVec3 *outPosition)
{
   CALL(2, "%s, %s, %p", glhckHandleRepr(handle), glhckHandleRepr(object), outPosition);
   assert(handle > 0 && object > 0 && outPosition);

#if 0
   kmMat4 matrix;
   kmMat4Multiply(&matrix, &gobject->view.matrix, &object->transformedMatrix);
   outPosition->x = matrix.mat[12];
   outPosition->y = matrix.mat[13];
   outPosition->z = matrix.mat[14];
#endif
}

/* vim: set ts=8 sw=3 tw=0 :*/
