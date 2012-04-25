#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_CAMERA

/**
 * Builds a direction vector from input vector.
 * Input vector is assumed to be rotation vector composed from 3 Euler angle rotations, in degrees.
 * The forwards vector will be rotated by the input vector
 *
 * Code ported from Irrlicht: http://irrlicht.sourceforge.net/
 */
kmVec3* kmVec3RotationToDirection(kmVec3* pOut, const kmVec3* pIn, const kmVec3* forwards)
{
   const kmScalar xr = kmDegreesToRadians(pIn->x);
   const kmScalar yr = kmDegreesToRadians(pIn->y);
   const kmScalar zr = kmDegreesToRadians(pIn->z);
   const kmScalar cr = cos(xr), sr = sin(xr);
   const kmScalar cp = cos(yr), sp = cos(yr);
   const kmScalar cy = cos(zr), sy = cos(zr);

   const kmScalar srsp = sr*sp;
   const kmScalar crsp = cr*sp;

   const kmScalar pseudoMatrix[] = {
      (cp*cy), (cp*sy), (-sp),
      (srsp*cy-cr*sy), (srsp*sy+cr*cy), (sr*cp),
      (crsp*cy+sr*sy), (crsp*sy-sr*cy), (cr*cp)
   };

   pOut->x = forwards->x * pseudoMatrix[0] +
             forwards->y * pseudoMatrix[3] +
             forwards->z * pseudoMatrix[6];

   pOut->y = forwards->x * pseudoMatrix[1] +
             forwards->y * pseudoMatrix[4] +
             forwards->z * pseudoMatrix[7];

   pOut->z = forwards->x * pseudoMatrix[2] +
             forwards->y * pseudoMatrix[5] +
             forwards->z * pseudoMatrix[8];

   return pOut;
}

