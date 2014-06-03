#include "view.h"

#include <kazmath/kazmath.h>

#include "handle.h"
#include "trace.h"
#include "system/tls.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_OBJECT

enum pool {
   $matrix, // kmMat4
   $affectionMatrix, // kmMat4
   $bounding, // kmAABB
   $aabb, // kmAABB
   $aabbFull, // kmAABB
   $obb, // kmAABB
   $obbFull, // kmAABB
   $translation, // kmVec3
   $target, // kmVec3
   $rotation, // kmVec3
   $scaling, // kmVec3
   $parentAffection, // unsigned char
   $dirty, // unsigned char
   $wasFlipped, // unsigned char
   POOL_LAST
};

static unsigned int pool_sizes[POOL_LAST] = {
   sizeof(kmMat4), // matrix
   sizeof(kmMat4), // affectionMatrix
   sizeof(kmAABB), // bounding
   sizeof(kmAABB), // aabb
   sizeof(kmAABB), // aabbFull
   sizeof(kmAABB), // obb
   sizeof(kmAABB), // obbFull
   sizeof(kmVec3), // translation
   sizeof(kmVec3), // target
   sizeof(kmVec3), // rotation
   sizeof(kmVec3), // scaling
   sizeof(unsigned char), // parentAffection
   sizeof(unsigned char), // dirty
   sizeof(unsigned char), // wasFlipped
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

static void buildTranslation(const glhckHandle handle, const kmVec3 *bias, kmMat4 *outTranslation)
{
   assert(handle > 0 && outTranslation);

   kmVec3 translated, biasScaling;
   kmVec3Mul(&biasScaling, (bias ? bias : &(kmVec3){0,0,0}), glhckViewGetScale(handle));
   kmVec3Add(&translated, glhckViewGetPosition(handle), &biasScaling);
   kmMat4Translation(outTranslation, translated.x, translated.y, translated.z);
}

static void buildRotation(const glhckHandle handle, kmMat4 *outRotation)
{
   assert(handle > 0 && outRotation);

   kmMat4 tmp;
   const kmVec3 *rotation = glhckViewGetRotation(handle);
   kmMat4RotationAxisAngle(outRotation, &(kmVec3){0,0,1}, kmDegreesToRadians(rotation->z));
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){0,1,0}, kmDegreesToRadians(rotation->y));
   kmMat4Multiply(outRotation, outRotation, &tmp);
   kmMat4RotationAxisAngle(&tmp, &(kmVec3){1,0,0}, kmDegreesToRadians(rotation->z));
   kmMat4Multiply(outRotation, outRotation, &tmp);
}

static void buildScaling(const glhckHandle handle, const kmVec3 *scale, kmMat4 *outScaling)
{
   assert(handle > 0 && outScaling);

   kmVec3 scaled;
   kmVec3Add(&scaled, glhckViewGetScale(handle), (scale ? scale : &(kmVec3){1,1,1}));
   kmMat4Scaling(outScaling, scaled.x, scaled.y, scaled.z);
}

static void buildAffectionMatrix(const glhckHandle handle, const unsigned char flags, kmMat4 *outAffectionMatrix)
{
   assert(outAffectionMatrix);

   kmMat4 matrix, tmp;
   kmMat4Identity(&matrix);

   if (flags & GLHCK_AFFECT_SCALING) {
      /* don't use _glhckObjectBuildScaling, as we don't want parent->geometry->scale affection
       * XXX: Should we do same for the translation below? */
      const kmVec3 *scaling = glhckViewGetScale(handle);
      kmMat4Scaling(&tmp, scaling->x, scaling->y, scaling->z);
      kmMat4Multiply(&matrix, &tmp, &matrix);
   }

   if (flags & GLHCK_AFFECT_ROTATION) {
      buildRotation(handle, &tmp);
      kmMat4Multiply(&matrix, &tmp, &matrix);
   }

   if (flags& GLHCK_AFFECT_TRANSLATION) {
      buildTranslation(handle, NULL, &tmp);
      kmMat4Multiply(&matrix, &tmp, &matrix);
   }

   kmMat4Multiply(&matrix, get($affectionMatrix, handle), &matrix);
   memcpy(outAffectionMatrix, &matrix, sizeof(matrix));
}

