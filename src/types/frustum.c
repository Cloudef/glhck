/* frustum builder stolen from KGLT
 * https://github.com/Kazade/KGLT
 *
 * Thanks for the elegant and nice code Kazade. */

#include <glhck/glhck.h>
#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_FRUSTUM

/* \brief build frustum from modelview projection matrix */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *object, const kmMat4 *mvp)
{
   CALL(2, "%p, %p", object, mvp);
   assert(object && mvp);

   kmPlane *planes = object->planes;
   kmVec3 *corners = object->corners;

   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_LEFT],   mvp,  1);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_RIGHT],  mvp, -1);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_BOTTOM], mvp,  2);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_TOP],    mvp, -2);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_NEAR],   mvp,  3);
   kmPlaneExtractFromMat4(&planes[GLHCK_FRUSTUM_PLANE_FAR],    mvp, -3);

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_NEAR_TOP_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_NEAR_TOP_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_NEAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_BOTTOM],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_FAR_TOP_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_RIGHT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );

   kmPlaneGetIntersection(
         &corners[GLHCK_FRUSTUM_CORNER_FAR_TOP_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_LEFT],
         &planes[GLHCK_FRUSTUM_PLANE_TOP],
         &planes[GLHCK_FRUSTUM_PLANE_FAR]
         );
}

/* \brief point inside frustum? */
GLHCKAPI int glhckFrustumContainsPoint(const glhckFrustum *object, const kmVec3 *point)
{
   for (unsigned int i = 0; i < GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (object->planes[i].a * point->x +
          object->planes[i].b * point->y +
          object->planes[i].c * point->z +
          object->planes[i].d < 0)
         return 0;
      /* <= treat points that are right on the plane as outside.
       * <  for inverse behaviour */
   }

   return 1;
}

/* \brief sphere inside frustum? */
GLHCKAPI kmScalar glhckFrustumContainsSphere(const glhckFrustum *object, const kmVec3 *point, kmScalar radius)
{
   kmScalar d = 0.0f;

   for (unsigned int i = 0; i < GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      d = object->planes[i].a * point->x +
          object->planes[i].b * point->y +
          object->planes[i].c * point->z +
          object->planes[i].d;

      if (d < -radius)
         return 0.0f;
   }

   return d + radius;
}

/* \brief sphere inside frustum? (with extra checks, OUTSIDE, INSIDE, PARTIAL) */
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsSphereEx(const glhckFrustum *object, const kmVec3 *point, kmScalar radius)
{
   unsigned int c = 0;

   for (unsigned int i = 0; i < GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      kmScalar d = object->planes[i].a * point->x +
                   object->planes[i].b * point->y +
                   object->planes[i].c * point->z +
                   object->planes[i].d;

      if (d < -radius)
         return GLHCK_FRUSTUM_OUTSIDE;

      if (d >= radius)
         ++c;
   }

   return (c == 6 ? GLHCK_FRUSTUM_INSIDE : GLHCK_FRUSTUM_PARTIAL);
}

/* \brief aabb inside frustum? */
GLHCKAPI int glhckFrustumContainsAABB(const glhckFrustum *object, const kmAABB *aabb)
{
   const kmVec3 bounds[] = {
      { aabb->max.x, aabb->min.y, aabb->min.z },
      { aabb->min.x, aabb->max.y, aabb->min.z },
      { aabb->max.x, aabb->max.y, aabb->min.z },
      { aabb->min.x, aabb->min.y, aabb->max.z },
      { aabb->max.x, aabb->min.y, aabb->max.z },
      { aabb->min.x, aabb->max.y, aabb->max.z },
   };

   unsigned int i;
   for (i = 0; i < GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (kmPlaneDotCoord(&object->planes[i], &aabb->min) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[0]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[1]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[2]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[3]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[4]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[5]) > 0)
         continue;
      if (kmPlaneDotCoord(&object->planes[i], &aabb->max) > 0)
         continue;

      return 0;
   }

   /* Regular frustum failing to reject the big object behind the camera near plane,
    * because it's so big it does intersect some of the side planes of the frustum.
    *
    * source: http://www.iquilezles.org/www/articles/frustumcorrect/frustumcorrect.htm */
   int out;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].x > aabb->max.x)?1:0); if (out == 8) return 0;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].x < aabb->min.x)?1:0); if (out == 8) return 0;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].y > aabb->max.y)?1:0); if (out == 8) return 0;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].y < aabb->min.y)?1:0); if (out == 8) return 0;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].z > aabb->max.z)?1:0); if (out == 8) return 0;
   out = 0; for (i = 0; i < GLHCK_FRUSTUM_CORNER_LAST; ++i) out += ((object->corners[i].z < aabb->min.z)?1:0); if (out == 8) return 0;
   return 1;
}

/* \brief aabb inside frustum? (with extra checks, OUTSIDE, INSIDE, PARTIAL) */
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsAABBEx(const glhckFrustum *object, const kmAABB *aabb)
{
   const kmVec3 bounds[] = {
      { aabb->max.x, aabb->min.y, aabb->min.z },
      { aabb->min.x, aabb->max.y, aabb->min.z },
      { aabb->max.x, aabb->max.y, aabb->min.z },
      { aabb->min.x, aabb->min.y, aabb->max.z },
      { aabb->max.x, aabb->min.y, aabb->max.z },
      { aabb->min.x, aabb->max.y, aabb->max.z },
   };

   unsigned int c2 = 0;
   for (unsigned int i = 0, c = 0; i < GLHCK_FRUSTUM_PLANE_LAST; ++i) {
      if (kmPlaneDotCoord(&object->planes[i], &aabb->min) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[0]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[1]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[2]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[3]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[4]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &bounds[5]) > 0)
         ++c;
      if (kmPlaneDotCoord(&object->planes[i], &aabb->max) > 0)
         ++c;

      if (c == 0)
         return GLHCK_FRUSTUM_OUTSIDE;

      if (c == 8)
         ++c2;
   }

   return (c2 == 6 ? GLHCK_FRUSTUM_INSIDE : GLHCK_FRUSTUM_PARTIAL);
}

/* vim: set ts=8 sw=3 tw=0 :*/
