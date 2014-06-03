#include "internal.h"
#include <float.h> /* for FLT_MAX */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_COLLISION

/* XXX: Lots of stuff still incomplete here */

glhckObject* glhckCubeFromKazmathAABBExtent(const kmAABBExtent *aabb)
{
   glhckObject *o = glhckCubeNew(1.0);
   glhckObjectPosition(o, &aabb->point);
   glhckObjectScalef(o, aabb->extent.x, aabb->extent.y, aabb->extent.z);
   return o;
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

typedef struct _glhckCollisionHandle {
   /* pool indexes */
   chckPoolIndex id, selfId;

   /* world this handle belongs to */
   struct _glhckCollisionWorld *world;

   /* user data */
   void *userData;
} _glhckCollisionHandle;

typedef struct _glhckCollisionPrimitive {
   /* shape for primitive */
   struct _glhckCollisionShape shape;

   /* handle to public id */
   struct _glhckCollisionHandle *id;
} _glhckCollisionPrimitive;

typedef struct _glhckCollisionWorld {
   /* number of collision packets going on */
   unsigned int packets, rejected;

   /* collision primitives living under the world */
   chckPool *primitives;
   chckPool *ids;

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
   typedef const kmVec3* (*_positionFunc)(const void *a, kmVec3 *outPoint);
   typedef const kmVec3* (*_contactFunc)(const void *a, const kmVec3 *point, kmVec3 *outPoint);
   _positionFunc packetPosition = _glhckCollisionPositionFunctions[packetShape->type];
   _contactFunc packetContact = _glhckCollisionContactFunctions[packetShape->type];
   _contactFunc primitiveContact = _glhckCollisionContactFunctions[primitiveShape->type];
   assert(packetShape && primitiveShape && outContact);

   /* get contact points for packet and primitive */
   kmVec3 packetCenter, penetrativeContact, difference;
   packetPosition(packetShape->any, &packetCenter);
   primitiveContact(primitiveShape->any, &packetCenter, outContact);
   packetContact(packetShape->any, outContact, &penetrativeContact);

   /* figure out real contact normal from comparing the distances */
   kmScalar contactDist = kmVec3Length(kmVec3Subtract(&difference, outContact, &packetCenter));
   kmScalar penetrativeDist = kmVec3Length(kmVec3Subtract(&difference, &penetrativeContact, &packetCenter));
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
   if (packet->data->test && !packet->data->test(packet->data, primitive->id))
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
      outData.collider = primitive->id;
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
   kmVec3 inverseVelocity, nearestContact, beforePoint;
   packetPosition(packet->shape->any, &beforePoint);
   kmVec3Scale(&inverseVelocity, &packet->velocity, -1.0f);
   velocity(packet->shape->any, &inverseVelocity);

   /* run through primitives to see what we need to sweep against */
   float nearestSweepDistance = FLT_MAX;
   _glhckCollisionPrimitive *p, *nearestSweepPrimitive = NULL;
   for (chckPoolIndex iter = 0; (p = chckPoolIter(world->primitives, &iter));) {
      /* ask user if we should even bother testing */
      if (packet->data->test && !packet->data->test(packet->data, p->id))
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
   if (!nearestSweepPrimitive)
      return RETURN_FALSE;

   /* do we need to sweep? */
   if (packet->data->response && nearestSweepDistance > 1.0f) {
      kmVec3 pushVector;
      float velocityDist = kmVec3Length(&packet->velocity);
      kmVec3Scale(&pushVector, &packet->velocity, -nearestSweepDistance/velocityDist);

      /* send response */
      glhckCollisionOutData outData;
      outData.world = world;
      outData.collider = nearestSweepPrimitive->id;
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
   _glhckCollisionPacket packet;
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

      _glhckCollisionPrimitive *p;
      for (chckPoolIndex iter = 0; (p = chckPoolIter(world->primitives, &iter));)
         _glhckCollisionWorldTestPacketAgainstPrimitive(world, &packet, p);

      if (!data->response || !data->velocity)
         break;
   }

   if (world->packets > 0)
      --world->packets;

   if (world->packets == 0) {
      if (world->rejected)
         DEBUG(GLHCK_DBG_CRAP, "-!- RECURSION LOOP: %u (rejected)", world->rejected);
      world->rejected = 0;
   }

   return packet.collisions;
}

static void _glhckCollisionShapeFree(_glhckCollisionShape *shape)
{
   /* don't free references */
   if (shape->reference)
      return;

   switch (shape->type) {
      default:
         IFDO(_glhckFree, shape->any);
         break;
   }
}

static void _glhckCollisionPrimitiveFree(_glhckCollisionPrimitive *primitive)
{
   assert(primitive);
   chckPoolRemove(primitive->id->world->ids, primitive->id->selfId);
   _glhckFree(primitive->id);
   _glhckCollisionShapeFree(&primitive->shape);
}

static glhckCollisionHandle* _glhckCollisionWorldAddPrimitive(glhckCollisionWorld *world, _glhckCollisionType type, void *shape, void *userData)
{
   glhckCollisionHandle *id = NULL;
   _glhckCollisionPrimitive *primitive = NULL;
   assert(world && shape);

   if (!world->ids && !(world->ids = chckPoolNew(32, 1, sizeof(glhckCollisionHandle*))))
      goto fail;

   if (!world->primitives && !(world->primitives = chckPoolNew(32, 1, sizeof(_glhckCollisionPrimitive))))
      goto fail;

   if (!(id = _glhckCalloc(1, sizeof(glhckCollisionHandle))))
      goto fail;

   id->world = world;
   id->userData = userData;

   if (!chckPoolAdd(world->ids, &id, &id->selfId))
      goto fail;

   if (!(primitive = chckPoolAdd(world->primitives, NULL, &id->id)))
      goto fail;

   primitive->id = id;
   primitive->shape.type = type;
   primitive->shape.any = shape;
   return id;

fail:
   IFDO(_glhckCollisionPrimitiveFree, primitive);
   IFDO(_glhckFree, id);
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
   assert(object);

   if (object->primitives) {
      chckPoolIterCall(object->primitives, _glhckCollisionPrimitiveFree);
      chckPoolFree(object->primitives);
   }

   IFDO(chckPoolFree, object->ids);
   _glhckFree(object);
}

GLHCKAPI void* glhckCollisionWorldGetUserData(const glhckCollisionWorld *object)
{
   assert(object);
   return object->userData;
}

GLHCKAPI glhckCollisionHandle* glhckCollisionWorldAddEllipse(glhckCollisionWorld *object, const kmEllipse *ellipse, void *userData)
{
   assert(object && ellipse);

   kmEllipse *ellipseCopy = NULL;
   if (!(ellipseCopy = _glhckMalloc(sizeof(kmEllipse))))
      goto fail;

   memcpy(ellipseCopy, ellipse, sizeof(kmEllipse));

   glhckCollisionHandle *id;
   if (!(id = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_ELLIPSE, ellipseCopy, userData)))
      goto fail;

   return id;

fail:
   IFDO(_glhckFree, ellipseCopy);
   return NULL;
}

GLHCKAPI glhckCollisionHandle* glhckCollisionWorldAddAABBRef(glhckCollisionWorld *object, const kmAABB *aabb, void *userData)
{
   glhckCollisionHandle *id = NULL;
   assert(object && aabb);

   if (!(id = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABB, (kmAABB*)aabb, userData)))
      return NULL;

   _glhckCollisionPrimitive *primitive = chckPoolGet(id->world->primitives, id->id);
   primitive->shape.reference = 1;
   return id;
}

