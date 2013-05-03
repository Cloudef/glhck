#include "internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_MATERIAL

/* \brief allocate new material object */
GLHCKAPI glhckMaterial* glhckMaterialNew(void)
{
   glhckMaterial *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckMaterial))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(material, object, glhckMaterial*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference material object */
GLHCKAPI glhckMaterial* glhckMaterialRef(glhckMaterial *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free material object */
GLHCKAPI unsigned int glhckMaterialFree(glhckMaterial *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free texture */
   IFDO(glhckTextureFree, object->texture);

   /* free shader */
   IFDO(glhckShaderFree, object->shader);

   /* remove from world */
   _glhckWorldRemove(material, object, glhckMaterial*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
