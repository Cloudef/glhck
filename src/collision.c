#include "internal.h"
#include <float.h> /* for FLT_MAX */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_COLLISION

/* XXX: Lots of stuff still incomplete here */

/***
 * Kazmath extension
 * When stuff is tested and working
 * Try to get much as we can to upstream
 ***/

typedef struct kmAABBExtent {
   kmVec3 point;
   kmVec3 extent;
} kmAABBExtent;

typedef struct kmSphere {
   kmVec3 point;
   kmScalar radius;
} kmSphere;

typedef struct kmCapsule {
   kmVec3 pointA;
   kmVec3 pointB;
   kmScalar radius;
} kmCapsule;

typedef struct kmEllipse {
   kmVec3 point;
   kmVec3 radius;
} kmEllipse;

typedef struct kmOBB {
   struct kmAABBExtent aabb;
   kmVec3 orientation[3];
} kmOBB;

typedef struct kmTriangle {
   kmVec3 v1, v2, v3;
} kmTriangle;


glhckObject* glhckCubeFromKazmathAABBExtent(const kmAABBExtent *aabb)
{
   glhckObject *o = glhckCubeNew(1.0);
   glhckObjectPosition(o, &aabb->point);
   glhckObjectScalef(o, aabb->extent.x, aabb->extent.y, aabb->extent.z);
   return o;
}

kmAABBExtent* kmAABBToAABBExtent(kmAABBExtent* pOut, const kmAABB* aabb)
{
  kmVec3Subtract(&pOut->extent, &aabb->max, &aabb->min);
  kmVec3Scale(&pOut->extent, &pOut->extent, 0.5f);
  kmVec3Add(&pOut->point, &aabb->min, &pOut->extent);
  return pOut;
}

kmAABB* kmAABBExtentToAABB(kmAABB* pOut, const kmAABBExtent* aabbExtent)
{
  kmVec3Subtract(&pOut->min, &aabbExtent->point, &aabbExtent->extent);
  kmVec3Add(&pOut->max, &aabbExtent->point, &aabbExtent->extent);
  return pOut;
}

kmVec3* kmVec3Abs(kmVec3 *pOut, const kmVec3 *pV1)
{
   pOut->x = fabs(pV1->x);
   pOut->y = fabs(pV1->y);
   pOut->z = fabs(pV1->z);
   return pOut;
}

kmVec3* kmVec3Divide(kmVec3 *pOut, const kmVec3 *pV1, const kmVec3 *pV2)
{
   pOut->x = pV1->x / pV2->x;
   pOut->y = pV1->y / pV2->y;
   pOut->z = pV1->z / pV2->z;
   return pOut;
}

kmVec3* kmVec3Multiply(kmVec3 *pOut, const kmVec3 *pV1, const kmVec3 *pV2)
{
   pOut->x = pV1->x * pV2->x;
   pOut->y = pV1->y * pV2->y;
   pOut->z = pV1->z * pV2->z;
   return pOut;
}

/* compute distance between segment and point */
kmScalar kmVec3LengthSqSegment(const kmVec3 *a, const kmVec3 *b, const kmVec3 *c)
{
   kmVec3 ab, ac, bc;
   kmScalar e, f;
   kmVec3Subtract(&ab, b, a);
   kmVec3Subtract(&ac, c, a);
   kmVec3Subtract(&bc, c, b);
   e = kmVec3Dot(&ac, &ab);

   /* handle cases where c projects outside ab */
   if (e <= 0.0f) return kmVec3LengthSq(&ac);

   f = kmVec3LengthSq(&ab);
   if (e >= f) return kmVec3LengthSq(&bc);

   /* handle case where c projects onto ab */
   return kmVec3LengthSq(&ac) - e * e / f;
}

kmBool kmAABBExtentContainsPoint(const kmAABBExtent* a, const kmVec3* p)
{
  if(p->x < a->point.x - a->extent.x || p->x > a->point.x + a->extent.x) return KM_FALSE;
  if(p->y < a->point.y - a->extent.y || p->y > a->point.y + a->extent.y) return KM_FALSE;
  if(p->z < a->point.z - a->extent.z || p->z > a->point.z + a->extent.z) return KM_FALSE;
  return KM_TRUE;
}

/* calculate closest squared length from segments */
kmScalar kmClosestPointFromSegments(const kmVec3 *a1, const kmVec3 *b1, const kmVec3 *a2, const kmVec3 *b2,
      kmScalar *s, kmScalar *t, kmVec3 *c1, kmVec3 *c2)
{
   kmVec3 d1, d2, r, cd;
   kmScalar a, e, f, c, b, denom;
   assert(s && t && c1 && c2);

   kmVec3Subtract(&d1, b1, a1);
   kmVec3Subtract(&d2, b2, a2);
   kmVec3Subtract(&r, a1, a2);
   a = kmVec3Dot(&d1, &d1);
   e = kmVec3Dot(&d2, &d2);
   f = kmVec3Dot(&d2, &r);

   /* check if either or both segments degenerate into points */
   if (a <= kmEpsilon && e <= kmEpsilon) {
      /* both segments degenerate into points */
      *s = *t = 0.0f;
      kmVec3Assign(c1, a1);
      kmVec3Assign(c2, a2);
      kmVec3Subtract(&cd, c1, c2);
      return kmVec3Dot(&cd, &cd);
   }
   if (a <= kmEpsilon) {
      /* first segment degenerates into a point */
      *s = 0.0f;
      *t = f / e; // s = 0 => t = (b*s + f) / e = f / e
      *t = kmClamp(*t, 0.0f, 1.0f);
   } else {
      c = kmVec3Dot(&d1, &r);
      if (e <= kmEpsilon) {
         /* second segment degenerates into a point */
         *t = 0.0f;
         *s = kmClamp(-c / a, 0.0f, 1.0f); // t = 0 => s = (b*t - c) / a = -c / a
      } else {
         /* the general nondegenerate case starts here */
         b = kmVec3Dot(&d1, &d2);
         denom = a*e-b*b; /* always nonnegative */

         /* if segments not parallel, compute closest point on L1 to L2, and
          * clamp to segment S1. Else pick arbitrary s (here 0) */
         if (denom != 0.0f) {
            *s = kmClamp((b*f - c*e) / denom, 0.0f, 1.0f);
         } else *s = 0.0f;

         /* compute point on L2 closest to S1(s) using
          * t = Dot((P1+D1*s)-P2,D2) / Dot(D2,D2) = (b*s + f) / e */
         *t = (b*(*s) + f) / e;

         /* if t in [0,1] done. Else clamp t, recompute s for the new value
          * of t using s = Dot((P2+D2*t)-P1,D1) / Dot(D1,D1)= (t*b - c) / a
          * and clamp s to [0, 1] */
         if (*t < 0.0f) {
            *t = 0.0f;
            *s = kmClamp(-c / a, 0.0f, 1.0f);
         } else if (*t > 1.0f) {
            *t = 1.0f;
            *s = kmClamp((b - c) / a, 0.0f, 1.0f);
         }
      }
   }

   kmVec3Add(c1, a1, &d1);
   kmVec3Scale(c1, c1, *s);
   kmVec3Add(c2, a2, &d2);
   kmVec3Scale(c2, c2, *t);
   kmVec3Subtract(&cd, c1, c2);
   return kmVec3Dot(&cd, &cd);
}

