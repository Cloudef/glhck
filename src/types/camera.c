#include "camera.h"
#include "view.h"

#include <kazmath/kazmath.h>

#include "handle.h"
#include "trace.h"
#include "system/tls.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_CAMERA

struct clip {
   kmScalar near, far, fov;
};

struct transform {
   kmMat4 view, projection, viewProjection;
   kmVec3 upVector;
};

enum pool {
   $view, // glhckHandle
   $frustum, // glhckFrustum
   $viewport, // glhckRect
   $clip, // struct clip
   $transform, // struct transform
   $type, // glhckProjectionType
   $dirty, // unsigned char
   $dirtyViewport, // unsigned char
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // view
   sizeof(glhckFrustum), // frustum
   sizeof(glhckRect), // viewport
   sizeof(struct clip), // clip
   sizeof(struct transform), // transform
   sizeof(glhckProjectionType), // type
   sizeof(unsigned char), // dirty
   sizeof(unsigned char), // dirtyViewport
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];
static _GLHCK_TLS glhckHandle active;

#include "handlefun.h"

/* the cameras in glhck contain right-handed projection
 *
 * WORLD/3D
 *    y     -z FAR
 * -x | x   /
 *   -y    z NEAR
 */

static void calculateProjection(const glhckProjectionType type, const glhckHandle view, const glhckRect *viewport, const struct clip *clip, struct transform *transform)
{
   assert(view > 0 && viewport && transform);
   assert(viewport->w > 0.0f && viewport->h > 0.0f);

   switch(type) {
      case GLHCK_PROJECTION_ORTHOGRAPHIC:
	 {
	    kmScalar w = (viewport->w > viewport->h ? 1.0f : viewport->w / viewport->h);
	    kmScalar h = (viewport->w < viewport->h ? 1.0f : viewport->h / viewport->w);

	    kmVec3 toTarget;
	    kmVec3Subtract(&toTarget, glhckViewGetPosition(view), glhckViewGetTarget(view));

	    kmScalar distanceFromTarget = kmVec3Length(&toTarget);
	    w *= (distanceFromTarget + clip->near) * 0.5f;
	    h *= (distanceFromTarget + clip->near) * 0.5f;

	    kmMat4OrthographicProjection(&transform->projection, -w, w, -h, h, clip->near, clip->far);
	 }
	 break;

      case GLHCK_PROJECTION_PERSPECTIVE:
      default:
	 kmMat4PerspectiveProjection(&transform->projection, clip->fov, (float)viewport->w / (float)viewport->h, clip->near, clip->far);
	 break;
   }
}

static void calculateView(const glhckHandle view, glhckFrustum *frustum, struct transform *transform)
{
   assert(view > 0 && transform);

   /* build rotation for upvector */
   kmMat4 rotation, tmp;
   const kmVec3 *rot = glhckViewGetRotation(view);
   kmMat4RotationAxisAngle(&rotation, &(kmVec3){0,0,1}, kmDegreesToRadians(rot->z));
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){0,1,0}, kmDegreesToRadians(rot->y));
   kmMat4Multiply(&rotation, &rotation, &tmp);
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){1,0,0}, kmDegreesToRadians(rot->x));
   kmMat4Multiply(&rotation, &rotation, &tmp);

   /* assuming upvector is normalized */
   kmVec3 upVector;
   kmVec3Transform(&upVector, &transform->upVector, &rotation);

   /* build view matrix */
   kmMat4LookAt(&transform->view, glhckViewGetPosition(view), glhckViewGetTarget(view), &upVector);
   kmMat4Multiply(&transform->viewProjection, &transform->projection, &transform->view);
   glhckFrustumBuild(frustum, &transform->viewProjection);
}

/* \brief update the camera stack after window resize */
void _glhckCameraWorldUpdate(const int width, const int height, const int oldWidth, const int oldHeight)
{
   CALL(2, "%d, %d", width, height);

   /* get difference of old dimensions and now */
   const int diffw = width - oldWidth;
   const int diffh = height - oldHeight;

   glhckRect *v;
   for (chckPoolIndex iter = 0; (v = chckPoolIter(pools[$viewport], &iter));) {
      v->w += diffw;
      v->h += diffh;
      set($dirtyViewport, iter - 1, (unsigned char[]){1});
      set($dirty, iter - 1, (unsigned char[]){1});
   }

   /* no camera binded, upload default projection */
   if (!v) {
      _glhckRenderDefaultProjection(width, height);
      glhckRenderViewporti(0, 0, width, height);
   } else {
      glhckCameraUpdate(active);
   }
}

static glhckHandle getView(const glhckHandle handle)
{
   return *(glhckHandle*)get($view, handle);
}

static void setView(const glhckHandle handle, const glhckHandle view)
{
   setHandle($view, handle, view);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   releaseHandle($view, handle);
}

/***
 * public api
 ***/

