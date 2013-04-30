#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDERBUFFER

/* \brief create new renderbuffer */
GLHCKAPI glhckRenderbuffer* glhckRenderbufferNew(int width, int height, glhckTextureFormat format)
{
   glhckRenderbuffer *object = NULL;
   unsigned int obj;
   TRACE(0);

   /* generate renderbuffer */
   GLHCKRA()->renderbufferGenerate(1, &obj);
   if (!obj)
      goto fail;

   if (!(object = _glhckCalloc(1, sizeof(glhckRenderbuffer))))
      goto fail;

   /* init */
   object->refCounter++;
   object->object = obj;
   object->width  = width;
   object->height = height;
   object->format = format;

   glhckRenderbufferBind(object);
   GLHCKRA()->renderbufferStorage(object->width, object->height, object->format);
   glhckRenderbufferBind(NULL);

   /* insert to world */
   _glhckWorldInsert(renderbuffer, object, glhckRenderbuffer*);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference renderbuffer object */
GLHCKAPI glhckRenderbuffer* glhckRenderbufferRef(glhckRenderbuffer *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free renderbuffer object */
GLHCKAPI unsigned int glhckRenderbufferFree(glhckRenderbuffer *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* delete framebuffer object */
   GLHCKRA()->framebufferDelete(1, &object->object);

   /* remove from world */
   _glhckWorldRemove(renderbuffer, object, glhckRenderbuffer*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind/unbind renderbuffer */
GLHCKAPI void glhckRenderbufferBind(glhckRenderbuffer *object)
{
   CALL(2, "%p", object);
   GLHCKRA()->renderbufferBind((object?object->object:0));
}

