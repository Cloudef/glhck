#include <stdlib.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATION

enum pool {
   $rotations, // list handle (glhckAnimationQuaternionKey)
   $translations, // list handle (glhckAnimationVectorKey)
   $scalings, // list handle (glhckAnimationVectorKey)
   $boneName, // string handle
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // rotations
   sizeof(glhckHandle), // translations
   sizeof(glhckHandle), // sclaings
   sizeof(glhckHandle), // boneName
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckAnimationNodeBoneName(handle, NULL);
   glhckAnimationNodeInsertTranslations(handle, NULL, 0);
   glhckAnimationNodeInsertRotations(handle, NULL, 0);
   glhckAnimationNodeInsertScalings(handle, NULL, 0);
}

/* \brief allocate new key animation node object */
GLHCKAPI glhckHandle glhckAnimationNodeNew(void)
{
   TRACE(0);
   const glhckHandle handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_ANIMATION_NODE, pools, pool_sizes, POOL_LAST, destructor);
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;
}

/* \brief set bone index to animation node */
GLHCKAPI void glhckAnimationNodeBoneName(const glhckHandle handle, const char *name)
{
   setCStr($boneName, handle, name);
}

/* \brief return bone index from animation node */
GLHCKAPI const char* glhckAnimationNodeGetBoneName(const glhckHandle handle)
{
   return getCStr($boneName, handle);
}

/* \brief set translations to key animation node */
GLHCKAPI int glhckAnimationNodeInsertTranslations(const glhckHandle handle, const glhckAnimationVectorKey *keys, const size_t memb)
{
   return setList($translations, handle, keys, memb, sizeof(glhckAnimationVectorKey));
}

/* \brief return animation's translation keys */
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeTranslations(const glhckHandle handle, size_t *outMemb)
{
   return getList($translations, handle, outMemb);
}

/* \brief set scalings to key animation node */
GLHCKAPI int glhckAnimationNodeInsertScalings(const glhckHandle handle, const glhckAnimationVectorKey *keys, const size_t memb)
{
   return setList($scalings, handle, keys, memb, sizeof(glhckAnimationVectorKey));
}

/* \brief return animation's scaling keys */
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeScalings(const glhckHandle handle, size_t *outMemb)
{
   return getList($scalings, handle, outMemb);
}

/* \brief set rotations to key animation node */
GLHCKAPI int glhckAnimationNodeInsertRotations(const glhckHandle handle, const glhckAnimationQuaternionKey *keys, const size_t memb)
{
   return setList($rotations, handle, keys, memb, sizeof(glhckAnimationQuaternionKey));
}

/* \brief return animation's rotation keys */
GLHCKAPI const glhckAnimationQuaternionKey* glhckAnimationNodeRotations(const glhckHandle handle, size_t *outMemb)
{
   return getList($rotations, handle, outMemb);
}

/* vim: set ts=8 sw=3 tw=0 :*/
