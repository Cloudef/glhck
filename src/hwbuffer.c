#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_HWBUFFER

/* \brief allocate new hardware buffer object */
GLHCKAPI glhckHwBuffer* glhckHwBufferNew(void)
{
   glhckHwBuffer *object = NULL;
   unsigned int obj;
   TRACE(0);

   /* generate hw buffer object */
   GLHCKRA()->hwBufferGenerate(1, &obj);
   if (!obj) goto fail;

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckHwBuffer))))
      goto fail;

   /* init */
   object->refCounter++;
   object->object  = obj;
   object->created = 0;

   /* insert to world */
   _glhckWorldInsert(hlist, object, glhckHwBuffer*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference hardware buffer object */
GLHCKAPI glhckHwBuffer* glhckHwBufferRef(glhckHwBuffer *object)
{
   CALL(3, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free hardware buffer object */
GLHCKAPI size_t glhckHwBufferFree(glhckHwBuffer *object)
{
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* unbind from active slot */
   if (GLHCKRD()->hwBuffer[object->targetType] == object)
      glhckHwBufferUnbind(object->targetType);

   /* delete object */
   if (object->object)
      GLHCKRA()->hwBufferDelete(1, &object->object);

   /* remove from world */
   _glhckWorldRemove(hlist, object, glhckHwBuffer*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind hardware buffer object */
GLHCKAPI void glhckHwBufferBind(glhckHwBuffer *object)
{
   CALL(3, "%p", object);
   assert(object);
   if (GLHCKRD()->hwBuffer[object->targetType] == object) return;
   GLHCKRA()->hwBufferBind(object->targetType, object->object);
   GLHCKRD()->hwBuffer[object->targetType] = object;
}

/* \brief bind hardware buffer from target type slot */
GLHCKAPI void glhckHwBufferUnbind(glhckHwBufferType type)
{
   CALL(3, "%d", type);
   if (!GLHCKRD()->hwBuffer[type]) return;
   GLHCKRA()->hwBufferBind(type, 0);
   GLHCKRD()->hwBuffer[type] = NULL;
}

/* \brief return current hardware buffer's target type */
GLHCKAPI glhckHwBufferType glhckHwBufferGetType(glhckHwBuffer *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%d", object->targetType);
   return object->targetType;
}

/* \brief return current hardware buffer's store type */
GLHCKAPI glhckHwBufferStoreType glhckHwBufferGetStoreType(glhckHwBuffer *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%d", object->storeType);
   return object->storeType;
}

/* \brief create/initialize hardware buffer object */
GLHCKAPI void glhckHwBufferCreate(glhckHwBuffer *object, glhckHwBufferType type, int size, const void *data, glhckHwBufferStoreType usage)
{
   glhckHwBuffer *old;
   CALL(3, "%p, %d, %p, %d", object, size, data, usage);
   assert(object);

   /* store old buffer */
   old = GLHCKRD()->hwBuffer[type];

   /* unbind from old slot */
   if (object->created) {
      if (GLHCKRD()->hwBuffer[object->targetType] == object)
         glhckHwBufferUnbind(object->targetType);
   }

   object->targetType = type;
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferCreate(type, size, data, usage);
   object->storeType  = usage;
   object->created    = 1;

   /* bind back old buffer, or NULL */
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->targetType);
}

/* \brief bind hardware buffer object to index */
GLHCKAPI void glhckHwBufferBindBase(glhckHwBuffer *object, unsigned int index)
{
   CALL(0, "%p, %u", object, index);
   assert(object && object->created);
   GLHCKRA()->hwBufferBindBase(object->targetType, index, object->object);
}

/* \brief bind hardware buffer object to index with range */
GLHCKAPI void glhckHwBufferBindRange(glhckHwBuffer *object, unsigned int index, int offset, int size)
{
   CALL(0, "%p, %u, %p, %d", object, index, offset, size);
   assert(object && object->created);
   GLHCKRA()->hwBufferBindRange(object->targetType, index, object->object, offset, size);
}

/* \brief fill subdata to hardware buffer object */
GLHCKAPI void glhckHwBufferFill(glhckHwBuffer *object, int offset, int size, const void *data)
{
   glhckHwBuffer *old;
   CALL(3, "%p, %p, %p, %p", object, offset, size, data);
   assert(object && object->created);

   old = GLHCKRD()->hwBuffer[object->targetType];
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferFill(object->targetType, offset, size, data);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->targetType);
}

/* \brief map hardware buffer object */
GLHCKAPI void* glhckHwBufferMap(glhckHwBuffer *object, glhckHwBufferAccessType access)
{
   void *data;
   glhckHwBuffer *old;
   CALL(3, "%p, %d", object, access);
   assert(object && object->created);

   old = GLHCKRD()->hwBuffer[object->targetType];
   glhckHwBufferBind(object);
   data = GLHCKRA()->hwBufferMap(object->targetType, access);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->targetType);

   RET(3, "%p", data);
   return data;
}

/* \brief unmap hardware buffer object */
GLHCKAPI void glhckHwBufferUnmap(glhckHwBuffer *object)
{
   glhckHwBuffer *old;
   CALL(3, "%p", object);
   assert(object && object->created);

   old = GLHCKRD()->hwBuffer[object->targetType];
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferUnmap(object->targetType);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->targetType);
}

/* vim: set ts=8 sw=3 tw=0 :*/