/* \brief allocate new camera */
GLHCKAPI glhckHandle glhckCameraNew(void)
{
   TRACE(0);

   glhckHandle handle = 0, view = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_VIEW, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   if (!(view = glhckViewNew()))
      goto fail;

   setView(handle, view);
   glhckHandleRelease(view);

   glhckCameraReset(handle);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(glhckHandleRelease, view);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* \brief update the camera */
GLHCKAPI void glhckCameraUpdate(const glhckHandle handle)
{
   CALL(2, "%s", glhckHandleRepr(handle));

   if (!handle) {
      active = handle;
      return;
   }

   unsigned char *dirty = (unsigned char*)get($dirty, handle);
   unsigned char *dirtyViewport = (unsigned char*)get($dirtyViewport, handle);
   struct transform *transform = (struct transform*)get($transform, handle);
   const glhckHandle view = getView(handle);

   if (active != handle || *dirtyViewport) {
      glhckRenderViewport(glhckCameraGetViewport(handle));
      *dirtyViewport = 0;
   }

   if (*dirty || glhckViewGetDirty(view)) {
      glhckProjectionType type = *(glhckProjectionType*)get($type, handle);

      // if (type == GLHCK_PROJECTION_ORTHOGRAPHIC)
	 calculateProjection(type, view, (glhckRect*)glhckCameraGetViewport(handle), get($clip, handle), transform);

      calculateView(view, (glhckFrustum*)glhckCameraGetFrustum(handle), transform);
      *dirty = 0;
   }

   glhckRenderFlip(0);
   glhckRenderProjection(&transform->projection);
   glhckRenderView(&transform->view);
   active = handle;
}

/* \brief reset camera to default state */
GLHCKAPI void glhckCameraReset(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckCameraProjection(handle, GLHCK_PROJECTION_PERSPECTIVE);
   glhckCameraRange(handle, 1.0f, 100.0f);
   glhckCameraFov(handle, 35.0f);

   struct transform *transform = (struct transform*)get($transform, handle);
   kmVec3Fill(&transform->upVector, 0, 1, 0);
   glhckCameraRotationf(handle, 0, 0, 0);
   glhckCameraPositionf(handle, 0, 0, 0);

   glhckCameraViewporti(handle, 0, 0, glhckRenderGetWidth(), glhckRenderGetHeight());
}

/* \brief set camera's projection type */
GLHCKAPI void glhckCameraProjection(const glhckHandle handle, const glhckProjectionType projectionType)
{
   set($type, handle, &projectionType);
   set($dirty, handle, (unsigned char[]){1});
}

GLHCKAPI glhckProjectionType glhckCameraGetProjection(const glhckHandle handle)
{
   return *(glhckProjectionType*)get($type, handle);
}

/* \brief get camera's frustum */
GLHCKAPI const glhckFrustum* glhckCameraGetFrustum(const glhckHandle handle)
{
   return get($frustum, handle);
}

/* \brief get camera's view matrix */
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckHandle handle)
{
   return &((const struct transform*)get($transform, handle))->view;
}

/* \brief get camera's projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckHandle handle)
{
   return &((const struct transform*)get($transform, handle))->projection;
}

/* \brief get camera's model view projection matrix */
GLHCKAPI const kmMat4* glhckCameraGetVPMatrix(const glhckHandle handle)
{
   return &((const struct transform*)get($transform, handle))->viewProjection;
}

/* \brief set camera's up vector */
GLHCKAPI void glhckCameraUpVector(const glhckHandle handle, const kmVec3 *upVector)
{
   struct transform *transform = (struct transform*)get($transform, handle);
   kmVec3Assign(&transform->upVector, upVector);
   set($dirty, handle, (unsigned char[]){1});
}

GLHCKAPI const kmVec3* glhckCameraGetUpVector(const glhckHandle handle)
{
   return &((const struct transform*)get($transform, handle))->upVector;
}

/* \brief set camera's fov */
GLHCKAPI void glhckCameraFov(const glhckHandle handle, const kmScalar fov)
{
   struct clip *clip = (struct clip*)get($clip, handle);
   clip->fov = fov;
   set($dirty, handle, (unsigned char[]){1});
}

GLHCKAPI kmScalar glhckCameraGetFov(const glhckHandle handle)
{
   return ((const struct clip*)get($clip, handle))->fov;
}

/* \brief set camera's range */
GLHCKAPI void glhckCameraRange(const glhckHandle handle, const kmScalar near, const kmScalar far)
{
   struct clip *clip = (struct clip*)get($clip, handle);
   clip->near = near;
   clip->far = far;
   set($dirty, handle, (unsigned char[]){1});
}

GLHCKAPI void glhckCameraGetRange(const glhckHandle handle, kmScalar *outNear, kmScalar *outFar)
{
   if (outNear) *outNear = ((const struct clip*)get($clip, handle))->near;
   if (outFar) *outFar = ((const struct clip*)get($clip, handle))->far;
}

/* \brief set camera's viewport */
GLHCKAPI void glhckCameraViewport(const glhckHandle handle, const glhckRect *viewport)
{
   set($viewport, handle, viewport);
   set($dirty, handle, (unsigned char[]){1});
   set($dirtyViewport, handle, (unsigned char[]){1});
}

