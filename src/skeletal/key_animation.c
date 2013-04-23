#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_KEY_ANIMATION

/* \brief allocate new key animation node object */
GLHCKAPI glhckKeyAnimationNode* glhckKeyAnimationNodeNew(void)
{
   glhckKeyAnimationNode *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckKeyAnimationNode))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(keyAnimationNode, object, glhckKeyAnimationNode*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference key animation node object */
GLHCKAPI glhckKeyAnimationNode* glhckKeyAnimationNodeRef(glhckKeyAnimationNode *object)
{
   CALL(3, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free key animation node object */
GLHCKAPI unsigned int glhckKeyAnimationNodeFree(glhckKeyAnimationNode *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free keys from node */
   glhckKeyAnimationNodeInsertTranslations(object, NULL, 0);
   glhckKeyAnimationNodeInsertRotations(object, NULL, 0);
   glhckKeyAnimationNodeInsertScalings(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(keyAnimationNode, object, glhckKeyAnimationNode*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set bone index to animation node */
GLHCKAPI void glhckKeyAnimationNodeBoneIndex(glhckKeyAnimationNode *object, unsigned int boneIndex)
{
   CALL(0, "%p, %u", object, boneIndex);
   assert(object);
   object->boneIndex = boneIndex;
}

/* \brief return bone index from animation node */
GLHCKAPI unsigned int glhckKeyAnimationNodeGetBoneIndex(glhckKeyAnimationNode *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%d", object->boneIndex);
   return object->boneIndex;
}

/* \brief set translations to key animation node */
GLHCKAPI int glhckKeyAnimationNodeInsertTranslations(glhckKeyAnimationNode *object, const glhckKeyAnimationVector *keys, unsigned int memb)
{
   glhckKeyAnimationVector *keysCopy = NULL;
   CALL(0, "%p, %p, %zu", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb)))
      goto fail;

   IFDO(_glhckFree, object->translations);
   object->translations = keysCopy;
   object->numTranslations = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's translation keys */
GLHCKAPI const glhckKeyAnimationVector* glhckKeyAnimationNodeTranslations(glhckKeyAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numTranslations;
   RET(2, "%p", object->translations);
   return object->translations;
}

/* \brief set scalings to key animation node */
GLHCKAPI int glhckKeyAnimationNodeInsertScalings(glhckKeyAnimationNode *object, const glhckKeyAnimationVector *keys, unsigned int memb)
{
   glhckKeyAnimationVector *keysCopy = NULL;
   CALL(0, "%p, %p, %zu", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb)))
      goto fail;

   IFDO(_glhckFree, object->scalings);
   object->scalings = keysCopy;
   object->numScalings = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's scaling keys */
GLHCKAPI const glhckKeyAnimationVector* glhckKeyAnimationNodeScalings(glhckKeyAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numScalings;
   RET(2, "%p", object->scalings);
   return object->scalings;
}

/* \brief set rotations to key animation node */
GLHCKAPI int glhckKeyAnimationNodeInsertRotations(glhckKeyAnimationNode *object, const glhckKeyAnimationQuaternion *keys, unsigned int memb)
{
   glhckKeyAnimationQuaternion *keysCopy = NULL;
   CALL(0, "%p, %p, %zu", object, keys, memb);
   assert(object);

   /* copy keys, if they exist */
   if (keys && !(keysCopy = _glhckCopy(keys, memb)))
      goto fail;

   IFDO(_glhckFree, object->rotations);
   object->rotations = keysCopy;
   object->numRotations = (keysCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's rotation keys */
GLHCKAPI const glhckKeyAnimationQuaternion* glhckKeyAnimationNodeRotations(glhckKeyAnimationNode *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numRotations;
   RET(2, "%p", object->rotations);
   return object->rotations;
}

/* \brief allocate new key animation object */
GLHCKAPI glhckKeyAnimation* glhckKeyAnimationNew(void)
{
   glhckKeyAnimation *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckKeyAnimation))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* insert to world */
   _glhckWorldInsert(keyAnimation, object, glhckKeyAnimation*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference key animation object */
GLHCKAPI glhckKeyAnimation* glhckKeyAnimationRef(glhckKeyAnimation *object)
{
   CALL(3, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(3, "%p", object);
   return object;
}

/* \brief free key animation object */
GLHCKAPI unsigned int glhckKeyAnimationFree(glhckKeyAnimation *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free nodes */
   glhckKeyAnimationInsertNodes(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(keyAnimation, object, glhckKeyAnimation*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%d", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set nodes to animation */
GLHCKAPI int glhckKeyAnimationInsertNodes(glhckKeyAnimation *object, glhckKeyAnimationNode **nodes, unsigned int memb)
{
   unsigned int i;
   glhckKeyAnimationNode **nodesCopy = NULL;
   CALL(0, "%p, %p, %zu", object, nodes, memb);
   assert(object);

   /* copy nodes, if they exist */
   if (nodes && !(nodesCopy = _glhckCopy(nodes, memb)))
      goto fail;

   /* free old nodes */
   if (object->nodes) {
      for (i = 0; i != object->numNodes; ++i)
         glhckKeyAnimationNodeFree(object->nodes[i]);
      _glhckFree(object->nodes);
   }

   object->nodes = nodesCopy;
   object->numNodes = (nodesCopy?memb:0);

   /* reference new nodes */
   if (object->nodes) {
      for (i = 0; i != object->numNodes; ++i)
         glhckKeyAnimationNodeRef(object->nodes[i]);
   }

   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief return animation's nodes */
GLHCKAPI glhckKeyAnimationNode** glhckKeyAnimationNodes(glhckKeyAnimation *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numNodes;
   RET(2, "%p", object->nodes);
   return object->nodes;
}

/* vim: set ts=8 sw=3 tw=0 :*/
