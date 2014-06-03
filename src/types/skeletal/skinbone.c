#include <stdlib.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

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
static void _glhckSkinBoneUpdateBones(const glhckHandle *skinBones, const size_t memb)
{
   /* 1. Transform bone to 0,0,0 using offset matrix so we can transform it locally
    * 2. Apply all transformations from parent bones
    * 3. We'll end up back to bone space with the transformed matrix */

   for (size_t i = 0; i < memb; ++i) {
      glhckHandle bone;
      if (!(bone = glhckSkinBoneGetBone(skinBones[i])))
         continue;

      kmMat4 transformed;
      memcpy(&transformed, glhckBoneGetTransformationMatrix(bone), sizeof(kmMat4));
      for (glhckHandle parent = glhckBoneGetParentBone(bone); parent; parent = glhckBoneGetParentBone(parent))
         kmMat4Multiply(&transformed, glhckBoneGetTransformationMatrix(parent), &transformed);
   }
}

#if 0
/* \brief transform object with its skin bones */
void _glhckSkinBoneTransformObject(glhckHandle object, int updateBones)
{
   /* we are root, perform this transform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT && object->childs)
      chckArrayIterCall(object->childs, _glhckSkinBoneTransformObject, updateBones);

   /* ah, we can't do this ;_; */
   if (!object->geometry || !object->skinBones)
      return;

   /* update bones, if requested */
   if (updateBones)
      _glhckSkinBoneUpdateBones(object->skinBones);

   assert(object->zero && object->bind);
   __GLHCKvertexType *type = GLHCKVT(object->geometry->vertexType);
   memcpy(object->geometry->vertices, object->zero, object->geometry->vertexCount * type->size);

   /* type api is public, so convert to C Array */
   size_t memb;
   glhckSkinBone **skinBones = chckArrayToCArray(object->skinBones, &memb);
   type->api.transform(object->geometry, object->bind, skinBones, memb);

   /* update bounding box for object */
   glhckGeometryCalculateBB(object->geometry, &object->view.bounding);
   _glhckObjectUpdateBoxes(object);
}
#endif

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
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_BONE, pools, pool_sizes, POOL_LAST, destructor, NULL)))
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
