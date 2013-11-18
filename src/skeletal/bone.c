#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_BONE

#define PERFORM_ON_CHILDS(parent, function, ...) \
   { unsigned int _cbc_;                                   \
   for (_cbc_ = 0; _cbc_ != parent->numChilds; ++_cbc_)    \
      function(parent->childs[_cbc_], ##__VA_ARGS__); }

/* \brief transform V3F object */
static void _glhckBoneTransformObjectV3F(glhckObject *object)
{
   unsigned int i, w;
   kmMat4 bias, biasinv;
   static glhckVector3f zero = {0,0,0};

   /* scale is always 1.0f, we can skip it */
   kmMat4Translation(&bias, object->geometry->bias.x, object->geometry->bias.y, object->geometry->bias.z);
   kmMat4Inverse(&biasinv, &bias);

   for (i = 0; i != (unsigned int)object->geometry->vertexCount; ++i) {
      memcpy(&object->geometry->vertices.v3f[i].vertex, &zero, sizeof(glhckVector3f));
      memcpy(&object->geometry->vertices.v3f[i].normal, &zero, sizeof(glhckVector3f));
   }

   for (i = 0; i != object->numBones; ++i) {
      kmMat3 transformedNormal;
      kmMat4 transformedVertex, transformedMatrix, offsetMatrix;
      glhckBone *bone = object->bones[i];

      kmMat4Multiply(&transformedMatrix, &biasinv, &bone->transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &bone->offsetMatrix, &bias);
      kmMat4Multiply(&transformedVertex, &transformedMatrix, &offsetMatrix);
      kmMat3AssignMat4(&transformedNormal, &transformedVertex);

      for (w = 0; w != bone->numWeights; ++w) {
         glhckVector3f bindVertex, bindNormal;
         glhckVertexWeight *weight = &bone->weights[w];

         memcpy(&bindVertex, &object->bindGeometry->vertices.v3f[weight->vertexIndex].vertex, sizeof(glhckVector3f));
         memcpy(&bindNormal, &object->bindGeometry->vertices.v3f[weight->vertexIndex].normal, sizeof(glhckVector3f));
         kmVec3MultiplyMat4((kmVec3*)&bindVertex, (kmVec3*)&bindVertex, &transformedVertex);
         kmVec3MultiplyMat3((kmVec3*)&bindNormal, (kmVec3*)&bindNormal, &transformedNormal);

         object->geometry->vertices.v3f[weight->vertexIndex].vertex.x += bindVertex.x * weight->weight;
         object->geometry->vertices.v3f[weight->vertexIndex].vertex.y += bindVertex.y * weight->weight;
         object->geometry->vertices.v3f[weight->vertexIndex].vertex.z += bindVertex.z * weight->weight;
         object->geometry->vertices.v3f[weight->vertexIndex].normal.x += bindNormal.x * weight->weight;
         object->geometry->vertices.v3f[weight->vertexIndex].normal.y += bindNormal.y * weight->weight;
         object->geometry->vertices.v3f[weight->vertexIndex].normal.z += bindNormal.z * weight->weight;
      }
   }
}

/* \brief transform V3FS object */
static void _glhckBoneTransformObjectV3FS(glhckObject *object)
{
   unsigned int i, w;
   kmMat4 bias, biasinv;
   static glhckVector3f zero3f = {0,0,0};
   static glhckVector3s zero3s = {0,0,0};

   /* scale is always 1.0f, we can skip it */
   kmMat4Translation(&bias, object->geometry->bias.x, object->geometry->bias.y, object->geometry->bias.z);
   kmMat4Inverse(&biasinv, &bias);

   for (i = 0; i != (unsigned int)object->geometry->vertexCount; ++i) {
      memcpy(&object->geometry->vertices.v3fs[i].vertex, &zero3f, sizeof(glhckVector3f));
      memcpy(&object->geometry->vertices.v3fs[i].normal, &zero3s, sizeof(glhckVector3s));
   }

   for (i = 0; i != object->numBones; ++i) {
      kmMat3 transformedNormal;
      kmMat4 transformedVertex, transformedMatrix, offsetMatrix;
      glhckBone *bone = object->bones[i];

      kmMat4Multiply(&transformedMatrix, &biasinv, &bone->transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &bone->offsetMatrix, &bias);
      kmMat4Multiply(&transformedVertex, &transformedMatrix, &offsetMatrix);
      kmMat3AssignMat4(&transformedNormal, &transformedVertex);

      for (w = 0; w != bone->numWeights; ++w) {
         glhckVector3f bindVertex, bindNormal;
         glhckVertexWeight *weight = &bone->weights[w];

         memcpy(&bindVertex, &object->bindGeometry->vertices.v3fs[weight->vertexIndex].vertex, sizeof(glhckVector3f));
         glhckSetV3(&bindNormal, &object->bindGeometry->vertices.v3fs[weight->vertexIndex].normal);
         kmVec3MultiplyMat4((kmVec3*)&bindVertex, (kmVec3*)&bindVertex, &transformedVertex);
         kmVec3MultiplyMat3((kmVec3*)&bindNormal, (kmVec3*)&bindNormal, &transformedNormal);

         object->geometry->vertices.v3fs[weight->vertexIndex].vertex.x += bindVertex.x * weight->weight;
         object->geometry->vertices.v3fs[weight->vertexIndex].vertex.y += bindVertex.y * weight->weight;
         object->geometry->vertices.v3fs[weight->vertexIndex].vertex.z += bindVertex.z * weight->weight;
         object->geometry->vertices.v3fs[weight->vertexIndex].normal.x += bindNormal.x * weight->weight;
         object->geometry->vertices.v3fs[weight->vertexIndex].normal.y += bindNormal.y * weight->weight;
         object->geometry->vertices.v3fs[weight->vertexIndex].normal.z += bindNormal.z * weight->weight;
      }
   }
}

/* \brief transform V3S object */
static void _glhckBoneTransformObjectV3S(glhckObject *object)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   static glhckVector3s zero = {0,0,0};

   kmMat4Translation(&bias, object->geometry->bias.x, object->geometry->bias.y, object->geometry->bias.z);
   kmMat4Scaling(&scale, object->geometry->scale.x, object->geometry->scale.y, object->geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i != object->numBones; ++i) {
      glhckBone *bone = object->bones[i];
      for (w = 0; w != bone->numWeights; ++w) {
         glhckVertexWeight *weight = &bone->weights[w];
         memcpy(&object->geometry->vertices.v3s[weight->vertexIndex].vertex, &zero, sizeof(glhckVector3s));
         memcpy(&object->geometry->vertices.v3s[weight->vertexIndex].normal, &zero, sizeof(glhckVector3s));
      }
   }

   for (i = 0; i != object->numBones; ++i) {
      kmMat3 transformedNormal;
      kmMat4 transformedVertex, transformedMatrix, offsetMatrix;
      glhckBone *bone = object->bones[i];

      kmMat4Multiply(&transformedMatrix, &biasinv, &bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &bone->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&transformedVertex, &transformedMatrix, &offsetMatrix);
      kmMat3AssignMat4(&transformedNormal, &transformedVertex);

      for (w = 0; w != bone->numWeights; ++w) {
         glhckVector3f bindVertex, bindNormal;
         glhckVertexWeight *weight = &bone->weights[w];

         glhckSetV3(&bindVertex, &object->bindGeometry->vertices.v3s[weight->vertexIndex].vertex);
         glhckSetV3(&bindNormal, &object->bindGeometry->vertices.v3s[weight->vertexIndex].normal);
         kmVec3MultiplyMat4((kmVec3*)&bindVertex, (kmVec3*)&bindVertex, &transformedVertex);
         kmVec3MultiplyMat3((kmVec3*)&bindNormal, (kmVec3*)&bindNormal, &transformedNormal);

         object->geometry->vertices.v3s[weight->vertexIndex].vertex.x += bindVertex.x * weight->weight;
         object->geometry->vertices.v3s[weight->vertexIndex].vertex.y += bindVertex.y * weight->weight;
         object->geometry->vertices.v3s[weight->vertexIndex].vertex.z += bindVertex.z * weight->weight;
         object->geometry->vertices.v3s[weight->vertexIndex].normal.x += bindNormal.x * weight->weight;
         object->geometry->vertices.v3s[weight->vertexIndex].normal.y += bindNormal.y * weight->weight;
         object->geometry->vertices.v3s[weight->vertexIndex].normal.z += bindNormal.z * weight->weight;
      }
   }
}

/* \brief transform V3B object */
static void _glhckBoneTransformObjectV3B(glhckObject *object)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   static glhckVector3b zero = {0,0,0};

   kmMat4Translation(&bias, object->geometry->bias.x, object->geometry->bias.y, object->geometry->bias.z);
   kmMat4Scaling(&scale, object->geometry->scale.x, object->geometry->scale.y, object->geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i != object->numBones; ++i) {
      glhckBone *bone = object->bones[i];
      for (w = 0; w != bone->numWeights; ++w) {
         glhckVertexWeight *weight = &bone->weights[w];
         memcpy(&object->geometry->vertices.v3b[weight->vertexIndex].vertex, &zero, sizeof(glhckVector3s));
         memcpy(&object->geometry->vertices.v3b[weight->vertexIndex].normal, &zero, sizeof(glhckVector3s));
      }
   }

   for (i = 0; i != object->numBones; ++i) {
      kmMat3 transformedNormal;
      kmMat4 transformedVertex, transformedMatrix, offsetMatrix;
      glhckBone *bone = object->bones[i];

      kmMat4Multiply(&transformedMatrix, &biasinv, &bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &bone->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&transformedVertex, &transformedMatrix, &offsetMatrix);
      kmMat3AssignMat4(&transformedNormal, &transformedVertex);

      for (w = 0; w != bone->numWeights; ++w) {
         glhckVector3f bindVertex, bindNormal;
         glhckVertexWeight *weight = &bone->weights[w];

         glhckSetV3(&bindVertex, &object->bindGeometry->vertices.v3b[weight->vertexIndex].vertex);
         glhckSetV3(&bindNormal, &object->bindGeometry->vertices.v3b[weight->vertexIndex].normal);
         kmVec3MultiplyMat4((kmVec3*)&bindVertex, (kmVec3*)&bindVertex, &transformedVertex);
         kmVec3MultiplyMat3((kmVec3*)&bindNormal, (kmVec3*)&bindNormal, &transformedNormal);

         object->geometry->vertices.v3b[weight->vertexIndex].vertex.x += bindVertex.x * weight->weight;
         object->geometry->vertices.v3b[weight->vertexIndex].vertex.y += bindVertex.y * weight->weight;
         object->geometry->vertices.v3b[weight->vertexIndex].vertex.z += bindVertex.z * weight->weight;
         object->geometry->vertices.v3b[weight->vertexIndex].normal.x += bindNormal.x * weight->weight;
         object->geometry->vertices.v3b[weight->vertexIndex].normal.y += bindNormal.y * weight->weight;
         object->geometry->vertices.v3b[weight->vertexIndex].normal.z += bindNormal.z * weight->weight;
      }
   }
}