static const kmVec3* kmVec3Max(kmVec3 *out, const kmVec3 *a, const kmVec3 *b)
{
   out->x = fmax(a->x, b->x);
   out->y = fmax(a->y, b->y);
   out->z = fmax(a->z, b->z);
   return out;
}

static const kmVec3* kmVec3Min(kmVec3 *out, const kmVec3 *a, const kmVec3 *b)
{
   out->x = fmin(a->x, b->x);
   out->y = fmin(a->y, b->y);
   out->z = fmin(a->z, b->z);
   return out;
}

void buildBoundingBoxes(const glhckHandle handle, const kmAABB *childAABBs, const kmAABB *childOBBs, const size_t memb)
{
   kmVec3 min, max;
   kmVec3 mixxyz, mixyyz, mixyzz;
   kmVec3 maxxyz, maxyyz, maxyzz;

   kmAABB *obb = (kmAABB*)glhckViewGetOBB(handle);
   const kmMat4 *matrix = glhckViewGetMatrix(handle);
   const kmAABB *bounding = get($bounding, handle);

   /* update transformed obb */
   kmVec3Transform(&obb->min, &bounding->min, matrix);
   kmVec3Transform(&obb->max, &bounding->max, matrix);

   /* update transformed aabb */
   maxxyz.x = bounding->max.x;
   maxxyz.y = bounding->min.y;
   maxxyz.z = bounding->min.z;

   maxyyz.x = bounding->min.x;
   maxyyz.y = bounding->max.y;
   maxyyz.z = bounding->min.z;

   maxyzz.x = bounding->min.x;
   maxyzz.y = bounding->min.y;
   maxyzz.z = bounding->max.z;

   mixxyz.x = bounding->min.x;
   mixxyz.y = bounding->max.y;
   mixxyz.z = bounding->max.z;

   mixyyz.x = bounding->max.x;
   mixyyz.y = bounding->min.y;
   mixyyz.z = bounding->max.z;

   mixyzz.x = bounding->max.x;
   mixyzz.y = bounding->max.y;
   mixyzz.z = bounding->min.z;

   /* transform edges */
   kmVec3Transform(&mixxyz, &mixxyz, matrix);
   kmVec3Transform(&maxxyz, &maxxyz, matrix);
   kmVec3Transform(&mixyyz, &mixyyz, matrix);
   kmVec3Transform(&maxyyz, &maxyyz, matrix);
   kmVec3Transform(&mixyzz, &mixyzz, matrix);
   kmVec3Transform(&maxyzz, &maxyzz, matrix);

   /* find max edges */
   memcpy(&max, &obb->max, sizeof(kmVec3));
   kmVec3Max(&max, &max, &maxxyz);
   kmVec3Max(&max, &max, &maxyyz);
   kmVec3Max(&max, &max, &maxyzz);
   kmVec3Max(&max, &max, &mixxyz);
   kmVec3Max(&max, &max, &mixyyz);
   kmVec3Max(&max, &max, &mixyzz);

   /* find min edges */
   memcpy(&min, &obb->min, sizeof(kmVec3));
   kmVec3Min(&min, &min, &mixxyz);
   kmVec3Min(&min, &min, &mixyyz);
   kmVec3Min(&min, &min, &mixyzz);
   kmVec3Min(&min, &min, &maxxyz);
   kmVec3Min(&min, &min, &maxyyz);
   kmVec3Min(&min, &min, &maxyzz);

   /* set edges */
   kmAABB *aabb = (kmAABB*)glhckViewGetAABB(handle);
   memcpy(&aabb->max, &max, sizeof(kmVec3));
   memcpy(&aabb->min, &min, sizeof(kmVec3));

   kmAABB *aabbFull = (kmAABB*)glhckViewGetAABBWithChildren(handle);
   kmAABB *obbFull = (kmAABB*)glhckViewGetOBBWithChildren(handle);
   memcpy(aabbFull, aabb, sizeof(kmAABB));
   memcpy(obbFull, obb, sizeof(kmAABB));

   /* update full aabb && obb */
   for (size_t i = 0; i < memb; ++i) {
	kmVec3Min(&aabbFull->min, &aabbFull->min, &childAABBs[i].min);
	kmVec3Max(&aabbFull->max, &aabbFull->max, &childAABBs[i].max);
	kmVec3Min(&obbFull->min, &obbFull->min, &childOBBs[i].min);
	kmVec3Max(&obbFull->max, &obbFull->max, &childOBBs[i].max);
   }
}

