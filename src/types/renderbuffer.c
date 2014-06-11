#include <glhck/glhck.h>

#include "system/tls.h"
#include "renderers/render.h"
#include "trace.h"
#include "handle.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDERBUFFER

struct dimensions {
   int width, height;
};

enum pool {
   $dimensions, // struct dimensions
   $object, // unsigned int
   $format, // glhckTextureFormat
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(struct dimensions), // rect
   sizeof(unsigned int), // object
   sizeof(glhckTextureFormat), // target
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];
static _GLHCK_TLS glhckHandle active;

#include "handlefun.h"

unsigned int _glhckRenderbufferGetObject(const glhckHandle handle)
{
   return *(unsigned int*)get($object, handle);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   unsigned int *object = (unsigned int*)get($object, handle);
   if (*object)
      _glhckRenderGetAPI()->framebufferDelete(1, object);
}

GLHCKAPI glhckHandle glhckRenderbufferNew(const int width, const int height, const glhckTextureFormat format)
{
   TRACE(0);

   /* generate renderbuffer */
   unsigned int object = 0;
   _glhckRenderGetAPI()->renderbufferGenerate(1, &object);

   if (!object)
      goto fail;

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_RENDERBUFFER, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   const glhckHandle old = active;
   glhckRenderbufferBind(object);
   _glhckRenderGetAPI()->renderbufferStorage(width, height, format);
   glhckRenderbufferBind(old);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   if (object) _glhckRenderGetAPI()->renderbufferDelete(1, &object);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI glhckHandle glhckRenderbufferCurrent(void)
{
   return active;
}

GLHCKAPI void glhckRenderbufferBind(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));

   if (active == handle)
      return;

   _glhckRenderGetAPI()->renderbufferBind(handle);
   active = handle;
}

/* vim: set ts=8 sw=3 tw=0 :*/