/* \brief update bone structure's transformed matrices */
static void _glhckBoneUpdateBones(glhckBone **bones, unsigned int memb)
{
   glhckBone *parent;
   unsigned int n;
   assert(bones);

   /* 1. Transform bone to 0,0,0 using offset matrix so we can transform it locally
    * 2. Apply all transformations from parent bones
    * 3. We'll end up back to bone space with the transformed matrix */

   for (n = 0; n != memb; ++n) {
      memcpy(&bones[n]->transformedMatrix, &bones[n]->transformationMatrix, sizeof(kmMat4));
      for (parent = bones[n]->parent; parent; parent = parent->parent)
         kmMat4Multiply(&bones[n]->transformedMatrix, &parent->transformationMatrix, &bones[n]->transformedMatrix);
   }
}


/* \brief transform object with it's bones */
void _glhckBoneTransformObject(glhckObject *object, int updateBones)
{
   /* we are root, perform this transform on childs as well */
   if (object->flags & GLHCK_OBJECT_ROOT) {
      PERFORM_ON_CHILDS(object, _glhckBoneTransformObject, updateBones);
   }

   /* ah, we can't do this ;_; */
   if (!object->geometry || !object->bones) return;

   /* update bones, if requested */
   if (updateBones) _glhckBoneUpdateBones(object->bones, object->numBones);

   /* CPU transformation path.
    * Since glhck supports multiple vertex formats, this path is bit more expensive than usual.
    * GPU transformation should be cheaper. */

   /* NOTE: at the moment CPU transformation only works with floating point vertex types */

   if (!object->bindGeometry) object->bindGeometry = _glhckGeometryCopy(object->geometry);

   /* TODO: Fix internal vertextype system by using table.
    * 1. we avoid branching like this.
    * 2. code will be much cleaner.
    * 3. library user can register his own vertex types
    * 4. we can drop V3FS since it can be registered by user anyways (or introduce it later when glhck is famous)
    * 5. we probably gain a lot of performance since branching happens in quite tight loops.
    */
   if (object->geometry->vertexType == GLHCK_VERTEX_V3F) _glhckBoneTransformObjectV3F(object);
   if (object->geometry->vertexType == GLHCK_VERTEX_V3FS) _glhckBoneTransformObjectV3FS(object);
   if (object->geometry->vertexType == GLHCK_VERTEX_V3S) _glhckBoneTransformObjectV3S(object);
   if (object->geometry->vertexType == GLHCK_VERTEX_V3B) _glhckBoneTransformObjectV3B(object);

   /* update bounding box for object */
   glhckGeometryCalculateBB(object->geometry, &object->view.bounding);
   _glhckObjectUpdateBoxes(object);
}

