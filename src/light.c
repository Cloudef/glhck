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
   _glhckWorldInsert(light, object, glhckLight*);

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
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free light's object */
   NULLDO(glhckObjectFree, object->object);

   /* remove object from world */
   _glhckWorldRemove(light, object, glhckLight*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief bind/unbind glhck light */
GLHCKAPI void glhckLightBind(glhckLight *object)
{
   CALL(2, "%p", object);
   GLHCKRA()->lightBind(object);
   GLHCKRD()->light = object;
}

/* \brief begin projection from light with camera */
GLHCKAPI void glhckLightBeginProjectionWithCamera(glhckLight *object, glhckCamera *camera)
{
   glhckObject *cobj, *lobj;
   assert(object && camera);
   lobj = glhckLightGetObject(object);
   cobj = glhckCameraGetObject(camera);

   /* this is pointless */
   memcpy(&object->current.objectView, &lobj->view, sizeof(__GLHCKobjectView));

   /* store old camera view state */
   memcpy(&object->old.cameraView, &camera->view, sizeof(__GLHCKcameraView));
   memcpy(&object->old.objectView, &cobj->view, sizeof(__GLHCKobjectView));

   /* set new camera view state */
   memcpy(&cobj->view, &object->current.objectView, sizeof(__GLHCKobjectView));
   //memcpy(&camera->view, &object->current.cameraView, sizeof(__GLHCKcameraView));
   glhckCameraUpdate(camera);
}

/* \brief end projection from light with camera */
GLHCKAPI void glhckLightEndProjectionWithCamera(glhckLight *object, glhckCamera *camera)
{
   glhckObject *cobj;
   assert(object && camera);
   cobj = glhckCameraGetObject(camera);

   /* restore old camera view state */
   memcpy(&cobj->view, &object->old.objectView, sizeof(__GLHCKobjectView));
   memcpy(&camera->view, &object->old.cameraView, sizeof(__GLHCKcameraView));
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
   CALL(2, "%p, "COLBS, object, COLB(color));
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
   CALL(2, "%p, %f", object, factor);
   assert(object);
   object->options.pointLightFactor = factor;
}

/* \brief get light's point light factor */
GLHCKAPI kmScalar glhckLightGetPointLightFactor(glhckLight *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->options.pointLightFactor);
   return object->options.pointLightFactor;
}

/* \brief set light's atten factor */
GLHCKAPI void glhckLightAtten(glhckLight *object, const kmVec3 *atten)
{
   CALL(2, "%p, "VEC3S, object, VEC3(atten));
   assert(object && atten);
   kmVec3Assign(&object->options.atten, atten);
   object->options.atten.x *= 0.01;
   object->options.atten.y *= 0.01;
   object->options.atten.z *= 0.01;
}

/* \brief set light's atten factor (kmScalar) */
GLHCKAPI void glhckLightAttenf(glhckLight *object, kmScalar constant, kmScalar linear, kmScalar quadratic)
{
   kmVec3 atten = {constant, linear, quadratic};
   glhckLightAtten(object, &atten);
}

/* \brief get light's atten */
GLHCKAPI const kmVec3* glhckLightGetAtten(glhckLight *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->options.atten);
   return &object->options.atten;
}

/* \brief set light's cutout (kmScalar) */
GLHCKAPI void glhckLightCutoutf(glhckLight *object, kmScalar outer, kmScalar inner)
{
   kmVec2 cutout = {outer, inner};
   glhckLightCutout(object, &cutout);
}

/* \brief set light's cutout */
GLHCKAPI void glhckLightCutout(glhckLight *object, const kmVec2 *cutout)
{
   CALL(2, "%p, "VEC2S, object, VEC2(cutout));
   kmVec2Assign(&object->options.cutout, cutout);
}

/* \brief get light's cutout */
GLHCKAPI const kmVec2* glhckLightGetCutout(glhckLight *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->options.cutout);
   return &object->options.cutout;
}

/* vim: set ts=8 sw=3 tw=0 :*/
