#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_CAMERA

/* the cameras in glhck contain right-handed projection
 *
 * WORLD/3D
 *    y     -z FAR
 * -x | x   /
 *   -y    z NEAR
 */

/* \brief calculate projection matrix */
static void _glhckCameraProjectionMatrix(glhckCamera *object)
{
   kmScalar w, h, distanceFromTarget;
   kmVec3 toTarget;

   CALL(2, "%p", object);
   assert(object);

   switch(object->view.projectionType)
   {
      case GLHCK_PROJECTION_ORTHOGRAPHIC:
         w = object->view.viewport.w > object->view.viewport.h ? 1 :
               object->view.viewport.w / object->view.viewport.h;
         h = object->view.viewport.w < object->view.viewport.h ? 1 :
               object->view.viewport.h / object->view.viewport.w;

         kmVec3Subtract(&toTarget, &object->object->view.translation, &object->object->view.target);
         distanceFromTarget = kmVec3Length(&toTarget);

         w *= (distanceFromTarget+object->view.near)/2;
         h *= (distanceFromTarget+object->view.near)/2;

         kmMat4OrthographicProjection(&object->view.projection,
            -w, w, -h, h, object->view.near, object->view.far);
         break;

      case GLHCK_PROJECTION_PERSPECTIVE:
      default:
         kmMat4PerspectiveProjection(
               &object->view.projection,
               object->view.fov,
               (float)object->view.viewport.w/(float)object->view.viewport.h,
               object->view.near, object->view.far);
         break;
   }
}

/* \brief calcualate view matrix */
static void _glhckCameraViewMatrix(glhckCamera *object)
{
   kmMat4 rotation, tmp;
   kmVec3 upvector;
   CALL(2, "%p", object);
   assert(object);

   /* build rotation for upvector */
   kmMat4RotationAxisAngle(&rotation, &(kmVec3){0,0,1},
         kmDegreesToRadians(object->object->view.rotation.z));
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){0,1,0},
         kmDegreesToRadians(object->object->view.rotation.y));
   kmMat4Multiply(&rotation, &rotation, &tmp);
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){1,0,0},
         kmDegreesToRadians(object->object->view.rotation.x));
   kmMat4Multiply(&rotation, &rotation, &tmp);

   /* assuming upvector is normalized */
   kmVec3Transform(&upvector, &object->view.upVector, &rotation);

   /* build view matrix */
   kmMat4LookAt(&object->view.view, &object->object->view.translation,
         &object->object->view.target, &upvector);
   kmMat4Multiply(&object->view.viewProj,
         &object->view.projection, &object->view.view);
   glhckFrustumBuild(&object->frustum, &object->view.viewProj);
}

/* \brief update the camera stack after window resize */
void _glhckCameraWorldUpdate(int width, int height)
{
   glhckCamera *camera;
   int diffw, diffh;
   CALL(2, "%d, %d", width, height);

   /* get difference of old dimensions and now */
   diffw = width  - GLHCKR()->width;
   diffh = height - GLHCKR()->height;

   for (camera = GLHCKW()->camera; camera; camera = camera->next) {
      glhckCameraViewportf(camera,
            camera->view.viewport.x,
            camera->view.viewport.y,
            camera->view.viewport.w + diffw,
            camera->view.viewport.h + diffh);
   }

   /* no camera binded, upload default projection */
   if (!(camera = GLHCKRD()->camera)) {
      _glhckRenderDefaultProjection(width, height);
      glhckRenderViewport(0, 0, width, height);
   } else {
      /* update camera */
      GLHCKRD()->camera = NULL;
      camera->view.update = 1;
      glhckCameraUpdate(camera);
   }
}

/***
 * public api
 ***/

