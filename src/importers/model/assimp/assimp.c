#include "importers/importer.h"

#include <assimp/cimport.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "trace.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#define ASSIMP_BONES_MAX 256

/* \brief get file extension */
static const char* getExt(const char *file)
{
   const char *ext = strrchr(file, '.');
   return (ext ? ext + 1 : NULL);
}

const struct aiNode* findNode(const struct aiNode *nd, const char *name)
{
   for (unsigned int i = 0; i != nd->mNumChildren; ++i) {
      if (!strcmp(nd->mChildren[i]->mName.data, name))
         return nd->mChildren[i];

      const struct aiNode *child;
      if ((child = findNode(nd->mChildren[i], name)))
         return child;
   }
   return NULL;
}

const struct aiBone* findBone(const struct aiMesh *mesh, const char *name)
{
   for (unsigned int i = 0; i != mesh->mNumBones; ++i) {
      if (!strcmp(mesh->mBones[i]->mName.data, name))
         return mesh->mBones[i];
   }
   return NULL;
}

glhckObject* findObject(glhckObject *object, const char *name)
{
   if (!object || !name)
      return NULL;

   if (object->file && !strcmp(object->file, name))
      return object;

   glhckObject *child;
   for (chckArrayIndex iter = 0; (child = chckArrayIter(object->childs, &iter));) {
      glhckObject *result;
      if ((result = findObject(child, name)))
         return result;
   }
   return NULL;
}

static glhckBone* processBones(const struct aiNode *boneNd, const struct aiNode *nd, const struct aiMesh *mesh, glhckBone **bones, glhckSkinBone **skinBones, unsigned int *numBones)
{
   glhckBone *bone = NULL;
   glhckSkinBone *skinBone = NULL;
   assert(boneNd && nd);
   assert(mesh);
   assert(numBones);

   /* something went wrong */
   if (!numBones || *numBones >= ASSIMP_BONES_MAX)
      goto fail;

   /* don't allow duplicates */
   for (unsigned int i = 0; i != *numBones; ++i) {
      if (!strcmp(glhckBoneGetName(bones[i]), boneNd->mName.data))
         goto fail;
   }

   /* create new bone */
   if (!(bone = glhckBoneNew()) || !(skinBone = glhckSkinBoneNew()))
      goto fail;

   /* set bone to list */
   bones[*numBones] = bone;
   skinBones[*numBones] = skinBone;
   *numBones = *numBones+1;

   /* store bone name */
   glhckBoneName(bone, boneNd->mName.data);
   glhckSkinBoneBone(skinBone, bone);

   /* skip this part by creating dummy bone, if this information is not found */
   kmMat4 matrix, transposed;
   const struct aiBone *assimpBone;
   if ((assimpBone = findBone(mesh, bone->name))) {
      /* get offset matrix */
      matrix.mat[0]  = assimpBone->mOffsetMatrix.a1;
      matrix.mat[1]  = assimpBone->mOffsetMatrix.a2;
      matrix.mat[2]  = assimpBone->mOffsetMatrix.a3;
      matrix.mat[3]  = assimpBone->mOffsetMatrix.a4;

      matrix.mat[4]  = assimpBone->mOffsetMatrix.b1;
      matrix.mat[5]  = assimpBone->mOffsetMatrix.b2;
      matrix.mat[6]  = assimpBone->mOffsetMatrix.b3;
      matrix.mat[7]  = assimpBone->mOffsetMatrix.b4;

      matrix.mat[8]  = assimpBone->mOffsetMatrix.c1;
      matrix.mat[9]  = assimpBone->mOffsetMatrix.c2;
      matrix.mat[10] = assimpBone->mOffsetMatrix.c3;
      matrix.mat[11] = assimpBone->mOffsetMatrix.c4;

      matrix.mat[12] = assimpBone->mOffsetMatrix.d1;
      matrix.mat[13] = assimpBone->mOffsetMatrix.d2;
      matrix.mat[14] = assimpBone->mOffsetMatrix.d3;
      matrix.mat[15] = assimpBone->mOffsetMatrix.d4;

      /* get weights */
      glhckVertexWeight weights[assimpBone->mNumWeights];
      memset(weights, 0, sizeof(weights));
      for (unsigned int i = 0; i != assimpBone->mNumWeights; ++i) {
         weights[i].vertexIndex = assimpBone->mWeights[i].mVertexId;
         weights[i].weight = assimpBone->mWeights[i].mWeight;
      }

      /* assimp is row-major, kazmath is column-major */
      kmMat4Transpose(&transposed, &matrix);
      glhckSkinBoneOffsetMatrix(skinBone, &transposed);
      glhckSkinBoneInsertWeights(skinBone, weights, assimpBone->mNumWeights);
   }

   /* get transformation matrix */
   matrix.mat[0]  = boneNd->mTransformation.a1;
   matrix.mat[1]  = boneNd->mTransformation.a2;
   matrix.mat[2]  = boneNd->mTransformation.a3;
   matrix.mat[3]  = boneNd->mTransformation.a4;

   matrix.mat[4]  = boneNd->mTransformation.b1;
   matrix.mat[5]  = boneNd->mTransformation.b2;
   matrix.mat[6]  = boneNd->mTransformation.b3;
   matrix.mat[7]  = boneNd->mTransformation.b4;

   matrix.mat[8]  = boneNd->mTransformation.c1;
   matrix.mat[9]  = boneNd->mTransformation.c2;
   matrix.mat[10] = boneNd->mTransformation.c3;
   matrix.mat[11] = boneNd->mTransformation.c4;

   matrix.mat[12] = boneNd->mTransformation.d1;
   matrix.mat[13] = boneNd->mTransformation.d2;
   matrix.mat[14] = boneNd->mTransformation.d3;
   matrix.mat[15] = boneNd->mTransformation.d4;

   /* assimp is row-major, kazmath is column-major */
   kmMat4Transpose(&transposed, &matrix);
   glhckBoneTransformationMatrix(bone, &transposed);

   /* process children bones */
   for (unsigned int i = 0; i != boneNd->mNumChildren; ++i) {
      glhckBone *child = processBones(findNode(nd, boneNd->mChildren[i]->mName.data), nd, mesh, bones, skinBones, numBones);
      if (child) glhckBoneParentBone(child, bone);
   }
   return bone;

fail:
   IFDO(glhckBoneFree, bone);
   IFDO(glhckSkinBoneFree, skinBone);
   return NULL;
}

