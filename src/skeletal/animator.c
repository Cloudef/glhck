#include "../internal.h"
#include <float.h>  /* for float */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATOR

/* TODO: Eventually do interpolation using splines for even smoother animation
 * We can also safe some keyframes in glhckm format by doing this. */

/* \brief interpolate beetwen two vector keys */
static void _glhckAnimatorInterpolateVectorKeys(kmVec3 *out, float time, float duration,
      const glhckAnimationVectorKey *current, const glhckAnimationVectorKey *next)
{
   float interp;
   assert(out);
   interp = (time < next->time ? time/next->time : time/(duration+next->time));
   out->x = current->vector.x + (next->vector.x - current->vector.x) * interp;
   out->y = current->vector.y + (next->vector.y - current->vector.y) * interp;
   out->z = current->vector.z + (next->vector.z - current->vector.z) * interp;
}

/* \brief interpolate beetwen two quartenions keys */
static void _glhckAnimatorInterpolateQuaternionKeys(kmQuaternion *out, float time, float duration,
      const glhckAnimationQuaternionKey *current, const glhckAnimationQuaternionKey *next)
{
   float interp;
   assert(out);
   interp = (time < next->time ? time/next->time : time/(duration+next->time));
   kmQuaternionSlerp(out, &current->quaternion, &next->quaternion, interp);
}

/* \brief lookup bone from animator */
static glhckBone* _glhckAnimatorLookupBone(glhckAnimator *object, const char *name)
{
   unsigned int i;
   for (i = 0; i != object->numBones && strcmp(object->bones[i]->name, name); ++i);
   return (i<object->numBones?object->bones[i]:NULL);
}

/* \brief resetup animation after bone/animation change */
static void _glhckAnimatorSetupAnimation(glhckAnimator *object)
{
   unsigned int i;
   if (!object->bones || !object->animation) return;
   for (i = 0; i != object->animation->numNodes; ++i)
      object->previousNodes[i].bone = _glhckAnimatorLookupBone(object, object->animation->nodes[i]->boneName);
   object->lastTime = FLT_MAX;
}

