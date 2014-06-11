#include <stdlib.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

#include "types/geometry.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_SKINBONE

static const kmMat4 identity = {
   .mat = {
      1.0f,  0.0f, 0.0f, 0.0f,
      0.0f,  1.0f, 0.0f, 0.0f,
      0.0f,  0.0f, 1.0f, 0.0f,
      0.0f,  0.0f, 0.0f, 1.0f,
   }
};

enum pool {
   $offsetMatrix, // kmMat4
   $weights, // list handle (glhckVertexWeight)
   $bone, // glhckHandle
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(kmMat4), // offsetMatrix
   sizeof(glhckHandle), // weights
   sizeof(glhckHandle), // bone
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

/* \brief update bone structure's transformed matrices */
static void updateBones(const glhckGeometry *geometry, const glhckHandle *bones, const size_t memb)
{
   /* 1. Transform bone to 0,0,0 using offset matrix so we can transform it locally
    * 2. Apply all transformations from parent bones
    * 3. We'll end up back to bone space with the transformed matrix */

   kmMat4 bias, biasinv, scale, scaleinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (size_t i = 0; i < memb; ++i) {
      kmMat4 *transformed = (kmMat4*)glhckBoneGetTransformedMatrix(bones[i]);
      memcpy(transformed, glhckBoneGetTransformationMatrix(bones[i]), sizeof(kmMat4));

      const glhckHandle parent = glhckBoneGetParentBone(bones[i]);
      if (parent)
         kmMat4Multiply(transformed, glhckBoneGetTransformedMatrix(parent), transformed);

      kmMat4 transformedMatrix, offsetMatrix;
      kmMat4 *poseMatrix = (kmMat4*)glhckBoneGetPoseMatrix(bones[i]);
      kmMat4Multiply(&transformedMatrix, &biasinv, transformed);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, glhckBoneGetOffsetMatrix(bones[i]), &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(poseMatrix, &transformedMatrix, &offsetMatrix);
   }
}

/* \brief transform object with its skin bones */
void _glhckSkinBoneTransformObject(glhckHandle object, int dirty)
{
#if 0
   /* we are root, perform this transform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, _glhckSkinBoneTransformObject, updateBones);
#endif

   const glhckHandle geometry = glhckObjectGetGeometry(object);
   if (!geometry)
      return;

   size_t numBones = 0;
   const glhckHandle *bones = glhckObjectBones(object, &numBones);
   glhckGeometry *data = (glhckGeometry*)glhckGeometryGetStruct(geometry);

   if (dirty)
      updateBones(data, bones, numBones);

   size_t numSkinBones = 0;
   const glhckHandle *skinBones = glhckObjectSkinBones(object, &numSkinBones);

   if (!skinBones)
      return;

   const struct glhckPose *pose = glhckGeometryGetPose(geometry);
   assert(pose->zero && pose->bind);
   const struct glhckVertexType *type = _glhckGeometryGetVertexType(data->vertexType);
   memcpy(data->vertices, pose->zero, data->vertexCount * type->size);

   /* type api is public, so convert to C Array */
   type->api.transform(data, pose->bind, skinBones, numSkinBones);

#if 0
   /* update bounding box for object */
   glhckGeometryCalculateBB(object->geometry, &object->view.bounding);
   _glhckObjectUpdateBoxes(object);
#endif
}

/* \brief free skin bone object */
static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckSkinBoneInsertWeights(handle, NULL, 0);
   glhckSkinBoneBone(handle, 0);
}

/* \brief allocate new skin bone object */
GLHCKAPI glhckHandle glhckSkinBoneNew(void)
{
   TRACE(0);

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_BONE, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   set($offsetMatrix, handle, &identity);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(handle));
   return 0;
}

/* \brief set pointer to real bone from skinned bone */
GLHCKAPI void glhckSkinBoneBone(const glhckHandle handle, const glhckHandle bone)
{
   setHandle($bone, handle, bone);
}

/* \brief return pointer to real bone */
GLHCKAPI glhckHandle glhckSkinBoneGetBone(const glhckHandle handle)
{
   return *(glhckHandle*)get($bone, handle);
}

/* \brief get name of skin bone (needs real bone) */
GLHCKAPI const char* glhckSkinBoneGetName(const glhckHandle handle)
{
   const glhckHandle bone = glhckSkinBoneGetBone(handle);
   return (bone ? glhckBoneGetName(bone) : NULL);
}

/* \brief set weights to skin bone */
GLHCKAPI int glhckSkinBoneInsertWeights(const glhckHandle handle, const glhckVertexWeight *weights, const size_t memb)
{
   return setList($weights, handle, weights, memb, sizeof(glhckVertexWeight));
}

/* \brief get weights from skin bone */
GLHCKAPI const glhckVertexWeight* glhckSkinBoneWeights(const glhckHandle handle, size_t *outMemb)
{
   return getList($weights, handle, outMemb);
}

/* \brief set offset matrix to skin bone */
GLHCKAPI void glhckSkinBoneOffsetMatrix(const glhckHandle handle, const kmMat4 *offsetMatrix)
{
   set($offsetMatrix, handle, offsetMatrix);
}

/* \brief get offset matrix from skin bone */
GLHCKAPI const kmMat4* glhckSkinBoneGetOffsetMatrix(const glhckHandle handle)
{
   return get($offsetMatrix, handle);
}

/* vim: set ts=8 sw=3 tw=0 :*/
