#include <stdlib.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATION

enum pool {
   $name, // string handle
   $nodes, // list handle
   $ticksPerSecond, // kmScalar
   $duration, // kmScalar
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // name
   sizeof(glhckHandle), // nodes
   sizeof(kmScalar), // ticksPerSecond
   sizeof(kmScalar), // duration
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckAnimationName(handle, NULL);
   glhckAnimationInsertNodes(handle, NULL, 0);
}

GLHCKAPI glhckHandle glhckAnimationNew(void)
{
   TRACE(0);

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_ANIMATION, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   glhckAnimationTicksPerSecond(handle, 1.0f);
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI void glhckAnimationName(const glhckHandle handle, const char *name)
{
   setCStr($name, handle, name);
}

GLHCKAPI const char* glhckAnimationGetName(const glhckHandle handle)
{
   return getCStr($name, handle);
}

GLHCKAPI void glhckAnimationDuration(const glhckHandle handle, const kmScalar duration)
{
   set($duration, handle, &duration);
}

GLHCKAPI kmScalar glhckAnimationGetDuration(const glhckHandle handle)
{
   return *(kmScalar*)get($duration, handle);
}

GLHCKAPI void glhckAnimationTicksPerSecond(const glhckHandle handle, const kmScalar ticksPerSecond)
{
   set($ticksPerSecond, handle, &ticksPerSecond);
}

GLHCKAPI kmScalar glhckAnimationGetTicksPerSecond(const glhckHandle handle)
{
   return *(kmScalar*)get($ticksPerSecond, handle);
}

GLHCKAPI int glhckAnimationInsertNodes(const glhckHandle handle, const glhckHandle *nodes, const size_t memb)
{
   return setListHandles($nodes, handle, nodes, memb);
}

GLHCKAPI const glhckHandle* glhckAnimationNodes(const glhckHandle handle, size_t *outMemb)
{
   return getList($nodes, handle, outMemb);
}

/* vim: set ts=8 sw=3 tw=0 :*/
