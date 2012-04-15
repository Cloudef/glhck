#include "internal.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT
#define ifree(x) if (x) _glhckFree(x);

/* public api */

/* \brief new object */
GLHCKAPI glhckObject *glhckObjectNew(void)
{
   _glhckObject *object;
   TRACE();

   if (!(object = _glhckMalloc(sizeof(_glhckObject))))
      goto fail;

   /* init object */
   memset(object, 0, sizeof(_glhckObject));
   memset(&object->geometry, 0, sizeof(__GLHCKobjectGeometry));

   /* increase reference */
   object->refCounter++;

   RET("%p", object);
   return object;

fail:
   RET("%p", NULL);
   return NULL;
}

/* \brief free object */
GLHCKAPI short glhckObjectFree(glhckObject *object)
{
   assert(object);
   CALL("%p", object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   DEBUG(GLHCK_DBG_CRAP, "FREE object");

   /* free geometry */
   ifree(object->geometry.vertexData);
   ifree(object->geometry.indices);

   /* free */
   _glhckFree(object);

success:
   RET("%d", object->refCounter);
   return object->refCounter;
}

/* \brief draw object */
GLHCKAPI void glhckObjectDraw(glhckObject *object)
{
   assert(object);
   CALL("%p", object);
   _GLHCKlibrary.render.api.objectDraw(object);
}

/* \brief insert vertex data into object */
GLHCKAPI int glhckObjectInsertVertexData(
      _glhckObject *object,
      size_t memb,
      const glhckVertexData *vertexData)
{
   glhckVertexData *new;
   assert(object);
   CALL("%p, %zu, %p", object, memb, vertexData);

   /* allocate new buffer */
   if (!(new = (glhckVertexData*)_glhckMalloc(memb *
               sizeof(glhckVertexData))))
      goto fail;

   /* free old buffer */
   if (object->geometry.vertexData)
      _glhckFree(object->geometry.vertexData);

   /* assign new buffer */
   memcpy(new, vertexData, memb *
         sizeof(glhckVertexData));
   object->geometry.vertexData = new;
   object->geometry.vertexCount = memb;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert index data into object */
GLHCKAPI int glhckObjectInsertIndices(
      _glhckObject *object,
      size_t memb,
      const GLHCK_CAST_INDEX *indices)
{
   GLHCK_CAST_INDEX *new;
   assert(object);
   CALL("%p, %zu, %p", object, memb, indices);

   /* allocate new buffer */
   if (!(new = (GLHCK_CAST_INDEX*)_glhckMalloc(memb *
               sizeof(GLHCK_CAST_INDEX))))
      goto fail;

   /* free old buffer */
   if (object->geometry.indices)
      _glhckFree(object->geometry.indices);

   /* assign new buffer */
   memcpy(new, indices, memb *
         sizeof(GLHCK_CAST_INDEX));
   object->geometry.indices = new;
   object->geometry.indicesCount = memb;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}
