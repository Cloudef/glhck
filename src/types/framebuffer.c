#include <glhck/glhck.h>

#include "system/tls.h"
#include "renderbuffer.h"
#include "texture.h"
#include "renderers/render.h"
#include "handle.h"
#include "trace.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_FRAMEBUFFER

enum pool {
   $rect, // glhckRect
   $object, // unsigned int
   $target, // glhckFramebufferTarget
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(glhckRect), // rect
   sizeof(unsigned int), // object
   sizeof(glhckFramebufferTarget), // target
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];
static _GLHCK_TLS glhckHandle active[GLHCK_FRAMEBUFFER_TYPE_LAST];

#include "handlefun.h"

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckFramebufferTarget target = glhckFramebufferGetTarget(handle);
   if (active[target] == handle)
      glhckFramebufferUnbind(target);

   unsigned int *object = (unsigned int*)get($object, handle);
   _glhckRenderGetAPI()->framebufferDelete(1, object);
}

/* ---- PUBLIC API ---- */

/* \brief create new framebuffer */
GLHCKAPI glhckHandle glhckFramebufferNew(const glhckFramebufferTarget target)
{
   TRACE(0);

   /* generate framebuffer */
   unsigned int object = 0;
   _glhckRenderGetAPI()->framebufferGenerate(1, &object);

   if (!object)
      goto fail;

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_FRAMEBUFFER, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

#if 0
   /* GLES1 && GLES2 don't have DRAW/READ_FRAMEBUFFER */
   /* FIXME: force this on renderer */
   if (target != GLHCK_FRAMEBUFFER) target = GLHCK_FRAMEBUFFER;
#endif

   set($target, handle, &target);
   set($object, handle, &object);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   if (object) _glhckRenderGetAPI()->framebufferDelete(1, &object);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief get current bound framebuffer for target */
GLHCKAPI glhckHandle glhckFramebufferCurrentForTarget(const glhckFramebufferTarget target)
{
   CALL(1, "%u", target);
   RET(1, "%s", glhckHandleRepr(active[target]));
   return active[target];
}

GLHCKAPI glhckFramebufferTarget glhckFramebufferGetTarget(const glhckHandle handle)
{
   return *(glhckFramebufferTarget*)get($target, handle);
}

/* \brief bind hardware buffer */
GLHCKAPI void glhckFramebufferBind(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckFramebufferTarget target = glhckFramebufferGetTarget(handle);
   if (active[target] == handle)
      return;

   const unsigned int object = *(unsigned int*)get($object, handle);
   _glhckRenderGetAPI()->framebufferBind(target, object);
   active[target] = handle;
}

/* \brief unbind hardware buffer from target type slot */
GLHCKAPI void glhckFramebufferUnbind(glhckFramebufferTarget target)
{
   CALL(2, "%u", target);

   if (!active[target])
      return;

   _glhckRenderGetAPI()->framebufferBind(target, 0);
   active[target] = 0;
}

/* \brief begin render with the fbo */
GLHCKAPI void glhckFramebufferBegin(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckRect *rect = glhckFramebufferGetRect(handle);
   glhckRenderStatePush();
   glhckFramebufferBind(handle);
   glhckRenderResize(rect->w, rect->w);
   glhckRenderViewport(rect);
}

/* \brief end rendering with the fbo */
GLHCKAPI void glhckFramebufferEnd(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckFramebufferTarget target = glhckFramebufferGetTarget(handle);
   glhckFramebufferUnbind(target);
   glhckRenderStatePop();
}

/* \brief set framebuffer's visible area */
GLHCKAPI void glhckFramebufferRect(const glhckHandle handle, const glhckRect *rect)
{
   set($rect, handle, rect);
}

/* \brief set framebuffer's visible area (int) */
GLHCKAPI void glhckFramebufferRecti(const glhckHandle handle, const int x, const int y, const int w, const int h)
{
   const glhckRect rect = { x, y, w, h};
   glhckFramebufferRect(handle, &rect);
}

GLHCKAPI const glhckRect* glhckFramebufferGetRect(const glhckHandle handle)
{
   return get($rect, handle);
}

/* \brief attach texture to framebuffer */
GLHCKAPI int glhckFramebufferAttachTexture(const glhckHandle handle, const glhckHandle texture, const glhckFramebufferAttachmentType attachment)
{
   CALL(1, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckFramebufferTarget target = glhckFramebufferGetTarget(handle);
   const glhckHandle old = glhckFramebufferCurrentForTarget(target);

   glhckFramebufferBind(handle);
   const int ret = _glhckRenderGetAPI()->framebufferTexture(target, glhckTextureGetTarget(texture), _glhckTextureGetObject(texture), attachment);

   if (old) {
      glhckFramebufferBind(old);
   } else {
      glhckFramebufferUnbind(target);
   }

   RET(1, "%d", ret);
   return ret;
}

/* \brief attach renderbuffer to framebuffer */
GLHCKAPI int glhckFramebufferAttachRenderbuffer(const glhckHandle handle, const glhckHandle renderbuffer, const glhckFramebufferAttachmentType attachment)
{
   CALL(1, "%s, %s, %u", glhckHandleRepr(handle), glhckHandleRepr(renderbuffer), attachment);
   assert(handle > 0);

   const glhckFramebufferTarget target = glhckFramebufferGetTarget(handle);
   const glhckHandle oldfb = glhckFramebufferCurrentForTarget(target);
   const glhckHandle oldrb = glhckRenderbufferCurrent();

   glhckFramebufferBind(handle);
   glhckRenderbufferBind(renderbuffer);
   const int ret = _glhckRenderGetAPI()->framebufferRenderbuffer(target, attachment,  _glhckRenderbufferGetObject(renderbuffer));
   glhckRenderbufferBind(oldrb);

   if (oldfb) {
      glhckFramebufferBind(oldfb);
   } else {
      glhckFramebufferUnbind(target);
   }

   RET(1, "%d", ret);
   return ret;
}

/* vim: set ts=8 sw=3 tw=0 :*/
