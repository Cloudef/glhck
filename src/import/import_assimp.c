#include "../internal.h"
#include "import.h"
#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <stdio.h>

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* \brief get file extension */
static const char* getExt(const char *file)
{
   const char *ext = strrchr(file, '.');
   if (!ext) return NULL;
   return ext + 1;
}

const struct aiNode* findNode(const struct aiNode *nd, const char *name)
{
   unsigned int i;
   const struct aiNode *child;
   for (i = 0; i != nd->mNumChildren; ++i) {
      if (!strcmp(nd->mChildren[i]->mName.data, name))
         return nd->mChildren[i];
      if ((child = findNode(nd->mChildren[i], name)))
         return child;
   }
   return NULL;
}

const struct aiBone* findBone(const struct aiMesh *mesh, const char *name)
{
   unsigned int i;
   for (i = 0; i != mesh->mNumBones; ++i)
      if (!strcmp(mesh->mBones[i]->mName.data, name)) return mesh->mBones[i];
   return NULL;
}

static glhckBone* processBones(const struct aiNode *boneNd, const struct aiNode *nd, const struct aiMesh *mesh,
      glhckBone **bones, unsigned int *numBones)
{
   glhckBone *bone, *child;
   const struct aiBone *assimpBone;
   kmMat4 offsetMatrix, transformationMatrix;
   unsigned int i;
   assert(boneNd && nd);
   assert(mesh);
   assert(numBones);

   /* too much bones */
   if (!numBones || *numBones+1>mesh->mNumBones)
      return NULL;

   /* create new bone */
   if (!(bone = glhckBoneNew()))
      return NULL;

   /* set bone to list */
   bones[*numBones] = bone;
   *numBones = *numBones+1;

   /* store bone name */
   glhckBoneName(bone, boneNd->mName.data);
   printf(":: BONE: %s\n", glhckBoneGetName(bone));

   /* skip this part by creating dummy bone, if this information is not found */
   assimpBone = findBone(mesh, boneNd->mName.data);
   if (assimpBone) {
      /* get offset matrix */
      offsetMatrix.mat[0]  = assimpBone->mOffsetMatrix.a1;
      offsetMatrix.mat[1]  = assimpBone->mOffsetMatrix.a2;
      offsetMatrix.mat[2]  = assimpBone->mOffsetMatrix.a3;
      offsetMatrix.mat[3]  = assimpBone->mOffsetMatrix.a4;

      offsetMatrix.mat[4]  = assimpBone->mOffsetMatrix.b1;
      offsetMatrix.mat[5]  = assimpBone->mOffsetMatrix.b2;
      offsetMatrix.mat[6]  = assimpBone->mOffsetMatrix.b3;
      offsetMatrix.mat[7]  = assimpBone->mOffsetMatrix.b4;

      offsetMatrix.mat[8]  = assimpBone->mOffsetMatrix.c1;
      offsetMatrix.mat[9]  = assimpBone->mOffsetMatrix.c2;
      offsetMatrix.mat[10] = assimpBone->mOffsetMatrix.c3;
      offsetMatrix.mat[11] = assimpBone->mOffsetMatrix.c4;

      offsetMatrix.mat[12] = assimpBone->mOffsetMatrix.d1;
      offsetMatrix.mat[13] = assimpBone->mOffsetMatrix.d2;
      offsetMatrix.mat[14] = assimpBone->mOffsetMatrix.d3;
      offsetMatrix.mat[15] = assimpBone->mOffsetMatrix.d4;

      /* get weights */
      glhckVertexWeight weights[assimpBone->mNumWeights];
      memset(weights, 0, sizeof(weights));
      for (i = 0; i != assimpBone->mNumWeights; ++i) {
         weights[i].vertexIndex = assimpBone->mWeights[i].mVertexId;
         weights[i].weight = assimpBone->mWeights[i].mWeight;
      }

      glhckBoneInsertWeights(bone, weights, assimpBone->mNumWeights);
      glhckBoneOffsetMatrix(bone, &offsetMatrix);
   }

   /* get transformation matrix */
   transformationMatrix.mat[0]  = boneNd->mTransformation.a1;
   transformationMatrix.mat[1]  = boneNd->mTransformation.a2;
   transformationMatrix.mat[2]  = boneNd->mTransformation.a3;
   transformationMatrix.mat[3]  = boneNd->mTransformation.a4;

   transformationMatrix.mat[4]  = boneNd->mTransformation.b1;
   transformationMatrix.mat[5]  = boneNd->mTransformation.b2;
   transformationMatrix.mat[6]  = boneNd->mTransformation.b3;
   transformationMatrix.mat[7]  = boneNd->mTransformation.b4;

   transformationMatrix.mat[8]  = boneNd->mTransformation.c1;
   transformationMatrix.mat[9]  = boneNd->mTransformation.c2;
   transformationMatrix.mat[10] = boneNd->mTransformation.c3;
   transformationMatrix.mat[11] = boneNd->mTransformation.c4;

   transformationMatrix.mat[12] = boneNd->mTransformation.d1;
   transformationMatrix.mat[13] = boneNd->mTransformation.d2;
   transformationMatrix.mat[14] = boneNd->mTransformation.d3;
   transformationMatrix.mat[15] = boneNd->mTransformation.d4;

   glhckBoneTransformationMatrix(bone, &transformationMatrix);

   /* process children bones */
   for (i = 0; i != boneNd->mNumChildren; ++i) {
      child = processBones(findNode(nd, boneNd->mChildren[i]->mName.data), nd, mesh, bones, numBones);
      if (child) glhckBoneAddChildren(bone, child);
   }
   return bone;
}