GLHCKAPI glhckCollisionHandle* glhckCollisionWorldAddAABB(glhckCollisionWorld *object, const kmAABB *aabb, void *userData)
{
   assert(object && aabb);

   kmAABB *aabbCopy = NULL;
   if (!(aabbCopy = _glhckMalloc(sizeof(kmAABB))))
      goto fail;

   memcpy(aabbCopy, aabb, sizeof(kmAABB));

   glhckCollisionHandle *id;
   if (!(id = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABB, aabbCopy, userData)))
      goto fail;

   return id;

fail:
   IFDO(_glhckFree, aabbCopy);
   return NULL;
}

GLHCKAPI glhckCollisionHandle* glhckCollisionWorldAddAABBExtent(glhckCollisionWorld *object, const kmAABBExtent *aabbe, void *userData)
{
   assert(object && aabbe);

   kmAABBExtent *aabbeCopy = NULL;
   if (!(aabbeCopy = _glhckMalloc(sizeof(kmAABBExtent))))
      goto fail;

   memcpy(aabbeCopy, aabbe, sizeof(kmAABBExtent));

   glhckCollisionHandle *id;
   if (!(id = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_AABBE, aabbeCopy, userData)))
      goto fail;

   return id;

fail:
   IFDO(_glhckFree, aabbeCopy);
   return NULL;
}

GLHCKAPI glhckCollisionHandle* glhckCollisionWorldAddSphere(glhckCollisionWorld *object, const kmSphere *sphere, void *userData)
{
   assert(object && sphere);

   kmSphere *sphereCopy = NULL;
   if (!(sphereCopy = _glhckMalloc(sizeof(kmSphere))))
      goto fail;

   memcpy(sphereCopy, sphere, sizeof(kmSphere));

   glhckCollisionHandle *id;
   if (!(id = _glhckCollisionWorldAddPrimitive(object, GLHCK_COLLISION_SPHERE, sphereCopy, userData)))
      goto fail;

   return id;

fail:
   IFDO(_glhckFree, sphereCopy);
   return NULL;
}

GLHCKAPI void glhckCollisionWorldRemovePrimitive(glhckCollisionWorld *object, const glhckCollisionHandle *id)
{
   assert(object && id);
   assert(id->world == object);
   _glhckCollisionPrimitive *primitive = chckPoolGet(object->primitives, id->id);
   _glhckCollisionPrimitiveFree(primitive);
   chckPoolRemove(object->primitives, id->id);
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

GLHCKAPI void* glhckCollisionHandleGetUserData(const glhckCollisionHandle *id)
{
   assert(id);
   return id->userData;
}

/* vim: set ts=8 sw=3 tw=0 :*/
