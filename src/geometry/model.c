#include <glhck/glhck.h>
#include <glhck/import.h>

#include <stdlib.h>

#include "trace.h"
#include "pool/pool.h"
#include "lut/lut.h"
#include "importers/postprocess.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new object from supported model files */
GLHCKAPI glhckHandle glhckModelNew(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams)
{
   glhckIndexType itype = GLHCK_IDX_AUTO;
   glhckVertexType vtype = GLHCK_VTX_AUTO;
   // glhckGetGlobalPrecision(&itype, &vtype);
   return glhckModelNewEx(file, size, postParams, itype, vtype);
}

/* \brief create new object from supported model files you can specify the index and vertex precision here */
GLHCKAPI glhckHandle glhckModelNewEx(const char *file, const kmScalar size, const glhckPostProcessModelParameters *postParams, const glhckIndexType itype, const glhckVertexType vtype)
{
   CALL(0, "%s, %f, %p, %u, %u", file, size, postParams, itype, vtype);

   glhckImportModelStruct *import = NULL;
   static chckHashTable *cache = NULL;
   if (!cache) {
      cache = chckHashTableNew(256);
   }

   import = chckHashTableStrGet(cache, file);

   glhckHandle handle = 0, current = 0;
   if (!import && !(import = glhckImportModelFile(file)))
      goto fail;

   chckHashTableStrSet(cache, file, import, 0);

   chckIterPool *materials = chckIterPoolNew(8, import->materialCount, sizeof(glhckHandle));
   for (size_t i = 0; i < import->materialCount; ++i) {
      const glhckHandle material = glhckMaterialNew(0);

      if (import->materials[i].diffuseTexture) {
         const glhckHandle texture = glhckTextureNewFromFile(findTexturePath(import->materials[i].diffuseTexture, file), NULL, NULL);
         if (texture)
            glhckMaterialTexture(material, texture);
      }

      chckIterPoolAdd(materials, &material, NULL);
   }

   chckIterPool *bones = chckIterPoolNew(8, import->bonesCount, sizeof(glhckHandle));
   for (size_t i = 0; i < import->bonesCount; ++i) {
      const glhckHandle bone = glhckBoneNew();

      glhckBoneName(bone, import->bones[i].name);
      glhckBoneTransformationMatrix(bone, &import->bones[i].transformationMatrix);

      chckIterPoolAdd(bones, &bone, NULL);
   }

   for (size_t i = 0; i < import->bonesCount; ++i) {
      const glhckHandle bone = *(glhckHandle*)chckIterPoolGet(bones, i);

      if (import->bones[i].parent != i) {
         glhckBoneParentBone(bone, *(glhckHandle*)chckIterPoolGet(bones, import->bones[i].parent));
      }
   }

   chckIterPool *skins = chckIterPoolNew(8, import->skinCount, sizeof(glhckHandle));
   for (size_t i = 0; i < import->skinCount; ++i) {
      const glhckHandle skin = glhckSkinBoneNew();

      const glhckHandle *bone;
      for (chckPoolIndex iter = 0; (bone = chckIterPoolIter(bones, &iter));) {
         if (!strcmp(glhckBoneGetName(*bone), import->skins[i].name)) {
            glhckSkinBoneBone(skin, *bone);
            glhckBoneOffsetMatrix(*bone, &import->skins[i].offsetMatrix);
            break;
         }
      }

      glhckSkinBoneOffsetMatrix(skin, &import->skins[i].offsetMatrix);
      glhckSkinBoneInsertWeights(skin, import->skins[i].weights, import->skins[i].weightCount);

      chckIterPoolAdd(skins, &skin, NULL);
   }

   chckIterPool *animations = chckIterPoolNew(8, import->animationCount, sizeof(glhckHandle));
   for (size_t i = 0; i < import->animationCount; ++i) {
      float longestDuration = 0.0f;
      chckIterPool *nodes = chckIterPoolNew(8, import->animations[i].nodeCount, sizeof(glhckHandle));
      for (size_t n = 0; n < import->animations[i].nodeCount; ++n) {
         const glhckHandle node = glhckAnimationNodeNew();

         glhckAnimationNodeBoneName(node, import->animations[i].nodes[n].name);

         if (import->animations[i].nodes[n].translationCount)
            glhckAnimationNodeInsertTranslations(node, import->animations[i].nodes[n].translationKeys, import->animations[i].nodes[n].translationCount);

         if (import->animations[i].nodes[n].scalingCount)
            glhckAnimationNodeInsertScalings(node, import->animations[i].nodes[n].scalingKeys, import->animations[i].nodes[n].scalingCount);

         if (import->animations[i].nodes[n].rotationCount)
            glhckAnimationNodeInsertRotations(node, import->animations[i].nodes[n].rotationKeys, import->animations[i].nodes[n].rotationCount);

         for (size_t k = 0; k < import->animations[i].nodes[n].translationCount; ++k)
            longestDuration = fmax(import->animations[i].nodes[n].rotationKeys[k].time, longestDuration);

         for (size_t k = 0; k < import->animations[i].nodes[n].scalingCount; ++k)
            longestDuration = fmax(import->animations[i].nodes[n].rotationKeys[k].time, longestDuration);

         for (size_t k = 0; k < import->animations[i].nodes[n].rotationCount; ++k)
            longestDuration = fmax(import->animations[i].nodes[n].rotationKeys[k].time, longestDuration);

         chckIterPoolAdd(nodes, &node, NULL);
      }

      const glhckHandle animation = glhckAnimationNew();

      glhckAnimationName(animation, import->animations[i].name);
      glhckAnimationInsertNodes(animation, chckIterPoolToCArray(nodes, NULL), import->animations[i].nodeCount);
      glhckAnimationDuration(animation, longestDuration);

      chckIterPoolAdd(animations, &animation, NULL);
   }

   for (size_t i = 0; i < import->meshCount; ++i) {
      if (!(current = glhckObjectNew()))
         goto fail;

      glhckObjectInsertVertices(current, vtype, import->meshes[i].vertexData, import->meshes[i].vertexCount);
      glhckObjectInsertIndices(current, itype, import->meshes[i].indexData, import->meshes[i].indexCount);

      const glhckHandle geometry = glhckObjectGetGeometry(current);
      glhckGeometry *data = glhckGeometryGetStruct(geometry);
      data->type = import->meshes[i].geometryType;

      if (import->meshes[i].skinCount > 0) {
         chckIterPool *cskins = chckIterPoolNew(8, import->meshes[i].skinCount, sizeof(glhckHandle));
         for (size_t s = 0; s < import->meshes[i].skinCount; ++s)
            chckIterPoolAdd(cskins, chckIterPoolGet(skins, import->meshes[i].skins[s]), NULL);

         glhckObjectInsertSkinBones(current, chckIterPoolToCArray(cskins, NULL), import->meshes[i].skinCount);
      }

      if (import->meshes[i].materialCount) {
         int mat = import->meshes[i].materials[0];
         glhckObjectMaterial(current, *(glhckHandle*)chckIterPoolGet(materials, mat));
      }

      glhckObjectScalef(current, size, size, size);

      if (!handle) {
         handle = current;
         current = 0;
      } else {
         glhckObjectAddChild(handle, current);
      }
   }

   if (import->bonesCount)
      glhckObjectInsertBones(handle, chckIterPoolToCArray(bones, NULL), import->bonesCount);

   if (import->animationCount)
      glhckObjectInsertAnimations(handle, chckIterPoolToCArray(animations, NULL), import->animationCount);

   chckIterPoolFree(materials);
   chckIterPoolFree(skins);
   chckIterPoolFree(bones);
   chckIterPoolFree(animations);

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   IFDO(glhckHandleRelease, handle);
   IFDO(glhckHandleRelease, current);
   IFDO(free, import);
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
