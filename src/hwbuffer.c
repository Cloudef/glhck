#include "internal.h"
#include <stdio.h> /* temporary printf */

#define GLHCK_CHANNEL GLHCK_CHANNEL_HWBUFFER

/* \brief free current list of uniform buffer's uniforms */
static void _glhckHwBufferFreeUniforms(glhckHwBuffer *object)
{
   _glhckHwBufferShaderUniform *u, *un;
   assert(object);
   for (u = object->uniforms; u; u = un) {
      un = u->next;
      _glhckFree(u->name);
      _glhckFree(u);
   }
   object->uniforms = NULL;
}

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
   _glhckWorldInsert(hwbuffer, object, glhckHwBuffer*);

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
   _glhckWorldRemove(hwbuffer, object, glhckHwBuffer*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief get current bound hardware buffer for target */
GLHCKAPI glhckHwBuffer* glhckHwBufferCurrentForTarget(glhckHwBufferTarget target)
{
   CALL(1, "%d", target);
   RET(1, "%p", GLHCKRD()->hwBuffer[target]);
   return GLHCKRD()->hwBuffer[target];
}

/* \brief bind hardware buffer object */
GLHCKAPI void glhckHwBufferBind(glhckHwBuffer *object)
{
   CALL(3, "%p", object);
   assert(object);
   if (GLHCKRD()->hwBuffer[object->target] == object) return;
   GLHCKRA()->hwBufferBind(object->target, object->object);
   GLHCKRD()->hwBuffer[object->target] = object;
}

/* \brief bind hardware buffer from target type slot */
GLHCKAPI void glhckHwBufferUnbind(glhckHwBufferTarget target)
{
   CALL(3, "%d", target);
   if (!GLHCKRD()->hwBuffer[target]) return;
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
   glhckHwBuffer *old;
   CALL(3, "%p, %d, %p, %d", object, size, data, usage);
   assert(object);

   /* store old buffer */
   old = glhckHwBufferCurrentForTarget(target);

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
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->target);
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
   CALL(0, "%p, %u, %p, %d", object, index, offset, size);
   assert(object && object->created);
   GLHCKRA()->hwBufferBindRange(object->target, index, object->object, offset, size);
}

/* \brief fill subdata to hardware buffer object */
GLHCKAPI void glhckHwBufferFill(glhckHwBuffer *object, int offset, int size, const void *data)
{
   glhckHwBuffer *old;
   CALL(3, "%p, %p, %p, %p", object, offset, size, data);
   assert(object && object->created);

   old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferFill(object->target, offset, size, data);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->target);
}

/* \brief map hardware buffer object */
GLHCKAPI void* glhckHwBufferMap(glhckHwBuffer *object, glhckHwBufferAccessType access)
{
   void *data;
   glhckHwBuffer *old;
   CALL(3, "%p, %d", object, access);
   assert(object && object->created);

   old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   data = GLHCKRA()->hwBufferMap(object->target, access);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->target);

   RET(3, "%p", data);
   return data;
}

/* \brief unmap hardware buffer object */
GLHCKAPI void glhckHwBufferUnmap(glhckHwBuffer *object)
{
   glhckHwBuffer *old;
   CALL(3, "%p", object);
   assert(object && object->created);

   old = glhckHwBufferCurrentForTarget(object->target);
   glhckHwBufferBind(object);
   GLHCKRA()->hwBufferUnmap(object->target);
   if (old) glhckHwBufferBind(old);
   else glhckHwBufferUnbind(object->target);
}

/*
 * Uniform Buffer Object related functions
 */

/* \brief create and intialize uniform buffer from shader */
GLHCKAPI int glhckHwBufferCreateUniformBufferFromShader(glhckHwBuffer *object, glhckShader *shader, const char *uboName, glhckHwBufferStoreType usage)
{
   char *bufferName = NULL;
   int size;
   _glhckHwBufferShaderUniform *uniforms = NULL, *u, *un;
   assert(object && shader);

   /* list ubo information from program */
   if (!(uniforms = GLHCKRA()->programUniformBufferList(shader->program, uboName, &size)))
      goto fail;

   /* copy buffer name */
   if (!(bufferName = _glhckStrdup(uboName)))
      goto fail;

   /* initialize uniform buffer with the size */
   glhckHwBufferCreate(object, GLHCK_UNIFORM_BUFFER, size, NULL, usage);

   /* store the name and uniforms for this hw buffer */
   if (object->name) _glhckFree(object->name);
   object->name = bufferName;
   object->uniforms = uniforms;

   for (u = object->uniforms; u; u = u->next)
      printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);

   return RETURN_OK;

fail:
   IFDO(_glhckFree, bufferName);
   for (u = uniforms; u; u = un) {
      un = u->next;
      _glhckFree(u->name);
      _glhckFree(u);
   }
   return RETURN_FAIL;
}

/* \brief append information about UBO from shader */
GLHCKAPI int glhckHwBufferAppendInformationFromShader(glhckHwBuffer *object, glhckShader *shader)
{
   int size;
   _glhckHwBufferShaderUniform *uniforms = NULL, *u, *ua, *up, *un;
   assert(object && shader);
   assert(object->created);

   /* list ubo information from program */
   if (!(uniforms = GLHCKRA()->programUniformBufferList(shader->program, object->name, &size)))
      goto fail;

   /* filter out stuff from list that we already have */
   for (u = object->uniforms; u; u = u->next) {
      for (ua = uniforms, up = NULL; ua; up = ua, ua = un) {
         un = ua->next;
         if (u->offset != ua->offset) continue;
         if (ua == uniforms) uniforms = ua->next;
         else if (up) up->next = ua->next;
         _glhckFree(ua->name);
         _glhckFree(ua);
         break;
      }
   }

   /* there is nothing to do */
   if (!uniforms) return RETURN_OK;

   /* initialize uniform buffer with the size */
   glhckHwBufferCreate(object, GLHCK_UNIFORM_BUFFER, size, NULL, object->storeType);

   /* append the uniforms */
   for (u = object->uniforms; u && u->next; u = u->next);
   if (u) u->next = uniforms;
   else object->uniforms = uniforms;

   for (u = uniforms; u; u = u->next)
      printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);

   return RETURN_OK;

fail:
   for (u = uniforms; u; u = un) {
      un = u->next;
      _glhckFree(u->name);
      _glhckFree(u);
   }
   return RETURN_FAIL;
}

/* \brief fill single uniform in uniform buffer */
GLHCKAPI void glhckHwBufferFillUniform(glhckHwBuffer *object, const char *uniform, int size, const void *data)
{
   _glhckHwBufferShaderUniform *u;
   assert(object);
   assert(object->target == GLHCK_UNIFORM_BUFFER);

   /* search uniform */
   for (u = object->uniforms; u; u = u->next)
      if (!strcmp(uniform, u->name)) break;

   /* uniform not found */
   if (!u) return;

   /* fill the uniform */
   glhckHwBufferFill(object, u->offset, size, data);
}

/* vim: set ts=8 sw=3 tw=0 :*/
