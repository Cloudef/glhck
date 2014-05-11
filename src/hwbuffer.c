#include "internal.h"
#include <stdio.h> /* temporary printf */

#define GLHCK_CHANNEL GLHCK_CHANNEL_HWBUFFER

/* \brief free current list of uniform buffer's uniforms */
static void _glhckHwBufferFreeUniforms(glhckHwBuffer *object)
{
   assert(object);

   _glhckHwBufferShaderUniform *u;
   for (chckPoolIndex iter = 0; (u = chckPoolIter(object->uniforms, &iter));)
      _glhckFree(u->name);

   IFDO(chckPoolFree, object->uniforms);
   object->uniforms = NULL;
}

/* \brief allocate new hardware buffer object */
GLHCKAPI glhckHwBuffer* glhckHwBufferNew(void)
{
   glhckHwBuffer *object = NULL;
   TRACE(0);

   /* generate hw buffer object */
   unsigned int obj;
   GLHCKRA()->hwBufferGenerate(1, &obj);

   if (!obj)
      goto fail;

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckHwBuffer))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* init */
   object->object  = obj;
   object->created = 0;

   /* insert to world */
   _glhckWorldAdd(&GLHCKW()->hwbuffers, object);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference hardware buffer object */
GLHCKAPI glhckHwBuffer* glhckHwBufferRef(glhckHwBuffer *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free hardware buffer object */
GLHCKAPI unsigned int glhckHwBufferFree(glhckHwBuffer *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* unbind from active slot */
   if (GLHCKRD()->hwBuffer[object->target] == object)
      glhckHwBufferUnbind(object->target);

   /* delete object */
   if (object->object)
      GLHCKRA()->hwBufferDelete(1, &object->object);

   IFDO(_glhckFree, object->name);
   _glhckHwBufferFreeUniforms(object);

   /* remove from world */
   _glhckWorldRemove(&GLHCKW()->hwbuffers, object);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", (object ? object->refCounter : 0));
   return (object ? object->refCounter : 0);
}

/* \brief get current bound hardware buffer for target */
GLHCKAPI glhckHwBuffer* glhckHwBufferCurrentForTarget(glhckHwBufferTarget target)
{
   CALL(2, "%d", target);
   RET(2, "%p", GLHCKRD()->hwBuffer[target]);
   return GLHCKRD()->hwBuffer[target];
}

/* \brief bind hardware buffer object */
GLHCKAPI void glhckHwBufferBind(glhckHwBuffer *object)
{
   CALL(2, "%p", object);
   assert(object);

   if (GLHCKRD()->hwBuffer[object->target] == object)
      return;

   GLHCKRA()->hwBufferBind(object->target, object->object);
   GLHCKRD()->hwBuffer[object->target] = object;
}

/* \brief bind hardware buffer from target type slot */
GLHCKAPI void glhckHwBufferUnbind(glhckHwBufferTarget target)
{
   CALL(2, "%d", target);

   if (!GLHCKRD()->hwBuffer[target])
      return;

   GLHCKRA()->hwBufferBind(target, 0);
   GLHCKRD()->hwBuffer[target] = NULL;
}

/* \brief return current hardware buffer's target type */
GLHCKAPI glhckHwBufferTarget glhckHwBufferGetType(glhckHwBuffer *object)
{
   CALL(1, "%p", object);
   assert(object);
   RET(1, "%d", object->target);
   return object->target;
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
GLHCKAPI void glhckHwBufferCreate(glhckHwBuffer *object, glhckHwBufferTarget target, int size, const void *data, glhckHwBufferStoreType usage)
{
   CALL(2, "%p, %d, %d, %p, %d", object, target, size, data, usage);
   assert(object);

   /* store old buffer */
   glhckHwBuffer *old = glhckHwBufferCurrentForTarget(target);

   /* unbind from old slot */
   if (object->created) {
      if (GLHCKRD()->hwBuffer[object->target] == object)
         glhckHwBufferUnbind(object->target);
   }

   object->target = target;
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferCreate(target, size, data, usage);
   object->size      = size;
   object->storeType = usage;
   object->created   = 1;

   /* bind back old buffer, or NULL */
   if (old) {
      glhckHwBufferBind(old);
   } else {
      glhckHwBufferUnbind(object->target);
   }
}

/* \brief bind hardware buffer object to index */
GLHCKAPI void glhckHwBufferBindBase(glhckHwBuffer *object, unsigned int index)
{
   CALL(0, "%p, %u", object, index);
   assert(object && object->created);
   GLHCKRA()->hwBufferBindBase(object->target, index, object->object);
}

/* \brief bind hardware buffer object to index with range */
GLHCKAPI void glhckHwBufferBindRange(glhckHwBuffer *object, unsigned int index, int offset, int size)
{
   CALL(0, "%p, %u, %d, %d", object, index, offset, size);
   assert(object && object->created);
   GLHCKRA()->hwBufferBindRange(object->target, index, object->object, offset, size);
}

/* \brief fill subdata to hardware buffer object */
GLHCKAPI void glhckHwBufferFill(glhckHwBuffer *object, int offset, int size, const void *data)
{
   CALL(2, "%p, %d, %d, %p", object, offset, size, data);
   assert(object && object->created);

   glhckHwBuffer *old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferFill(object->target, offset, size, data);

   if (old) {
      glhckHwBufferBind(old);
   } else {
      glhckHwBufferUnbind(object->target);
   }
}

/* \brief map hardware buffer object */
GLHCKAPI void* glhckHwBufferMap(glhckHwBuffer *object, glhckHwBufferAccessType access)
{
   CALL(2, "%p, %d", object, access);
   assert(object && object->created);

   glhckHwBuffer *old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   void *data = GLHCKRA()->hwBufferMap(object->target, access);

   if (old) {
      glhckHwBufferBind(old);
   } else {
      glhckHwBufferUnbind(object->target);
   }

   RET(2, "%p", data);
   return data;
}

/* \brief unmap hardware buffer object */
GLHCKAPI void glhckHwBufferUnmap(glhckHwBuffer *object)
{
   CALL(2, "%p", object);
   assert(object && object->created);

   glhckHwBuffer *old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferUnmap(object->target);

   if (old) {
      glhckHwBufferBind(old);
   } else {
      glhckHwBufferUnbind(object->target);
   }
}

/*
 * Uniform Buffer Object related functions
 */

/* \brief create and intialize uniform buffer from shader */
GLHCKAPI int glhckHwBufferCreateUniformBufferFromShader(glhckHwBuffer *object, glhckShader *shader, const char *uboName, glhckHwBufferStoreType usage)
{
   chckPool *pool = NULL;
   char *bufferName = NULL;
   CALL(2, "%p, %p, %s, %u", object, shader, uboName, usage);
   assert(object && shader);

   /* copy buffer name */
   if (!(bufferName = _glhckStrdup(uboName)))
      goto fail;

   /* list ubo information from program */
   int size;
   if (!(pool = GLHCKRA()->programUniformBufferPool(shader->program, uboName, &size)))
      goto fail;

   /* initialize uniform buffer with the size */
   glhckHwBufferCreate(object, GLHCK_UNIFORM_BUFFER, size, NULL, usage);

   /* store the name and uniforms for this hw buffer */
   if (object->name)
      _glhckFree(object->name);

   object->name = bufferName;
   object->uniforms = pool;

   _glhckHwBufferShaderUniform *u;
   for (chckPoolIndex iter = 0; (u = chckPoolIter(pool, &iter));)
      printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, bufferName);
   IFDO(chckPoolFree, pool);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief append information about UBO from shader */
GLHCKAPI int glhckHwBufferAppendInformationFromShader(glhckHwBuffer *object, glhckShader *shader)
{
   chckPool *pool = NULL;
   CALL(2, "%p, %p", object, shader);
   assert(object && shader);
   assert(object->created);

   /* list ubo information from program */
   int size;
   if (!(pool = GLHCKRA()->programUniformBufferPool(shader->program, object->name, &size)))
      goto fail;

   /* filter out stuff from list that we already have */
   int dirty = 0;
   _glhckHwBufferShaderUniform *u;
   for (chckPoolIndex iter = 0; (u = chckPoolIter(pool, &iter));) {
      chckPoolIndex iter2;
      _glhckHwBufferShaderUniform *uo;
      for (iter2 = 0; (uo = chckPoolIter(object->uniforms, &iter2));) {
         if (u->offset == uo->offset)
            continue;

         if ((uo = chckPoolAdd(object->uniforms, u, NULL))) {
            printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);
            dirty = 1;
         }
         break;
      }
   }

   chckPoolFree(pool);

   /* nothing to do */
   if (!dirty)
      return RETURN_OK;

   /* initialize uniform buffer with the size */
   glhckHwBufferCreate(object, GLHCK_UNIFORM_BUFFER, size, NULL, object->storeType);
   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(chckPoolFree, pool);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief fill single uniform in uniform buffer */
GLHCKAPI void glhckHwBufferFillUniform(glhckHwBuffer *object, const char *uniform, int size, const void *data)
{
   CALL(2, "%p, %s, %d, %p", object, uniform, size, data);
   assert(object);
   assert(object->target == GLHCK_UNIFORM_BUFFER);

   /* search uniform
    * TODO: use chckHashTable */
   _glhckHwBufferShaderUniform *u;
   for (chckPoolIndex iter = 0; (u = chckPoolIter(object->uniforms, &iter));) {
      if (!strcmp(uniform, u->name))
         break;
   }

   /* uniform not found */
   if (!u)
      return;

   /* fill the uniform */
   glhckHwBufferFill(object, u->offset, size, data);
}

/* vim: set ts=8 sw=3 tw=0 :*/