static int processAnimations(glhckObject *object, const struct aiScene *sc)
{
   unsigned int numAnimations = 0;
   glhckAnimation *animations[sc->mNumAnimations];
   for (unsigned int i = 0; i < sc->mNumAnimations; ++i) {
      const struct aiAnimation *assimpAnimation = sc->mAnimations[i];

      /* allocate new animation */
      glhckAnimation *animation;
      if (!(animation = glhckAnimationNew()))
         continue;

      /* set animation properties */
      glhckAnimationTicksPerSecond(animation, assimpAnimation->mTicksPerSecond);
      glhckAnimationDuration(animation, assimpAnimation->mDuration);
      glhckAnimationName(animation, assimpAnimation->mName.data);

      unsigned int numNodes = 0;
      glhckAnimationNode *nodes[assimpAnimation->mNumChannels];
      for (unsigned int n = 0; n != assimpAnimation->mNumChannels; ++n) {
         const struct aiNodeAnim *assimpNode = assimpAnimation->mChannels[n];

         /* allocate new animation node */
         glhckAnimationNode *node;
         if (!(node = glhckAnimationNodeNew()))
            continue;

         /* set bone name for this node */
         glhckAnimationNodeBoneName(node, assimpNode->mNodeName.data);

         /* translation keys */
         glhckAnimationVectorKey *vectorKeys = _glhckCalloc(assimpNode->mNumPositionKeys, sizeof(glhckAnimationVectorKey));
         if (!vectorKeys) continue;

         for (unsigned int k = 0; k < assimpNode->mNumPositionKeys; ++k) {
            vectorKeys[k].vector.x = assimpNode->mPositionKeys[k].mValue.x;
            vectorKeys[k].vector.y = assimpNode->mPositionKeys[k].mValue.y;
            vectorKeys[k].vector.z = assimpNode->mPositionKeys[k].mValue.z;
            vectorKeys[k].time = assimpNode->mPositionKeys[k].mTime;
         }
         glhckAnimationNodeInsertTranslations(node, vectorKeys, assimpNode->mNumPositionKeys);
         _glhckFree(vectorKeys);

         /* scaling keys */
         vectorKeys = _glhckCalloc(assimpNode->mNumScalingKeys, sizeof(glhckAnimationVectorKey));
         if (!vectorKeys) continue;

         for (unsigned int k = 0; k < assimpNode->mNumScalingKeys; ++k) {
            vectorKeys[k].vector.x = assimpNode->mScalingKeys[k].mValue.x;
            vectorKeys[k].vector.y = assimpNode->mScalingKeys[k].mValue.y;
            vectorKeys[k].vector.z = assimpNode->mScalingKeys[k].mValue.z;
            vectorKeys[k].time = assimpNode->mScalingKeys[k].mTime;
         }
         glhckAnimationNodeInsertScalings(node, vectorKeys, assimpNode->mNumScalingKeys);
         _glhckFree(vectorKeys);

         /* rotation keys */
         glhckAnimationQuaternionKey *quaternionKeys = _glhckCalloc(assimpNode->mNumRotationKeys, sizeof(glhckAnimationQuaternionKey));
         if (!quaternionKeys) continue;

         for (unsigned int k = 0; k < assimpNode->mNumRotationKeys; ++k) {
            quaternionKeys[k].quaternion.x = assimpNode->mRotationKeys[k].mValue.x;
            quaternionKeys[k].quaternion.y = assimpNode->mRotationKeys[k].mValue.y;
            quaternionKeys[k].quaternion.z = assimpNode->mRotationKeys[k].mValue.z;
            quaternionKeys[k].quaternion.w = assimpNode->mRotationKeys[k].mValue.w;
            quaternionKeys[k].time = assimpNode->mRotationKeys[k].mTime;
         }
         glhckAnimationNodeInsertRotations(node, quaternionKeys, assimpNode->mNumRotationKeys);
         _glhckFree(quaternionKeys);

         /* increase imported nodes count */
         nodes[numNodes++] = node;
      }

      /* set nodes to animation */
      if (numNodes) {
         glhckAnimationInsertNodes(animation, nodes, numNodes);
         for (unsigned int n = 0; n != numNodes; ++n)
            glhckAnimationNodeFree(nodes[n]);
      }

      /* increase imported animations count */
      animations[numAnimations++] = animation;
   }

   /* insert animations to object */
   if (numAnimations) {
      glhckObjectInsertAnimations(object, animations, numAnimations);
      for (unsigned int i = 0; i != numAnimations; ++i)
         glhckAnimationFree(animations[i]);
   }
   return RETURN_OK;
}

