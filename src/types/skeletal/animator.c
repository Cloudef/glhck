#include <stdlib.h>
#include <float.h>

#include "system/tls.h"
#include "trace.h"
#include "handle.h"
#include "pool/pool.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATOR

struct state {
   glhckHandle bone;
   unsigned int translationFrame;
   unsigned int rotationFrame;
   unsigned int scalingFrame;
};

enum pool {
   $animation, // glhckHandle
   $states, // list handle (struct state)
   $bones, // list handle
   $lastTime, // kmScalar
   $dirty, // unsigned char
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(glhckHandle), // animation
   sizeof(glhckHandle), // states
   sizeof(glhckHandle), // bones
   sizeof(kmScalar), // lastTime
   sizeof(unsigned char), // dirty
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

#include "handlefun.h"

/* TODO: Eventually do interpolation using splines for even smoother animation
 * We can also save some keyframes in glhckm format by doing this. */

/* \brief interpolate beetwen two vector keys */
static void interpolateVectorKeys(kmVec3 *out, const kmScalar time, const kmScalar duration, const glhckAnimationVectorKey *current, const glhckAnimationVectorKey *next)
{
   assert(out);
   kmScalar interp = (time < next->time ? time/next->time : time/(duration+next->time));
   out->x = current->vector.x + (next->vector.x - current->vector.x) * interp;
   out->y = current->vector.y + (next->vector.y - current->vector.y) * interp;
   out->z = current->vector.z + (next->vector.z - current->vector.z) * interp;
}

/* \brief interpolate beetwen two quartenions keys */
static void interpolateQuaternionKeys(kmQuaternion *out, const kmScalar time, const kmScalar duration, const glhckAnimationQuaternionKey *current, const glhckAnimationQuaternionKey *next)
{
   assert(out);
   kmScalar interp = (time < next->time ? time/next->time : time/(duration+next->time));
   kmQuaternionSlerp(out, &current->quaternion, &next->quaternion, interp);
}

/* \brief lookup bone from animator */
static glhckHandle lookupBone(const glhckHandle handle, const char *name)
{
   size_t memb;
   const glhckHandle *bones = getList($bones, handle, &memb);

   for (size_t i = 0; i < memb; ++i) {
      const char *bname = glhckBoneGetName(bones[i]);
      if (bname && !strcmp(bname, name))
         return bones[i];
   }

   return 0;
}

/* \brief resetup animation after bone/animation change */
static void setupAnimation(const glhckHandle handle)
{
   const glhckHandle animation = glhckAnimatorGetAnimation(handle);
   const glhckHandle bones = *(glhckHandle*)get($bones, handle);

   if (!bones || !animation)
      return;

   glhckHandle states = *(glhckHandle*)get($states, handle);
   IFDO(glhckListFlush, states);

   size_t numNodes = 0;
   const glhckHandle *nodes = glhckAnimationNodes(animation, &numNodes);

   if (!nodes)
      return;

   if (!states) {
      if (!(states = glhckListNew(numNodes, sizeof(struct state))))
         return;

      set($states, handle, &states);
   }

   for (size_t i = 0; i < numNodes; ++i) {
      struct state *state = glhckListAdd(states, NULL);
      state->bone = lookupBone(handle, glhckAnimationNodeGetBoneName(nodes[i]));
   }

   set($lastTime, handle, (kmScalar[]){FLT_MAX});
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckAnimatorAnimation(handle, 0);
   glhckAnimatorInsertBones(handle, NULL, 0);
}

/* \brief allocate new key animator object */
GLHCKAPI glhckHandle glhckAnimatorNew(void)
{
   TRACE(0);
   const glhckHandle handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_ANIMATOR, pools, pool_sizes, POOL_LAST, destructor);
   RET(0, "%s", glhckHandleRepr(handle));
   return handle;
}

/* \brief set animation to animator */
GLHCKAPI void glhckAnimatorAnimation(const glhckHandle handle, const glhckHandle animation)
{
   if (setHandle($animation, handle, animation) && animation)
      setupAnimation(handle);
}

/* \brief return the current animation of animator */
GLHCKAPI glhckHandle glhckAnimatorGetAnimation(const glhckHandle handle)
{
   return *(glhckHandle*)get($animation, handle);
}

/* \brief set bones to animator */
GLHCKAPI int glhckAnimatorInsertBones(const glhckHandle handle, const glhckHandle *bones, const size_t memb)
{
   const int ret = setListHandles($bones, handle, bones, memb);

   if (ret)
      setupAnimation(handle);

   return ret;
}