/* \brief set camera's viewport (int) */
GLHCKAPI void glhckCameraViewporti(const glhckHandle handle, const int x, const int y, const int w, const int h)
{
   const glhckRect viewport = { x, y, w, h };
   glhckCameraViewport(handle, &viewport);
}

GLHCKAPI const glhckRect* glhckCameraGetViewport(const glhckHandle handle)
{
   return get($viewport, handle);
}

GLHCKAPI const kmVec3* glhckCameraGetPosition(const glhckHandle handle)
{
   return glhckViewGetPosition(getView(handle));
}

/* \brief position object */
GLHCKAPI void glhckCameraPosition(const glhckHandle handle, const kmVec3 *position)
{
   glhckViewPosition(getView(handle), position);
}

/* \brief position object (with kmScalar) */
GLHCKAPI void glhckCameraPositionf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewPositionf(getView(handle), x, y, z);
}

/* \brief move object */
GLHCKAPI void glhckCameraMove(const glhckHandle handle, const kmVec3 *move)
{
   glhckViewMove(getView(handle), move);
}

/* \brief move object (with kmScalar) */
GLHCKAPI void glhckCameraMovef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewMovef(getView(handle), x, y, z);
}

/* \brief get object rotation */
GLHCKAPI const kmVec3* glhckCameraGetRotation(const glhckHandle handle)
{
   return glhckViewGetRotation(getView(handle));
}

/* \brief set object rotation */
GLHCKAPI void glhckCameraRotation(const glhckHandle handle, const kmVec3 *rotation)
{
   glhckViewRotation(getView(handle), rotation);
}

/* \brief set object rotation (with kmScalar) */
GLHCKAPI void glhckCameraRotationf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewRotationf(getView(handle), x, y, z);
}

/* \brief rotate object */
GLHCKAPI void glhckCameraRotate(const glhckHandle handle, const kmVec3 *rotate)
{
   glhckViewRotate(getView(handle), rotate);
}

/* \brief rotate object (with kmScalar) */
GLHCKAPI void glhckCameraRotatef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewRotatef(getView(handle), x, y, z);
}

/* \brief get object target */
GLHCKAPI const kmVec3* glhckCameraGetTarget(const glhckHandle handle)
{
   return glhckViewGetTarget(getView(handle));
}

/* \brief set object target */
GLHCKAPI void glhckCameraTarget(const glhckHandle handle, const kmVec3 *target)
{
   glhckViewTarget(getView(handle), target);
}

/* \brief set object target (with kmScalar) */
GLHCKAPI void glhckCameraTargetf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewTargetf(getView(handle), x, y, z);
}

/* \brief get object scale */
GLHCKAPI const kmVec3* glhckCameraGetScale(const glhckHandle handle)
{
   return glhckViewGetScale(getView(handle));
}

/* \brief scale object */
GLHCKAPI void glhckCameraScale(const glhckHandle handle, const kmVec3 *scale)
{
   glhckViewScale(getView(handle), scale);
}

/* \brief scale object (with kmScalar) */
GLHCKAPI void glhckCameraScalef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   glhckViewScalef(getView(handle), x, y, z);
}

/* \brief cast a ray from camera at specified relative coordinate */
GLHCKAPI const kmRay3* glhckCameraCastRayFromPoint(const glhckHandle handle, const kmVec2 *point, kmRay3 *pOut)
{
   CALL(2, "%s, %p, %p", glhckHandleRepr(handle), point, pOut);

   const glhckFrustum *frustum = glhckCameraGetFrustum(handle);

   kmVec3 nu, nv, fu, fv;
   kmVec3Subtract(&nu,
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT],
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT]);
   kmVec3Subtract(&nv,
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_NEAR_TOP_LEFT],
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT]);
   kmVec3Subtract(&fu,
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_RIGHT],
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT]);
   kmVec3Subtract(&fv,
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_FAR_TOP_LEFT],
	 &frustum->corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT]);

   kmVec3Scale(&nu, &nu, point->x);
   kmVec3Scale(&nv, &nv, point->y);
   kmVec3Scale(&fu, &fu, point->x);
   kmVec3Scale(&fv, &fv, point->y);

   pOut->start = frustum->corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT];
   pOut->dir = frustum->corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT];

   kmVec3Add(&pOut->start, &pOut->start, &nu);
   kmVec3Add(&pOut->start, &pOut->start, &nv);
   kmVec3Add(&pOut->dir, &pOut->dir, &fu);
   kmVec3Add(&pOut->dir, &pOut->dir, &fv);
   kmVec3Subtract(&pOut->dir, &pOut->dir, &pOut->start);

   RET(2, "%p", pOut);
   return pOut;
}

/* \brief cast a ray from camera at specified relative coordinate (with kmScalar) */
GLHCKAPI const kmRay3* glhckCameraCastRayFromPointf(const glhckHandle handle, const kmScalar x, const kmScalar y, kmRay3 *pOut)
{
   const kmVec2 point = { x, y };
   return glhckCameraCastRayFromPoint(handle, &point, pOut);
}

/* vim: set ts=8 sw=3 tw=0 :*/