/* \brief allocate new bone object */
GLHCKAPI glhckBone* glhckBoneNew(void)
{
   glhckBone *object;
   TRACE(0);

   /* allocate object */
   if (!(object = _glhckCalloc(1, sizeof(glhckBone))))
      goto fail;

   /* increase reference */
   object->refCounter++;

   /* identity matrices */
   kmMat4Identity(&object->offsetMatrix);
   kmMat4Identity(&object->transformedMatrix);
   kmMat4Identity(&object->transformationMatrix);

   /* insert to world */
   _glhckWorldInsert(bone, object, glhckBone*);

   RET(0, "%p", object);
   return object;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief reference bone object */
GLHCKAPI glhckBone* glhckBoneRef(glhckBone *object)
{
   CALL(2, "%p", object);

   /* increase ref counter */
   object->refCounter++;

   RET(2, "%p", object);
   return object;
}

/* \brief free bone object */
GLHCKAPI unsigned int glhckBoneFree(glhckBone *object)
{
   if (!glhckInitialized()) return 0;
   CALL(FREE_CALL_PRIO(object), "%p", object);
   assert(object);

   /* there is still references to this object alive */
   if (--object->refCounter != 0) goto success;

   /* free the name */
   IFDO(_glhckFree, object->name);

   /* unref parent bone */
   glhckBoneParentBone(object, NULL);

   /* free the weights */
   glhckBoneInsertWeights(object, NULL, 0);

   /* remove from world */
   _glhckWorldRemove(bone, object, glhckBone*);

   /* free */
   NULLDO(_glhckFree, object);

success:
   RET(FREE_RET_PRIO(object), "%u", object?object->refCounter:0);
   return object?object->refCounter:0;
}

/* \brief set bone name */
GLHCKAPI void glhckBoneName(glhckBone *object, const char *name)
{
   char *nameCopy = NULL;
   CALL(2, "%p, %s", object, name);

   if (name && !(nameCopy = _glhckStrdup(name)))
      return;

   IFDO(_glhckFree, object->name);
   object->name = nameCopy;
}

/* \brief get bone name */
GLHCKAPI const char* glhckBoneGetName(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%s", object->name);
   return object->name;
}

/* \brief set weights to bone */
GLHCKAPI int glhckBoneInsertWeights(glhckBone *object, const glhckVertexWeight *weights, unsigned int memb)
{
   glhckVertexWeight *weightsCopy = NULL;
   CALL(0, "%p, %p, %u", object, weights, memb);
   assert(object);

   /* copy weights, if they exist */
   if (weights && !(weightsCopy = _glhckCopy(weights, memb * sizeof(glhckVertexWeight))))
      goto fail;

   IFDO(_glhckFree, object->weights);
   object->weights = weightsCopy;
   object->numWeights = (weightsCopy?memb:0);
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

/* \brief get weights from bone */
GLHCKAPI const glhckVertexWeight* glhckBoneWeights(glhckBone *object, unsigned int *memb)
{
   CALL(2, "%p, %p", object, memb);
   if (memb) *memb = object->numWeights;
   RET(2, "%p", object->weights);
   return object->weights;
}

/* \brief set parent bone to this bone */
GLHCKAPI void glhckBoneParentBone(glhckBone *object, glhckBone *parentBone)
{
   CALL(0, "%p, %p", object, parentBone);
   assert(object);
   object->parent = parentBone;
}

/* \brief return parent bone index of this bone */
GLHCKAPI glhckBone* glhckBoneGetParentBone(glhckBone *object)
{
   CALL(2, "%p", object);
   assert(object);
   RET(2, "%p", object->parent);
   return object->parent;
}

/* \brief set offset matrix to bone */
GLHCKAPI void glhckBoneOffsetMatrix(glhckBone *object, const kmMat4 *offsetMatrix)
{
   CALL(2, "%p, %p", object, offsetMatrix);
   assert(object && offsetMatrix);
   memcpy(&object->offsetMatrix, offsetMatrix, sizeof(kmMat4));
}

/* \brief get offset matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetOffsetMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->offsetMatrix);
   return &object->offsetMatrix;
}

/* \brief set transformation matrix to bone */
GLHCKAPI void glhckBoneTransformationMatrix(glhckBone *object, const kmMat4 *transformationMatrix)
{
   CALL(2, "%p, %p", object, transformationMatrix);
   assert(object && transformationMatrix);
   memcpy(&object->transformationMatrix, transformationMatrix, sizeof(kmMat4));
}

/* \brief get transformation matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->transformationMatrix);
   return &object->transformationMatrix;
}

/* \brief get transformed matrix from bone */
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(glhckBone *object)
{
   CALL(2, "%p", object);
   RET(2, "%p", &object->transformedMatrix);
   return &object->transformedMatrix;
}

/* vim: set ts=8 sw=3 tw=0 :*/
