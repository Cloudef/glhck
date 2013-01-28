#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_SHADER

/* \brief compile new shader object
 * contentsFromMemory may be null
 *
 * NOTE: you need to free shaderObjects yourself! */
GLHCKAPI unsigned int glhckCompileShaderObject(glhckShaderType type,
      const char *effectKey, const char *contentsFromMemory)
{
   unsigned int obj = 0;
   CALL(0, "%u, %s, %s", type, effectKey, contentsFromMemory);
   assert(effectKey);
   obj = _GLHCKlibrary.render.api.compileShader(type, effectKey, contentsFromMemory);
   RET(0, "%u", obj);
   return obj;
}

/* \brief delete compiled shader object */
GLHCKAPI void glhckDeleteShaderObject(unsigned int shaderObject)
{
   CALL(0, "%u", shaderObject);
   _GLHCKlibrary.render.api.deleteShader(shaderObject);
}

/* \brief allocate new shader object
 * contentsFromMemory may be null */
GLHCKAPI glhckShader* glhckShaderNew(
      const char *vertexEffect, const char *fragmentEffect, const char *contentsFromMemory)
{
   glhckShader *object = NULL;
   unsigned int vsobj, fsobj;
   CALL(0, "%s, %s, %s", vertexEffect, fragmentEffect, contentsFromMemory);

   /* compile base shaders */
   vsobj = glhckCompileShaderObject(GLHCK_VERTEX_SHADER, vertexEffect, contentsFromMemory);
   fsobj = glhckCompileShaderObject(GLHCK_FRAGMENT_SHADER, fragmentEffect, contentsFromMemory);

   /* fail compiling shaders */
   if (!vsobj || !fsobj)
      goto fail;

   object = glhckShaderNewWithShaderObjects(vsobj, fsobj);
   RET(0, "%p", object);
   return object;

fail:
   if (vsobj) glhckDeleteShaderObject(vsobj);
   if (fsobj) glhckDeleteShaderObject(fsobj);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief allocate new shader object with GL shader objects */
GLHCKAPI glhckShader* glhckShaderNewWithShaderObjects(
      unsigned int vertexShader, unsigned int fragmentShader)
{
   glhckShader *object = NULL;
   CALL(0, "%u, %u", vertexShader, fragmentShader);
   assert(vertexShader != 0 && fragmentShader != 0);

   if (!(object = _glhckCalloc(1, sizeof(glhckShader))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* link shader program */
   if (!(object->program = _GLHCKlibrary.render.api.linkProgram(vertexShader, fragmentShader)))
      goto fail;

   /* insert to world */
   _glhckWorldInsert(slist, object, _glhckShader*);

   RET(0, "%p", object);
   return object;

fail:
   if (object && object->program) {
      _GLHCKlibrary.render.api.deleteProgram(object->program);
   }
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference shader object */
GLHCKAPI glhckShader* glhckShaderRef(glhckShader *object)
{
   CALL(3, "%p", object);
   assert(object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free shader object */
GLHCKAPI size_t glhckShaderFree(glhckShader *object)
{
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   if (object->program)
      _GLHCKlibrary.render.api.deleteProgram(object->program);

   /* remove from world */
   _glhckWorldRemove(slist, object, _glhckShader*);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind shader */
GLHCKAPI void glhckShaderBind(glhckShader *object)
{
   assert(object);
   _GLHCKlibrary.render.api.bindProgram(object->program);
}

/* vim: set ts=8 sw=3 tw=0 :*/