/* \brief update view matrix of object */
static void updateMatrixMany(const glhckHandle *handles, const glhckHandle *parentHandles, const size_t memb, const kmVec3 *bias, const kmVec3 *scale)
{
   for (size_t i = 0; i < memb; ++i) {
      kmMat4 translation, rotation, scaling;
      buildTranslation(handles[i], bias, &translation);
      buildRotation(handles[i], &rotation);
      buildScaling(handles[i], scale, &scaling);

      /* affection of parent matrix */
      if (parentHandles[i]) {
         const kmMat4 *affectionMatrix = get($affectionMatrix, handles[i]);
         buildAffectionMatrix(parentHandles[i], glhckViewGetParentAffection(handles[i]), (kmMat4*)affectionMatrix);
         kmMat4Multiply(&translation, affectionMatrix, &translation);
      }

      /* build model matrix */
      kmMat4Multiply(&scaling, &rotation, &scaling);
      kmMat4Multiply((kmMat4*)glhckViewGetMatrix(handles[i]), &translation, &scaling);

#if 0
      /* we need to flip the matrix for 2D drawing */
      if (GLHCKRD()->view.flippedProjection)
         kmMat4Multiply(&object->view.matrix, &object->view.matrix, _glhckRenderGetFlipMatrix());
#endif

      set($dirty, handles[i], (unsigned char[]){0});
   }
}

static void updateTargetFromRotationMany(const glhckHandle *handles, const size_t memb)
{
   CALL(2, "%s", _glhckHandleReprArray(GLHCK_TYPE_VIEW, handles, memb));

   kmVec3 rotToDir;
   for (size_t i = 0; i < memb; ++i) {
	const kmVec3 forwards = { 0, 0, 1 };
	kmVec3RotationToDirection(&rotToDir, glhckViewGetRotation(handles[i]), &forwards);
	kmVec3Add((kmVec3*)glhckViewGetTarget(handles[i]), glhckViewGetPosition(handles[i]), &rotToDir);
   }
}

static void updateRotationFromTargetMany(const glhckHandle *handles, const size_t memb)
{
   CALL(2, "%s", _glhckHandleReprArray(GLHCK_TYPE_VIEW, handles, memb));

   kmVec3 toTarget;
   for (size_t i = 0; i < memb; ++i) {
	kmVec3Subtract(&toTarget, glhckViewGetTarget(handles[i]), glhckViewGetPosition(handles[i]));
	kmVec3GetHorizontalAngle((kmVec3*)glhckViewGetRotation(handles[i]), &toTarget);
   }
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);
}

GLHCKAPI glhckHandle glhckViewNew(void)
{
   TRACE(0);

   glhckHandle handle;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_VIEW, pools, pool_sizes, POOL_LAST, destructor, NULL)))
      goto fail;

   kmMat4Identity((kmMat4*)glhckViewGetMatrix(handle)),
   kmMat4Identity((kmMat4*)get($affectionMatrix, handle)),

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI int glhckViewGetDirty(const glhckHandle handle)
{
   return *(unsigned char*)get($dirty, handle);
}

GLHCKAPI void glhckViewParentAffection(const glhckHandle handle, const unsigned char affectionFlags)
{
   set($parentAffection, handle, &affectionFlags);
}

