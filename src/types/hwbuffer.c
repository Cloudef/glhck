#include "hwbuffer.h"

#include <stdio.h> /* temporary printf */
#include <stdlib.h>

#include "trace.h"
#include "handle.h"
#include "system/tls.h"
#include "renderers/render.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_HWBUFFER

enum pool {
   $uniforms, // list handle (struct glhckHwBufferShaderUniform)
   $name, // string handle
   $object, // unsigned int
   $size, // int
   $target, // glhckHwBufferTarget
   $type, // glhckHwBufferStoreType
   $created, // unsigned char
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // uniforms
   sizeof(glhckHandle), // name
   sizeof(unsigned int), // object
   sizeof(int), // size
   sizeof(glhckHwBufferTarget), // target
   sizeof(glhckHwBufferStoreType), // type
   sizeof(unsigned char), // created
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];
static _GLHCK_TLS glhckHandle active[GLHCK_HWBUFFER_TYPE_LAST];

#include "handlefun.h"

/* \brief free current list of uniform buffer's uniforms */
static void freeUniforms(const glhckHandle handle)
{
   assert(handle > 0);

   size_t memb;
   const struct glhckShaderUniform *uniforms = getList($uniforms, handle, &memb);
   for (size_t i = 0; i < memb; ++i)
      free((char*)uniforms[i].name);

   releaseHandle($uniforms, handle);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   if (active[target] == handle)
      glhckHwBufferUnbind(target);

   unsigned int *object = (unsigned int*)get($object, handle);
   if (*object)
      _glhckRenderGetAPI()->hwBufferDelete(1, object);

   releaseHandle($name, handle);
   freeUniforms(handle);
}

/* \brief allocate new hardware buffer object */
GLHCKAPI glhckHandle glhckHwBufferNew(void)
{
   TRACE(0);

   unsigned int object;
   _glhckRenderGetAPI()->hwBufferGenerate(1, &object);

   if (!object)
      goto fail;

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_HWBUFFER, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   set($object, handle, &object);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   if (object) _glhckRenderGetAPI()->hwBufferDelete(1, &object);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief get current bound hardware buffer for target */
GLHCKAPI glhckHandle glhckHwBufferCurrentForTarget(const glhckHwBufferTarget target)
{
   CALL(2, "%u", target);
   RET(2, "%s", glhckHandleRepr(active[target]));
   return active[target];
}

/* \brief bind hardware buffer object */
GLHCKAPI void glhckHwBufferBind(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   if (active[target] == handle)
      return;

   unsigned int object = *(unsigned int*)get($object, handle);
   _glhckRenderGetAPI()->hwBufferBind(target, object);
   active[target] = object;
}

/* \brief bind hardware buffer from target type slot */
GLHCKAPI void glhckHwBufferUnbind(const glhckHwBufferTarget target)
{
   CALL(2, "%u", target);

   if (!active[target])
      return;

   _glhckRenderGetAPI()->hwBufferBind(target, 0);
   active[target] = 0;
}

/* \brief return current hardware buffer's target */
GLHCKAPI glhckHwBufferTarget glhckHwBufferGetTarget(const glhckHandle handle)
{
   return *(glhckHwBufferTarget*)get($target, handle);
}

/* \brief return current hardware buffer's store type */
GLHCKAPI glhckHwBufferStoreType glhckHwBufferGetStoreType(const glhckHandle handle)
{
   return *(glhckHwBufferStoreType*)get($type, handle);
}

/* \brief create/initialize hardware buffer object */
GLHCKAPI void glhckHwBufferCreate(const glhckHandle handle, const glhckHwBufferTarget target, const int size, const void *data, const glhckHwBufferStoreType usage)
{
   CALL(2, "%s, %d, %d, %p, %d", glhckHandleRepr(handle), target, size, data, usage);
   assert(handle > 0);

   const glhckHwBufferTarget ctarget = glhckHwBufferGetTarget(handle);
   if (ctarget != target && active[ctarget] == handle)
      glhckHwBufferUnbind(ctarget);

   set($target, handle, &target);

   glhckHwBufferBind(handle);
   _glhckRenderGetAPI()->hwBufferCreate(target, size, data, usage);

   set($size, handle, &size);
   set($type, handle, &usage);
   set($created, handle, (unsigned char[]){1});
}

/* \brief bind hardware buffer object to index */
GLHCKAPI void glhckHwBufferBindBase(const glhckHandle handle, const unsigned int index)
{
   CALL(0, "%s, %u", glhckHandleRepr(handle), index);
   assert(handle > 0 && *(unsigned char*)get($created, handle));
   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   unsigned int object = *(unsigned int*)get($object, handle);
   _glhckRenderGetAPI()->hwBufferBindBase(target, index, object);
}

/* \brief bind hardware buffer object to index with range */
GLHCKAPI void glhckHwBufferBindRange(const glhckHandle handle, const unsigned int index, const int offset, const int size)
{
   CALL(0, "%s, %u, %d, %d", glhckHandleRepr(handle), index, offset, size);
   assert(handle > 0 && *(unsigned char*)get($created, handle));
   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   unsigned int object = *(unsigned int*)get($object, handle);
   _glhckRenderGetAPI()->hwBufferBindRange(target, index, object, offset, size);
}

/* \brief fill subdata to hardware buffer object */
GLHCKAPI void glhckHwBufferFill(const glhckHandle handle, const int offset, const int size, const void *data)
{
   CALL(2, "%s, %d, %d, %p", glhckHandleRepr(handle), offset, size, data);
   assert(handle > 0 && *(unsigned char*)get($created, handle));
   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   glhckHwBufferBind(handle);
   _glhckRenderGetAPI()->hwBufferFill(target, offset, size, data);
}

/* \brief map hardware buffer object */
GLHCKAPI void* glhckHwBufferMap(const glhckHandle handle, const glhckHwBufferAccessType access)
{
   CALL(2, "%s, %d", glhckHandleRepr(handle), access);
   assert(handle > 0 && *(unsigned char*)get($created, handle));

   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   glhckHwBufferBind(handle);
   void *data = _glhckRenderGetAPI()->hwBufferMap(target, access);

   RET(2, "%p", data);
   return data;
}

/* \brief unmap hardware buffer object */
GLHCKAPI void glhckHwBufferUnmap(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0 && *(unsigned char*)get($created, handle));

   const glhckHwBufferTarget target = glhckHwBufferGetTarget(handle);
   glhckHwBufferBind(handle);
   _glhckRenderGetAPI()->hwBufferUnmap(target);
}