static const kmVec3* kmAABBExtentClosestPointTo(const kmAABBExtent *pIn, const kmVec3 *point, kmVec3 *pOut)
{
   kmVec3 v;
   kmVec3Assign(&v, point);

   if (point->x < pIn->point.x - pIn->extent.x) v.x = pIn->point.x - pIn->extent.x;
   if (point->x > pIn->point.x + pIn->extent.x) v.x = pIn->point.x + pIn->extent.x;
   if (point->y < pIn->point.y - pIn->extent.y) v.y = pIn->point.y - pIn->extent.y;
   if (point->y > pIn->point.y + pIn->extent.y) v.y = pIn->point.y + pIn->extent.y;
   if (point->z < pIn->point.z - pIn->extent.z) v.z = pIn->point.z - pIn->extent.z;
   if (point->z > pIn->point.z + pIn->extent.z) v.z = pIn->point.z + pIn->extent.z;

   if (kmVec3AreEqual(&v, point)) {
      kmVec3 delta;
      kmVec3Subtract(&delta, &v, &pIn->point);
      if (fabs(delta.y) > fabs(delta.x) && fabs(delta.y) > fabs(delta.z)) {
         if (delta.y > 0.0f) v.y = pIn->point.y + pIn->extent.y;
         if (delta.y < 0.0f) v.y = pIn->point.y - pIn->extent.y;
      } else if (fabs(delta.x) > fabs(delta.y) && fabs(delta.x) > fabs(delta.z)) {
         if (delta.x > 0.0f) v.x = pIn->point.x + pIn->extent.x;
         if (delta.x < 0.0f) v.x = pIn->point.x - pIn->extent.x;
      } else {
         if (delta.z > 0.0f) v.z = pIn->point.z + pIn->extent.z;
         if (delta.z < 0.0f) v.z = pIn->point.z - pIn->extent.z;
      }
   }

   pOut->x = v.x;
   pOut->y = v.y;
   pOut->z = v.z;
   return pOut;
}

static const kmVec3* kmAABBClosestPointTo(const kmAABB *pIn, const kmVec3 *point, kmVec3 *pOut)
{
   kmVec3 v;
   kmVec3Assign(&v, point);

   if (point->x < pIn->min.x) v.x = pIn->min.x;
   if (point->x > pIn->max.x) v.x = pIn->max.x;
   if (point->y < pIn->min.y) v.y = pIn->min.y;
   if (point->y > pIn->max.y) v.y = pIn->max.y;
   if (point->z < pIn->min.z) v.z = pIn->min.z;
   if (point->z > pIn->max.z) v.z = pIn->max.z;

   if (kmVec3AreEqual(&v, point)) {
      kmVec3 center, delta;
      kmAABBCentre(pIn, &center);
      kmVec3Subtract(&delta, &v, &center);
      if (fabs(delta.y) > fabs(delta.x) && fabs(delta.y) > fabs(delta.z)) {
         if (delta.y > 0.0f) v.y = pIn->max.y;
         if (delta.y < 0.0f) v.y = pIn->min.y;
      } else if (fabs(delta.x) > fabs(delta.y) && fabs(delta.x) > fabs(delta.z)) {
         if (delta.x > 0.0f) v.x = pIn->max.x;
         if (delta.x < 0.0f) v.x = pIn->min.x;
      } else {
         if (delta.z > 0.0f) v.z = pIn->max.z;
         if (delta.z < 0.0f) v.z = pIn->min.z;
      }
   }

   pOut->x = v.x;
   pOut->y = v.y;
   pOut->z = v.z;
   return pOut;
}

static const kmVec3* kmSphereClosestPointTo(const kmSphere *pIn, const kmVec3 *point, kmVec3 *pOut)
{
   kmVec3 pointMinusCenter;
   kmVec3Subtract(&pointMinusCenter, point, &pIn->point);
   float l = kmVec3Length(&pointMinusCenter);
   if (kmAlmostEqual(l, 0.0f)) l = 1.0f;
   pOut->x = point->x + (point->x - pIn->point.x) * ((pIn->radius-l)/l);
   pOut->y = point->y + (point->y - pIn->point.y) * ((pIn->radius-l)/l);
   pOut->z = point->z + (point->z - pIn->point.z) * ((pIn->radius-l)/l);
   return pOut;
}

static kmScalar kmSqDistPointAABB(const kmVec3 *p, const kmAABB *aabb)
{
   kmScalar sqDist = 0.0f;
   if (p->x < aabb->min.x) sqDist += (aabb->min.x - p->x) * (aabb->min.x - p->x);
   if (p->x > aabb->max.x) sqDist += (p->x - aabb->max.x) * (p->x - aabb->max.x);
   if (p->y < aabb->min.y) sqDist += (aabb->min.y - p->y) * (aabb->min.y - p->y);
   if (p->y > aabb->max.y) sqDist += (p->y - aabb->max.y) * (p->y - aabb->max.y);
   if (p->z < aabb->min.z) sqDist += (aabb->min.z - p->z) * (aabb->min.z - p->z);
   if (p->z > aabb->max.z) sqDist += (p->z - aabb->max.z) * (p->z - aabb->max.z);
   return sqDist;
}

static kmScalar kmSqDistPointAABBExtent(const kmVec3 *p, const kmAABBExtent *aabbe)
{
   kmAABB aabb;
   aabb.min.x = aabbe->point.x - aabbe->extent.x;
   aabb.max.x = aabbe->point.x + aabbe->extent.x;
   aabb.min.y = aabbe->point.y - aabbe->extent.y;
   aabb.max.y = aabbe->point.y + aabbe->extent.y;
   aabb.min.z = aabbe->point.z - aabbe->extent.z;
   aabb.max.z = aabbe->point.z + aabbe->extent.z;
   return kmSqDistPointAABB(p, &aabb);
}

static kmBool kmAABBExtentIntersectsLine(const kmAABBExtent* a, const kmVec3* p1, const kmVec3* p2)
{
   /* d = (p2 - p1) * 0.5 */
   kmVec3 d;
   kmVec3Subtract(&d, p2, p1);
   kmVec3Scale(&d, &d, 0.5f);

   /* c = p1 + d - (min + max) * 0.5; */
   kmVec3 c;
   kmVec3 min;
   kmVec3Scale(&min, &a->extent, -1.0f);
   kmVec3Add(&c, &min, &a->extent);
   kmVec3Scale(&c, &c, 0.5f);
   kmVec3Subtract(&c, &d, &c);
   kmVec3Add(&c, &c, p1);

   /* ad = abs(d) */
   kmVec3 ad;
   kmVec3Abs(&ad, &d);

   /* alias for clarity */
   const kmVec3* e = &a->extent;

   if (fabs(c.x) > e->x + ad.x) return KM_FALSE;
   if (fabs(c.y) > e->y + ad.y) return KM_FALSE;
   if (fabs(c.z) > e->z + ad.z) return KM_FALSE;
   if (fabs(d.y * c.z - d.z * c.y) > e->y * ad.z + e->z * ad.y + kmEpsilon) return KM_FALSE;
   if (fabs(d.z * c.x - d.x * c.z) > e->z * ad.x + e->x * ad.z + kmEpsilon) return KM_FALSE;
   if (fabs(d.x * c.y - d.y * c.x) > e->x * ad.y + e->y * ad.x + kmEpsilon) return KM_FALSE;

   return KM_TRUE;
}

static void kmVec3Swap(kmVec3 *a, kmVec3 *b)
{
   const kmVec3 tmp = *a;
   *a = *b;
   *b = tmp;
}

static void kmSwap(kmScalar *a, kmScalar *b)
{
   const kmScalar tmp = *a;
   *a = *b;
   *b = tmp;
}

kmScalar kmPlaneDistanceTo(const kmPlane *pIn, const kmVec3 *pV1)
{
   const kmVec3 normal = {pIn->a, pIn->b, pIn->c};
   return kmVec3Dot(pV1, &normal) * pIn->d;
}

static const kmVec3* kmAABBGetPosition(const kmAABB *aabb, kmVec3 *pOut)
{
   kmAABBCentre(aabb, pOut);
   return pOut;
}

static const kmVec3* kmAABBExtentGetPosition(const kmAABBExtent *aabbe, kmVec3 *pOut)
{
   kmVec3Assign(pOut, &aabbe->point);
   return pOut;
}

static const kmVec3* kmSphereGetPosition(const kmSphere *sphere, kmVec3 *pOut)
{
   kmVec3Assign(pOut, &sphere->point);
   return pOut;
}

static void kmAABBApplyVelocity(kmAABB *aabb, const kmVec3 *velocity)
{
   kmVec3Add(&aabb->min, &aabb->min, velocity);
   kmVec3Add(&aabb->max, &aabb->max, velocity);
}