GLHCKAPI unsigned char glhckViewGetParentAffection(const glhckHandle handle)
{
   return *(unsigned char*)get($parentAffection, handle);
}

GLHCKAPI void glhckViewUpdateMany(const glhckHandle *handles, const glhckHandle *parents, const size_t memb)
{
   CALL(2, "%s, %s, %zu", glhckHandleReprArray(handles, memb), glhckHandleReprArray(parents, memb), memb);
   updateMatrixMany(handles, parents, memb, NULL, NULL);
}

GLHCKAPI const kmVec3* glhckViewGetPosition(const glhckHandle handle)
{
   return get($translation, handle);
}

GLHCKAPI void glhckViewPosition(const glhckHandle handle, const kmVec3 *position)
{
   set($translation, handle, position);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewPositionf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 position = { x, y, z };
   glhckViewPosition(handle, &position);
}

GLHCKAPI void glhckViewMove(const glhckHandle handle, const kmVec3 *move)
{
   const kmVec3 *translation = get($translation, handle);
   kmVec3Add((kmVec3*)translation, translation, move);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewMovef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 move = { x, y, z };
   glhckViewMove(handle, &move);
}

GLHCKAPI const kmVec3* glhckViewGetRotation(const glhckHandle handle)
{
   return get($rotation, handle);
}

GLHCKAPI void glhckViewRotation(const glhckHandle handle, const kmVec3 *rotation)
{
   set($rotation, handle, rotation);
   updateTargetFromRotationMany((glhckHandle[]){handle}, 1);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewRotationf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotation = { x, y, z };
   glhckViewRotation(handle, &rotation);
}

GLHCKAPI void glhckViewRotate(const glhckHandle handle, const kmVec3 *rotate)
{
   const kmVec3 *rotation = get($rotation, handle);
   kmVec3Add((kmVec3*)rotation, rotation, rotate);
   updateTargetFromRotationMany((glhckHandle[]){handle}, 1);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewRotatef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 rotate = { x, y, z };
   glhckViewRotate(handle, &rotate);
}

GLHCKAPI const kmVec3* glhckViewGetTarget(const glhckHandle handle)
{
   return get($target, handle);
}

GLHCKAPI void glhckViewTarget(const glhckHandle handle, const kmVec3 *target)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   set($target, handle, target);
   updateRotationFromTargetMany((glhckHandle[]){handle}, 1);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewTargetf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 target = { x, y, z };
   glhckViewTarget(handle, &target);
}

GLHCKAPI const kmVec3* glhckViewGetScale(const glhckHandle handle)
{
   return get($scaling, handle);
}

GLHCKAPI void glhckViewScale(const glhckHandle handle, const kmVec3 *scale)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   set($scaling, handle, scale);
   set($dirty, handle, (int[]){1});
}

GLHCKAPI void glhckViewScalef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z)
{
   const kmVec3 scaling = { x, y, z };
   glhckViewScale(handle, &scaling);
}

GLHCKAPI const kmAABB* glhckViewGetOBBWithChildren(const glhckHandle handle)
{
   return get($obbFull, handle);
}

GLHCKAPI const kmAABB* glhckViewGetOBB(const glhckHandle handle)
{
   return get($obb, handle);
}

GLHCKAPI const kmAABB* glhckViewGetAABBWithChildren(const glhckHandle handle)
{
   return get($aabbFull, handle);
}

GLHCKAPI const kmAABB* glhckViewGetAABB(const glhckHandle handle)
{
   return get($aabb, handle);
}

GLHCKAPI const kmMat4* glhckViewGetMatrix(const glhckHandle handle)
{
   return get($matrix, handle);
}

GLHCKAPI void glhckViewUpdateFromGeometry(const glhckHandle handle, const glhckHandle geometry)
{
   kmAABB bounding;
   glhckGeometryCalculateBB(geometry, &bounding);
   set($bounding, handle, &bounding);
}

/* vim: set ts=6 sw=3 tw=0 :*/