static int processAnimations(glhckObject *object, const struct aiScene *sc)
{
   unsigned int i, n, k, numAnimations, numNodes, numChildren;
   glhckAnimation *animation;
   glhckAnimationNode *node;
   glhckObject **children;
   const struct aiNodeAnim *assimpNode;
   const struct aiAnimation *assimpAnimation;

   glhckAnimation *animations[sc->mNumAnimations];
   for (i = 0, numAnimations = 0; i != sc->mNumAnimations; ++i) {
      assimpAnimation = sc->mAnimations[i];

      /* allocate new animation */
      if (!(animation = glhckAnimationNew()))
         continue;

      /* set animation properties */
      glhckAnimationTicksPerSecond(animation, assimpAnimation->mTicksPerSecond);
      glhckAnimationDuration(animation, assimpAnimation->mDuration);
      glhckAnimationName(animation, assimpAnimation->mName.data);
      printf(":: ANIMATION: %s\n", glhckAnimationGetName(animation));

      glhckAnimationNode *nodes[assimpAnimation->mNumChannels];
      for (n = 0, numNodes = 0; n != assimpAnimation->mNumChannels; ++n) {
         assimpNode = assimpAnimation->mChannels[n];

         /* allocate new animation node */
         if (!(node = glhckAnimationNodeNew()))
            continue;

         /* set bone name for this node */
         glhckAnimationNodeBoneName(node, assimpNode->mNodeName.data);

         /* translation keys */
         glhckAnimationVectorKey translations[assimpNode->mNumPositionKeys];
         for (k = 0; k != assimpNode->mNumPositionKeys; ++k) {
            translations[k].vector.x = assimpNode->mPositionKeys[k].mValue.x;
            translations[k].vector.y = assimpNode->mPositionKeys[k].mValue.y;
            translations[k].vector.z = assimpNode->mPositionKeys[k].mValue.z;
            translations[k].time = assimpNode->mPositionKeys[k].mTime;
         }
         glhckAnimationNodeInsertTranslations(node, translations, assimpNode->mNumPositionKeys);

         /* scaling keys */
         glhckAnimationVectorKey scalings[assimpNode->mNumScalingKeys];
         for (k = 0; k != assimpNode->mNumScalingKeys; ++k) {
            scalings[k].vector.x = assimpNode->mScalingKeys[k].mValue.x;
            scalings[k].vector.y = assimpNode->mScalingKeys[k].mValue.y;
            scalings[k].vector.z = assimpNode->mScalingKeys[k].mValue.z;
            scalings[k].time = assimpNode->mScalingKeys[k].mTime;
         }
         glhckAnimationNodeInsertScalings(node, scalings, assimpNode->mNumScalingKeys);

         /* rotation keys */
         glhckAnimationQuaternionKey rotations[assimpNode->mNumRotationKeys];
         for (k = 0; k != assimpNode->mNumRotationKeys; ++k) {
            rotations[k].quaternion.x = assimpNode->mRotationKeys[k].mValue.x;
            rotations[k].quaternion.y = assimpNode->mRotationKeys[k].mValue.y;
            rotations[k].quaternion.z = assimpNode->mRotationKeys[k].mValue.z;
            rotations[k].quaternion.w = assimpNode->mRotationKeys[k].mValue.w;
            rotations[k].time = assimpNode->mRotationKeys[k].mTime;
         }
         glhckAnimationNodeInsertRotations(node, rotations, assimpNode->mNumRotationKeys);

         /* increase imported nodes count */
         nodes[numNodes++] = node;
      }

      /* set nodes to animation */
      if (numNodes) glhckAnimationInsertNodes(animation, nodes, numNodes);

      /* increase imported animations count */
      animations[numAnimations++] = animation;
   }

   /* insert animations to object */
   if (numAnimations) {
      glhckObjectInsertAnimations(object, animations, numAnimations);
      children = glhckObjectChildren(object, &numChildren);
      for (i = 0; i != numChildren; ++i) {
         glhckObjectInsertAnimations(children[i], animations, numAnimations);
      }
   }
   return RETURN_OK;
}

