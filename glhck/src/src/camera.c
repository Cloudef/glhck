#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_CAMERA

/* \brief calculate projection matrix */
static void _glhckCameraProjectionMatrix(_glhckCamera *camera)
{
   CALL(2, "%p", camera);
   assert(camera);

   kmMat4PerspectiveProjection(
         &camera->view.projection,
         camera->view.fov,
         camera->view.viewport.z/camera->view.viewport.w,
         camera->view.near, camera->view.far);
}

/* \brief calcualate view matrix */
static void _glhckCameraViewMatrix(_glhckCamera *camera)
{
   kmVec3 tgtv, upvector;
   kmMat4 view;
   CALL(2, "%p", camera);
   assert(camera);

   kmVec3Subtract(&tgtv, &camera->view.target,
         &camera->view.translation);
   kmVec3Normalize(&tgtv, &tgtv);
   kmVec3Normalize(&upvector, &camera->view.upVector);

   if (kmVec3Dot(&tgtv, &upvector) == 1.f)
      upvector.x += 0.5f;

   kmMat4LookAt(&view, &camera->view.translation,
         &camera->view.target, &upvector);

   /* TODO: frustrum calculation here */

   kmMat4Multiply(&camera->view.matrix,
         &camera->view.projection, &view);
}

/* \brief update the camera stack after window resize */
void _glhckCameraWorldUpdate(int width, int height)
{
   _glhckCamera *camera;
   int diffw, diffh;
   CALL(2, "%d, %d", width, height);

   /* get difference of old dimensions and now */
   diffw = width  - _GLHCKlibrary.render.width;
   diffh = height - _GLHCKlibrary.render.height;

   for (camera = _GLHCKlibrary.world.clist;
        camera; camera = camera->next) {
      glhckCameraViewportf(camera,
            camera->view.viewport.x,
            camera->view.viewport.y,
            camera->view.viewport.z + diffw,
            camera->view.viewport.w + diffh);
   }

   /* no camera binded, upload default projection */
   if (!(camera = _GLHCKlibrary.render.draw.camera))
      _glhckDefaultProjection(width, height);
   else {
      /* update camera */
      _GLHCKlibrary.render.draw.camera = NULL;
      camera->view.update = 1;
      glhckCameraUpdate(camera);
   }
}

/* public api */

/* \brief allocate new camera */
GLHCKAPI glhckCamera* glhckCameraNew(void)
{
   _glhckCamera *camera;
   TRACE(0);

   /* allocate acmera */
   if (!(camera = _glhckMalloc(sizeof(_glhckCamera))))
      goto fail;

   /* init */
   memset(camera, 0, sizeof(_glhckCamera));
   memset(&camera->view, 0, sizeof(__GLHCKcameraView));

   /* defaults */
   camera->view.near = 1.0f;
   camera->view.far  = 100.0f;
   camera->view.fov  = 35.0f;

   /* reset */
   glhckCameraReset(camera);

   /* increase reference */
   camera->refCounter++;

   /* insert to world */
   _glhckWorldInsert(clist, camera, _glhckCamera*);

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
   CALL(0, "%p", camera);
   assert(camera);

   /* increase reference */
   camera->refCounter++;

   RET(0, "%p", camera);
   return camera;
}

/* \brief free camera */
GLHCKAPI short glhckCameraFree(glhckCamera *camera)
{
   CALL(0, "%p", camera);
   assert(camera);

   /* there is still references to this object alive */
   if (--camera->refCounter != 0) goto success;

   /* remove camera from global state */
   if (_GLHCKlibrary.render.draw.camera == camera)
      _GLHCKlibrary.render.draw.camera = NULL;

   /* remove camera from world */
   _glhckWorldRemove(clist, camera, _glhckCamera*);

   /* free */
   _glhckFree(camera);
   camera = NULL;

success:
   RET(0, "%d", camera?camera->refCounter:0);
   return camera?camera->refCounter:0;
}

/* \brief update the camera */
GLHCKAPI void glhckCameraUpdate(glhckCamera *camera)
{
   CALL(2, "%p", camera);

   /* bind none */
   if (!camera) {
      _GLHCKlibrary.render.draw.camera = NULL;
      return;
   }

   if (_GLHCKlibrary.render.draw.camera != camera)
      _GLHCKlibrary.render.api.viewport(
            camera->view.viewport.x,
            camera->view.viewport.y,
            camera->view.viewport.z,
            camera->view.viewport.w);

   if (camera->view.update) {
      _glhckCameraViewMatrix(camera);
      camera->view.update = 0;
   }

   /* assign camera to global state */
   _GLHCKlibrary.render.api.setProjection(
         &camera->view.matrix);
   _GLHCKlibrary.render.draw.camera = camera;
}