static int processBonesAndAnimations(glhckObject *object, const struct aiScene *sc)
{
   const struct aiMesh *mesh;
   glhckBone **bones = NULL;
   glhckSkinBone **skinBones = NULL;

   /* import bones */
   if (!(bones = _glhckCalloc(ASSIMP_BONES_MAX, sizeof(glhckBone*))))
      goto fail;

   if (!(skinBones = _glhckCalloc(ASSIMP_BONES_MAX, sizeof(glhckSkinBone*))))
      goto fail;

   unsigned int numBones = 0;
   for (unsigned int i = 0; i != sc->mNumMeshes; ++i) {
      mesh = sc->mMeshes[i];

      /* FIXME: UGLY */
      char pointer[16];
      snprintf(pointer, sizeof(pointer), "%p", mesh);

      glhckObject *child;
      if (!(child = findObject(object, pointer)))
         continue;

      if (mesh->mNumBones)  {
         unsigned int oldNumBones = numBones;
         processBones(sc->mRootNode, sc->mRootNode, mesh, bones, skinBones, &numBones);

         if (numBones)
            glhckObjectInsertSkinBones(child, skinBones + oldNumBones, numBones - oldNumBones);

         for (unsigned int b = oldNumBones; b < numBones; ++b)
            glhckSkinBoneFree(skinBones[b]);
      }
   }

   /* we don't need skin bones anymore */
   NULLDO(_glhckFree, skinBones);

   /* store all bones in root object */
   if (numBones)
      glhckObjectInsertBones(object, bones, numBones);

   for (unsigned int b = 0; b < numBones; ++b)
      glhckBoneFree(bones[b]);

   NULLDO(_glhckFree, bones);

   /* import animations */
   if (sc->mNumAnimations && processAnimations(object, sc) != RETURN_OK)
      goto fail;

   return RETURN_OK;

fail:
   if (bones) {
      for (unsigned int i = 0; i < ASSIMP_BONES_MAX; ++i)
         if (bones[i]) glhckBoneFree(bones[i]);
      _glhckFree(bones);
   }
   if (skinBones) {
      for (unsigned int i = 0; i < ASSIMP_BONES_MAX; ++i)
         if (skinBones[i]) glhckSkinBoneFree(skinBones[i]);
      _glhckFree(skinBones);
   }
   return RETURN_FAIL;
}

