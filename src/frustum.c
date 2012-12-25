/* frustum builder stolen from KGLT
 * https://github.com/Kazade/KGLT
 *
 * Thanks for the elegant and nice code Kazade. */

#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_FRUSTUM

/* \brief build frustum from modelview projection matrix */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *frustum, const kmMat4 *mvp)
{
   kmPlane *planes;
   kmVec3 *nearCorners, *farCorners;

   CALL(0, "%p, %p", frustum, mvp);
   assert(frustum && mvp);

   planes      = frustum->planes;
   nearCorners = frustum->nearCorners;
   farCorners  = frustum->farCorners;

   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_LEFT],   mvp,  1);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_RIGHT],  mvp, -1);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_BOTTOM], mvp,  2);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_TOP],    mvp, -2);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_NEAR],   mvp,  3);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_FAR],    mvp, -3);

   kmPlaneGetIntersection(
         &nearCorners[GLHCK_FRUSTUM_CORNER_BOTTOM_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &nearCorners[GLHCK_FRUSTUM_CORNER_BOTTOM_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &nearCorners[GLHCK_FRUSTUM_CORNER_TOP_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &nearCorners[GLHCK_FRUSTUM_CORNER_TOP_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &farCorners[GLHCK_FRUSTUM_CORNER_BOTTOM_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &farCorners[GLHCK_FRUSTUM_CORNER_BOTTOM_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &farCorners[GLHCK_FRUSTUM_CORNER_TOP_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &farCorners[GLHCK_FRUSTUM_CORNER_TOP_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );
}

/* \brief draw frustum immediatly */
GLHCKAPI void glhckFrustumRender(glhckFrustum *frustum)
{
   CALL(2, "%p", frustum);
   assert(frustum);
   _GLHCKlibrary.render.api.frustumDraw(frustum);
}

/* \brief point inside frustum? */
GLHCKAPI int glhckFrustumContainsPoint(const glhckFrustum *frustum, const kmVec3 *point)
{
   unsigned int i;
   for (i = 0; i != GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (frustum->planes[i].a * point->x +
          frustum->planes[i].b * point->y +
          frustum->planes[i].c * point->z +
          frustum->planes[i].d <= 0) return 0;
      /* <= treat points that are right on the plane as outside.
       * <  for inverse behaviour */
   }
   return 1;
}

/* \brief sphere inside frustum? */
GLHCKAPI kmScalar glhckFrustumContainsSphere(const glhckFrustum *frustum, const kmVec3 *point, kmScalar radius)
{
   unsigned int i;
   kmScalar d;
   for (i = 0; i != GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      d = frustum->planes[i].a * point->x +
          frustum->planes[i].b * point->y +
          frustum->planes[i].c * point->z +
          frustum->planes[i].d;
      if (d <= -radius) return 0;
   }
   return d + radius;
}

/* \brief aabb inside frustum? */
GLHCKAPI int glhckFrustumContainsAABB(const glhckFrustum *frustum, const kmAABB *aabb)
{
   kmVec3 center;
   /* accurate enough for most cases, but there is error for small planes */
   return (glhckFrustumContainsPoint(frustum, kmAABBCentre(aabb, &center)) ||
           glhckFrustumContainsPoint(frustum, &aabb->min) ||
           glhckFrustumContainsPoint(frustum, &aabb->max));
}

/* vim: set ts=8 sw=3 tw=0 :*/