/* \brief allocate new key animator object */
GLHCKAPI glhckAnimator* glhckAnimatorNew(void)
{
   glhckAnimator *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckAnimator))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(animator, object, glhckAnimator*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference key animator object */
GLHCKAPI glhckAnimator* glhckAnimatorRef(glhckAnimator *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free key animator object */
GLHCKAPI unsigned int glhckAnimatorFree(glhckAnimator *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* release animation */
   glhckAnimatorAnimation(object, NULL);

   /* release bones */
   glhckAnimatorInsertBones(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(animator, object, glhckAnimator*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set animation to animator */
GLHCKAPI void glhckAnimatorAnimation(glhckAnimator *object, glhckAnimation *animation)
{
   CALL(2, "%p, %p", object, animation);
   assert(object);

   IFDO(glhckAnimationFree, object->animation);
   object->animation = (animation?glhckAnimationRef(animation):NULL);
   IFDO(_glhckFree, object->previousNodes);

   if (animation) {
      object->previousNodes = _glhckCalloc(animation->numNodes, sizeof(_glhckAnimatorState));
      _glhckAnimatorSetupAnimation(object);
   }
}

/* \brief return the current animation of animator */
GLHCKAPI glhckAnimation* glhckAnimatorGetAnimation(glhckAnimator *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->animation);
   return object->animation;
}

/* \brief set bones to animator */
GLHCKAPI int glhckAnimatorInsertBones(glhckAnimator *object, glhckBone **bones, unsigned int memb)
{
   unsigned int i;
   glhckBone **bonesCopy = NULL;
   CALL(0, "%p, %p, %u", object, bones, memb);
   assert(object);

   /* copy bones, if they exist */
   if (bones && !(bonesCopy = _glhckCopy(bones, memb * sizeof(glhckBone*))))
      goto fail;

   /* free old bones */
   if (object->bones) {
      for (i = 0; i != object->numBones; ++i)
         glhckBoneFree(object->bones[i]);
      _glhckFree(object->bones);
   }

   object->bones = bonesCopy;
   object->numBones = (bonesCopy?memb:0);

   /* reference new bones */
   if (object->bones) {
      for (i = 0; i != object->numBones; ++i)
         glhckBoneRef(object->bones[i]);
   }

   /* resetup animation */
   _glhckAnimatorSetupAnimation(object);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief get bones assigned to this animator */
GLHCKAPI glhckBone** glhckAnimatorBones(glhckAnimator *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numBones;
   RET(2, "%p", object->bones);
   return object->bones;
}

/* \brief transform object with the animation */
GLHCKAPI void glhckAnimatorTransform(glhckAnimator *object, glhckObject *gobject)
{
   CALL(2, "%p, %p", object, gobject);
   assert(object && gobject);

   /* we don't have to anything! */
   if (gobject->transformedGeometryTime == object->lastTime)
      return;

   /* transform the object */
   _glhckSkinBoneTransformObject(gobject, object->dirty);

   /* store the time for transformation */
   gobject->transformedGeometryTime = object->lastTime;
   object->dirty = 0;
}

/* \brief update the skeletal animation to next tick */
GLHCKAPI void glhckAnimatorUpdate(glhckAnimator *object, float playTime)
{
   _glhckAnimatorState *lastNode;
   glhckBone *bone;
   glhckAnimation *animation;
   glhckAnimationNode *node;
   kmVec3 currentTranslation, currentScaling;
   kmQuaternion currentRotation;
   kmMat4 matrix, tmp;
   float ticksPerSecond, duration, time;
   unsigned int n, frame, nextFrame;
   CALL(2, "%p", object);
   assert(object);

   /* not enough memory(?) */
   if (!object->previousNodes) return;
   if (!object->animation) return;
   if (!object->bones) return;

   animation = object->animation;
   duration = animation->duration;
   if (duration <= 0.0)  return;

   ticksPerSecond = (animation->ticksPerSecond!=0.0f?animation->ticksPerSecond:25.0f);
   playTime *= ticksPerSecond;
   time = fmod(playTime, duration);
   if (time == object->lastTime) return;

   for (n = 0; n != animation->numNodes; ++n) {
      node = animation->nodes[n];
      lastNode = &object->previousNodes[n];

      /* we don't have bone for this */
      if (!(bone = lastNode->bone))
         continue;

      /* reset */
      memset(&currentTranslation, 0, sizeof(kmVec3));
      kmVec3Fill(&currentScaling, 1.0f, 1.0f, 1.0f);
      kmQuaternionIdentity(&currentRotation);

      /* translate using translation keys */
      if (node->translations) {
         frame = (time >= object->lastTime?lastNode->translationFrame:0);
         for (; frame < node->numTranslations-1 && time > node->translations[frame].time; ++frame);

         nextFrame = (frame+1)%node->numTranslations;
         _glhckAnimatorInterpolateVectorKeys(&currentTranslation, time, duration,
               &node->translations[frame], &node->translations[nextFrame]);
         lastNode->translationFrame = frame;
      }

      /* scale using scaling keys */
      if (node->scalings) {
         frame = (time >= object->lastTime?lastNode->scalingFrame:0);
         for (; frame < node->numScalings-1 && time > node->scalings[frame].time; ++frame);

         nextFrame = (frame+1)%node->numScalings;
         _glhckAnimatorInterpolateVectorKeys(&currentScaling, time, duration,
               &node->scalings[frame], &node->scalings[nextFrame]);
         lastNode->scalingFrame = frame;
      }

      /* rotate using rotation keys */
      if (node->rotations) {
         frame = (time >= object->lastTime?lastNode->rotationFrame:0);
         for (; frame < node->numRotations-1 && time > node->rotations[frame].time; ++frame);

         nextFrame = (frame+1)%node->numRotations;
         _glhckAnimatorInterpolateQuaternionKeys(&currentRotation, time, duration,
               &node->rotations[frame], &node->rotations[nextFrame]);
         lastNode->rotationFrame = frame;
      }

      /* build transformation matrix */
      kmMat4Identity(&matrix);
      kmMat4Scaling(&tmp, currentScaling.x, currentScaling.y, currentScaling.z);
      kmMat4Multiply(&matrix, &tmp, &matrix);
      kmMat4RotationQuaternion(&tmp, &currentRotation);
      kmMat4Multiply(&matrix, &tmp, &matrix);
      kmMat4Translation(&tmp, currentTranslation.x, currentTranslation.y, currentTranslation.z);
      kmMat4Multiply(&matrix, &tmp, &matrix);
      bone->transformationMatrix = matrix;
   }

   /* store last time and mark dirty */
   object->lastTime = time;
   object->dirty = 1;
}

/* vim: set ts=8 sw=3 tw=0 :*/