static int buildModel(glhckObject *object, unsigned int numIndices, unsigned int numVertices,
      const glhckImportIndexData *indices, const glhckImportVertexData *vertexData,
      unsigned char itype, unsigned char vtype)
{
   if (!numVertices)
      return RETURN_OK;

   /* triangle strip geometry */
   unsigned int numStrippedIndices = 0;
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   glhckImportIndexData *stripIndices = NULL;
   if (!(stripIndices = _glhckTriStrip(indices, numIndices, &numStrippedIndices))) {
      /* failed, use non stripped geometry */
      geometryType = GLHCK_TRIANGLES;
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
   assert(mesh);

   for (unsigned int f = 0, skip = 0; f != mesh->mNumFaces; ++f) {
      const struct aiFace *face = &mesh->mFaces[f];
      if (!face) continue;

      for (unsigned int i = 0; i != face->mNumIndices; ++i) {
         glhckImportIndexData index = face->mIndices[i];

         if (mesh->mVertices) {
            vertexData[index].vertex.x = mesh->mVertices[index].x;
            vertexData[index].vertex.y = mesh->mVertices[index].y;
            vertexData[index].vertex.z = mesh->mVertices[index].z;
         }

         if (mesh->mNormals) {
            vertexData[index].normal.x = mesh->mNormals[index].x;
            vertexData[index].normal.y = mesh->mNormals[index].y;
            vertexData[index].normal.z = mesh->mNormals[index].z;
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
      }

      skip += face->mNumIndices;
   }
}

glhckTexture* textureFromMaterial(const char *file, const struct aiMaterial *mtl)
{
   assert(file && mtl);

   if (!aiGetMaterialTextureCount(mtl, aiTextureType_DIFFUSE))
      return NULL;

   float blend;
   unsigned int uvwIndex, flags;
   struct aiString textureName;
   enum aiTextureOp op;
   enum aiTextureMapping textureMapping;
   enum aiTextureMapMode textureMapMode[3] = {0,0,0};
   if (aiGetMaterialTexture(mtl, aiTextureType_DIFFUSE, 0,
            &textureName, &textureMapping, &uvwIndex, &blend, &op,
            textureMapMode, &flags) != AI_SUCCESS)
      return NULL;

   glhckTextureParameters params;
   memcpy(&params, glhckTextureDefaultParameters(), sizeof(glhckTextureParameters));
   switch (textureMapMode[0]) {
      case aiTextureMapMode_Clamp:
      case aiTextureMapMode_Decal:
         params.wrapR = GLHCK_CLAMP_TO_EDGE;
         params.wrapS = GLHCK_CLAMP_TO_EDGE;
         params.wrapT = GLHCK_CLAMP_TO_EDGE;
         break;
      case aiTextureMapMode_Mirror:
         params.wrapR = GLHCK_MIRRORED_REPEAT;
         params.wrapS = GLHCK_MIRRORED_REPEAT;
         params.wrapT = GLHCK_MIRRORED_REPEAT;
      break;
      default:break;
   }

   char *texturePath;
   if (!(texturePath = _glhckImportTexturePath(textureName.data, file)))
      return NULL;

   DEBUG(0, "%s", texturePath);
   glhckTexture *texture = glhckTextureNewFromFile(texturePath, NULL, &params);
   _glhckFree(texturePath);
   return texture;
}

static int processModel(const char *file, glhckObject *object,
      glhckObject *current, const struct aiScene *sc, const struct aiNode *nd,
      unsigned char itype, unsigned char vtype, const glhckImportModelParameters *params)
{
   unsigned int numVertices = 0, numIndices = 0;
   glhckImportIndexData *indices = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckMaterial *material = NULL;
   glhckTexture **textureList = NULL, *texture = NULL;
   glhckAtlas *atlas = NULL;
   int canFreeCurrent = 0;
   int hasTexture = 0;
   assert(file);
   assert(object && current);
   assert(sc && nd);

   /* combine && atlas loading path */
   if (params->flatten) {
      /* prepare atlas for texture combining */
      if (!(atlas = glhckAtlasNew()))
         goto assimp_no_memory;

      /* texturelist for offseting coordinates */
      if (!(textureList = _glhckCalloc(nd->mNumMeshes, sizeof(_glhckTexture*))))
         goto assimp_no_memory;

      /* gather statistics */
      for (unsigned int m = 0; m != nd->mNumMeshes; ++m) {
         const struct aiMesh *mesh = sc->mMeshes[nd->mMeshes[m]];
         if (!mesh->mVertices) continue;

         for (unsigned int f = 0; f != mesh->mNumFaces; ++f) {
            const struct aiFace *face = &mesh->mFaces[f];
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
         if (glhckAtlasPack(atlas, GLHCK_RGBA, 1, 0, glhckTextureDefaultParameters()) != RETURN_OK)
            goto fail;
      } else {
         NULLDO(glhckAtlasFree, atlas);
         NULLDO(_glhckFree, textureList);
      }

      /* join vertex data */
      for (unsigned int m = 0, ioffset = 0, voffset = 0; m != nd->mNumMeshes; ++m) {
         const struct aiMesh *mesh = sc->mMeshes[nd->mMeshes[m]];

         if (!mesh->mVertices)
            continue;

         if (textureList) {
            texture = textureList[m];
         } else {
            texture = NULL;
         }

         joinMesh(mesh, voffset, indices + ioffset, vertexData + voffset, atlas, texture);

         for (unsigned int f = 0; f != mesh->mNumFaces; ++f) {
            const struct aiFace *face = &mesh->mFaces[f];
            if (!face)
               goto fail;

            ioffset += face->mNumIndices;
         }

         voffset += mesh->mNumVertices;
      }

      /* create material */
      if (hasTexture && !(material = glhckMaterialNew(texture)))
         goto assimp_no_memory;

      /* finally build the model */
      if (buildModel(current, numIndices,  numVertices,
               indices, vertexData, itype, vtype)  == RETURN_OK) {
         _glhckObjectFile(current, nd->mName.data);

         if (material)
            glhckObjectMaterial(current, material);

         if (!(current = glhckObjectNew()))
            goto fail;

         glhckObjectAddChild(object, current);
         glhckObjectFree(current);
         canFreeCurrent = 1;
      }

      /* free stuff */
      IFDO(glhckAtlasFree, atlas);
      IFDO(glhckMaterialFree, material);
      IFDO(_glhckFree, textureList);
      NULLDO(_glhckFree, vertexData);
      NULLDO(_glhckFree, indices);
   } else {
      /* default loading path */
      for (unsigned int m = 0; m != nd->mNumMeshes; ++m) {
         const struct aiMesh *mesh = sc->mMeshes[nd->mMeshes[m]];
         if (!mesh->mVertices)
            continue;

         /* gather statistics */
         numIndices = 0;
         for (unsigned int f = 0; f != mesh->mNumFaces; ++f) {
            const struct aiFace *face = &mesh->mFaces[f];
            if (!face)
               goto fail;

            numIndices += face->mNumIndices;
         }
         numVertices = mesh->mNumVertices;

         // FIXME: create materialFromAssimpMaterial
         // that returns glhckMaterial with correct stuff

         /* get texture */
         hasTexture = 0;
         if ((texture = textureFromMaterial(file, sc->mMaterials[mesh->mMaterialIndex])))
            hasTexture = 1;

         /* create material */
         if (hasTexture && !(material = glhckMaterialNew(texture)))
            goto assimp_no_memory;

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
                  indices, vertexData, itype, vtype) == RETURN_OK) {

            /* FIXME: UGLY */
            char pointer[16];
            snprintf(pointer, sizeof(pointer), "%p", mesh);
            _glhckObjectFile(current, pointer);

            if (material)
               glhckObjectMaterial(current, material);

            if (!(current = glhckObjectNew()))
               goto fail;

            glhckObjectAddChild(object, current);
            glhckObjectFree(current);
            canFreeCurrent = 1;
         }

         /* free stuff */
         NULLDO(_glhckFree, vertexData);
         NULLDO(_glhckFree, indices);
         IFDO(glhckTextureFree, texture);
         IFDO(glhckMaterialFree, material);
      }
   }

   /* process childrens */
   for (unsigned int m = 0; m != nd->mNumChildren; ++m) {
      if (processModel(file, object, current, sc, nd->mChildren[m], itype, vtype, params) == RETURN_OK) {
         if (!(current = glhckObjectNew()))
            goto fail;

         glhckObjectAddChild(object, current);
         glhckObjectFree(current);
         canFreeCurrent = 1;
      }
   }

   /* we din't do anything to the next
    * allocated object, so free it */
   if (canFreeCurrent)
      glhckObjectRemoveFromParent(current);

   return RETURN_OK;

assimp_no_memory:
   DEBUG(GLHCK_DBG_ERROR, "Assimp not enough memory.");
fail:
   IFDO(_glhckFree, vertexData);
   IFDO(_glhckFree, indices);
   IFDO(_glhckFree, textureList);
   IFDO(glhckTextureFree, texture);
   IFDO(glhckMaterialFree, material);
   IFDO(glhckAtlasFree, atlas);

   if (canFreeCurrent)
      glhckObjectRemoveFromParent(current);

   return RETURN_FAIL;
}

static int check(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);
   RET(0, "%d", RETURN_OK)
   return RETURN_OK;
}

static int import(chckBuffer *buffer, glhckImportModelStruct *import)
{
   const struct aiScene *scene = NULL;
   CALL(0, "%p, %p", buffer, import);

   /* import the model using assimp
    * TODO: make import hints tunable?
    * Needs changes to import protocol! */
   unsigned int aflags = aiProcessPreset_TargetRealtime_Fast | aiProcess_OptimizeGraph;

   size_t size = chckBufferGetSize(buffer);
   if (!(scene = aiImportFileFromMemory(chckBufferGetPointer(buffer), size, aflags, NULL)))
      goto assimp_fail;

   NULLDO(aiReleaseImport, scene);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

assimp_fail:
   DEBUG(GLHCK_DBG_ERROR, "%s", aiGetErrorString());
fail:
   IFDO(aiReleaseImport, scene);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->modelImport = import;

   RET(0, "%s", "glhck-importer-assimp");
   return "glhck-importer-assimp";
}

/* vim: set ts=8 sw=3 tw=0 :*/
