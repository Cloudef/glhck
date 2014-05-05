#include "../internal.h"
#include <float.h>  /* for float */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATOR

/* TODO: Eventually do interpolation using splines for even smoother animation
 * We can also save some keyframes in glhckm format by doing this. */

/* \brief interpolate beetwen two vector keys */
static void _glhckAnimatorInterpolateVectorKeys(kmVec3 *out, float time, float duration,
      const glhckAnimationVectorKey *current, const glhckAnimationVectorKey *next)
{
   assert(out);
   float interp = (time < next->time ? time/next->time : time/(duration+next->time));
   out->x = current->vector.x + (next->vector.x - current->vector.x) * interp;
   out->y = current->vector.y + (next->vector.y - current->vector.y) * interp;
   out->z = current->vector.z + (next->vector.z - current->vector.z) * interp;
}

/* \brief interpolate beetwen two quartenions keys */
static void _glhckAnimatorInterpolateQuaternionKeys(kmQuaternion *out, float time, float duration,
      const glhckAnimationQuaternionKey *current, const glhckAnimationQuaternionKey *next)
{
   assert(out);
   float interp = (time < next->time ? time/next->time : time/(duration+next->time));
   kmQuaternionSlerp(out, &current->quaternion, &next->quaternion, interp);
}

/* \brief lookup bone from animator */
static glhckBone* _glhckAnimatorLookupBone(glhckAnimator *object, const char *name)
{
   glhckBone *bone;
   for (chckArrayIndex iter = 0; (bone = chckArrayIter(object->bones, &iter));) {
      const char *bname = glhckBoneGetName(bone);
      if (bone && !strcmp(bname, name))
         return bone;
   }

   return NULL;
}

/* \brief resetup animation after bone/animation change */
static void _glhckAnimatorSetupAnimation(glhckAnimator *object)
{
   if (!object->bones || !object->animation)
      return;

   IFDO(chckIterPoolFree, object->previousNodes);
   object->previousNodes = chckIterPoolNew(32, chckArrayCount(object->animation->nodes), sizeof(_glhckAnimatorState));

   glhckAnimationNode *node;
   for (chckArrayIndex iter = 0; (node = chckArrayIter(object->animation->nodes, &iter));) {
      _glhckAnimatorState *pnode = chckIterPoolAdd(object->previousNodes, NULL, NULL);
      pnode->bone = _glhckAnimatorLookupBone(object, node->boneName);
   }

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
   _glhckWorldAdd(&GLHCKW()->animators, object);

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
   _glhckWorldRemove(&GLHCKW()->animators, object);

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
   object->animation = (animation ? glhckAnimationRef(animation) : NULL);
   IFDO(chckIterPoolFree, object->previousNodes);

   if (animation)
      _glhckAnimatorSetupAnimation(object);
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
   CALL(0, "%p, %p, %u", object, bones, memb);
   assert(object);

   /* free old bones */
   if (object->bones) {
      chckArrayIterCall(object->bones, glhckBoneFree);
      chckArrayFree(object->bones);
      object->bones = NULL;
   }

   if (bones && memb > 0) {
      if ((object->bones = chckArrayNewFromCArray(bones, memb, 32))) {
         chckArrayIterCall(object->bones, glhckBoneRef);
      } else {
         goto fail;
      }
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

   size_t nmemb;
   glhckBone **bones = chckArrayToCArray(object->bones, &nmemb);

   if (memb)
      *memb = nmemb;

   RET(2, "%p", object->bones);
   return bones;
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
   CALL(2, "%p", object);
   assert(object);

   /* not enough memory(?) */
   if (!object->previousNodes)
      return;

   if (!object->animation)
      return;

   if (!object->bones)
      return;

   glhckAnimation *animation = object->animation;
   float duration = animation->duration;
   if (duration <= 0.0)  return;

   float time = fmod(playTime, duration);
   if (time == object->lastTime)
      return;

   for (chckArrayIndex n = 0; n != chckArrayCount(animation->nodes); ++n) {
      glhckAnimationNode *node = chckArrayGet(animation->nodes, n);
      _glhckAnimatorState *lastNode = chckIterPoolGet(object->previousNodes, n);

      glhckBone *bone;
      if (!(bone = lastNode->bone))
         continue;

      kmQuaternion currentRotation;
      kmVec3 currentTranslation, currentScaling;
      memset(&currentTranslation, 0, sizeof(kmVec3));
      kmVec3Fill(&currentScaling, 1.0f, 1.0f, 1.0f);
      kmQuaternionIdentity(&currentRotation);

      /* translate using translation keys */
      if (node->translations) {
         unsigned int frame = (time >= object->lastTime ? lastNode->translationFrame : 0);
         glhckAnimationVectorKey *frameNode = chckIterPoolGet(node->translations, frame);
         for (; frame < chckIterPoolCount(node->translations) - 1 && time > (frameNode = chckIterPoolGet(node->translations, frame))->time; ++frame);

         unsigned int nextFrame = ((frame + 1) % chckIterPoolCount(node->translations));
         _glhckAnimatorInterpolateVectorKeys(&currentTranslation, time, duration, frameNode, chckIterPoolGet(node->translations, nextFrame));
         lastNode->translationFrame = frame;
      }

      /* scale using scaling keys */
      if (node->scalings) {
         unsigned int frame = (time >= object->lastTime ? lastNode->scalingFrame : 0);
         glhckAnimationVectorKey *frameNode = chckIterPoolGet(node->scalings, frame);
         for (; frame < chckIterPoolCount(node->scalings) - 1 && time > (frameNode = chckIterPoolGet(node->scalings, frame))->time; ++frame);

         unsigned int nextFrame = ((frame + 1) % chckIterPoolCount(node->scalings));
         _glhckAnimatorInterpolateVectorKeys(&currentScaling, time, duration, frameNode, chckIterPoolGet(node->scalings, nextFrame));
         lastNode->scalingFrame = frame;
      }

      /* rotate using rotation keys */
      if (node->rotations) {
         unsigned int frame = (time >= object->lastTime ? lastNode->rotationFrame : 0);
         glhckAnimationQuaternionKey *frameNode = chckIterPoolGet(node->rotations, frame);
         for (; frame < chckIterPoolCount(node->rotations) - 1 && time > (frameNode = chckIterPoolGet(node->rotations, frame))->time; ++frame);

         unsigned int nextFrame = ((frame + 1) % chckIterPoolCount(node->rotations));
         _glhckAnimatorInterpolateQuaternionKeys(&currentRotation, time, duration, frameNode, chckIterPoolGet(node->rotations, nextFrame));
         lastNode->rotationFrame = frame;
      }

      /* build transformation matrix */
      kmMat4 matrix, tmp;
      kmMat4Scaling(&matrix, currentScaling.x, currentScaling.y, currentScaling.z);
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