/* \brief add camera to global stack */
static int _glhckCameraInsertStack(_glhckCamera *camera)
{
   __GLHCKcameraStack *stack;
   CALL("%p", camera);
   assert(camera);

   stack = _GLHCKlibrary.camera.stack;
   if (!stack) {
      stack = _GLHCKlibrary.camera.stack =
         _glhckMalloc(sizeof(__GLHCKcameraStack));
   } else {
      for (; stack && stack->next; stack = stack->next);
      stack = stack->next =
         _glhckMalloc(sizeof(__GLHCKcameraStack));
   }

   if (!stack)
      goto fail;

   /* init stack */
   memset(stack, 0, sizeof(__GLHCKcameraStack));
   stack->camera = camera;

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static void _glhckCameraRemoveStack(_glhckCamera *camera)
{
   __GLHCKcameraStack *stack, *found;
   CALL("%p", camera);
   assert(camera);

   if (!(stack = _GLHCKlibrary.camera.stack))
      return;

   if (stack->camera == camera) {
      _GLHCKlibrary.camera.stack = stack->next;
      _glhckFree(stack);
   } else {
      for (; stack && stack->next &&
             stack->next->camera != camera;
             stack = stack->next);
      if ((found = stack->next)) {
         stack->next = found->next;
         _glhckFree(found);
      }
   }

}

/* \brief release camera stack */
void _glhckCameraStackRelease(void)
{
   __GLHCKcameraStack *stack, *next;
   TRACE();

   if (!(stack = _GLHCKlibrary.camera.stack))
      return;

   for (; stack; stack = next) {
      next = stack->next;
      _glhckFree(stack);
   }

   _GLHCKlibrary.camera.stack = NULL;
}

/* \brief calculate projection matrix */
static void _glhckCameraProjectionMatrix(_glhckCamera *camera)
{
   CALL("%p", camera);
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
   CALL("%p", camera);
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
void _glhckCameraStackUpdate(int width, int height)
{
   _glhckCamera *active;
   __GLHCKcameraStack *stack;
   CALL("%d, %d", width, height);

   for (stack = _GLHCKlibrary.camera.stack;
        stack; stack = stack->next) {
      glhckCameraViewportf(stack->camera,
            stack->camera->view.viewport.x,
            stack->camera->view.viewport.y,
            width,
            height);
   }

   /* no camera binded, upload default projection */
   if (!(active = _GLHCKlibrary.camera.bind))
      _glhckDefaultProjection();
   else {
      /* update camera */
      _GLHCKlibrary.camera.bind = NULL;
      active->view.update = 1;
      glhckCameraBind(active);
   }
}

/* public api */

/* \brief allocate new camera */
GLHCKAPI glhckCamera* glhckCameraNew(void)
{
   _glhckCamera *camera;
   TRACE();

   /* allocate acmera */
   if (!(camera = _glhckMalloc(sizeof(_glhckCamera))))
      goto fail;

   /* insert camera to stack */
   if (_glhckCameraInsertStack(camera) != RETURN_OK)
      goto fail;

   /* init */
   memset(camera, 0, sizeof(_glhckCamera));
   memset(&camera->view, 0, sizeof(__GLHCKcameraView));

   /* defaults */
   camera->view.near = 0.1f;
   camera->view.far  = 100.0f;
   camera->view.fov  = 35.0f;

   /* reset */
   glhckCameraReset(camera);

   /* increase reference */
   camera->refCounter++;

   RET("%p", camera);
   return camera;

fail:
   IFDO(_glhckFree, camera);
   RET("%p", NULL);
   return NULL;
}

/* \brief reference camera */
GLHCKAPI glhckCamera* glhckCameraRef(glhckCamera *camera)
{
   CALL("%p", camera);
   assert(camera);

   /* increase reference */
   camera->refCounter++;

   RET("%p", camera);
   return camera;
}

/* \brief free camera */
GLHCKAPI short glhckCameraFree(glhckCamera *camera)
{
   CALL("%p", camera);
   assert(camera);

   /* there is still references to this object alive */
   if (--camera->refCounter != 0) goto success;

   /* remove camera from global state */
   if (_GLHCKlibrary.camera.bind == camera)
      _GLHCKlibrary.camera.bind = NULL;

   /* remove camera from stack */
   _glhckCameraRemoveStack(camera);

   /* free */
   _glhckFree(camera);
   camera = NULL;

success:
   RET("%d", camera?camera->refCounter:0);
   return camera?camera->refCounter:0;
}

/* \brief bind camera */
GLHCKAPI void glhckCameraBind(glhckCamera *camera)
{
   CALL("%p", camera);

   if (_GLHCKlibrary.camera.bind == camera)
      return;

   /* bind none */
   if (!camera) {
      _GLHCKlibrary.camera.bind = NULL;
      return;
   }

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
   _GLHCKlibrary.camera.bind = camera;
}

/* \brief reset camera to default state */
GLHCKAPI void glhckCameraReset(glhckCamera *camera)
{
   CALL("%p", camera);
   assert(camera);

   camera->view.update = 1;
   kmVec3Fill(&camera->view.upVector, 0, 1, 0);
   kmVec3Fill(&camera->view.rotation, 0, 0, 0);
   kmVec3Fill(&camera->view.target, 0, 0, -10);
   kmVec3Fill(&camera->view.translation, 0, 0, 0);
   kmVec4Fill(&camera->view.viewport, 0, 0,
         _GLHCKlibrary.render.width,
         _GLHCKlibrary.render.height);

   _glhckCameraProjectionMatrix(camera);
}

/* \brief set camera's fov */
GLHCKAPI void glhckCameraFov(glhckCamera *camera, const kmScalar fov)
{
   CALL("%p, %f", camera, fov);
   assert(camera);
   camera->view.fov = fov;
}

/* \brief set camera's range */
GLHCKAPI void glhckCameraRange(glhckCamera *camera,
      const kmScalar near, const kmScalar far)
{
   CALL("%p, %f, %f", camera, near, far);
   assert(camera);
   camera->view.near = near;
   camera->view.far  = far;
}

/* \brief set camera's viewport */
GLHCKAPI void glhckCameraViewport(glhckCamera *camera, const kmVec4 *viewport)
{
   CALL("%p, "VEC4S, camera, VEC4(viewport));
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
   CALL("%p, "VEC3S, camera, VEC3(position));
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
   CALL("%p, "VEC3S, camera, VEC3(move));
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
   CALL("%p, "VEC3S, camera, VEC3(rotation));
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
   CALL("%p, "VEC3S, camera, VEC3(target));
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
