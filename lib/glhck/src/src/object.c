#include "internal.h"
#include <assert.h>  /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT
#define ifree(x)  if (x) _glhckFree(x);
#define VEC3(v)   v->x, v->y, v->z
#define VEC3S     "vec3[%f, %f, %f]"

/* \brief update view matrix of object */
static void _glhckObjectUpdateMatrix(__GLHCKobjectView *view)
{
   kmMat4 translation, rotation, scaling, temp;
   CALL("%p", view);

   /* translation */
   kmMat4Translation(&translation,
         view->translation.x, view->translation.y, view->translation.z);

   /* rotation */
   kmMat4RotationX(&rotation, kmDegreesToRadians(view->rotation.x));
   kmMat4Multiply(&rotation, &rotation,
         kmMat4RotationY(&temp, kmDegreesToRadians(view->rotation.y)));
   kmMat4Multiply(&rotation, &rotation,
         kmMat4RotationZ(&temp, kmDegreesToRadians(view->rotation.z)));

   /* scaling */
   kmMat4Scaling(&scaling,
         view->scaling.x, view->scaling.y, view->scaling.z);

   /* build matrix */
   kmMat4Multiply(&translation, &translation, &rotation);
   kmMat4Multiply(&view->matrix, &translation, &scaling);

   /* done */
   view->update = 0;
}

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
   memset(&object->view, 0, sizeof(__GLHCKobjectView));

   /* default view matrix */
   object->view.scaling.x = 100;
   object->view.scaling.y = 100;
   object->view.scaling.z = 100;
   _glhckObjectUpdateMatrix(&object->view);

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

   /* does view matrix need update? */
   if (object->view.update)
      _glhckObjectUpdateMatrix(&object->view);

   _GLHCKlibrary.render.api.objectDraw(object);
}

/* \brief position object */
GLHCKAPI void glhckObjectPosition(glhckObject *object, const kmVec3 *position)
{
   assert(object && position);
   CALL("%p, "VEC3S, object, VEC3(position));

   object->view.translation = *position;
   object->view.update      = 1;
}

/* \brief position object (with kmScalar) */
GLHCKAPI void glhckObjectPositionf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 position = { x, y, z };
   glhckObjectPosition(object, &position);
}

/* \brief move object */
GLHCKAPI void glhckObjectMove(glhckObject *object, const kmVec3 *move)
{
   assert(object && move);
   CALL("%p, "VEC3S, object, VEC3(move));

   kmVec3Add(&object->view.translation,
         &object->view.translation, move);
   object->view.update = 1;
}

/* \brief move object (with kmScalar) */
GLHCKAPI void glhckObjectMovef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 move = { x, y, z };
   glhckObjectMove(object, &move);
}

/* \brief rotate object */
GLHCKAPI void glhckObjectRotate(glhckObject *object, const kmVec3 *rotate)
{
   assert(object && rotate);
   CALL("%p, "VEC3S, object, VEC3(rotate));

   object->view.rotation   = *rotate;
   object->view.update     = 1;
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckObjectRotatef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotation = { x, y, z };
   glhckObjectRotate(object, &rotation);
}

/* \brief scale object */
GLHCKAPI void glhckObjectScale(glhckObject *object, const kmVec3 *scale)
{
   assert(object && scale);
   CALL("%p, "VEC3S, object, VEC3(scale));

   object->view.scaling = *scale;
   object->view.update  = 1;
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckObjectScalef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 scaling = { x, y, z };
   glhckObjectScale(object, &scaling);
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
