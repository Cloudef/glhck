#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_SHADER

/* \brief allocate new shader object */
GLHCKAPI glhckShader* glhckShaderNew(
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

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