/* \brief get bones assigned to this animator */
GLHCKAPI const glhckHandle* glhckAnimatorBones(const glhckHandle handle, size_t *outMemb)
{
   return getList($bones, handle, outMemb);
}

/* \brief transform object with the animation */
GLHCKAPI void glhckAnimatorTransform(const glhckHandle handle, const glhckHandle object)
{
   CALL(2, "%s, %s", glhckHandleRepr(handle), glhckHandleRepr(object));
   assert(handle > 0 && object > 0);

   unsigned char *dirty = (unsigned char*)get($dirty, handle);
   _glhckSkinBoneTransformObject(object, *dirty);

#if 0
   /* we don't have to anything! */
   if (gobject->transformedGeometryTime == object->lastTime)
      return;

   /* transform the object */
   _glhckSkinBoneTransformObject(gobject, object->dirty);

   /* store the time for transformation */
   gobject->transformedGeometryTime = object->lastTime;
#endif

   *dirty = 0;
}

/* \brief update the skeletal animation to next tick */
GLHCKAPI void glhckAnimatorUpdate(const glhckHandle handle, const float playTime)
{
   CALL(2, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   const glhckHandle animation = glhckAnimatorGetAnimation(handle);
   struct state *states = (struct state*)getList($states, handle, NULL);

   if (!states || !animation)
      return;

   const kmScalar duration = glhckAnimationGetDuration(animation);
   if (duration <= 0.0)
      return;

   const kmScalar time = fmod(playTime, duration);
   const kmScalar lastTime = *(kmScalar*)get($lastTime, handle);
   if (time == lastTime)
      return;

   size_t memb = 0;
   const glhckHandle *nodes = glhckAnimationNodes(animation, &memb);
   for (size_t n = 0; n < memb; ++n) {
      if (!states[n].bone)
         continue;

      kmQuaternion currentRotation;
      kmVec3 currentTranslation, currentScaling;
      memset(&currentTranslation, 0, sizeof(kmVec3));
      kmVec3Fill(&currentScaling, 1.0f, 1.0f, 1.0f);
      kmQuaternionIdentity(&currentRotation);

      size_t numTranslations = 0;
      const glhckAnimationVectorKey *translations = glhckAnimationNodeTranslations(nodes[n], &numTranslations);

      /* translate using translation keys */
      if (numTranslations > 0) {
         unsigned int frame = (time >= lastTime ? states[n].translationFrame : 0);
         for (; frame < numTranslations - 1 && time > translations[frame].time; ++frame);

         unsigned int nextFrame = (frame + 1) % numTranslations;
         interpolateVectorKeys(&currentTranslation, time, duration, &translations[frame], &translations[nextFrame]);
         states[n].translationFrame = frame;
      }

      size_t numScalings = 0;
      const glhckAnimationVectorKey *scalings = glhckAnimationNodeScalings(nodes[n], &numScalings);

      /* scale using scaling keys */
      if (numScalings > 0) {
         unsigned int frame = (time >= lastTime ? states[n].scalingFrame : 0);
         for (; frame < numScalings - 1 && time > scalings[frame].time; ++frame);

         unsigned int nextFrame = (frame + 1) % numScalings;
         interpolateVectorKeys(&currentScaling, time, duration, &scalings[frame], &scalings[nextFrame]);
         states[n].scalingFrame = frame;
      }

      size_t numRotations = 0;
      const glhckAnimationQuaternionKey *rotations = glhckAnimationNodeRotations(nodes[n], &numRotations);

      /* rotate using rotation keys */
      if (numRotations > 0) {
         unsigned int frame = (time >= lastTime ? states[n].rotationFrame : 0);
         for (; frame < numRotations - 1 && time > rotations[frame].time; ++frame);

         unsigned int nextFrame = (frame + 1) % numRotations;
         interpolateQuaternionKeys(&currentRotation, time, duration, &rotations[frame], &rotations[nextFrame]);
         states[n].rotationFrame = frame;
      }

      /* build transformation matrix */
      kmMat4 matrix, tmp;
      kmMat4Scaling(&matrix, currentScaling.x, currentScaling.y, currentScaling.z);
      kmMat4RotationQuaternion(&tmp, &currentRotation);
      kmMat4Multiply(&matrix, &tmp, &matrix);
      kmMat4Translation(&tmp, currentTranslation.x, currentTranslation.y, currentTranslation.z);
      kmMat4Multiply(&matrix, &tmp, &matrix);
      glhckBoneTransformationMatrix(states[n].bone, &matrix);
   }

   /* store last time and mark dirty */
   set($lastTime, handle, &time);
   set($dirty, handle, (unsigned char[]){1});
}

/* vim: set ts=8 sw=3 tw=0 :*/
