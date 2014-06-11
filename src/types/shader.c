#include "shader.h"

#include <stdio.h> /* temporary printf */
#include <stdlib.h>

#include "trace.h"
#include "handle.h"
#include "system/tls.h"
#include "renderers/render.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_SHADER

enum pool {
   $attributes, // list handle
   $uniforms, // list handle
   $program, // unsigned int
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // attributes
   sizeof(glhckHandle), // uniforms
   sizeof(unsigned int), // program
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];
static _GLHCK_TLS glhckHandle active;

#include "handlefun.h"

/* \brief free current list of shader attributes */
static void freeAttributes(const glhckHandle handle)
{
   assert(handle > 0);

   size_t memb;
   const struct glhckShaderAttribute *attributes = getList($attributes, handle, &memb);
   for (size_t i = 0; i < memb; ++i)
      free((char*)attributes[i].name);

   releaseHandle($attributes, handle);
}

/* \brief free current list of shader uniforms */
static void freeUniforms(const glhckHandle handle)
{
   assert(handle > 0);

   size_t memb;
   const struct glhckShaderUniform *uniforms = getList($uniforms, handle, &memb);
   for (size_t i = 0; i < memb; ++i)
      free((char*)uniforms[i].name);

   releaseHandle($uniforms, handle);
}

unsigned int _glhckShaderGetProgram(const glhckHandle handle)
{
   return *(unsigned int*)get($program, handle);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   if (active == handle)
      glhckShaderBind(0);

   const unsigned int program = _glhckShaderGetProgram(handle);
   if (program)
      _glhckRenderGetAPI()->programDelete(program);

   freeAttributes(handle);
   freeUniforms(handle);
}

/*
 * public api
 */

/* \brief compile new shader object contentsFromMemory may be null
 *
 * NOTE: you need to free shaderObjects yourself! */
GLHCKAPI unsigned int glhckCompileShaderObject(const glhckShaderType type, const char *effectKey, const char *contentsFromMemory)
{
   CALL(0, "%u, %s, %s", type, effectKey, contentsFromMemory);
   unsigned int object = _glhckRenderGetAPI()->shaderCompile(type, effectKey, contentsFromMemory);
   RET(0, "%u", object);
   return object;
}

/* \brief delete compiled shader object */
GLHCKAPI void glhckDeleteShaderObject(const unsigned int shaderObject)
{
   CALL(0, "%u", shaderObject);
   _glhckRenderGetAPI()->shaderDelete(shaderObject);
}

/* \brief allocate new shader object contentsFromMemory may be null */
GLHCKAPI glhckHandle glhckShaderNew(const char *vertexEffect, const char *fragmentEffect, const char *contentsFromMemory)
{
   CALL(0, "%s, %s, %s", vertexEffect, fragmentEffect, contentsFromMemory);

   /* compile base shaders */
   unsigned int vsobj = glhckCompileShaderObject(GLHCK_VERTEX_SHADER, vertexEffect, contentsFromMemory);
   unsigned int fsobj = glhckCompileShaderObject(GLHCK_FRAGMENT_SHADER, fragmentEffect, contentsFromMemory);

   /* fail compiling shaders */
   if (!vsobj || !fsobj)
      goto fail;

   glhckHandle handle;

   if (!(handle = glhckShaderNewWithShaderObjects(vsobj, fsobj)))
      goto fail;

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   if (vsobj) glhckDeleteShaderObject(vsobj);
   if (fsobj) glhckDeleteShaderObject(fsobj);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief allocate new shader object with GL shader objects */
GLHCKAPI glhckHandle glhckShaderNewWithShaderObjects(const unsigned int vertexShader, const unsigned int fragmentShader)
{
   unsigned int program = 0;
   chckIterPool *attributes = NULL, *uniforms = NULL;
   CALL(0, "%u, %u", vertexShader, fragmentShader);
   assert(vertexShader != 0 && fragmentShader != 0);

   if (!(program = _glhckRenderGetAPI()->programLink(vertexShader, fragmentShader)))
      goto fail;

   if ((attributes = _glhckRenderGetAPI()->programAttributePool(program))) {
      struct glhckShaderAttribute *a;
      for (chckPoolIndex iter = 0; (a = chckIterPoolIter(attributes, &iter));)
         printf("(%s:%u) %d : %d [%s]\n", a->name, a->location, a->type, a->size, a->typeName);
   }

   if ((uniforms = _glhckRenderGetAPI()->programUniformPool(program))) {
      struct glhckShaderUniform *u;
      for (chckPoolIndex iter = 0; (u = chckIterPoolIter(uniforms, &iter));)
         printf("(%s:%u) %d : %d [%s]\n", u->name, u->location, u->type, u->size, u->typeName);
   }

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_SHADER, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   set($program, handle, &program);
   setListFromPool($attributes, handle, attributes, sizeof(struct glhckShaderAttribute));
   setListFromPool($uniforms, handle, attributes, sizeof(struct glhckShaderUniform));
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   if (program) _glhckRenderGetAPI()->programDelete(program);
   IFDO(chckIterPoolFree, attributes);
   IFDO(chckIterPoolFree, uniforms);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief bind shader */
GLHCKAPI void glhckShaderBind(const glhckHandle handle)
{
   if (active == handle)
      return;

   _glhckRenderGetAPI()->programBind(_glhckShaderGetProgram(handle));
   active = handle;
}

GLHCKAPI const void* glhckShaderGetUniform(const glhckHandle handle, const char *uniform)
{
   CALL(0, "%s, %s", glhckHandleRepr(handle), uniform);
   assert(handle > 0);

   size_t memb;
   const struct glhckShaderUniform *uniforms = getList($uniforms, handle, &memb);
   for (size_t i = 0; i < memb; ++i) {
      if (!strcmp(uniform, uniforms[i].name)) {
         RET(0, "%p", &uniforms[i]);
         return &uniforms[i];
      }
   }

   RET(0, "%p", NULL);
   return NULL;
}

GLHCKAPI void glhckShaderUniform(const glhckHandle handle, const void *uniform, const int count, const void *value)
{
   const glhckHandle old = active;
   glhckShaderBind(handle);

   _glhckRenderGetAPI()->programUniform(_glhckShaderGetProgram(handle), uniform, count, value);

   if (old)
      glhckShaderBind(old);
}

/* \brief attach uniform buffer object to shader name can be left NULL, if uniform buffer was created from shader before */
GLHCKAPI int glhckShaderAttachHwBuffer(const glhckHandle handle, const glhckHandle hwBuffer, const char *name, const unsigned int index)
{
   assert(handle > 0 && hwBuffer > 0);
   assert(index != 0 && "index 0 is already taken by GLhck");

   if (!name && !(name = glhckHwBufferGetName(hwBuffer)))
      return RETURN_FAIL;

   const unsigned int location = _glhckRenderGetAPI()->programAttachUniformBuffer(_glhckShaderGetProgram(handle), name, index);

   glhckHwBufferBindRange(hwBuffer, index, 0, glhckHwBufferGetSize(hwBuffer));
   return (location ? RETURN_OK : RETURN_FAIL);
}

/* vm: set ts=8 sw=3 tw=0 :*/
