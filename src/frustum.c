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
GLHCKAPI void glhckFrustumRender(glhckFrustum *frustum, const kmMat4 *model)
{
   CALL(2, "%p, %p", frustum, model);
   assert(frustum && model);
   _GLHCKlibrary.render.api.frustumDraw(frustum, model);
}

/* vim: set ts=8 sw=3 tw=0 :*/
