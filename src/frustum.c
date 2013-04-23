/* frustum builder stolen from KGLT
 * https://github.com/Kazade/KGLT
 *
 * Thanks for the elegant and nice code Kazade. */

#include "internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_FRUSTUM

/* \brief build frustum from modelview projection matrix */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *object, const kmMat4 *mvp)
{
   kmPlane *planes;
   kmVec3 *nearCorners, *farCorners;

   CALL(0, "%p, %p", object, mvp);
   assert(object && mvp);

   planes      = object->planes;
   nearCorners = object->nearCorners;
   farCorners  = object->farCorners;

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
GLHCKAPI void glhckFrustumRender(glhckFrustum *object)
{
   CALL(2, "%p", object);
   assert(object);
   GLHCKRA()->frustumRender(object);
}

/* \brief point inside frustum? */
GLHCKAPI int glhckFrustumContainsPoint(const glhckFrustum *object, const kmVec3 *point)
{
   unsigned int i;
   for (i = 0; i != GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (object->planes[i].a * point->x +
          object->planes[i].b * point->y +
          object->planes[i].c * point->z +
          object->planes[i].d < 0) return 0;
      /* <= treat points that are right on the plane as outside.
       * <  for inverse behaviour */
   }
   return 1;
}

/* \brief sphere inside frustum? */
GLHCKAPI kmScalar glhckFrustumContainsSphere(const glhckFrustum *object, const kmVec3 *point, kmScalar radius)
{
   unsigned int i;
   kmScalar d;
   for (i = 0; i != GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      d = object->planes[i].a * point->x +
          object->planes[i].b * point->y +
          object->planes[i].c * point->z +
          object->planes[i].d;
      if (d < -radius) return 0;
   }
   return d + radius;
}

/* \brief aabb inside frustum? */
GLHCKAPI int glhckFrustumContainsAABB(const glhckFrustum *object, const kmAABB *aabb)
{
   unsigned int i;
   for (i = 0; i != GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (max(object->planes[i].a * aabb->min.x, object->planes[i].a * aabb->max.x) +
          max(object->planes[i].b * aabb->min.y, object->planes[i].b * aabb->max.y) +
          max(object->planes[i].c * aabb->min.z, object->planes[i].c * aabb->max.z) +
          object->planes[i].d < 0) return 0;
   }
   return 1;
}

/* vim: set ts=8 sw=3 tw=0 :*/