static int buildModel(glhckObject *object, unsigned int numIndices, unsigned int numVertices,
      const glhckImportIndexData *indices, const glhckImportVertexData *vertexData,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype, unsigned int flags)
{
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   unsigned int numStrippedIndices = 0;
   glhckImportIndexData *stripIndices = NULL;
   (void)flags;

   if (!numVertices)
      return RETURN_OK;

   /* triangle strip geometry */
   if (!(stripIndices = _glhckTriStrip(indices, numIndices, &numStrippedIndices))) {
      /* failed, use non stripped geometry */
      geometryType       = GLHCK_TRIANGLES;
      numStrippedIndices = numIndices;
   }

   /* set geometry */
   glhckObjectInsertIndices(object, itype, (stripIndices?stripIndices:indices), numStrippedIndices);
   glhckObjectInsertVertices(object, vtype, vertexData, numVertices);
   object->geometry->type = geometryType;
   IFDO(_glhckFree, stripIndices);
   return RETURN_OK;
}

static void joinMesh(const struct aiMesh *mesh, unsigned int indexOffset,
      glhckImportIndexData *indices, glhckImportVertexData *vertexData,
      glhckAtlas *atlas, glhckTexture *texture)
{
   unsigned int f, i, skip;
   glhckImportIndexData index;
   const struct aiFace *face;
   assert(mesh);

   for (f = 0, skip = 0; f != mesh->mNumFaces; ++f) {
      face = &mesh->mFaces[f];
      if (!face) continue;

      for (i = 0; i != face->mNumIndices; ++i) {
         index = face->mIndices[i];

         if (mesh->mVertices) {
            vertexData[index].vertex.x = mesh->mVertices[index].x;
            vertexData[index].vertex.y = mesh->mVertices[index].y;
            vertexData[index].vertex.z = mesh->mVertices[index].z*-1;
         }

         if (mesh->mNormals) {
            vertexData[index].normal.x = mesh->mNormals[index].x;
            vertexData[index].normal.y = mesh->mNormals[index].y;
            vertexData[index].normal.z = mesh->mNormals[index].z*-1;
         }

         if (mesh->mTextureCoords[0]) {
            vertexData[index].coord.x = mesh->mTextureCoords[0][index].x;
            vertexData[index].coord.y = mesh->mTextureCoords[0][index].y;

            /* fix coords */
            if (vertexData[index].coord.x < 0.0f)
               vertexData[index].coord.x += 1;
            if (vertexData[index].coord.y < 0.0f)
               vertexData[index].coord.y += 1;
         }

         /* offset texture coords to fit atlas texture */
         if (atlas && texture) {
            kmVec2 coord;
            coord.x = vertexData[index].coord.x;
            coord.y = vertexData[index].coord.y;
            glhckAtlasTransformCoordinates(atlas, texture, &coord, &coord);
            vertexData[index].coord.x = coord.x;
            vertexData[index].coord.y = coord.y;
         }

         indices[skip+i] = indexOffset + index;
      } skip += face->mNumIndices;
   }
}

