#include <stdlib.h>
#include <assert.h>
#include "common.h"

int main(int argc, char **argv)
{
   (void)argc, (void)argv;

   assert(glhckInit(argc, argv));
   assert(glhckDisplayCreate(32, 32, getStubRenderer()));

   glhckHandle object;
   assert((object = glhckObjectNew()));

   glhckObjectParentAffection(object, 0);
   assert(glhckObjectGetParentAffection(object) == 0);

   glhckHandle material;
   assert(!glhckObjectGetMaterial(object));
   assert((material = glhckMaterialNew(0)));

   glhckObjectMaterial(object, material);
   assert(glhckObjectGetMaterial(object) == material);

   glhckObjectMaterial(object, 0);
   assert(!glhckObjectGetMaterial(object));

   glhckObjectMaterial(object, material);
   assert(glhckObjectGetMaterial(object) == material);

   assert(!glhckObjectBones(object, NULL));
   assert(!glhckObjectGetBone(object, "foobar"));

   assert(!glhckObjectSkinBones(object, NULL));

   assert(!glhckObjectAnimations(object, NULL));

   assert(!glhckObjectGetRoot(object));
   assert(!glhckObjectGetVertexColors(object));
   assert(glhckObjectGetCull(object));
   assert(glhckObjectGetDepth(object));
   assert(!glhckObjectGetDrawAABB(object));
   assert(!glhckObjectGetDrawOBB(object));
   assert(!glhckObjectGetDrawSkeleton(object));
   assert(!glhckObjectGetDrawWireframe(object));

   glhckObjectRoot(object, 1);
   assert(glhckObjectGetRoot(object));
   glhckObjectRoot(object, 0);
   assert(!glhckObjectGetRoot(object));

   glhckObjectVertexColors(object, 1);
   assert(glhckObjectGetVertexColors(object));
   glhckObjectVertexColors(object, 0);
   assert(!glhckObjectGetVertexColors(object));

   glhckObjectCull(object, 0);
   assert(!glhckObjectGetCull(object));

   glhckObjectDepth(object, 0);
   assert(!glhckObjectGetDepth(object));

   glhckObjectDrawAABB(object, 1);
   assert(glhckObjectGetDrawAABB(object));

   glhckObjectDrawOBB(object, 1);
   assert(glhckObjectGetDrawOBB(object));

   glhckObjectDrawSkeleton(object, 1);
   assert(glhckObjectGetDrawSkeleton(object));

   glhckObjectDrawWireframe(object, 1);
   assert(glhckObjectGetDrawWireframe(object));

   assert(glhckObjectGetOBB(object));
   assert(glhckObjectGetOBBWithChildren(object));
   assert(glhckObjectGetAABB(object));
   assert(glhckObjectGetAABBWithChildren(object));
   assert(glhckObjectGetMatrix(object));
   assert(glhckObjectGetPosition(object));
   assert(glhckObjectGetRotation(object));
   assert(glhckObjectGetTarget(object));
   assert(glhckObjectGetScale(object));

   glhckHandle geometry;
   assert(!glhckObjectGetGeometry(object));
   assert((geometry = glhckGeometryNew()));
   glhckObjectGeometry(object, geometry);
   assert(glhckObjectGetGeometry(object) == geometry);

   glhckObjectGeometry(object, 0);
   assert(!glhckObjectGetGeometry(object));

   glhckHandle child;
   assert(!glhckObjectChildren(object, NULL));
   assert((child = glhckObjectNew()));
   glhckObjectAddChild(object, child);
   assert(glhckObjectChildren(object, NULL)[0] == child);

   glhckObjectRemoveChild(object, child);
   assert(!glhckObjectChildren(object, NULL));

   glhckObjectAddChild(object, child);
   glhckObjectRemoveChildren(object);
   assert(!glhckObjectChildren(object, NULL));

   glhckObjectAddChild(object, child);
   glhckObjectRemoveFromParent(child);
   assert(!glhckObjectChildren(object, NULL));

   glhckObjectAddChild(object, child);
   assert(glhckHandleRelease(child) == 1);
   glhckObjectRemoveFromParent(child);

   glhckHandle copy;
   assert((copy = glhckObjectCopy(object)));
   assert(glhckObjectGetParentAffection(copy) == glhckObjectGetParentAffection(object));
   assert(glhckObjectGetMaterial(copy) == glhckObjectGetMaterial(object));
   assert(glhckObjectGetCull(copy) == glhckObjectGetCull(object));
   assert(glhckObjectGetDepth(copy) == glhckObjectGetDepth(object));

   glhckDisplayClose();
   glhckTerminate();

   return EXIT_SUCCESS;
}
