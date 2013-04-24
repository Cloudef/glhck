#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATOR

/* \brief interpolate beetwen two vector keys */
static inline void _glhckAnimatorInterpolateVectorKeys(kmVec3 *out, float time, float duration,
      glhckAnimationVectorKey *current, glhckAnimationVectorKey *next)
{
   float interp;
   float diffTime = next->time - current->time;
   assert(out);

   if (diffTime < 0.0f) diffTime += duration;
   if (diffTime > 0.0f) {
      interp = (time - current->time)/diffTime;
      out->x = current->vector.x + (next->vector.x - current->vector.x) * interp;
      out->y = current->vector.y + (next->vector.y - current->vector.y) * interp;
      out->z = current->vector.z + (next->vector.z - current->vector.z) * interp;
   } else {
      memcpy(out, &current->vector, sizeof(kmVec3));
   }
}

/* \brief interpolate beetwen two quartenions keys */
static inline void _glhckAnimatorInterpolateQuaternionKeys(kmQuaternion *out, float time, float duration,
      glhckAnimationQuaternionKey *current, glhckAnimationQuaternionKey *next)
{
   float interp;
   float diffTime = next->time - current->time;
   assert(out);

   if (diffTime < 0.0f) diffTime += duration;
   if (diffTime > 0.0f) {
      interp = (time - current->time)/diffTime;
      kmQuaternionSlerp(out, &current->quaternion, &next->quaternion, interp);
   } else {
      memcpy(out, &current->quaternion, sizeof(kmQuaternion));
   }
}

/* \brief update bone structure's transformed matrices */
static inline void _glhckAnimatorUpdateBones(glhckAnimator *object)
{
   glhckBone *bone, *parent;
   unsigned int n;
   assert(object);

   for (n = 0; n != object->numBones; ++n) {
      bone = parent = object->bones[n];
      memcpy(&bone->transformedMatrix, &bone->offsetMatrix, sizeof(kmMat4));
      for (; parent; parent = parent->parent)
         kmMat4Multiply(&bone->transformedMatrix, &bone->transformedMatrix, &parent->transformationMatrix);
   }
}

/* \brief lookup bone from animator */
static glhckBone* _glhckAnimatorLookupBone(glhckAnimator *object, const char *name)
{
   unsigned int i;
   for (i = 0; i != object->numBones; ++i) {
      if (!strcmp(object->bones[i]->name, name)) return object->bones[i];
   }
   return NULL;
}

/* \brief resetup animation after bone/animation change */
static void _glhckAnimatorSetupAnimation(glhckAnimator *object)
{
   unsigned int i;
   if (!object->bones || !object->animation) return;
   for (i = 0; i != object->animation->numNodes; ++i) {
      object->previousNodes[i].bone = _glhckAnimatorLookupBone(object, object->animation->nodes[i]->boneName);
   }
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
   CALL(3, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
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

   IFDO(_glhckFree, object->animation);
   object->animation = (animation?NULL:glhckAnimationRef(animation));
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

/* \brief update the skeletal animation to next tick */
GLHCKAPI void glhckAnimatorUpdate(glhckAnimator *object, float playTime)
{
   _glhckAnimatorState *lastNode;
   glhckBone *bone;
   glhckAnimation *animation;
   glhckAnimationNode *node;
   kmVec3 currentTranslation, currentScaling;
   kmQuaternion currentRotation;
   float ticksPerSecond, duration, time = 0.0f;
   unsigned int n, frame, nextFrame;
   CALL(3, "%p", object);
   assert(object);

   /* not enough memory(?) */
   if (!object->previousNodes) return;

   animation = object->animation;
   duration = animation->duration;
   ticksPerSecond = (animation->ticksPerSecond!=0.0f?animation->ticksPerSecond:25.0f);
   playTime *= ticksPerSecond;
   if (animation->duration > 0.0) time = fmod(playTime, animation->duration);

   memset(&currentTranslation, 0, sizeof(kmVec3));
   memset(&currentScaling, 0, sizeof(kmVec3));
   memset(&currentRotation, 0, sizeof(kmQuaternion));
   for (n = 0; n != animation->numNodes; ++n) {
      node = animation->nodes[n];
      lastNode = &object->previousNodes[n];

      /* we don't have bone for this */
      if (!(bone = lastNode->bone))
         continue;

      /* translate using translation keys */
      if (node->translations) {
         frame = (time >= object->lastTime?lastNode->translationFrame:0);
         for (frame = frame+1; frame != node->numTranslations; ++frame) {
            if (time < node->translations[frame].time)
               break;
         }

         nextFrame = (frame+1)%node->numTranslations;
         _glhckAnimatorInterpolateVectorKeys(&currentTranslation, time, duration,
               &node->translations[frame], &node->translations[nextFrame]);
         lastNode->translationFrame = frame;
      }

      /* scale using scaling keys */
      if (node->scalings) {
         frame = (time >= object->lastTime?lastNode->scalingFrame:0);
         for (frame = frame+1; frame != node->numScalings; ++frame) {
            if (time < node->scalings[frame].time)
               break;
         }

         nextFrame = (frame+1)%node->numScalings;
         _glhckAnimatorInterpolateVectorKeys(&currentScaling, time, duration,
               &node->scalings[frame], &node->scalings[nextFrame]);
         lastNode->scalingFrame = frame;
      }

      /* rotate using rotation keys */
      if (node->rotations) {
         frame = (time >= object->lastTime?lastNode->rotationFrame:0);
         for (frame = frame+1; frame != node->numRotations; ++frame) {
            if (time < node->rotations[frame].time)
               break;
         }

         nextFrame = (frame+1)%node->numRotations;
         _glhckAnimatorInterpolateQuaternionKeys(&currentRotation, time, duration,
               &node->rotations[frame], &node->rotations[nextFrame]);
         lastNode->rotationFrame = frame;
      }

      /* build transformation matrix */
      kmMat4 *m = &bone->transformationMatrix;
      kmMat4RotationQuaternion(m, &currentRotation);
      m->mat[0] *= currentScaling.x; m->mat[4] *= currentScaling.x; m->mat[8]  *= currentScaling.x;
      m->mat[1] *= currentScaling.y; m->mat[5] *= currentScaling.y; m->mat[9]  *= currentScaling.y;
      m->mat[2] *= currentScaling.z; m->mat[6] *= currentScaling.z; m->mat[10] *= currentScaling.z;
      m->mat[3]  = currentTranslation.x;
      m->mat[7]  = currentTranslation.y;
      m->mat[11] = currentTranslation.z;
   }

   /* store last time */
   object->lastTime = time;

   /* update bone transformations */
   _glhckAnimatorUpdateBones(object);
}

/* vim: set ts=8 sw=3 tw=0 :*/
