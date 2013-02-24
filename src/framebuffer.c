#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_FRAMEBUFFER

/* \brief create new framebuffer */
GLHCKAPI glhckFramebuffer* glhckFramebufferNew(glhckFramebufferTarget target)
{
   glhckFramebuffer *object = NULL;
   unsigned int obj;
   TRACE(0);

   /* generate framebuffer */
   GLHCKRA()->framebufferGenerate(1, &obj);
   if (!obj)
      goto fail;

   if (!(object = _glhckCalloc(1, sizeof(glhckFramebuffer))))
      goto fail;

   /* init */
   object->refCounter++;
   object->object = obj;
   object->target = target;

   /* insert to world */
   _glhckWorldInsert(framebuffer, object, glhckFramebuffer*);

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
   CALL(3, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free framebuffer object */
GLHCKAPI size_t glhckFramebufferFree(glhckFramebuffer *object)
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
   _glhckWorldRemove(framebuffer, object, glhckFramebuffer*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
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
   CALL(3, "%p", object);
   assert(object);
   if (GLHCKRD()->framebuffer[object->target] == object) return;
   GLHCKRA()->framebufferBind(object->target, object->object);
   GLHCKRD()->framebuffer[object->target] = object;
}

/* \brief unbind hardware buffer from target type slot */
GLHCKAPI void glhckFramebufferUnbind(glhckFramebufferTarget target)
{
   CALL(3, "%d", target);
   if (!GLHCKRD()->framebuffer[target]) return;
   GLHCKRA()->framebufferBind(target, 0);
   GLHCKRD()->framebuffer[target] = NULL;
}

/* \brief begin render with the fbo */
GLHCKAPI void glhckFramebufferBegin(glhckFramebuffer *object)
{
   CALL(3, "%p", object);
   assert(object);
   glhckFramebufferBind(object);
   glhckRenderViewport(object->rect.x, object->rect.y, object->rect.w, object->rect.h);
}

/* \brief end rendering with the fbo */
GLHCKAPI void glhckFramebufferEnd(glhckFramebuffer *object)
{
   CALL(3, "%p", object);
   assert(object);
   glhckFramebufferUnbind(object->target);
   if (GLHCKRD()->camera) {
      GLHCKRD()->camera->view.updateViewport = 1;
      glhckCameraUpdate(GLHCKRD()->camera);
   } else glhckRenderViewport(0, 0, GLHCKR()->width, GLHCKR()->height);
}

/* \brief set framebuffer's visible area */
GLHCKAPI void glhckFramebufferRect(glhckFramebuffer *object, glhckRect *rect)
{
   CALL(1, "%p, %p", object, rect);
   assert(object && rect);
   memcpy(&object->rect, rect, sizeof(glhckRect));
}

/* \brief set framebuffer's visible area (int) */
GLHCKAPI void glhckFramebufferRecti(glhckFramebuffer *object, int x, int y, int w, int h)
{
   glhckRect rect = {x,y,w,h};
   glhckFramebufferRect(object, &rect);
}

/* \brief attach texture to framebuffer */
GLHCKAPI int glhckFramebufferAttachTexture(glhckFramebuffer *object, glhckTexture *texture, glhckFramebufferAttachmentType attachment)
{
   int ret;
   glhckFramebuffer *old;
   CALL(1, "%p", object);
   assert(object);

   old = glhckFramebufferCurrentForTarget(object->target);
   glhckFramebufferBind(object);
   ret = GLHCKRA()->framebufferTexture(object->target, texture->target, texture->object, attachment);
   if (old) glhckFramebufferBind(old);
   else glhckFramebufferUnbind(object->target);

   RET(1, "%d", ret);
   return ret;
}

/* \brief attach renderbuffer to framebuffer */
GLHCKAPI int glhckFramebufferAttachRenderbuffer(glhckFramebuffer *object, glhckRenderbuffer *buffer,
      glhckFramebufferAttachmentType attachment)
{
   int ret;
   glhckFramebuffer *old;
   CALL(1, "%p, %p", object, buffer);
   assert(object);

   old = glhckFramebufferCurrentForTarget(object->target);
   glhckFramebufferBind(object);
   glhckRenderbufferBind(buffer);
   ret = GLHCKRA()->framebufferRenderbuffer(object->target, attachment,  buffer->object);
   glhckRenderbufferBind(NULL);
   if (old) glhckFramebufferBind(old);
   else glhckFramebufferUnbind(object->target);

   RET(1, "%d", ret);
   return ret;
}

/* \brief fill texture with framebuffer's pixels */
GLHCKAPI int glhckFramebufferFillTexture(glhckFramebuffer *object, glhckTexture *texture)
{
   glhckFramebuffer *old;
   unsigned char *data = NULL;
   int size;
   CALL(1, "%p", object);
   assert(object);

   /* get texture size */
   size = _glhckSizeForTexture(texture->target, texture->width, texture->height, texture->depth,
         texture->format, GLHCK_DATA_UNSIGNED_BYTE);

   /* remember old framebuffer */
   old = glhckFramebufferCurrentForTarget(object->target);
   glhckFramebufferBind(object);

   /* allocate data for getPixels */
   data = _glhckMalloc(size);

   if (!data)
      goto fail;

   /* fill the texture */
   GLHCKRA()->bufferGetPixels(0, 0, texture->width, texture->height, texture->format, data);
   glhckTextureRecreate(texture, texture->format, GLHCK_DATA_UNSIGNED_BYTE, size, data);

   /* apply internal flags for texture */
   if (texture->format == GLHCK_RGBA) texture->importFlags |= GLHCK_TEXTURE_IMPORT_ALPHA;

   /* free data */
   NULLDO(_glhckFree, data);

   /* restore old framebuffer */
   if (old) glhckFramebufferBind(old);
   else glhckFramebufferUnbind(object->target);

   RET(1, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   if (old) glhckFramebufferBind(old);
   else glhckFramebufferUnbind(object->target);
   IFDO(_glhckFree, data);
   RET(1, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