static void kmAABBExtentApplyVelocity(kmAABBExtent *aabbe, const kmVec3 *velocity)
{
   kmVec3Add(&aabbe->point, &aabbe->point, velocity);
}

static void kmSphereApplyVelocity(kmSphere *sphere, const kmVec3 *velocity)
{
   kmVec3Add(&sphere->point, &sphere->point, velocity);
}

kmSphere* kmSphereFromAABB(kmSphere *sphere, const kmAABB *aabb)
{
   kmAABBCentre(aabb, &sphere->point);
   sphere->radius = kmAABBDiameterX(aabb);
   sphere->radius = fmax(sphere->radius, kmAABBDiameterY(aabb));
   sphere->radius = fmax(sphere->radius, kmAABBDiameterZ(aabb));
   return sphere;
}

kmBool kmSphereIntersectsAABBExtent(const kmSphere *a, const kmAABBExtent *b)
{
   if(kmAABBExtentContainsPoint(b, &a->point)) return KM_TRUE;
   kmScalar distance = kmSqDistPointAABBExtent(&a->point, b);
   return (distance < a->radius * a->radius);
}

kmBool kmSphereIntersectsAABB(const kmSphere *a, const kmAABB *b)
{
   kmScalar distance = kmSqDistPointAABB(&a->point, b);
   return (distance < a->radius * a->radius);
}

kmBool kmSphereIntersectsSphere(const kmSphere *a, const kmSphere *b)
{
   kmVec3 vector;
   kmScalar distance, radiusSum;
   kmVec3Subtract(&vector, &a->point, &b->point);
   distance = kmVec3LengthSq(&vector) + 1.0f;
   radiusSum = a->radius + b->radius;
   return (distance < radiusSum * radiusSum);
}

kmBool kmSphereIntersectsCapsule(const kmSphere *a, const kmCapsule *b)
{
   kmScalar distance, radiusSum;
   distance = kmVec3LengthSqSegment(&b->pointA, &b->pointB, &a->point) + 1.0f;
   radiusSum = a->radius + b->radius;
   return (distance < radiusSum * radiusSum);
}

kmBool kmCapsuleIntersectsCapsule(const kmCapsule *a, const kmCapsule *b)
{
   kmScalar s;
   kmVec3 c1, c2;
   kmScalar distance, radiusSum;
   distance = kmClosestPointFromSegments(&a->pointA, &a->pointB, &b->pointA, &b->pointB, &s, &s, &c1, &c2) + 1.0f;
   radiusSum = a->radius + b->radius;
   return (distance < radiusSum * radiusSum);
}

kmBool kmSphereIntersectsPlane(const kmSphere *a, const kmPlane *b)
{
   kmVec3 n = {b->a, b->b, b->c};
   kmScalar distance;
   distance = kmVec3Dot(&a->point, &n) - b->d + 1.0f;
   return (fabs(distance) < a->radius);
}

kmMat3* kmOBBGetMat3(const kmOBB *pIn, kmMat3 *pOut)
{
   int i;
   for (i = 0; i < 3; ++i) {
      pOut->mat[i*3+0] = pIn->orientation[i].x;
      pOut->mat[i*3+1] = pIn->orientation[i].y;
      pOut->mat[i*3+2] = pIn->orientation[i].z;
   }
   return pOut;
}

