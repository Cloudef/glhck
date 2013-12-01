#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_ANIMATION

/* \brief allocate new key animation node object */
GLHCKAPI glhckAnimationNode* glhckAnimationNodeNew(void)
{
   glhckAnimationNode *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckAnimationNode))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(animationNode, object, glhckAnimationNode*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference key animation node object */
GLHCKAPI glhckAnimationNode* glhckAnimationNodeRef(glhckAnimationNode *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free key animation node object */
GLHCKAPI unsigned int glhckAnimationNodeFree(glhckAnimationNode *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free name */
   glhckAnimationNodeBoneName(object, NULL);

   /* free keys from node */
   glhckAnimationNodeInsertTranslations(object, NULL, 0);
   glhckAnimationNodeInsertRotations(object, NULL, 0);
   glhckAnimationNodeInsertScalings(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(animationNode, object, glhckAnimationNode*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set bone index to animation node */
GLHCKAPI void glhckAnimationNodeBoneName(glhckAnimationNode *object, const char *name)
{
   char *nameCopy = NULL;
   CALL(0, "%p, %s", object, name);
   assert(object);

   if (name && !(nameCopy = _glhckStrdup(name)))
      return;

   IFDO(_glhckFree, object->boneName);
   object->boneName = nameCopy;
}

/* \brief return bone index from animation node */
GLHCKAPI const char* glhckAnimationNodeGetBoneName(glhckAnimationNode *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%s", object->boneName);
   return object->boneName;
}

/* \brief set translations to key animation node */
GLHCKAPI int glhckAnimationNodeInsertTranslations(glhckAnimationNode *object, const glhckAnimationVectorKey *keys, unsigned int memb)
{
   glhckAnimationVectorKey *keysCopy = NULL;
   CALL(0, "%p, %p, %u", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb * sizeof(glhckAnimationVectorKey))))
      goto fail;

   IFDO(_glhckFree, object->translations);
   object->translations = keysCopy;
   object->numTranslations = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's translation keys */
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeTranslations(glhckAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numTranslations;
   RET(2, "%p", object->translations);
   return object->translations;
}

/* \brief set scalings to key animation node */
GLHCKAPI int glhckAnimationNodeInsertScalings(glhckAnimationNode *object, const glhckAnimationVectorKey *keys, unsigned int memb)
{
   glhckAnimationVectorKey *keysCopy = NULL;
   CALL(0, "%p, %p, %u", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb * sizeof(glhckAnimationVectorKey))))
      goto fail;

   IFDO(_glhckFree, object->scalings);
   object->scalings = keysCopy;
   object->numScalings = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's scaling keys */
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeScalings(glhckAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numScalings;
   RET(2, "%p", object->scalings);
   return object->scalings;
}

/* \brief set rotations to key animation node */
GLHCKAPI int glhckAnimationNodeInsertRotations(glhckAnimationNode *object, const glhckAnimationQuaternionKey *keys, unsigned int memb)
{
   glhckAnimationQuaternionKey *keysCopy = NULL;
   CALL(0, "%p, %p, %u", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb * sizeof(glhckAnimationQuaternionKey))))
      goto fail;

   IFDO(_glhckFree, object->rotations);
   object->rotations = keysCopy;
   object->numRotations = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's rotation keys */
GLHCKAPI const glhckAnimationQuaternionKey* glhckAnimationNodeRotations(glhckAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numRotations;
   RET(2, "%p", object->rotations);
   return object->rotations;
}

/* \brief allocate new key animation object */
GLHCKAPI glhckAnimation* glhckAnimationNew(void)
{
   glhckAnimation *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckAnimation))))
      goto fail;

   /* default ticks per second */
   object->ticksPerSecond = 1.0f;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(animation, object, glhckAnimation*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference key animation object */
GLHCKAPI glhckAnimation* glhckAnimationRef(glhckAnimation *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free key animation object */
GLHCKAPI unsigned int glhckAnimationFree(glhckAnimation *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free name */
   glhckAnimationName(object, NULL);

   /* free nodes */
   glhckAnimationInsertNodes(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(animation, object, glhckAnimation*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set name to key animation */
GLHCKAPI void glhckAnimationName(glhckAnimation *object, const char *name)
{
   char *nameCopy = NULL;
   CALL(0, "%p, %s", object, name);
   assert(object);

   if (name && !(nameCopy = _glhckStrdup(name)))
      return;

   IFDO(_glhckFree, object->name);
   object->name = nameCopy;
}

/* \brief get animation name */
GLHCKAPI const char* glhckAnimationGetName(glhckAnimation *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%s", object->name);
   return object->name;
}

/* \brief set animation duration */
GLHCKAPI void glhckAnimationDuration(glhckAnimation *object, float duration)
{
   CALL(2, "%p, %f", object, duration);
   assert(object);
   object->duration = duration;
}

/* \brief get animation duration */
GLHCKAPI float glhckAnimationGetDuration(glhckAnimation *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%f", object->duration);
   return object->duration;
}

/* \brief set animation ticks per second */
GLHCKAPI void glhckAnimationTicksPerSecond(glhckAnimation *object, float ticksPerSecond)
{
   CALL(2, "%p, %f", object, ticksPerSecond);
   assert(object);
   object->ticksPerSecond = ticksPerSecond;
}

/* \brief get animation ticks per second */
GLHCKAPI float glhckAnimationGetTicksPerSecond(glhckAnimation *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%f", object->ticksPerSecond);
   return object->ticksPerSecond;
}

/* \brief set nodes to animation */
GLHCKAPI int glhckAnimationInsertNodes(glhckAnimation *object, glhckAnimationNode **nodes, unsigned int memb)
{
   unsigned int i;
   glhckAnimationNode **nodesCopy = NULL;
   CALL(0, "%p, %p, %u", object, nodes, memb);
   assert(object);

   /* copy nodes, if they exist */
   if (nodes && !(nodesCopy = _glhckCopy(nodes, memb * sizeof(glhckAnimationNode*))))
      goto fail;

   /* free old nodes */
   if (object->nodes) {
      for (i = 0; i != object->numNodes; ++i)
         glhckAnimationNodeFree(object->nodes[i]);
      _glhckFree(object->nodes);
   }

   object->nodes = nodesCopy;
   object->numNodes = (nodesCopy?memb:0);

   /* reference new nodes */
   if (object->nodes) {
      for (i = 0; i != object->numNodes; ++i)
         glhckAnimationNodeRef(object->nodes[i]);
   }

   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's nodes */
GLHCKAPI glhckAnimationNode** glhckAnimationNodes(glhckAnimation *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numNodes;
   RET(2, "%p", object->nodes);
   return object->nodes;
}

/* vim: set ts=8 sw=3 tw=0 :*/
