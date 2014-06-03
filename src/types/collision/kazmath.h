#ifndef __glhck_kazmath_h__
#define __glhck_kazmath_h__

#include <kazmath/kazmath.h>

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

kmAABBExtent* kmAABBToAABBExtent(kmAABBExtent* pOut, const kmAABB* aabb);
kmAABB* kmAABBExtentToAABB(kmAABB* pOut, const kmAABBExtent* aabbExtent);
kmVec3* kmVec3Abs(kmVec3 *pOut, const kmVec3 *pV1);
kmVec3* kmVec3Divide(kmVec3 *pOut, const kmVec3 *pV1, const kmVec3 *pV2);
kmVec3* kmVec3Multiply(kmVec3 *pOut, const kmVec3 *pV1, const kmVec3 *pV2);
kmScalar kmVec3LengthSqSegment(const kmVec3 *a, const kmVec3 *b, const kmVec3 *c);
kmBool kmAABBExtentContainsPoint(const kmAABBExtent* a, const kmVec3* p);
kmScalar kmClosestPointFromSegments(const kmVec3 *a1, const kmVec3 *b1, const kmVec3 *a2, const kmVec3 *b2, kmScalar *s, kmScalar *t, kmVec3 *c1, kmVec3 *c2);
const kmVec3* kmAABBExtentClosestPointTo(const kmAABBExtent *pIn, const kmVec3 *point, kmVec3 *pOut);
const kmVec3* kmAABBClosestPointTo(const kmAABB *pIn, const kmVec3 *point, kmVec3 *pOut);
const kmVec3* kmSphereClosestPointTo(const kmSphere *pIn, const kmVec3 *point, kmVec3 *pOut);
kmScalar kmSqDistPointAABB(const kmVec3 *p, const kmAABB *aabb);
kmScalar kmSqDistPointAABBExtent(const kmVec3 *p, const kmAABBExtent *aabbe);
kmBool kmAABBExtentIntersectsLine(const kmAABBExtent* a, const kmVec3* p1, const kmVec3* p2);
void kmVec3Swap(kmVec3 *a, kmVec3 *b);
void kmSwap(kmScalar *a, kmScalar *b);
kmScalar kmPlaneDistanceTo(const kmPlane *pIn, const kmVec3 *pV1);
kmSphere* kmSphereFromAABB(kmSphere *sphere, const kmAABB *aabb);
kmBool kmSphereIntersectsAABBExtent(const kmSphere *a, const kmAABBExtent *b);
kmBool kmSphereIntersectsAABB(const kmSphere *a, const kmAABB *b);
kmBool kmSphereIntersectsSphere(const kmSphere *a, const kmSphere *b);
kmBool kmSphereIntersectsCapsule(const kmSphere *a, const kmCapsule *b);
kmBool kmCapsuleIntersectsCapsule(const kmCapsule *a, const kmCapsule *b);
kmBool kmSphereIntersectsPlane(const kmSphere *a, const kmPlane *b);
kmMat3* kmOBBGetMat3(const kmOBB *pIn, kmMat3 *pOut);
kmBool kmOBBIntersectsOBB(const kmOBB *a, const kmOBB *b);
kmBool kmAABBIntersectsAABB(const kmAABB *a, const kmAABB *b);
kmBool kmAABBIntersectsSphere(const kmAABB *a, const kmSphere *b);
kmBool kmAABBExtentIntersectsAABBExtent(const kmAABBExtent *a, const kmAABBExtent *b);
kmBool kmAABBExtentIntersectsSphere(const kmAABBExtent *a, const kmSphere *b);
kmBool kmAABBIntersectsAABBExtent(const kmAABB *a, const kmAABBExtent *b);
kmBool kmAABBExtentIntersectsAABB(const kmAABBExtent *a, const kmAABB *b);
kmBool kmAABBExtentIntersectsOBB(const kmAABBExtent *a, const kmOBB *b);
kmBool kmOBBIntersectsAABBExtent(const kmOBB *a, const kmAABBExtent *b);
kmBool kmAABBIntersectsOBB(const kmAABB *a, const kmOBB *b);
kmBool kmOBBIntersectsAABB(const kmOBB *a, const kmAABB *b);
kmBool kmAABBExtentIntersectsCapsule(const kmAABBExtent *a, const kmCapsule *b);
kmBool kmRay3IntersectAABBExtent(const kmRay3 *ray, const kmAABBExtent *aabbe, float *outMin, kmVec3 *outIntersection);
kmBool kmRay3IntersectAABB(const kmRay3 *ray, const kmAABB *aabb, float *outMin, kmVec3 *outIntersection);
kmBool kmRay3IntersectTriangle(const kmRay3 *ray, const kmTriangle *tri, kmVec3 *outIntersection, kmScalar *tOut, kmScalar *sOut);

#endif /* __glhck_kazmath_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
