#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_CAMERA

/* \brief calculate projection matrix */
static void _glhckCameraProjectionMatrix(_glhckCamera *camera)
{
   kmScalar w, h;
   float distanceFromZero;

   CALL(2, "%p", camera);
   assert(camera);

   switch(camera->view.projectionType)
   {
      case GLHCK_PROJECTION_ORTHOGRAPHIC:
         w = camera->view.viewport.w > camera->view.viewport.h ? 1 :
               camera->view.viewport.w / camera->view.viewport.h;
         h = camera->view.viewport.w < camera->view.viewport.h ? 1 :
               camera->view.viewport.h / camera->view.viewport.w;

         distanceFromZero = sqrtf(camera->object->view.translation.x * camera->object->view.translation.x +
                                  camera->object->view.translation.y * camera->object->view.translation.y +
                                  camera->object->view.translation.z * camera->object->view.translation.z);

         w *= (distanceFromZero+camera->view.near)/2;
         h *= (distanceFromZero+camera->view.near)/2;

         kmMat4OrthographicProjection(&camera->view.projection,
            -w, w, -h, h, camera->view.near, camera->view.far);
         break;

      case GLHCK_PROJECTION_PERSPECTIVE:
      default:
         kmMat4PerspectiveProjection(
               &camera->view.projection,
               camera->view.fov,
               (float)camera->view.viewport.w/(float)camera->view.viewport.h,
               camera->view.near, camera->view.far);
         break;
   }
}

/* \brief calcualate view matrix */
static void _glhckCameraViewMatrix(_glhckCamera *camera)
{
   kmMat4 rotation, tmp;
   kmVec3 upvector;
   CALL(2, "%p", camera);
   assert(camera);

   /* build rotation for upvector */
   kmMat4RotationAxisAngle(&rotation, &(kmVec3){0,0,1},
         kmDegreesToRadians(camera->object->view.rotation.z));
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){0,1,0},
         kmDegreesToRadians(camera->object->view.rotation.y));
   kmMat4Multiply(&rotation, &rotation, &tmp);
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){1,0,0},
         kmDegreesToRadians(camera->object->view.rotation.x));
   kmMat4Multiply(&rotation, &rotation, &tmp);

   /* assuming upvector is normalized */
   kmVec3Transform(&upvector, &camera->view.upVector, &rotation);

   /* build view matrix */
   kmMat4LookAt(&camera->view.view, &camera->object->view.translation,
         &camera->object->view.target, &upvector);
   kmMat4Multiply(&camera->view.viewProj,
         &camera->view.projection, &camera->view.view);
   glhckFrustumBuild(&camera->frustum, &camera->view.viewProj);
}

/* \brief update the camera stack after window resize */
void _glhckCameraWorldUpdate(int width, int height)
{
   _glhckCamera *camera;
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
   _glhckCamera *camera;
   TRACE(0);

   /* allocate camera */
   if (!(camera = _glhckCalloc(1, sizeof(_glhckCamera))))
      goto fail;

   /* increase reference */
   camera->refCounter++;

   /* initialize camera's object */
   if (!(camera->object = glhckObjectNew()))
      goto fail;

   /* defaults */
   camera->view.projectionType = GLHCK_PROJECTION_PERSPECTIVE;
   camera->view.near = 1.0f;
   camera->view.far  = 100.0f;
   camera->view.fov  = 35.0f;

   /* reset */
   glhckCameraReset(camera);

   /* insert to world */
   _glhckWorldInsert(camera, camera, _glhckCamera*);

   RET(0, "%p", camera);
   return camera;

fail:
   IFDO(_glhckFree, camera);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference camera */
GLHCKAPI glhckCamera* glhckCameraRef(glhckCamera *camera)
{
   CALL(3, "%p", camera);
   assert(camera);

   /* increase reference */
   camera->refCounter++;

   RET(3, "%p", camera);
   return camera;
}

/* \brief free camera */
GLHCKAPI size_t glhckCameraFree(glhckCamera *camera)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(camera), "%p", camera);
   assert(camera);

   /* there is still references to this object alive */
   if (--camera->refCounter != 0) goto success;

   /* remove camera from global state */
   if (GLHCKRD()->camera == camera)
      GLHCKRD()->camera = NULL;

   /* free camera's object */
   NULLDO(glhckObjectFree, camera->object);

   /* remove camera from world */
   _glhckWorldRemove(camera, camera, _glhckCamera*);

   /* free */
   NULLDO(_glhckFree, camera);

success:
   RET(FREE_RET_PRIO(camera), "%d", camera?camera->refCounter:0);
   return camera?camera->refCounter:0;
}