GLHCKAPI const char* glhckHwBufferGetName(const glhckHandle handle)
{
   return getCStr($name, handle);
}

GLHCKAPI int glhckHwBufferGetSize(const glhckHandle handle)
{
   return *(int*)get($size, handle);
}

/*
 * Uniform Buffer Object related functions
 */

/* \brief create and intialize uniform buffer from shader */
GLHCKAPI int glhckHwBufferCreateUniformBufferFromShader(const glhckHandle handle, const glhckHandle shader, const char *uboName, const glhckHwBufferStoreType usage)
{
   chckIterPool *pool = NULL;
   CALL(2, "%s, %s, %s, %u", glhckHandleRepr(handle), glhckHandleRepr(shader), uboName, usage);
   assert(handle > 0 && shader);

   /* list ubo information from program */
   int size;
   pool = _glhckRenderGetAPI()->programUniformBufferPool(_glhckShaderGetProgram(shader), uboName, &size);

   if (!size)
      goto fail;

   if (pool) {
      struct glhckHwBufferShaderUniform *u;
      for (chckPoolIndex iter = 0; (u = chckIterPoolIter(pool, &iter));)
         printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);
   }

   /* store the name and uniforms for this hw buffer */
   if (setCStr($name, handle, uboName) != RETURN_OK)
      goto fail;

   /* initialize uniform buffer with the size */
   glhckHwBufferCreate(handle, GLHCK_UNIFORM_BUFFER, size, NULL, usage);

   freeUniforms(handle);
   setListFromPool($uniforms, handle, pool, sizeof(struct glhckHwBufferShaderUniform));

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(chckIterPoolFree, pool);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief append information about UBO from shader */
GLHCKAPI int glhckHwBufferAppendInformationFromShader(const glhckHandle handle, const glhckHandle shader)
{
   chckIterPool *pool = NULL;
   CALL(2, "%s, %s", glhckHandleRepr(handle), glhckHandleRepr(shader));
   assert(handle > 0 && shader);
   assert(*(unsigned char*)get($created, handle));

   /* list ubo information from program */
   int size;
   pool = _glhckRenderGetAPI()->programUniformBufferPool(_glhckShaderGetProgram(shader), glhckHwBufferGetName(handle), &size);

   if (!size)
      goto fail;

   /* filter out stuff from list that we already have */
   int dirty = 0;
   struct glhckHwBufferShaderUniform *u;
   for (chckPoolIndex iter = 0; (u = chckIterPoolIter(pool, &iter));) {
      unsigned char found = 0;

      size_t memb;
      const struct glhckHwBufferShaderUniform *uniforms = getList($uniforms, handle, &memb);
      for (size_t i = 0; i < memb; ++i) {
         if (u->offset == uniforms[i].offset) {
            found = 1;
            break;
         }
      }

      const glhckHandle list = *(glhckHandle*)get($uniforms, handle);
      if (!found && glhckListAdd(list, u)) {
         printf("UBO: (%s:%u) %d : %d [%s]\n", u->name, u->offset, u->type, u->size, u->typeName);
         dirty = 1;
      }
   }

   chckIterPoolFree(pool);

   /* nothing to do */
   if (!dirty)
      goto success;

   /* initialize uniform buffer with the size */
   unsigned int object = *(unsigned int*)get($object, handle);
   const glhckHwBufferStoreType type = glhckHwBufferGetStoreType(handle);
   glhckHwBufferCreate(object, GLHCK_UNIFORM_BUFFER, size, NULL, type);

success:
   RET(2, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(chckIterPoolFree, pool);
   RET(2, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const void* glhckHwBufferGetUniform(const glhckHandle handle, const char *uniform)
{
   CALL(0, "%s, %s", glhckHandleRepr(handle), uniform);
   assert(handle > 0);

   size_t memb;
   const struct glhckHwBufferShaderUniform *uniforms = getList($uniforms, handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      if (!strcmp(uniform, uniforms[i].name)) {
         RET(0, "%p", &uniforms[i]);
         return &uniforms[i];
      }
   }

   RET(0, "%p", NULL);
   return NULL;
}

/* \brief fill single uniform in uniform buffer */
GLHCKAPI void glhckHwBufferFillUniform(const glhckHandle handle, const void *uniform, const int size, const void *data)
{
   CALL(2, "%s, %p, %d, %p", glhckHandleRepr(handle), uniform, size, data);
   assert(handle > 0 && glhckHwBufferGetTarget(handle) == GLHCK_UNIFORM_BUFFER);

   const struct glhckHwBufferShaderUniform *u = uniform;
   const unsigned int object = *(const unsigned int*)get($object, handle);
   glhckHwBufferFill(object, u->offset, size, data);
}

/* vim: set ts=8 sw=3 tw=0 :*/