kmBool kmOBBIntersectsOBB(const kmOBB *a, const kmOBB *b)
{
   int i, j;
   kmScalar ra, rb;
   kmMat3 mat, absMat;
   kmVec3 tmp, translation;

   /* compute rotation matrix expressing b in a's coordinate frame */
   for (i = 0; i < 3; ++i) for (j = 0; j < 3; ++j)
      mat.mat[i+j*3] = kmVec3Dot(&a->orientation[i], &b->orientation[j]);

   /* bring translations into a's coordinate frame */
   kmVec3Subtract(&tmp, &a->aabb.point, &b->aabb.point);
   translation.x = kmVec3Dot(&tmp, &a->orientation[0]);
   translation.y = kmVec3Dot(&tmp, &a->orientation[1]);
   translation.z = kmVec3Dot(&tmp, &a->orientation[2]);

   /* compute common subexpressions. add in and epsilon term to
    * counteract arithmetic errors when two edges are parallel and
    * their cross product is (near) null. */
   for (i = 0; i < 3; ++i) for (j = 0; j < 3; ++j)
      absMat.mat[i+j*3] = fabs(mat.mat[i+j*3]) + kmEpsilon;

   /* test axes L = A0, L = A1, L = A2 */
   for (i = 0; i < 3; ++i) {
      ra = (i==0?a->aabb.extent.x:i==1?a->aabb.extent.y:a->aabb.extent.z);
      rb = b->aabb.extent.x * absMat.mat[i+0*3] + b->aabb.extent.y * absMat.mat[i+1*3] + b->aabb.extent.z * absMat.mat[i+2*3];
      if (fabs((i==0?translation.x:i==1?translation.y:translation.z)) > ra + rb) return KM_FALSE;
   }

   /* test axes L = B0, L = B1, L = B2 */
   for (i = 0; i < 3; ++i) {
      ra = a->aabb.extent.x * absMat.mat[0+i*3] + a->aabb.extent.y * absMat.mat[1+i*3] + a->aabb.extent.z * absMat.mat[2+i*3];
      rb = (i==0?b->aabb.extent.x:i==1?b->aabb.extent.y:b->aabb.extent.z);
      if (fabs(translation.x * mat.mat[0+i*3] + translation.y * mat.mat[1+i*3] + translation.z * mat.mat[2+i*3]) > ra + rb) return KM_FALSE;
   }

   /* test axis L = A0 x B0 */
   ra = a->aabb.extent.y * absMat.mat[2+0*3] + a->aabb.extent.z * absMat.mat[1+0*3];
   rb = b->aabb.extent.y * absMat.mat[0+2*3] + b->aabb.extent.z * absMat.mat[0+1*3];
   if (fabs(translation.z * mat.mat[1+0*3] - translation.y * mat.mat[2+0*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A0 x B1 */
   ra = a->aabb.extent.y * absMat.mat[2+1*3] + a->aabb.extent.z * absMat.mat[1+1*3];
   rb = b->aabb.extent.x * absMat.mat[0+2*3] + b->aabb.extent.z * absMat.mat[0+0*3];
   if (fabs(translation.z * mat.mat[1+1*3] - translation.y * mat.mat[2+1*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A0 x B2 */
   ra = a->aabb.extent.y * absMat.mat[2+2*3] + a->aabb.extent.z * absMat.mat[1+2*3];
   rb = b->aabb.extent.x * absMat.mat[0+1*3] + b->aabb.extent.y * absMat.mat[0+0*3];
   if (fabs(translation.z * mat.mat[1+2*3] - translation.y * mat.mat[2+2*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A1 x B0 */
   ra = a->aabb.extent.x * absMat.mat[2+0*3] + a->aabb.extent.z * absMat.mat[0+0*3];
   rb = b->aabb.extent.y * absMat.mat[1+2*3] + b->aabb.extent.z * absMat.mat[1+1*3];
   if (fabs(translation.x * mat.mat[2+0*3] - translation.z * mat.mat[0+0*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A1 x B1 */
   ra = a->aabb.extent.x * absMat.mat[2+1*3] + a->aabb.extent.z * absMat.mat[0+1*3];
   rb = b->aabb.extent.x * absMat.mat[1+2*3] + b->aabb.extent.z * absMat.mat[1+0*3];
   if (fabs(translation.x * mat.mat[2+1*3] - translation.z * mat.mat[0+1*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A1 x B2 */
   ra = a->aabb.extent.x * absMat.mat[2+2*3] + a->aabb.extent.z * absMat.mat[0+2*3];
   rb = b->aabb.extent.x * absMat.mat[1+1*3] + b->aabb.extent.y * absMat.mat[1+0*3];
   if (fabs(translation.x * mat.mat[2+2*3] - translation.z * mat.mat[0+2*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A2 x B0 */
   ra = a->aabb.extent.x * absMat.mat[1+0*3] + a->aabb.extent.y * absMat.mat[0+0*3];
   rb = b->aabb.extent.y * absMat.mat[2+2*3] + b->aabb.extent.z * absMat.mat[2+1*3];
   if (fabs(translation.y * mat.mat[0+0*3] - translation.x * mat.mat[1+0*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A2 x B1 */
   ra = a->aabb.extent.x * absMat.mat[1+1*3] + a->aabb.extent.y * absMat.mat[0+1*3];
   rb = b->aabb.extent.x * absMat.mat[2+2*3] + b->aabb.extent.z * absMat.mat[2+0*3];
   if (fabs(translation.y * mat.mat[0+1*3] - translation.x * mat.mat[1+1*3]) > ra + rb) return KM_FALSE;

   /* test axis L = A2 x B2 */
   ra = a->aabb.extent.x * absMat.mat[1+2*3] + a->aabb.extent.y * absMat.mat[0+2*3];
   rb = b->aabb.extent.x * absMat.mat[2+1*3] + b->aabb.extent.y * absMat.mat[2+0*3];
   if (fabs(translation.y * mat.mat[0+2*3] - translation.x * mat.mat[1+2*3]) > ra + rb) return KM_FALSE;

   /* no seperating axis found */
   return KM_TRUE;
}

kmBool kmAABBIntersectsAABB(const kmAABB *a, const kmAABB *b)
{
   if (a->max.x < b->min.x || a->min.x > b->max.x) return KM_FALSE;
   if (a->max.y < b->min.y || a->min.y > b->max.y) return KM_FALSE;
   if (a->max.z < b->min.z || a->min.z > b->max.z) return KM_FALSE;
   return KM_TRUE;
}

kmBool kmAABBIntersectsSphere(const kmAABB *a, const kmSphere *b)
{
   return kmSphereIntersectsAABB(b, a);
}

kmBool kmAABBExtentIntersectsAABBExtent(const kmAABBExtent *a, const kmAABBExtent *b)
{
   if (fabs(a->point.x - b->point.x) > (a->extent.x + b->extent.x)) return KM_FALSE;
   if (fabs(a->point.y - b->point.y) > (a->extent.y + b->extent.y)) return KM_FALSE;
   if (fabs(a->point.z - b->point.z) > (a->extent.z + b->extent.z)) return KM_FALSE;
   return KM_TRUE;
}

kmBool kmAABBExtentIntersectsSphere(const kmAABBExtent *a, const kmSphere *b)
{
   return kmSphereIntersectsAABBExtent(b, a);
}

kmBool kmAABBIntersectsAABBExtent(const kmAABB *a, const kmAABBExtent *b)
{
   if (a->min.x > b->point.x + b->extent.x || a->max.x < b->point.x - b->extent.x) return KM_FALSE;
   if (a->min.y > b->point.y + b->extent.y || a->max.y < b->point.y - b->extent.y) return KM_FALSE;
   if (a->min.z > b->point.z + b->extent.z || a->max.z < b->point.z - b->extent.z) return KM_FALSE;
   return KM_TRUE;
}

kmBool kmAABBExtentIntersectsAABB(const kmAABBExtent *a, const kmAABB *b)
{
   return kmAABBIntersectsAABBExtent(b, a);
}

kmBool kmAABBExtentIntersectsOBB(const kmAABBExtent *a, const kmOBB *b)
{
   kmOBB obb = {*a, {{1,0,0},{0,1,0},{0,0,1}}};
   return kmOBBIntersectsOBB(&obb, b);
}

kmBool kmOBBIntersectsAABBExtent(const kmOBB *a, const kmAABBExtent *b)
{
   return kmAABBExtentIntersectsOBB(b, a);
}

kmBool kmAABBIntersectsOBB(const kmAABB *a, const kmOBB *b)
{
   kmAABBExtent aabbExtent;
   kmAABBToAABBExtent(&aabbExtent, a);
   return kmAABBExtentIntersectsOBB(&aabbExtent, b);
}

kmBool kmOBBIntersectsAABB(const kmOBB *a, const kmAABB *b)
{
   return kmAABBIntersectsOBB(b, a);
}

kmBool kmAABBExtentIntersectsCapsule(const kmAABBExtent *a, const kmCapsule *b)
{
   /* Quick rejection test using the smallest (quickly calculatable) capsule the AABB fits inside*/
   kmCapsule smallestContainingCapsule = {{0, 0, 0}, {0, 0, 0}, 0};

   if(a->extent.x >= a->extent.y && a->extent.x >= a->extent.z)
   {
      smallestContainingCapsule.radius = sqrt(a->extent.y * a->extent.y + a->extent.z * a->extent.z);
      smallestContainingCapsule.pointA.x = a->point.x - a->extent.x;
      smallestContainingCapsule.pointB.x = a->point.x + a->extent.x;
   }
   else if(a->extent.y >= a->extent.x && a->extent.y >= a->extent.z)
   {
      smallestContainingCapsule.radius = sqrt(a->extent.x * a->extent.x + a->extent.z * a->extent.z);
      smallestContainingCapsule.pointA.y = a->point.y - a->extent.y;
      smallestContainingCapsule.pointB.y = a->point.y + a->extent.y;
   }
   else
   {
      smallestContainingCapsule.radius = sqrt(a->extent.x * a->extent.x + a->extent.y * a->extent.y);
      smallestContainingCapsule.pointA.z = a->point.z - a->extent.z;
      smallestContainingCapsule.pointB.z = a->point.z + a->extent.z;
   }

   if(!kmCapsuleIntersectsCapsule(&smallestContainingCapsule, b)) return KM_FALSE;

   /* Quick acceptance test for capsule line */
   if(kmAABBExtentIntersectsLine(a, &b->pointA, &b->pointB)) return KM_TRUE;

   /* Quick acceptance tests for capsule end spheres */
   kmSphere spa = {b->pointA, b->radius};
   if(kmAABBExtentIntersectsSphere(a, &spa)) return KM_TRUE;

   kmSphere spb = {b->pointB, b->radius};
   if(kmAABBExtentIntersectsSphere(a, &spb)) return KM_TRUE;



   return KM_TRUE;
}

// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
kmBool kmRay3IntersectAABBExtent(const kmRay3 *ray, const kmAABBExtent *aabbe, float *outMin, kmVec3 *outIntersection)
{
   float tmin = 0.0f, tmax = FLT_MAX;

   if (fabs(ray->dir.x) < kmEpsilon) {
      if (ray->start.x < aabbe->point.x - aabbe->extent.x || ray->start.x > aabbe->point.x + aabbe->extent.x) return KM_FALSE;
   } else {
      float ood = 1.0f / ray->start.x;
      float t1 = ((aabbe->point.x - aabbe->extent.x) - ray->start.x) * ood;
      float t2 = ((aabbe->point.x + aabbe->extent.x) - ray->start.x) * ood;
      if (t1 > t2) kmSwap(&t1, &t2);
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   if (fabs(ray->dir.y) < kmEpsilon) {
      if (ray->start.y < aabbe->point.y - aabbe->extent.y || ray->start.y > aabbe->point.y + aabbe->extent.y) return KM_FALSE;
   } else {
      float ood = 1.0f / ray->start.y;
      float t1 = ((aabbe->point.y - aabbe->extent.y) - ray->start.y) * ood;
      float t2 = ((aabbe->point.y + aabbe->extent.y) - ray->start.y) * ood;
      if (t1 > t2) kmSwap(&t1, &t2);
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   if (fabs(ray->dir.z) < kmEpsilon) {
      // Ray is parallel to slab. No hit if origin not within slab
      if (ray->start.z < aabbe->point.z - aabbe->extent.z || ray->start.z > aabbe->point.z + aabbe->extent.z) return KM_FALSE;
   } else {
      // Compute intersection t value of ray with near and far plane of slab
      float ood = 1.0f / ray->start.z;
      float t1 = ((aabbe->point.z - aabbe->extent.z) - ray->start.z) * ood;
      float t2 = ((aabbe->point.z + aabbe->extent.z) - ray->start.z) * ood;

      // Make t1 be intersection with near plane, t2 with far plane
      if (t1 > t2) kmSwap(&t1, &t2);

      // Exit with no collision as soon as slab intersection becomes empty
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
   if (outIntersection) {
      kmVec3 dMultMin;
      kmVec3Scale(&dMultMin, &ray->dir, tmin);
      kmVec3Add(outIntersection, &ray->start, &dMultMin);
   }
   if (outMin) * outMin = tmin;
   return KM_TRUE;
}

// Intersect ray R(t) = p + t*d against AABB a. When intersecting,
// return intersection distance tmin and point q of intersection
kmBool kmRay3IntersectAABB(const kmRay3 *ray, const kmAABB *aabb, float *outMin, kmVec3 *outIntersection)
{
   float tmin = 0.0f, tmax = FLT_MAX;

   if (fabs(ray->dir.x) < kmEpsilon) {
      if (ray->start.x < aabb->min.x || ray->start.x > aabb->max.x) return KM_FALSE;
   } else {
      float ood = 1.0f / ray->start.x;
      float t1 = (aabb->min.x - ray->start.x) * ood;
      float t2 = (aabb->max.x - ray->start.x) * ood;
      if (t1 > t2) kmSwap(&t1, &t2);
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   if (fabs(ray->dir.y) < kmEpsilon) {
      if (ray->start.y < aabb->min.y || ray->start.y > aabb->max.y) return KM_FALSE;
   } else {
      float ood = 1.0f / ray->start.y;
      float t1 = (aabb->min.y - ray->start.y) * ood;
      float t2 = (aabb->max.y - ray->start.y) * ood;
      if (t1 > t2) kmSwap(&t1, &t2);
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   if (fabs(ray->dir.z) < kmEpsilon) {
      // Ray is parallel to slab. No hit if origin not within slab
      if (ray->start.z < aabb->min.z || ray->start.z > aabb->max.z) return KM_FALSE;
   } else {
      // Compute intersection t value of ray with near and far plane of slab
      float ood = 1.0f / ray->start.z;
      float t1 = (aabb->min.z - ray->start.z) * ood;
      float t2 = (aabb->max.z - ray->start.z) * ood;

      // Make t1 be intersection with near plane, t2 with far plane
      if (t1 > t2) kmSwap(&t1, &t2);

      // Exit with no collision as soon as slab intersection becomes empty
      if (t1 > tmin) tmin = t1;
      if (t2 > tmax) tmax = t2;
      if (tmin > tmax) return KM_FALSE;
   }

   // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
   if (outIntersection) {
      kmVec3 dMultMin;
      kmVec3Scale(&dMultMin, &ray->dir, tmin);
      kmVec3Add(outIntersection, &ray->start, &dMultMin);
   }
   if (outMin) * outMin = tmin;
   return KM_TRUE;
}


kmBool kmRay3IntersectTriangle(const kmRay3 *ray, const kmTriangle *tri, kmVec3 *outIntersection)
{
  kmVec3 u, v, n;
  kmVec3Subtract(&u,  &tri->v2,  &tri->v1);
  kmVec3Subtract(&v,  &tri->v3,  &tri->v1);
  kmVec3Cross(&n, &u, &v);

  kmPlane triPlane;
  kmPlaneFromPointAndNormal(&triPlane, &tri->v1, &n);

  kmVec3 p;
  if (!kmRay3IntersectPlane(&p, ray, &triPlane))
  {
    return KM_FALSE;
  }

  kmVec3 w;
  kmVec3Subtract(&w, &p, &tri->v1);

  const kmScalar uv = kmVec3Dot(&u, &v);
  const kmScalar wv = kmVec3Dot(&w, &v);
  const kmScalar vv = kmVec3Dot(&v, &v);
  const kmScalar wu = kmVec3Dot(&w, &u);
  const kmScalar uu = kmVec3Dot(&u, &u);

  const kmScalar d = uv * uv - uu * vv;
  const kmScalar s = (uv * wv - vv * wu) / d;
  const kmScalar t = (uv * wu - uu * wv) / d;

  if (s < 0 || t < 0 || s + t > 1)
  {
    return KM_FALSE;
  }

  *outIntersection = p;
  return KM_TRUE;
}


/***
 * Collision manager
 ***/

/* internal */
typedef enum _glhckCollisionType {
   GLHCK_COLLISION_AABB,
   GLHCK_COLLISION_AABBE,
   GLHCK_COLLISION_OBB,
   GLHCK_COLLISION_SPHERE,
   GLHCK_COLLISION_ELLIPSE,
   GLHCK_COLLISION_CAPSULE,
   GLHCK_COLLISION_LAST,
} _glhckCollisionType;

typedef struct _glhckCollisionShape {
   /* shape union pointer */
   union {
      kmAABB *aabb;
      kmAABBExtent *aabbe;
      kmOBB *obb;
      kmSphere *sphere;
      kmEllipse *ellipse;
      kmCapsule *capsule;
      void *any;
   };

   /* type of shape */
   _glhckCollisionType type;

   /* is the shape a reference? */
   char reference;
} _glhckCollisionShape;

typedef struct _glhckCollisionPrimitive {
   /* shape for primitive */
   struct _glhckCollisionShape shape;

   /* also a linked list */
   struct _glhckCollisionPrimitive *next;

   /* user data */
   void *userData;
} _glhckCollisionPrimitive;

typedef struct _glhckCollisionWorld {
   /* number of collision packets going on */
   unsigned int packets, rejected;

   /* collision primitives living under the world */
   struct _glhckCollisionPrimitive *primitives;

   /* user data */
   void *userData;
} _glhckCollisionWorld;

/* internal */
typedef struct _glhckCollisionPacket {
   /* shape for packet */
   struct _glhckCollisionShape *shape;

   /* sweep shape for packet (if any) */
   struct _glhckCollisionShape *sweep;

   /* internal sliding response velocity */
   kmVec3 velocity;

   /* input data from user for this packet */
   const glhckCollisionInData *data;

   /* number of collisions */
   unsigned int collisions;
} _glhckCollisionPacket;

/* safe global pointers */
static void *_glhckCollisionTestFunctions[GLHCK_COLLISION_LAST][GLHCK_COLLISION_LAST];
static void *_glhckCollisionVelocityFunctions[GLHCK_COLLISION_LAST];
static void *_glhckCollisionContactFunctions[GLHCK_COLLISION_LAST];
static void *_glhckCollisionPositionFunctions[GLHCK_COLLISION_LAST];
static void _glhckCollisionInit(void)
{
   /* setup test functions */
   memset(_glhckCollisionTestFunctions, 0, sizeof(_glhckCollisionTestFunctions));

   /* aabb vs. x */
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABB][GLHCK_COLLISION_AABB] = kmAABBIntersectsAABB;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABB][GLHCK_COLLISION_SPHERE] = kmAABBIntersectsSphere;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABB][GLHCK_COLLISION_AABBE] = kmAABBIntersectsAABBExtent;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABB][GLHCK_COLLISION_OBB] = kmAABBIntersectsOBB;

   /* aabbe vs. x */
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABBE][GLHCK_COLLISION_AABBE] = kmAABBExtentIntersectsAABBExtent;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABBE][GLHCK_COLLISION_SPHERE] = kmAABBExtentIntersectsSphere;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABBE][GLHCK_COLLISION_AABB] = kmAABBExtentIntersectsAABB;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_AABBE][GLHCK_COLLISION_OBB] = kmAABBExtentIntersectsOBB;

   /* sphere vs. x */
   _glhckCollisionTestFunctions[GLHCK_COLLISION_SPHERE][GLHCK_COLLISION_SPHERE] = kmSphereIntersectsSphere;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_SPHERE][GLHCK_COLLISION_AABB] = kmSphereIntersectsAABB;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_SPHERE][GLHCK_COLLISION_AABBE] = kmSphereIntersectsAABBExtent;

   /* obb vs. x */
   _glhckCollisionTestFunctions[GLHCK_COLLISION_OBB][GLHCK_COLLISION_OBB] = kmOBBIntersectsOBB;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_OBB][GLHCK_COLLISION_AABB] = kmOBBIntersectsAABB;
   _glhckCollisionTestFunctions[GLHCK_COLLISION_OBB][GLHCK_COLLISION_AABBE] = kmOBBIntersectsAABBExtent;

   /* setup velocity functions */
   memset(_glhckCollisionVelocityFunctions, 0, sizeof(_glhckCollisionVelocityFunctions));
   _glhckCollisionVelocityFunctions[GLHCK_COLLISION_AABB] = kmAABBApplyVelocity;
   _glhckCollisionVelocityFunctions[GLHCK_COLLISION_AABBE] = kmAABBExtentApplyVelocity;
   _glhckCollisionVelocityFunctions[GLHCK_COLLISION_SPHERE] = kmSphereApplyVelocity;

   /* setup contact functions */
   memset(_glhckCollisionContactFunctions, 0, sizeof(_glhckCollisionContactFunctions));
   _glhckCollisionContactFunctions[GLHCK_COLLISION_AABB] = kmAABBClosestPointTo;
   _glhckCollisionContactFunctions[GLHCK_COLLISION_AABBE] = kmAABBExtentClosestPointTo;
   _glhckCollisionContactFunctions[GLHCK_COLLISION_SPHERE] = kmSphereClosestPointTo;

   /* setup position functions */
   memset(_glhckCollisionPositionFunctions, 0, sizeof(_glhckCollisionPositionFunctions));
   _glhckCollisionPositionFunctions[GLHCK_COLLISION_AABB] = kmAABBGetPosition;
   _glhckCollisionPositionFunctions[GLHCK_COLLISION_AABBE] = kmAABBExtentGetPosition;
   _glhckCollisionPositionFunctions[GLHCK_COLLISION_SPHERE] = kmSphereGetPosition;
}

/* get string for collision shape type */
const char* _glhckCollisionTypeString(_glhckCollisionType type)
{
   switch (type) {
      case GLHCK_COLLISION_AABB:return "AABB";
      case GLHCK_COLLISION_AABBE:return "AABBE";
      case GLHCK_COLLISION_OBB:return "OBB";
      case GLHCK_COLLISION_SPHERE:return "SPHERE";
      case GLHCK_COLLISION_ELLIPSE:return "ELLIPSE";
      case GLHCK_COLLISION_CAPSULE:return "CAPSULE";
      default:break;
   }
   return "INVALID TYPE";
}

#ifndef NDEBUG
static int _glhckCollisionTestMissingImplementations(const _glhckCollisionShape *a, const _glhckCollisionShape *b)
{
   int ret = RETURN_OK;

   /* test for missing implementations */
      if (b && !_glhckCollisionTestFunctions[a->type][b->type]) {
         DEBUG(GLHCK_DBG_ERROR, "-!- Intersection test not implemented for %s <-> %s",
               _glhckCollisionTypeString(a->type),
               _glhckCollisionTypeString(b->type));
      }
      if (!_glhckCollisionContactFunctions[a->type]) {
         DEBUG(GLHCK_DBG_ERROR, "-!- Contact point function not implemented for %s", _glhckCollisionTypeString(a->type));
         ret = RETURN_FAIL;
      }
      if (b && !_glhckCollisionContactFunctions[b->type]) {
         DEBUG(GLHCK_DBG_ERROR, "-!- Contact point function not implemented for %s", _glhckCollisionTypeString(b->type));
         ret = RETURN_FAIL;
      }
      if (!_glhckCollisionPositionFunctions[a->type]) {
         DEBUG(GLHCK_DBG_ERROR, "-!- Shape position function not implemented for %s", _glhckCollisionTypeString(a->type));
         ret = RETURN_FAIL;
      }
      if (b && !_glhckCollisionPositionFunctions[b->type]) {
         DEBUG(GLHCK_DBG_ERROR, "-!- Shape position function not implemented for %s", _glhckCollisionTypeString(b->type));
         ret = RETURN_FAIL;
      }

   return ret;
}
#endif

/* calculate contact point and push vector for shape */
static void _glhckCollisionShapeShapeContact(const _glhckCollisionShape *packetShape, const _glhckCollisionShape *primitiveShape, kmVec3 *outContact, kmVec3 *outPush)
{
   kmVec3 packetCenter, penetrativeContact, difference;
   typedef const kmVec3* (*_positionFunc)(const void *a, kmVec3 *outPoint);
   typedef const kmVec3* (*_contactFunc)(const void *a, const kmVec3 *point, kmVec3 *outPoint);
   _positionFunc packetPosition = _glhckCollisionPositionFunctions[packetShape->type];
   _contactFunc packetContact = _glhckCollisionContactFunctions[packetShape->type];
   _contactFunc primitiveContact = _glhckCollisionContactFunctions[primitiveShape->type];
   assert(packetShape && primitiveShape && outContact);

   /* get contact points for packet and primitive */
   packetPosition(packetShape->any, &packetCenter);
   primitiveContact(primitiveShape->any, &packetCenter, outContact);
   packetContact(packetShape->any, outContact, &penetrativeContact);

   /* figure out real contact normal from comparing the distances */
   float contactDist = kmVec3Length(kmVec3Subtract(&difference, outContact, &packetCenter));
   float penetrativeDist = kmVec3Length(kmVec3Subtract(&difference, &penetrativeContact, &packetCenter));
   if (contactDist > penetrativeDist) kmVec3Swap(outContact, &penetrativeContact);
   if (outPush) kmVec3Subtract(outPush, outContact, &penetrativeContact);
}

/* test packet against primitive */
static void _glhckCollisionWorldTestPacketAgainstPrimitive(glhckCollisionWorld *world, _glhckCollisionPacket *packet, const _glhckCollisionPrimitive *primitive)
{
   typedef kmBool (*_collisionTestFunc)(const void *a, const void *b);
   _collisionTestFunc intersection = _glhckCollisionTestFunctions[packet->shape->type][primitive->shape.type];
   assert(world && packet && primitive);

#ifndef NDEBUG
   if (!_glhckCollisionTestMissingImplementations(packet->shape, &primitive->shape))
      return;
#endif

   /* this is reference to us, bad things will happen if we continue */
   if (packet->shape->any == primitive->shape.any)
      return;

   /* ask user if we should even bother testing */
   if (packet->data->test && !packet->data->test(packet->data, primitive))
      return;

   /* intersection test */
   if (!intersection(packet->shape->any, primitive->shape.any))
      return;

   /* penetrative push back response tests */
   if (packet->data->response) {
      static kmVec3 zero = {0,0,0};
      kmVec3 contactPoint, pushVector;
      _glhckCollisionShapeShapeContact(packet->shape, &primitive->shape, &contactPoint, &pushVector);

      /* we are not penetrating this round */
      if (kmVec3AreEqual(&pushVector, &zero)) return;

      /* early loop detection */
      if (packet->collisions > 0) {
         kmVec3 inverseVelocity;
         kmVec3Scale(&inverseVelocity, &packet->velocity, -1);
         if (kmVec3AreEqual(&inverseVelocity, &pushVector)) return;
      }

      /* send response */
      glhckCollisionOutData outData;
      outData.world = world;
      outData.collider = primitive;
      outData.pushVector = &pushVector;
      outData.contactPoint = &contactPoint;
      outData.velocity = &packet->velocity;
      outData.userData = packet->data->userData;
      packet->data->response(&outData);
      memcpy(&packet->velocity, &pushVector, sizeof(kmVec3));
   }

   ++packet->collisions;
}

/* sweep test against collision world primitives using the packet */
static int _glhckCollisionWorldTestPacketSweep(glhckCollisionWorld *world, _glhckCollisionPacket *packet)
{
   float nearestSweepDistance = FLT_MAX;
   glhckCollisionPrimitive *p, *nearestSweepPrimitive = NULL;
   kmVec3 inverseVelocity, nearestContact, beforePoint;
   typedef const kmVec3* (*_positionFunc)(const void *a, kmVec3 *outPoint);
   typedef kmBool (*_collisionTestFunc)(const void *a, const void *b);
   typedef void (*_velocityApplyFunc)(const void *a, const kmVec3 *velocity);
   _positionFunc packetPosition = _glhckCollisionPositionFunctions[packet->shape->type];
   _velocityApplyFunc velocity = _glhckCollisionVelocityFunctions[packet->shape->type];

#ifndef NDEBUG
   if (!_glhckCollisionTestMissingImplementations(packet->sweep, NULL))
      return 0;
#endif

   /* move packet shape to before contact phase */
   packetPosition(packet->shape->any, &beforePoint);
   kmVec3Scale(&inverseVelocity, &packet->velocity, -1.0f);
   velocity(packet->shape->any, &inverseVelocity);

   /* run through primitives to see what we need to sweep against */
   for (p = world->primitives; p; p = p->next) {
      /* ask user if we should even bother testing */
      if (packet->data->test && !packet->data->test(packet->data, p))
         continue;

#ifndef NDEBUG
      if (!_glhckCollisionTestMissingImplementations(packet->sweep, &p->shape))
         continue;
#endif

      /* intersection test */
      _collisionTestFunc intersection = _glhckCollisionTestFunctions[packet->sweep->type][p->shape.type];
      if (!intersection(packet->sweep->any, p->shape.any))
         continue;

      /* figure out contact point */
      kmVec3 difference, contact;
      _glhckCollisionShapeShapeContact(packet->shape, &p->shape, &contact, NULL);
      kmVec3Subtract(&difference, &contact, &beforePoint);
      float distance = kmVec3Length(&difference)+1.0f;
      if (distance < nearestSweepDistance) {
         memcpy(&nearestContact, &contact, sizeof(kmVec3));
         nearestSweepDistance = distance;
         nearestSweepPrimitive = p;
      }
   }

   /* no collision */
   if (!nearestSweepPrimitive) return RETURN_FALSE;

   /* do we need to sweep? */
   if (packet->data->response && nearestSweepDistance > 1.0f) {
      kmVec3 pushVector;
      float velocityDist = kmVec3Length(&packet->velocity);
      kmVec3Scale(&pushVector, &packet->velocity, -nearestSweepDistance/velocityDist);

      /* send response */
      glhckCollisionOutData outData;
      outData.world = world;
      outData.collider = nearestSweepPrimitive;
      outData.pushVector = &pushVector;
      outData.contactPoint = &nearestContact;
      outData.velocity = &packet->velocity;
      outData.userData = packet->data->userData;
      packet->data->response(&outData);
      memcpy(&packet->velocity, &pushVector, sizeof(kmVec3));
   } else {
      velocity(packet->shape->any, &packet->velocity);
   }

   /* collision */
   return RETURN_TRUE;
}

static unsigned int _glhckCollisionWorldCollide(glhckCollisionWorld *world, _glhckCollisionShape *shape, _glhckCollisionShape *sweep, const glhckCollisionInData *data)
{
   static const kmVec3 zero = {0,0,0};
   glhckCollisionPrimitive *p;
   _glhckCollisionPacket packet;
   assert(world && shape && data);

#if 0
   if (sweep->any) {
      /* debug */
      glhckObject *o = glhckCubeFromKazmathAABBExtent(sweep->aabbe);
      glhckMaterial *mat = glhckMaterialNew(NULL);
      glhckObjectMaterial(o, mat);
      glhckMaterialDiffuseb(mat, 0, 0, 255, 255);
      glhckObjectRender(o);
      glhckMaterialFree(mat);
      glhckObjectFree(o);
   }
#endif

   /* too much recursion, assume no collision and reject future packets */
   if (world->packets + world->rejected > 20) {
      ++world->rejected;
      return 0;
   }

   /* we are not moving anywhere, lets assume no collisions */
   if (data->velocity && kmVec3AreEqual(data->velocity, &zero))
      return 0;

   /* setup packet */
   memset(&packet, 0, sizeof(packet));
   packet.data = data;
   packet.shape = shape;
   if (sweep->any) packet.sweep = sweep;
   if (data->velocity) memcpy(&packet.velocity, data->velocity, sizeof(kmVec3));

   /* sweep test! */
   if (packet.sweep && !_glhckCollisionWorldTestPacketSweep(world, &packet))
      return 0;

   /* indicate that new packet is colliding in this world */
   ++world->packets;

   /* normal collision checks */
   unsigned int oldCollisions = -1;
   while (packet.collisions < 20 && oldCollisions != packet.collisions) {
      oldCollisions = packet.collisions;
      for (p = world->primitives; p; p = p->next) _glhckCollisionWorldTestPacketAgainstPrimitive(world, &packet, p);
      if (!data->response || !data->velocity) break;
   }

   if (world->packets > 0) --world->packets;
   if (world->packets == 0) {
      if (world->rejected) DEBUG(GLHCK_DBG_CRAP, "-!- RECURSION LOOP: %u (rejected)", world->rejected);
      world->rejected = 0;
   }
   return packet.collisions;
}

static void _glhckCollisionShapeFree(_glhckCollisionShape *shape)
{
   /* don't free references */
   if (shape->reference) return;

   switch (shape->type) {
      default:
         IFDO(_glhckFree, shape->any);
         break;
   }
}

static void _glhckCollisionPrimitiveFree(glhckCollisionPrimitive *primitive)
{
   assert(primitive);
   _glhckCollisionShapeFree(&primitive->shape);
   IFDO(_glhckFree, primitive);
}

static glhckCollisionPrimitive* _glhckCollisionWorldAddPrimitive(glhckCollisionWorld *world, _glhckCollisionType type, void *shape, void *userData)
{
   glhckCollisionPrimitive *primitive, *p;
   assert(world && shape);

   if (!(primitive = _glhckCalloc(1, sizeof(glhckCollisionPrimitive))))
      goto fail;

   primitive->shape.type = type;
   primitive->shape.any = shape;
   primitive->userData = userData;

   if (!(p = world->primitives)) {
      world->primitives = primitive;
   } else {
      for (; p && p->next; p = p->next);
      p->next = primitive;
   }

   return primitive;

fail:
   IFDO(_glhckFree, primitive);
   return NULL;
}

GLHCKAPI glhckCollisionWorld* glhckCollisionWorldNew(void *userData)
{
   glhckCollisionWorld *object;

   _glhckCollisionInit();
   if (!(object = _glhckCalloc(1, sizeof(glhckCollisionWorld))))
      goto fail;

   object->userData = userData;
   return object;

fail:
   return NULL;
}

GLHCKAPI void glhckCollisionWorldFree(glhckCollisionWorld *object)
{
   glhckCollisionPrimitive *p, *pn;
   assert(object);

   for (p = object->primitives; p; p = pn) {
      pn = p->next;
      _glhckCollisionPrimitiveFree(p);
   }

   _glhckFree(object);
}

GLHCKAPI void* glhckCollisionWorldGetUserData(const glhckCollisionWorld *object)
{
   assert(object);
   return object->userData;
}

GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddEllipse(glhckCollisionWorld *object, const kmEllipse *ellipse, void *userData)
{
   kmEllipse *ellipseCopy = NULL;
   glhckCollisionPrimitive *primitive = NULL;
   assert(object && ellipse);

   if (!(ellipseCopy = _glhckMalloc(sizeof(kmEllipse))))
      goto fail;

   memcpy(ellipseCopy, ellipse, sizeof(kmEllipse));
   if (!(primitive = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_ELLIPSE, ellipseCopy, userData)))
      goto fail;
   return primitive;

fail:
   IFDO(_glhckFree, primitive);
   IFDO(_glhckFree, ellipseCopy);
   return NULL;
}

GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddAABBRef(glhckCollisionWorld *object, const kmAABB *aabb, void *userData)
{
   glhckCollisionPrimitive *primitive = NULL;
   assert(object && aabb);

   if (!(primitive = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABB, (kmAABB*)aabb, userData)))
      return NULL;

   primitive->shape.reference = 1;
   return primitive;
}

GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddAABB(glhckCollisionWorld *object, const kmAABB *aabb, void *userData)
{
   kmAABB *aabbCopy = NULL;
   glhckCollisionPrimitive *primitive = NULL;
   assert(object && aabb);

   if (!(aabbCopy = _glhckMalloc(sizeof(kmAABB))))
      goto fail;

   memcpy(aabbCopy, aabb, sizeof(kmAABB));
   if (!(primitive = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABB, aabbCopy, userData)))
      goto fail;
   return primitive;

fail:
   IFDO(_glhckFree, aabbCopy);
   return NULL;
}

GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddAABBExtent(glhckCollisionWorld *object, const kmAABBExtent *aabbe, void *userData)
{
   kmAABBExtent *aabbeCopy = NULL;
   glhckCollisionPrimitive *primitive = NULL;
   assert(object && aabbe);

   if (!(aabbeCopy = _glhckMalloc(sizeof(kmAABBExtent))))
      goto fail;

   memcpy(aabbeCopy, aabbe, sizeof(kmAABBExtent));
   if (!(primitive = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABBE, aabbeCopy, userData)))
      goto fail;
   return primitive;

fail:
   IFDO(_glhckFree, aabbeCopy);
   return NULL;
}

GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddSphere(glhckCollisionWorld *object, const kmSphere *sphere, void *userData)
{
   kmSphere *sphereCopy = NULL;
   glhckCollisionPrimitive *primitive = NULL;
   assert(object && sphere);

   if (!(sphereCopy = _glhckMalloc(sizeof(kmSphere))))
      goto fail;

   memcpy(sphereCopy, sphere, sizeof(kmSphere));
   if (!(primitive = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_SPHERE, sphereCopy, userData)))
      goto fail;
   return primitive;

fail:
   IFDO(_glhckFree, sphereCopy);
   return NULL;
}

GLHCKAPI void glhckCollisionWorldRemovePrimitive(glhckCollisionWorld *object, glhckCollisionPrimitive *primitive)
{
   glhckCollisionPrimitive *p;
   assert(object && primitive);

   if (primitive == (p = object->primitives)) {
      object->primitives = primitive->next;
   } else {
      for (; p && p->next != primitive; p = p->next);
      if (p) p->next = primitive->next;
      else object->primitives = NULL;
   }

   _glhckCollisionPrimitiveFree(primitive);
}

GLHCKAPI unsigned int glhckCollisionWorldCollideAABB(glhckCollisionWorld *object, const kmAABB *aabb, const glhckCollisionInData *data)
{
   kmAABBExtent sweepAABB;
   _glhckCollisionShape shape, sweep;
   shape.type = GLHCK_COLLISION_AABB;
   shape.any = (kmAABB*)aabb;
   shape.reference = 1;

   /* create sweep volume, if needed */
   if (data->velocity &&
         (fabs(data->velocity->x) > (aabb->max.x-aabb->min.x)*0.5f ||
          fabs(data->velocity->y) > (aabb->max.y-aabb->min.y)*0.5f ||
          fabs(data->velocity->z) > (aabb->max.z-aabb->min.z)*0.5f)) {
      kmVec3 pointBefore, center;
      kmAABBCentre(aabb, &center);
      kmVec3Subtract(&pointBefore, &center, data->velocity);
      sweepAABB.point.x = (pointBefore.x+center.x)*0.5;
      sweepAABB.point.y = (pointBefore.y+center.y)*0.5;
      sweepAABB.point.z = (pointBefore.z+center.z)*0.5;
      sweepAABB.extent.x = fabs(center.x-sweepAABB.point.x)+aabb->max.x-aabb->min.x;
      sweepAABB.extent.y = fabs(center.y-sweepAABB.point.y)+aabb->max.y-aabb->min.y;
      sweepAABB.extent.z = fabs(center.z-sweepAABB.point.z)+aabb->max.z-aabb->min.z;
      sweep.type = GLHCK_COLLISION_AABBE;
      sweep.any = &sweepAABB;
      sweep.reference = 1;
   } else {
      memset(&sweep, 0, sizeof(sweep));
   }

   return _glhckCollisionWorldCollide(object, &shape, &sweep, data);
}

GLHCKAPI unsigned int glhckCollisionWorldCollideAABBExtent(glhckCollisionWorld *object, const kmAABBExtent *aabbe, const glhckCollisionInData *data)
{
   kmAABBExtent sweepAABB;
   _glhckCollisionShape shape, sweep;
   shape.type = GLHCK_COLLISION_AABBE;
   shape.any = (kmAABBExtent*)aabbe;
   shape.reference = 1;

   /* create sweep volume, if needed */
   if (data->velocity &&
         (fabs(data->velocity->x) > aabbe->extent.x*0.5f ||
          fabs(data->velocity->y) > aabbe->extent.y*0.5f ||
          fabs(data->velocity->z) > aabbe->extent.z*0.5f)) {
      kmVec3 pointBefore;
      kmVec3Subtract(&pointBefore, &aabbe->point, data->velocity);
      sweepAABB.point.x = (pointBefore.x+aabbe->point.x)*0.5;
      sweepAABB.point.y = (pointBefore.y+aabbe->point.y)*0.5;
      sweepAABB.point.z = (pointBefore.z+aabbe->point.z)*0.5;
      sweepAABB.extent.x = fabs(aabbe->point.x-sweepAABB.point.x)+aabbe->extent.x;
      sweepAABB.extent.y = fabs(aabbe->point.y-sweepAABB.point.y)+aabbe->extent.y;
      sweepAABB.extent.z = fabs(aabbe->point.z-sweepAABB.point.z)+aabbe->extent.z;
      sweep.type = GLHCK_COLLISION_AABBE;
      sweep.any = &sweepAABB;
      sweep.reference = 1;
   } else {
      memset(&sweep, 0, sizeof(sweep));
   }

   return _glhckCollisionWorldCollide(object, &shape, &sweep, data);
}

GLHCKAPI unsigned int glhckCollisionWorldCollideOBB(glhckCollisionWorld *object, const kmOBB *obb, const glhckCollisionInData *data)
{
   _glhckCollisionShape shape, sweep;
   memset(&sweep, 0, sizeof(sweep));
   shape.type = GLHCK_COLLISION_OBB;
   shape.any = (kmOBB*)obb;
   shape.reference = 1;

   return _glhckCollisionWorldCollide(object, &shape, &sweep, data);
}

GLHCKAPI unsigned int glhckCollisionWorldCollideSphere(glhckCollisionWorld *object, const kmSphere *sphere, const glhckCollisionInData *data)
{
   kmAABBExtent sweepAABB;
   _glhckCollisionShape shape, sweep;
   shape.type = GLHCK_COLLISION_SPHERE;
   shape.any = (kmSphere*)sphere;
   shape.reference = 1;

   /* create sweep volume, if needed */
   if (data->velocity &&
         (fabs(data->velocity->x) > sphere->radius*0.5f ||
          fabs(data->velocity->y) > sphere->radius*0.5f ||
          fabs(data->velocity->z) > sphere->radius*0.5f)) {
      kmVec3 pointBefore;
      kmVec3Subtract(&pointBefore, &sphere->point, data->velocity);
      sweepAABB.point.x = (pointBefore.x+sphere->point.x)*0.5f;
      sweepAABB.point.y = (pointBefore.y+sphere->point.y)*0.5f;
      sweepAABB.point.z = (pointBefore.z+sphere->point.z)*0.5f;
      sweepAABB.extent.x = fabs(sphere->point.x-sweepAABB.point.x)+sphere->radius;
      sweepAABB.extent.y = fabs(sphere->point.y-sweepAABB.point.y)+sphere->radius;
      sweepAABB.extent.z = fabs(sphere->point.z-sweepAABB.point.z)+sphere->radius;
      sweep.type = GLHCK_COLLISION_AABBE;
      sweep.any = &sweepAABB;
      sweep.reference = 1;
   } else {
      memset(&sweep, 0, sizeof(sweep));
   }

   return _glhckCollisionWorldCollide(object, &shape, &sweep, data);
}

GLHCKAPI void* glhckCollisionPrimitiveGetUserData(const glhckCollisionPrimitive *object)
{
   assert(object);
   return object->userData;
}

/* vim: set ts=8 sw=3 tw=0 :*/