/* \brief update the camera */
GLHCKAPI void glhckCameraUpdate(glhckCamera *camera)
{
   CALL(2, "%p", camera);

   /* bind none */
   if (!camera) {
      GLHCKRD()->camera = NULL;
      return;
   }

   if (GLHCKRD()->camera != camera || camera->view.updateViewport) {
      glhckRenderViewport(
            camera->view.viewport.x,
            camera->view.viewport.y,
            camera->view.viewport.w,
            camera->view.viewport.h);
      camera->view.updateViewport = 0;
   }

   if (camera->view.update || camera->object->view.update) {
      if (camera->view.projectionType == GLHCK_PROJECTION_ORTHOGRAPHIC)
         _glhckCameraProjectionMatrix(camera);
      _glhckCameraViewMatrix(camera);
      camera->view.update = 0;
   }

   /* assign camera to global state */
   GLHCKRA()->setProjection(&camera->view.projection);
   GLHCKRA()->setView(&camera->view.view);
   GLHCKRD()->camera = camera;
}

/* \brief reset camera to default state */
GLHCKAPI void glhckCameraReset(glhckCamera *camera)
{
   CALL(0, "%p", camera);
   assert(camera);

   camera->view.update = 1;
   kmVec3Fill(&camera->view.upVector, 0, 1, 0);
   kmVec3Fill(&camera->object->view.rotation, 0, 0, 0);
   kmVec3Fill(&camera->object->view.target, 0, 0, 0);
   kmVec3Fill(&camera->object->view.translation, 0, 0, 0);
   memset(&camera->view.viewport, 0, sizeof(glhckRect));
   camera->view.viewport.w = GLHCKR()->width;
   camera->view.viewport.h = GLHCKR()->height;

   _glhckCameraProjectionMatrix(camera);
}

/* \brief set camera's projection type */
GLHCKAPI void glhckCameraProjection(glhckCamera *camera, const glhckProjectionType projectionType)
{
   CALL(1, "%p, %d", camera, projectionType);
   assert(camera);
   camera->view.projectionType = projectionType;
   _glhckCameraProjectionMatrix(camera);
}

/* \brief get camera's frustum */
GLHCKAPI glhckFrustum* glhckCameraGetFrustum(glhckCamera *camera)
{
   CALL(1, "%p", camera);
   assert(camera);
   RET(1, "%p", &camera->frustum);
   return &camera->frustum;
}

/* \brief get camera's view matrix */
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckCamera *camera)
{
   CALL(1, "%p", camera);
   assert(camera);
   RET(1, "%p", &camera->view.view);
   return &camera->view.view;
}

/* \brief get camera's projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckCamera *camera)
{
   CALL(1, "%p", camera);
   assert(camera);
   RET(1, "%p", &camera->view.projection);
   return &camera->view.projection;
}

/* \brief get camera's model view projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetVPMatrix(const glhckCamera *camera)
{
   CALL(1, "%p", camera);
   assert(camera);
   RET(1, "%p", &camera->view.viewProj);
   return &camera->view.viewProj;
}

/* \brief set camera's up vector */
GLHCKAPI void glhckCameraUpVector(glhckCamera *camera, const kmVec3 *upVector)
{
   CALL(1, "%p, "VEC3S, camera, VEC3(upVector));
   kmVec3Assign(&camera->view.upVector, upVector);
}

/* \brief set camera's fov */
GLHCKAPI void glhckCameraFov(glhckCamera *camera, const kmScalar fov)
{
   CALL(1, "%p, %f", camera, fov);
   assert(camera);
   camera->view.fov = fov;
   _glhckCameraProjectionMatrix(camera);
}

/* \brief set camera's range */
GLHCKAPI void glhckCameraRange(glhckCamera *camera,
      const kmScalar near, const kmScalar far)
{
   CALL(1, "%p, %f, %f", camera, near, far);
   assert(camera);

   if (camera->view.near == near &&
       camera->view.far  == far)
      return;

   camera->view.near = near;
   camera->view.far  = far;
   _glhckCameraProjectionMatrix(camera);
}

/* \brief set camera's viewport */
GLHCKAPI void glhckCameraViewport(glhckCamera *camera, const glhckRect *viewport)
{
   CALL(1, "%p, "RECTS, camera, RECT(viewport));
   assert(camera);

   if (camera->view.viewport.x == viewport->x &&
       camera->view.viewport.y == viewport->y &&
       camera->view.viewport.w == viewport->w &&
       camera->view.viewport.h == viewport->h)
      return;

   memcpy(&camera->view.viewport, viewport, sizeof(glhckRect));
   _glhckCameraProjectionMatrix(camera);
   camera->view.update = 1;
   camera->view.updateViewport = 1;
}

/* \brief set camera's viewport (kmScalar) */
GLHCKAPI void glhckCameraViewportf(glhckCamera *camera,
      int x, int y, int w, int h)
{
   const glhckRect viewport = { x, y, w, h };
   glhckCameraViewport(camera, &viewport);
}

/* \brief get camera's object */
GLHCKAPI glhckObject* glhckCameraGetObject(const glhckCamera *camera)
{
   CALL(1, "%p", camera);
   assert(camera);
   RET(1, "%p", camera->object);
   return camera->object;
}
