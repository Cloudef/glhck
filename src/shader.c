#include "internal.h"
#include <stdio.h> /* temporary printf */

#define GLHCK_CHANNEL GLHCK_CHANNEL_SHADER

/* \brief free current list of shader attributes */
static void _glhckShaderFreeAttributes(glhckShader *object)
{
   _glhckShaderAttribute *a, *an;
   assert(object);
   for (a = object->attributes; a; a = an) {
      an = a->next;
      _glhckFree(a->name);
      _glhckFree(a);
   }
   object->attributes = NULL;
}

/* \brief free current list of shader uniforms */
static void _glhckShaderFreeUniforms(glhckShader *object)
{
   _glhckShaderUniform *u, *un;
   assert(object);
   for (u = object->uniforms; u; u = un) {
      un = u->next;
      _glhckFree(u->name);
      _glhckFree(u);
   }
   object->uniforms = NULL;
}

/*
 * public api
 */

/* \brief compile new shader object
 * contentsFromMemory may be null
 *
 * NOTE: you need to free shaderObjects yourself! */
GLHCKAPI unsigned int glhckCompileShaderObject(glhckShaderType type,
      const char *effectKey, const char *contentsFromMemory)
{
   unsigned int obj = 0;
   CALL(0, "%u, %s, %s", type, effectKey, contentsFromMemory);
   obj = GLHCKRA()->shaderCompile(type, effectKey, contentsFromMemory);
   RET(0, "%u", obj);
   return obj;
}

/* \brief delete compiled shader object */
GLHCKAPI void glhckDeleteShaderObject(unsigned int shaderObject)
{
   CALL(0, "%u", shaderObject);
   GLHCKRA()->shaderDelete(shaderObject);
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
   if (!(object->program = GLHCKRA()->programLink(vertexShader, fragmentShader)))
      goto fail;

   /* get uniform and attribute lists */
   object->attributes = GLHCKRA()->programAttributeList(object->program);
   object->uniforms   = GLHCKRA()->programUniformList(object->program);

   _glhckShaderAttribute *a;
   _glhckShaderUniform *u;
   for (a = object->attributes; a; a = a->next)
      printf("(%s:%u) %d : %d [%s]\n", a->name, a->location, a->type, a->size, a->typeName);
   for (u = object->uniforms; u; u = u->next)
      printf("(%s:%u) %d : %d [%s]\n", u->name, u->location, u->type, u->size, u->typeName);

   /* insert to world */
   _glhckWorldInsert(shader, object, glhckShader*);

   RET(0, "%p", object);
   return object;

fail:
   if (object && object->program) {
      GLHCKRA()->programDelete(object->program);
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
GLHCKAPI unsigned int glhckShaderFree(glhckShader *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* unbind from active slot */
   if (GLHCKRD()->shader == object)
      glhckShaderBind(NULL);

   /* delete object */
   if (object->program)
      GLHCKRA()->programDelete(object->program);

   /* free attib && uniform lists */
   _glhckShaderFreeAttributes(object);
   _glhckShaderFreeUniforms(object);

   /* remove from world */
   _glhckWorldRemove(shader, object, glhckShader*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind shader */
GLHCKAPI void glhckShaderBind(glhckShader *object)
{
   if (GLHCKRD()->shader == object) return;
   GLHCKRA()->programBind(object?object->program:0);
   GLHCKRD()->shader = object;
}

/* \brief set uniform to shader */
GLHCKAPI void glhckShaderUniform(glhckShader *object, const char *uniform, int count, void *value)
{
   glhckShader *old;
   _glhckShaderUniform *u;
   assert(object);

   /* store old shader */
   old = GLHCKRD()->shader;

   /* search uniform */
   for (u = object->uniforms; u; u = u->next)
      if (!strcmp(uniform, u->name)) break;

   /* uniform not found */
   if (!u) return;

   /* shader isn't binded, bind it. */
   if (GLHCKRD()->shader != object)
      glhckShaderBind(object);

   // printf("SET UNIFORM %s(%s), %d, %p\n", u->name, u->typeName, count, value);

   /* set the uniform to program */
   GLHCKRA()->programUniform(object->program, u, count, value);

   /* restore old bind */
   if (old) glhckShaderBind(old);
}

/* \brief attach uniform buffer object to shader
 * name can be left NULL, if uniform buffer was created from shader before */
GLHCKAPI int glhckShaderAttachHwBuffer(glhckShader *object, glhckHwBuffer *buffer, const char *name, unsigned int index)
{
   unsigned int location;
   assert(object && buffer);
   assert(name || buffer->name);
   assert(index != 0 && "index 0 is already taken by GLhck");
   location = GLHCKRA()->programAttachUniformBuffer(object->program, (name?name:buffer->name), index);
   glhckHwBufferBindRange(buffer, index, 0, buffer->size);
   return (location?RETURN_OK:RETURN_FAIL);
}

/* vm: set ts=8 sw=3 tw=0 :*/