/* \brief reset camera to default state */
GLHCKAPI void glhckCameraReset(glhckCamera *camera)
{
   CALL(0, "%p", camera);
   assert(camera);

   camera->view.update = 1;
   kmVec3Fill(&camera->view.upVector, 0, 1, 0);
   kmVec3Fill(&camera->view.rotation, 0, 0, 0);
   kmVec3Fill(&camera->view.target, 0, 0, 0);
   kmVec3Fill(&camera->view.translation, 0, 0, 0);
   kmVec4Fill(&camera->view.viewport, 0, 0,
         _GLHCKlibrary.render.width,
         _GLHCKlibrary.render.height);

   _glhckCameraProjectionMatrix(camera);
}

/* \brief set camera's fov */
GLHCKAPI void glhckCameraFov(glhckCamera *camera, const kmScalar fov)
{
   CALL(1, "%p, %f", camera, fov);
   assert(camera);
   camera->view.fov = fov;
}

/* \brief set camera's range */
GLHCKAPI void glhckCameraRange(glhckCamera *camera,
      const kmScalar near, const kmScalar far)
{
   CALL(1, "%p, %f, %f", camera, near, far);
   assert(camera);
   camera->view.near = near;
   camera->view.far  = far;
}

/* \brief set camera's viewport */
GLHCKAPI void glhckCameraViewport(glhckCamera *camera, const kmVec4 *viewport)
{
   CALL(1, "%p, "VEC4S, camera, VEC4(viewport));
   assert(camera);

   if (camera->view.viewport.x == viewport->x &&
       camera->view.viewport.y == viewport->y &&
       camera->view.viewport.z == viewport->z &&
       camera->view.viewport.w == viewport->w)
      return;

   kmVec4Assign(&camera->view.viewport, viewport);
   _glhckCameraProjectionMatrix(camera);
   camera->view.update = 1;
}

/* \brief set camera's viewport (kmScalar) */
GLHCKAPI void glhckCameraViewportf(glhckCamera *camera,
      const kmScalar x, const kmScalar y,
      const kmScalar w, const kmScalar h)
{
   const kmVec4 viewport = { x, y, w, h };
   glhckCameraViewport(camera, &viewport);
}

/* \brief position camera */
GLHCKAPI void glhckCameraPosition(glhckCamera *camera, const kmVec3 *position)
{
   CALL(2, "%p, "VEC3S, camera, VEC3(position));
   assert(camera);

   if (camera->view.translation.x == position->x &&
       camera->view.translation.y == position->y &&
       camera->view.translation.z == position->z)
      return;

   kmVec3Assign(&camera->view.translation, position);
   camera->view.update = 1;
}

/* \brief position camera (kmScalar) */
GLHCKAPI void glhckCameraPositionf(glhckCamera *camera,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 position = { x, y, z };
   glhckCameraPosition(camera, &position);
}

/* \brief move camera */
GLHCKAPI void glhckCameraMove(glhckCamera *camera, const kmVec3 *move)
{
   CALL(2, "%p, "VEC3S, camera, VEC3(move));
   assert(camera);

   kmVec3Add(&camera->view.translation,
         &camera->view.translation, move);
   camera->view.update = 1;
}

/* \brief move camera (kmScalar) */
GLHCKAPI void glhckCameraMovef(glhckCamera *camera,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 move = { x, y, z };
   glhckCameraMove(camera, &move);
}

/* \brief rotate camera */
GLHCKAPI void glhckCameraRotate(glhckCamera *camera, const kmVec3 *rotation)
{
   kmVec3 rotToDir;
   const kmVec3 forwards = { 0, 0, 1 };
   CALL(2, "%p, "VEC3S, camera, VEC3(rotation));
   assert(camera);

   if (camera->view.rotation.x == rotation->x &&
       camera->view.rotation.y == rotation->y &&
       camera->view.rotation.z == rotation->z)
      return;

   kmVec3Assign(&camera->view.rotation, rotation);

   /* update target */
   kmVec3RotationToDirection(&rotToDir,
         &camera->view.rotation, &forwards);
   kmVec3Add(&camera->view.target, &camera->view.translation, &rotToDir);

   camera->view.update = 1;
}

/* \brief rotate camera (kmScalar) */
GLHCKAPI void glhckCameraRotatef(glhckCamera *camera,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotation = { x, y, z };
   glhckCameraRotate(camera, &rotation);
}

/* \brief rotate camera towards point */
GLHCKAPI void glhckCameraTarget(glhckCamera *camera, const kmVec3 *target)
{
   kmVec3 toTarget;
   CALL(2, "%p, "VEC3S, camera, VEC3(target));
   assert(camera);

   if (camera->view.target.x == target->x &&
       camera->view.target.y == target->y &&
       camera->view.target.z == target->z)
      return;

   kmVec3Assign(&camera->view.target, target);

   /* update rotation */
   kmVec3Subtract(&toTarget,
         &camera->view.target, &camera->view.translation);
   kmVec3GetHorizontalAngle(&camera->view.rotation, &toTarget);

   camera->view.update = 1;
}

/* \brief rotate camera towards point (kmScalar) */
GLHCKAPI void glhckCameraTargetf(glhckCamera *camera,
      const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 target = { x, y, z };
   glhckCameraTarget(camera, &target);
}