glhckTexture* textureFromMaterial(const char *file, const struct aiMaterial *mtl)
{
   glhckTexture *texture;
   struct aiString textureName;
   enum aiTextureMapping textureMapping;
   enum aiTextureOp op;
   enum aiTextureMapMode textureMapMode[3] = {0,0,0};
   unsigned int uvwIndex, flags;
   float blend;
   char *texturePath;
   glhckTextureParameters params;
   assert(file && mtl);

   if (!aiGetMaterialTextureCount(mtl, aiTextureType_DIFFUSE))
      return NULL;

   if (aiGetMaterialTexture(mtl, aiTextureType_DIFFUSE, 0,
            &textureName, &textureMapping, &uvwIndex, &blend, &op,
            textureMapMode, &flags) != AI_SUCCESS)
      return NULL;

   memcpy(&params, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   switch (textureMapMode[0]) {
      case aiTextureMapMode_Clamp:
      case aiTextureMapMode_Decal:
         params.wrapR = GLHCK_WRAP_CLAMP_TO_EDGE;
         params.wrapS = GLHCK_WRAP_CLAMP_TO_EDGE;
         params.wrapT = GLHCK_WRAP_CLAMP_TO_EDGE;
         break;
      case aiTextureMapMode_Mirror:
         params.wrapR = GLHCK_WRAP_MIRRORED_REPEAT;
         params.wrapS = GLHCK_WRAP_MIRRORED_REPEAT;
         params.wrapT = GLHCK_WRAP_MIRRORED_REPEAT;
      break;
      default:break;
   }

   if (!(texturePath = _glhckImportTexturePath(textureName.data, file)))
      return NULL;

   DEBUG(0, "%s", texturePath);
   texture = glhckTextureNew(texturePath, 0, &params);
   free(texturePath);
   return texture;
}

static int processModel(const char *file, glhckObject *object,
      glhckObject *current, const struct aiScene *sc, const struct aiNode *nd,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype, unsigned int flags)
{
   unsigned int m, f, b;
   unsigned int numVertices = 0, numIndices = 0, numBones = 0;
   unsigned int ioffset, voffset;
   glhckImportIndexData *indices = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckTexture **textureList = NULL, *texture;
   glhckAtlas *atlas = NULL;
   const struct aiMesh *mesh;
   const struct aiFace *face;
   int canFreeCurrent = 0;
   int hasTexture = 0;
   assert(file);
   assert(object && current);
   assert(sc && nd);

   /* combine && atlas loading path */
   if (flags & GLHCK_MODEL_JOIN) {
      /* prepare atlas for texture combining */
      if (!(atlas = glhckAtlasNew()))
         goto assimp_no_memory;

      /* texturelist for offseting coordinates */
      if (!(textureList = _glhckCalloc(nd->mNumMeshes, sizeof(_glhckTexture*))))
         goto assimp_no_memory;

      /* gather statistics */
      for (m = 0; m != nd->mNumMeshes; ++m) {
         mesh = sc->mMeshes[nd->mMeshes[m]];
         if (!mesh->mVertices) continue;

         for (f = 0; f != mesh->mNumFaces; ++f) {
            face = &mesh->mFaces[f];
            if (!face) goto fail;
            numIndices += face->mNumIndices;
         }
         numVertices += mesh->mNumVertices;

         if ((texture = textureFromMaterial(file, sc->mMaterials[mesh->mMaterialIndex]))) {
            glhckAtlasInsertTexture(atlas, texture);
            glhckTextureFree(texture);
            textureList[m] = texture;
            hasTexture = 1;
         }
      }

      /* allocate vertices */
      if (!(vertexData = _glhckCalloc(numVertices, sizeof(glhckImportVertexData))))
         goto assimp_no_memory;

      /* allocate indices */
      if (!(indices = _glhckMalloc(numIndices * sizeof(glhckImportIndexData))))
         goto assimp_no_memory;

      /* pack combined textures */
      if (hasTexture) {
         if (glhckAtlasPack(atlas, 1, 0) != RETURN_OK)
            goto fail;
      } else {
         NULLDO(glhckAtlasFree, atlas);
         NULLDO(_glhckFree, textureList);
      }

      /* join vertex data */
      for (m = 0, ioffset = 0, voffset = 0; m != nd->mNumMeshes; ++m) {
         mesh = sc->mMeshes[nd->mMeshes[m]];
         if (!mesh->mVertices) continue;
         if (textureList) texture = textureList[m];
         else texture = NULL;

         joinMesh(mesh, voffset, indices+ioffset, vertexData+voffset, atlas, texture);

         for (f = 0; f != mesh->mNumFaces; ++f) {
            face = &mesh->mFaces[f];
            if (!face) goto fail;
            ioffset += face->mNumIndices;
         }
         voffset += mesh->mNumVertices;
      }

      /* finally build the model */
      if (buildModel(current, numIndices,  numVertices,
               indices, vertexData, itype, vtype, flags) == RETURN_OK) {
         if (hasTexture) glhckObjectTexture(current, glhckAtlasGetTexture(atlas));
         if (!(current = glhckObjectNew())) goto fail;
         glhckObjectAddChildren(object, current);
         glhckObjectFree(current);
         canFreeCurrent = 1;
      }

      /* free stuff */
      IFDO(glhckAtlasFree, atlas);
      IFDO(_glhckFree, textureList);
      NULLDO(_glhckFree, vertexData);
      NULLDO(_glhckFree, indices);
   } else {
      /* default loading path */
      for (m = 0, ioffset = 0, voffset = 0; m != nd->mNumMeshes; ++m) {
         mesh = sc->mMeshes[nd->mMeshes[m]];
         if (!mesh->mVertices) continue;

         /* import bones */
         if (flags & GLHCK_MODEL_ANIMATED) {
            if (mesh->mNumBones) {
               glhckBone *bones[mesh->mNumBones]; numBones = 0;
               if (processBones(sc->mRootNode, sc->mRootNode, mesh, bones, &numBones)) {
                  /* set bones to current object */
                  if (numBones) glhckObjectInsertBones(current, bones, numBones);
               }
               for (b = 0; b != numBones; ++b) glhckBoneFree(bones[b]);
            }
         }

         /* gather statistics */
         numIndices = 0;
         for (f = 0; f != mesh->mNumFaces; ++f) {
            face = &mesh->mFaces[f];
            if (!face) goto fail;
            numIndices += face->mNumIndices;
         }
         numVertices = mesh->mNumVertices;

         /* get texture */
         hasTexture = 0;
         if ((texture = textureFromMaterial(file, sc->mMaterials[mesh->mMaterialIndex]))) {
            hasTexture = 1;
         }

         /* allocate vertices */
         if (!(vertexData = _glhckCalloc(numVertices, sizeof(glhckImportVertexData))))
            goto assimp_no_memory;

         /* allocate indices */
         if (!(indices = _glhckMalloc(numIndices * sizeof(glhckImportIndexData))))
            goto assimp_no_memory;

         /* fill arrays */
         joinMesh(mesh, 0, indices, vertexData, NULL, NULL);

         /* build model */
         if (buildModel(current, numIndices,  numVertices,
                  indices, vertexData, itype, vtype, flags) == RETURN_OK) {
            if (hasTexture) glhckObjectTexture(current, texture);
            if (!(current = glhckObjectNew())) goto fail;
            glhckObjectAddChildren(object, current);
            glhckObjectFree(current);
            canFreeCurrent = 1;
         }

         /* free stuff */
         NULLDO(_glhckFree, vertexData);
         NULLDO(_glhckFree, indices);
         IFDO(glhckTextureFree, texture);
      }
   }

   /* process childrens */
   for (m = 0; m != nd->mNumChildren; ++m) {
      if (processModel(file, object, current, sc, nd->mChildren[m],
               itype, vtype, flags) == RETURN_OK) {
         if (!(current = glhckObjectNew())) goto fail;
         glhckObjectAddChildren(object, current);
         glhckObjectFree(current);
         canFreeCurrent = 1;
      }
   }

   /* we din't do anything to the next
    * allocated object, so free it */
   if (canFreeCurrent) glhckObjectRemoveFromParent(current);
   return RETURN_OK;

assimp_no_memory:
   DEBUG(GLHCK_DBG_ERROR, "Assimp not enough memory.");
fail:
   IFDO(_glhckFree, vertexData);
   IFDO(_glhckFree, indices);
   IFDO(_glhckFree, textureList);
   IFDO(glhckAtlasFree, atlas);
   if (canFreeCurrent) glhckObjectRemoveFromParent(current);
   return RETURN_FAIL;
}

/* \brief check if file is a Assimp supported file */
int _glhckFormatAssimp(const char *file)
{
   int ret;
   CALL(0, "%s", file);
   ret = (aiIsExtensionSupported(getExt(file))?RETURN_OK:RETURN_FAIL);
   RET(0, "%d", ret);
   return ret;
}

/* \brief import Assimp file */
int _glhckImportAssimp(glhckObject *object, const char *file, unsigned int flags,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   const struct aiScene *scene;
   glhckObject *first = NULL;
   unsigned int aflags;
   CALL(0, "%p, %s, %d", object, file, flags);

   /* import the model using assimp
    * TODO: make import hints tunable?
    * Needs changes to import protocol! */
   aflags = aiProcessPreset_TargetRealtime_MaxQuality | aiProcess_OptimizeGraph;
   scene = aiImportFile(file, aflags);
   if (!scene) goto assimp_fail;

   /* mark ourself as special root object.
    * this makes most functions called on root object echo to children */
   object->flags |= GLHCK_OBJECT_ROOT;

   /* this is going to be the first object in mesh,
    * the object returned by this importer is just invisible root object. */
   if (!(first = glhckObjectNew())) goto fail;
   glhckObjectAddChildren(object, first);
   glhckObjectFree(first);

   /* process the model */
   if (processModel(file, object, first, scene, scene->mRootNode,
            itype, vtype, flags) != RETURN_OK)
      goto fail;

   /* process the animations */
   if (flags & GLHCK_MODEL_ANIMATED && scene->mNumAnimations) {
      if (processAnimations(object, scene) != RETURN_OK)
         goto fail;
   }

   /* close file */
   NULLDO(aiReleaseImport, scene);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

assimp_fail:
   DEBUG(GLHCK_DBG_ERROR, aiGetErrorString());
fail:
   IFDO(aiReleaseImport, scene);
   IFDO(glhckObjectFree, first);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
