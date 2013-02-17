#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_LIGHT

/* \brief allocate new light object */
GLHCKAPI glhckLight* glhckLightNew(void)
{
   _glhckLight *object;
   TRACE(0);

   /* allocate light */
   if (!(object = _glhckCalloc(1, sizeof(glhckLight))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* initialize light's object */
   if (!(object->object = glhckObjectNew()))
      goto fail;

   /* defaults */
   glhckLightColorb(object, 255, 255, 255, 255);
   glhckLightAttenf(object, 0.0, 0.0, 0.1f);
   glhckLightPointLightFactor(object, 1.0);

    /* insert to world */
   _glhckWorldInsert(llist, object, glhckLight*);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference light */
GLHCKAPI glhckLight* glhckLightRef(glhckLight *object)
{
   CALL(3, "%p", object);
   assert(object);

   /* increase reference */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free light object */
GLHCKAPI size_t glhckLightFree(glhckLight *object)
{
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* not initialized */
   if (!_glhckInitialized) return 0;

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free light's object */
   NULLDO(glhckObjectFree, object->object);

   /* remove object from world */
   _glhckWorldRemove(llist, object, glhckLight*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind/unbind glhck light */
GLHCKAPI void glhckLightBind(glhckLight *object)
{
   CALL(3, "%p", object);
   GLHCKRA()->lightBind(object);
   GLHCKRD()->light = object;
}

/* \brief begin projection from light with camera */
GLHCKAPI void glhckLightBeginProjectionWithCamera(glhckLight *object, glhckCamera *camera)
{
   glhckObject *obj;
   assert(object && camera);
   obj = glhckCameraGetObject(camera);
   memcpy(&object->oldPosition, glhckObjectGetPosition(obj), sizeof(kmVec3));
   memcpy(&object->oldTarget, glhckObjectGetTarget(obj), sizeof(kmVec3));
   object->oldNear = camera->view.near;
   object->oldFar  = camera->view.far;
   glhckObjectPosition(obj, glhckObjectGetPosition(object->object));
   glhckObjectTarget(obj, glhckObjectGetTarget(object->object));
   glhckCameraUpdate(camera);
}

/* \brief end projection from light with camera */
GLHCKAPI void glhckLightEndProjectionWithCamera(glhckLight *object, glhckCamera *camera)
{
   glhckObject *obj;
   assert(object && camera);
   obj = glhckCameraGetObject(camera);
   glhckObjectPosition(obj, &object->oldPosition);
   glhckObjectTarget(obj, &object->oldTarget);
   glhckCameraRange(camera, object->oldNear, object->oldFar);
   glhckCameraUpdate(camera);
}

/* \brief return light's object */
GLHCKAPI glhckObject* glhckLightGetObject(const glhckLight *object)
{
   CALL(2, "%p", object);
   assert(object);

   RET(2, "%p", object->object);
   return object->object;
}

/* \brief color light */
GLHCKAPI void glhckLightColor(glhckLight *object, const glhckColorb *color)
{
   CALL(3, "%p, "COLBS, object, COLB(color));
   assert(object && color);
   glhckObjectColor(glhckLightGetObject(object), color);
}

/* \brief color light (char) */
GLHCKAPI void glhckLightColorb(glhckLight *object, char r, char g, char b, char a)
{
   glhckColorb color = {r, g, b, a};
   glhckLightColor(object, &color);
}

/* \brief 1.0 for pointlight */
GLHCKAPI void glhckLightPointLightFactor(glhckLight *object, kmScalar factor)
{
   CALL(3, "%p, %f", object, factor);
   assert(object);
   object->pointLightFactor = factor;
}

/* \brief set light's atten factor */
GLHCKAPI void glhckLightAtten(glhckLight *object, const kmVec3 *atten)
{
   CALL(3, "%p, "VEC3S, object, VEC3(atten));
   assert(object && atten);
   kmVec3Assign(&object->atten, atten);
}

/* \brief set light's atten factor (kmScalar) */
GLHCKAPI void glhckLightAttenf(glhckLight *object, kmScalar constant, kmScalar linear, kmScalar quadratic)
{
   kmVec3 atten = {constant, linear, quadratic};
   glhckLightAtten(object, &atten);
}

/* \brief set light's cutout */
GLHCKAPI void glhckLightCutout(glhckLight *object, const kmVec2 *cutout)
{
   CALL(3, "%p, "VEC2S, object, VEC2(cutout));
   kmVec2Assign(&object->cutout, cutout);
}

/* \brief set light's cutout (kmScalar) */
GLHCKAPI void glhckLightCutoutf(glhckLight *object, kmScalar outer, kmScalar inner)
{
   kmVec2 cutout = {outer, inner};
   glhckLightCutout(object, &cutout);
}

/* vim: set ts=8 sw=3 tw=0 :*/
