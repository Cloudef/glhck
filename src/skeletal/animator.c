#include "../internal.h"
#include <float.h>  /* for float */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATOR

#define PERFORM_ON_CHILDS(animator, parent, function, ...) \
   { unsigned int _cbc_;                                   \
   for (_cbc_ = 0; _cbc_ != parent->numChilds; ++_cbc_)    \
      function(animator, parent->childs[_cbc_], ##__VA_ARGS__); }

/* \brief interpolate beetwen two vector keys */
static void _glhckAnimatorInterpolateVectorKeys(kmVec3 *out, float time, float duration,
      const glhckAnimationVectorKey *current, const glhckAnimationVectorKey *next)
{
   float interp;
   float diffTime = next->time - current->time;
   assert(out);

   /* TODO: fix interpolation */
   if (diffTime < 0.0f) diffTime += duration;
   if (0 && diffTime > 0.0f) {
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
      const glhckAnimationQuaternionKey *current, const glhckAnimationQuaternionKey *next)
{
   float interp;
   float diffTime = next->time - current->time;
   assert(out);

   /* TODO: fix interpolation */
   if (diffTime < 0.0f) diffTime += duration;
   if (0 && diffTime > 0.0f) {
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

   /* 1. Transform bone to 0,0,0 using offset matrix so we can transform it locally
    * 2. Apply all transformations from bones and their parent bones
    * 3. We'll end up back to bone space with the transformed matrix */

   for (n = 0; n != object->numBones; ++n) {
      bone = parent = object->bones[n];
      memcpy(&bone->transformedMatrix, &bone->offsetMatrix, sizeof(kmMat4));
      for (; parent; parent = parent->parent)
         kmMat4Multiply(&bone->transformedMatrix, &parent->transformationMatrix, &bone->transformedMatrix);
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
   glhckBone *bone;
   unsigned int i, w;
   glhckVertexWeight *weight;
   kmMat4 bias, scale, transformedVertex;
   kmMat3 transformedNormal;
   glhckVector3f zero, bindVertex, bindNormal, currentVertex, currentNormal;
   CALL(2, "%p, %p", object, gobject);
   assert(object && gobject);

   /* we are root, perform this transform on childs as well */
   if (gobject->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, gobject, glhckAnimatorTransform);
   }

   /* ah, we can't do this ;_; */
   if (!gobject->geometry || !gobject->bones) return;

   /* update bone transformations */
   if (object->dirty) {
      _glhckAnimatorUpdateBones(object);
      object->dirty = 0;
   }

   /* CPU transformation path.
    * Since glhck supports multiple vertex formats, this path is bit more expensive than usual.
    * GPU transformation should be cheaper. */

   /* NOTE: at the moment CPU transformation only works with floating point vertex types */

   memset(&zero, 0, sizeof(glhckVector3f));
   if (!gobject->bindGeometry) gobject->bindGeometry = _glhckGeometryCopy(gobject->geometry);
   for (i = 0; i != (unsigned int)gobject->geometry->vertexCount; ++i) {
      glhckGeometrySetVertexDataForIndex(gobject->geometry, i, &zero, &zero, NULL, NULL);
   }

   kmMat4Translation(&bias, gobject->geometry->bias.x, gobject->geometry->bias.y, gobject->geometry->bias.z);
   kmMat4Scaling(&scale, gobject->geometry->scale.x, gobject->geometry->scale.y, gobject->geometry->scale.z);
   for (i = 0; i != gobject->numBones; ++i) {
      bone = gobject->bones[i];
      kmMat4Multiply(&transformedVertex, &scale, &bone->transformedMatrix);
      kmMat4Multiply(&transformedVertex, &transformedVertex, &bias);
      kmMat3AssignMat4(&transformedNormal, &transformedVertex);
      for (w = 0; w != bone->numWeights; ++w) {
         weight = &bone->weights[w];
         glhckGeometryGetVertexDataForIndex(gobject->bindGeometry, weight->vertexIndex, &bindVertex,
               &bindNormal, NULL, NULL);
         glhckGeometryGetVertexDataForIndex(gobject->geometry, weight->vertexIndex, &currentVertex,
               &currentNormal, NULL, NULL);
         kmVec3MultiplyMat4((kmVec3*)&bindVertex, (kmVec3*)&bindVertex, &transformedVertex);
         kmVec3MultiplyMat3((kmVec3*)&bindNormal, (kmVec3*)&bindNormal, &transformedNormal);

         currentVertex.x += bindVertex.x * weight->weight;
         currentVertex.y += bindVertex.y * weight->weight;
         currentVertex.z += bindVertex.z * weight->weight;
         currentNormal.x += bindNormal.x * weight->weight;
         currentNormal.y += bindNormal.y * weight->weight;
         currentNormal.z += bindNormal.z * weight->weight;

         glhckGeometrySetVertexDataForIndex(gobject->geometry, weight->vertexIndex, &currentVertex,
               &currentNormal, NULL, NULL);
      }
   }
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
      memset(&currentScaling, 0, sizeof(kmVec3));
      kmQuaternionIdentity(&currentRotation);

      /* translate using translation keys */
      if (node->translations) {
         frame = (time >= object->lastTime?lastNode->translationFrame:0);
         for (; frame < node->numTranslations-1; ++frame)
            if (time < node->translations[frame].time) break;

         nextFrame = (frame+1)%node->numTranslations;
         _glhckAnimatorInterpolateVectorKeys(&currentTranslation, time, duration,
               &node->translations[frame], &node->translations[nextFrame]);
         lastNode->translationFrame = frame;
      }

      /* scale using scaling keys */
      if (node->scalings) {
         frame = (time >= object->lastTime?lastNode->scalingFrame:0);
         for (; frame < node->numScalings-1; ++frame)
            if (time < node->scalings[frame].time) break;

         nextFrame = (frame+1)%node->numScalings;
         _glhckAnimatorInterpolateVectorKeys(&currentScaling, time, duration,
               &node->scalings[frame], &node->scalings[nextFrame]);
         lastNode->scalingFrame = frame;
      }

      /* rotate using rotation keys */
      if (node->rotations) {
         frame = (time >= object->lastTime?lastNode->rotationFrame:0);
         for (; frame < node->numRotations-1; ++frame)
            if (time < node->rotations[frame].time) break;

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
