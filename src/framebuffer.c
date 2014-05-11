#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_FRAMEBUFFER

/* ---- PUBLIC API ---- */

/* \brief create new framebuffer */
GLHCKAPI glhckFramebuffer* glhckFramebufferNew(glhckFramebufferTarget target)
{
   glhckFramebuffer *object = NULL;
   TRACE(0);

   /* generate framebuffer */
   unsigned int obj;
   GLHCKRA()->framebufferGenerate(1, &obj);

   if (!obj)
      goto fail;

   if (!(object = _glhckCalloc(1, sizeof(glhckFramebuffer))))
      goto fail;

   /* increase reference */
   object->refCounter++;

#if GLHCK_USE_GLES1 || GLHCK_USE_GLES2
   /* GLES1 && GLES2 don't have DRAW/READ_FRAMEBUFFER */
   if (target != GLHCK_FRAMEBUFFER) target = GLHCK_FRAMEBUFFER;
#endif

   /* init */
   object->object = obj;
   object->target = target;

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->framebuffers, object);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference framebuffer object */
GLHCKAPI glhckFramebuffer* glhckFramebufferRef(glhckFramebuffer *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free framebuffer object */
GLHCKAPI unsigned int glhckFramebufferFree(glhckFramebuffer *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* unbind from active slot */
   if (glhckFramebufferCurrentForTarget(object->target) == object)
      glhckFramebufferUnbind(object->target);

   /* delete framebuffer object */
   GLHCKRA()->framebufferDelete(1, &object->object);

   /* remove from world */
   _glhckWorldRemove(&GLHCKW()->framebuffers, object);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", (object ? object->refCounter : 0));
   return (object ? object->refCounter : 0);
}

/* \brief get current bound framebuffer for target */
GLHCKAPI glhckFramebuffer* glhckFramebufferCurrentForTarget(glhckFramebufferTarget target)
{
   CALL(1, "%d", target);
   RET(1, "%p", GLHCKRD()->framebuffer[target]);
   return GLHCKRD()->framebuffer[target];
}

/* \brief bind hardware buffer */
GLHCKAPI void glhckFramebufferBind(glhckFramebuffer *object)
{
   CALL(2, "%p", object);
   assert(object);

   if (GLHCKRD()->framebuffer[object->target] == object)
      return;

   GLHCKRA()->framebufferBind(object->target, object->object);
   GLHCKRD()->framebuffer[object->target] = object;
}

/* \brief unbind hardware buffer from target type slot */
GLHCKAPI void glhckFramebufferUnbind(glhckFramebufferTarget target)
{
   CALL(2, "%d", target);

   if (!GLHCKRD()->framebuffer[target])
      return;

   GLHCKRA()->framebufferBind(target, 0);
   GLHCKRD()->framebuffer[target] = NULL;
}

/* \brief begin render with the fbo */
GLHCKAPI void glhckFramebufferBegin(glhckFramebuffer *object)
{
   CALL(2, "%p", object);
   assert(object);
   glhckRenderStatePush();
   glhckFramebufferBind(object);
   glhckRenderResize(object->rect.w, object->rect.h);
   glhckRenderViewport(&object->rect);
}

/* \brief end rendering with the fbo */
GLHCKAPI void glhckFramebufferEnd(glhckFramebuffer *object)
{
   CALL(2, "%p", object);
   assert(object);
   glhckFramebufferUnbind(object->target);
   glhckRenderStatePop();
}

/* \brief set framebuffer's visible area */
GLHCKAPI void glhckFramebufferRect(glhckFramebuffer *object, const glhckRect *rect)
{
   CALL(1, "%p, %p", object, rect);
   assert(object && rect);
   memcpy(&object->rect, rect, sizeof(glhckRect));
}

/* \brief set framebuffer's visible area (int) */
GLHCKAPI void glhckFramebufferRecti(glhckFramebuffer *object, int x, int y, int w, int h)
{
   const glhckRect rect = { x, y, w, h};
   glhckFramebufferRect(object, &rect);
}

/* \brief attach texture to framebuffer */
GLHCKAPI int glhckFramebufferAttachTexture(glhckFramebuffer *object, glhckTexture *texture, glhckFramebufferAttachmentType attachment)
{
   CALL(1, "%p", object);
   assert(object);

   glhckFramebuffer *old = glhckFramebufferCurrentForTarget(object->target);
   glhckFramebufferBind(object);
   int ret = GLHCKRA()->framebufferTexture(object->target, texture->target, texture->object, attachment);

   if (old) {
      glhckFramebufferBind(old);
   } else {
      glhckFramebufferUnbind(object->target);
   }

   RET(1, "%d", ret);
   return ret;
}

/* \brief attach renderbuffer to framebuffer */
GLHCKAPI int glhckFramebufferAttachRenderbuffer(glhckFramebuffer *object, glhckRenderbuffer *buffer,
      glhckFramebufferAttachmentType attachment)
{
   CALL(1, "%p, %p", object, buffer);
   assert(object);

   glhckFramebuffer *old = glhckFramebufferCurrentForTarget(object->target);
   glhckFramebufferBind(object);
   glhckRenderbufferBind(buffer);
   int ret = GLHCKRA()->framebufferRenderbuffer(object->target, attachment,  buffer->object);
   glhckRenderbufferBind(NULL);

   if (old) {
      glhckFramebufferBind(old);
   } else {
      glhckFramebufferUnbind(object->target);
   }

   RET(1, "%d", ret);
   return ret;
}

/* vim: set ts=8 sw=3 tw=0 :*/
