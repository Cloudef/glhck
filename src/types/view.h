#ifndef __glhck_view_h__
#define __glhck_view_h__

#include <glhck/glhck.h>

GLHCKAPI glhckHandle glhckViewNew(void);
GLHCKAPI int glhckViewGetDirty(const glhckHandle handle);
GLHCKAPI void glhckViewParentAffection(const glhckHandle handle, const unsigned char affectionFlags);
GLHCKAPI unsigned char glhckViewGetParentAffection(const glhckHandle handle);
GLHCKAPI void glhckViewUpdateMany(const glhckHandle *handles, const glhckHandle *parents, const size_t memb);
GLHCKAPI const kmVec3* glhckViewGetPosition(const glhckHandle handle);
GLHCKAPI void glhckViewPosition(const glhckHandle handle, const kmVec3 *position);
GLHCKAPI void glhckViewPositionf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckViewMove(const glhckHandle handle, const kmVec3 *move);
GLHCKAPI void glhckViewMovef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckViewGetRotation(const glhckHandle handle);
GLHCKAPI void glhckViewRotation(const glhckHandle handle, const kmVec3 *rotation);
GLHCKAPI void glhckViewRotationf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckViewRotate(const glhckHandle handle, const kmVec3 *rotate);
GLHCKAPI void glhckViewRotatef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckViewGetTarget(const glhckHandle handle);
GLHCKAPI void glhckViewTarget(const glhckHandle handle, const kmVec3 *target);
GLHCKAPI void glhckViewTargetf(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckViewGetScale(const glhckHandle handle);
GLHCKAPI void glhckViewScale(const glhckHandle handle, const kmVec3 *scale);
GLHCKAPI void glhckViewScalef(const glhckHandle handle, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmAABB* glhckViewGetOBBWithChildren(const glhckHandle handle);
GLHCKAPI const kmAABB* glhckViewGetOBB(const glhckHandle handle);
GLHCKAPI const kmAABB* glhckViewGetAABBWithChildren(const glhckHandle handle);
GLHCKAPI const kmAABB* glhckViewGetAABB(const glhckHandle handle);
GLHCKAPI const kmMat4* glhckViewGetMatrix(const glhckHandle handle);
GLHCKAPI void glhckViewUpdateFromGeometry(const glhckHandle handle, const glhckHandle geometry);

#endif /* __glhck_view_h__ */