/* \brief allocate new camera */
GLHCKAPI glhckCamera* glhckCameraNew(void)
{
   glhckCamera *object;


   /* allocate camera */
   if (!(object = _glhckCalloc(1, sizeof(glhckCamera))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* initialize camera's object */
   if (!(object->object = glhckObjectNew()))
      goto fail;

   /* defaults */
   object->view.projectionType = GLHCK_PROJECTION_PERSPECTIVE;
   object->view.near = 1.0f;
   object->view.far  = 100.0f;
   object->view.fov  = 35.0f;

   /* reset */
   glhckCameraReset(object);

   /* insert to world */
   _glhckWorldInsert(camera, object, glhckCamera*);

   RET(0, "%p", object);
   return object;

fail:
   IFDO(_glhckFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference camera */
GLHCKAPI glhckCamera* glhckCameraRef(glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);

   /* increase reference */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free camera */
GLHCKAPI unsigned int glhckCameraFree(glhckCamera *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* remove camera from global state */
   if (GLHCKRD()->camera == object)
      GLHCKRD()->camera = NULL;

   /* free camera's object */
   NULLDO(glhckObjectFree, object->object);

   /* remove camera from world */
   _glhckWorldRemove(camera, object, glhckCamera*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief update the camera */
GLHCKAPI void glhckCameraUpdate(glhckCamera *object)
{
   CALL(2, "%p", object);

   /* bind none */
   if (!object) {
      GLHCKRD()->camera = NULL;
      return;
   }

   if (GLHCKRD()->camera != object || object->view.updateViewport) {
      glhckRenderViewport(
            object->view.viewport.x,
            object->view.viewport.y,
            object->view.viewport.w,
            object->view.viewport.h);
      object->view.updateViewport = 0;
   }

   if (object->view.update || object->object->view.update) {
      if (object->view.projectionType == GLHCK_PROJECTION_ORTHOGRAPHIC)
         _glhckCameraProjectionMatrix(object);
      _glhckCameraViewMatrix(object);
      object->view.update = 0;
   }

   /* assign camera to global state */
   glhckRenderProjection(&object->view.projection);
   glhckRenderView(&object->view.view);
   GLHCKRD()->camera = object;
}

/* \brief reset camera to default state */
GLHCKAPI void glhckCameraReset(glhckCamera *object)
{
   CALL(0, "%p", object);
   assert(object);

   object->view.update = 1;
   kmVec3Fill(&object->view.upVector, 0, 1, 0);
   kmVec3Fill(&object->object->view.rotation, 0, 0, 0);
   kmVec3Fill(&object->object->view.target, 0, 0, 0);
   kmVec3Fill(&object->object->view.translation, 0, 0, 0);
   memset(&object->view.viewport, 0, sizeof(glhckRect));
   object->view.viewport.w = GLHCKR()->width;
   object->view.viewport.h = GLHCKR()->height;
   _glhckCameraProjectionMatrix(object);
}

/* \brief set camera's projection type */
GLHCKAPI void glhckCameraProjection(glhckCamera *object, const glhckProjectionType projectionType)
{
   CALL(2, "%p, %d", object, projectionType);
   assert(object);
   object->view.projectionType = projectionType;
   _glhckCameraProjectionMatrix(object);
}

/* \brief get camera's frustum */
GLHCKAPI glhckFrustum* glhckCameraGetFrustum(glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", &object->frustum);
   return &object->frustum;
}

/* \brief get camera's view matrix */
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", &object->view.view);
   return &object->view.view;
}

/* \brief get camera's projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", &object->view.projection);
   return &object->view.projection;
}

/* \brief get camera's model view projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetVPMatrix(const glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", &object->view.viewProj);
   return &object->view.viewProj;
}

/* \brief set camera's up vector */
GLHCKAPI void glhckCameraUpVector(glhckCamera *object, const kmVec3 *upVector)
{
   CALL(1, "%p, "VEC3S, object, VEC3(upVector));
   kmVec3Assign(&object->view.upVector, upVector);
}

/* \brief set camera's fov */
GLHCKAPI void glhckCameraFov(glhckCamera *object, const kmScalar fov)
{
   CALL(1, "%p, %f", object, fov);
   assert(object);
   object->view.fov = fov;
   _glhckCameraProjectionMatrix(object);
}

/* \brief set camera's range */
GLHCKAPI void glhckCameraRange(glhckCamera *object,
      const kmScalar near, const kmScalar far)
{
   CALL(1, "%p, %f, %f", object, near, far);
   assert(object);

   if (object->view.near == near &&
       object->view.far  == far)
      return;

   object->view.near = near;
   object->view.far  = far;
   _glhckCameraProjectionMatrix(object);
}

/* \brief set camera's viewport */
GLHCKAPI void glhckCameraViewport(glhckCamera *object, const glhckRect *viewport)
{
   CALL(1, "%p, "RECTS, object, RECT(viewport));
   assert(object);

   if (object->view.viewport.x == viewport->x &&
       object->view.viewport.y == viewport->y &&
       object->view.viewport.w == viewport->w &&
       object->view.viewport.h == viewport->h)
      return;

   memcpy(&object->view.viewport, viewport, sizeof(glhckRect));
   _glhckCameraProjectionMatrix(object);
   object->view.update = 1;
   object->view.updateViewport = 1;
}

/* \brief set camera's viewport (kmScalar) */
GLHCKAPI void glhckCameraViewportf(glhckCamera *object,
      int x, int y, int w, int h)
{
   const glhckRect viewport = { x, y, w, h };
   glhckCameraViewport(object, &viewport);
}

/* \brief get camera's object */
GLHCKAPI glhckObject* glhckCameraGetObject(const glhckCamera *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->object);
   return object->object;
}
