#include "../internal.h"
#include "import.h"
#include "buffer/buffer.h"
#include <stdio.h>     /* for scanf */
#include <stdint.h>    /* for standard integers */

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

static const uint8_t GLHCKM_VERSION_NEWEST[] = {0,1};
static const uint8_t GLHCKM_VERSION_OLDEST[] = {0,1};
static const char *GLHCKM_HEADER = "glhckm";
static const char *GLHCKA_HEADER = "glhcka";

/* \brief read float from buffer */
static int chckBufferReadStringFloat(chckBuffer *buf, float *flt)
{
   char *str;
   assert(flt);

   if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f", flt);
   _glhckFree(str);
   return RETURN_OK;
}

/* \brief read vector2 from buffer */
static int chckBufferReadStringVector2(chckBuffer *buf, glhckVector2f *vec)
{
   char *str;
   assert(vec);

   if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f", &vec->x, &vec->y);
   _glhckFree(str);
   return RETURN_OK;
}

/* \brief read vector3 from buffer */
static int chckBufferReadStringVector3(chckBuffer *buf, glhckVector3f *vec)
{
   char *str;
   assert(vec);

   if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f", &vec->x, &vec->y, &vec->z);
   _glhckFree(str);
   return RETURN_OK;
}

/* \brief read quaternion from buffer */
static int chckBufferReadStringQuaternion(chckBuffer *buf, kmQuaternion *quat)
{
   char *str;
   assert(quat);

   if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f,%f", &quat->x, &quat->y, &quat->z, &quat->w);
   _glhckFree(str);
   return RETURN_OK;
}

/* \brief read matrix4 from buffer */
static int chckBufferReadStringMatrix4(chckBuffer *buf, kmMat4 *matrix)
{
   char *str;
   assert(matrix);

   if (chckBufferReadString(buf, sizeof(uint16_t), &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f",
         &matrix->mat[0],  &matrix->mat[4],  &matrix->mat[8],  &matrix->mat[12],
         &matrix->mat[1],  &matrix->mat[5],  &matrix->mat[9],  &matrix->mat[13],
         &matrix->mat[2],  &matrix->mat[6],  &matrix->mat[10], &matrix->mat[14],
         &matrix->mat[3],  &matrix->mat[7],  &matrix->mat[11], &matrix->mat[15]);
   _glhckFree(str);
   return RETURN_OK;
}

/* \brief read color3 from buffer */
static int chckBufferReadColor3(chckBuffer *buf, glhckColorb *color)
{
   uint8_t colors[3];
   assert(color);

   if (chckBufferRead(colors, 1, sizeof(colors), buf) != sizeof(colors))
      return RETURN_FAIL;

   color->r = colors[0];
   color->g = colors[1];
   color->b = colors[2];
   color->a = 255;
   return RETURN_OK;
}

/* \brief read color4 from buffer */
static int chckBufferReadColor4(chckBuffer *buf, glhckColorb *color)
{
   uint8_t colors[4];
   assert(color);

   if (chckBufferRead(colors, 1, sizeof(colors), buf) != sizeof(colors))
      return RETURN_FAIL;

   color->r = colors[0];
   color->g = colors[1];
   color->b = colors[2];
   color->a = colors[3];
   return RETURN_OK;
}

/* \brief read header block */
static int _glhckReadHeader(uint8_t *version, FILE *f, unsigned int *i)
{
   uint32_t r;
   assert(version && f && i);

   if (fread(&r, sizeof(r), 1, f) != 1)
      return RETURN_FAIL;

   if(chckBufferIsBigEndian()) chckBufferSwap(&r, sizeof(uint32_t), 1);
   if (i) *i = r;
   return RETURN_OK;
}

/* \brief read BND data block */
static int _glhckReadBND(uint8_t *version, FILE *f, glhckObject *root, const glhckImportModelParameters *params)
{
   unsigned int i, size;
   unsigned short boneCount;
   glhckBone **bones = NULL;
   chckBuffer *buf = NULL;
   unsigned short *parents = NULL;
   char *str;
   assert(version && f && root && params);

   if (_glhckReadHeader(version, f, &size) != RETURN_OK)
      return RETURN_FAIL;

   if (!params->animated)
      return (fseek(f, size, SEEK_CUR) == 0 ? RETURN_OK : RETURN_FAIL);

   /* create buffer for the size and hope it fits memory \o/ */
   if (!size || !(buf = chckBufferNew(size, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   /* read whole block to buffer */
   if (chckBufferFillFromFile(f, 1, size, buf) != size)
      goto fail;

   /* uint16_t: boneCount */
   if (chckBufferReadUInt16(buf, &boneCount) != RETURN_OK)
      goto fail;

   if (!(bones = _glhckCalloc(boneCount, sizeof(glhckBone*))))
      goto fail;

   if (!(parents = _glhckMalloc(boneCount * sizeof(unsigned short))))
      goto fail;

   /* BONE: bones[boneCount] */
   for (i = 0; i < boneCount; ++i) {
      kmMat4 matrix;

      if (!(bones[i] = glhckBoneNew()))
         goto fail;

      /* STRING: name */
      if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
         goto fail;

      glhckBoneName(bones[i], str);
      _glhckFree(str);

      /* MATRIX4x4: transformationMatrix */
      if (chckBufferReadStringMatrix4(buf, &matrix) != RETURN_OK)
         goto fail;

      /* uint16_t: parent */
      if (chckBufferReadUInt16(buf, &parents[i]) != RETURN_OK)
         goto fail;

      glhckBoneTransformationMatrix(bones[i], &matrix);
   }

   for (i = 0; i < boneCount; ++i) {
      if (parents[i] == i) continue;
      glhckBoneParentBone(bones[i], bones[parents[i]]);
   }

   IFDO(_glhckFree, parents);

   if (bones) {
      glhckBone **allBones = bones, **rootBones;
      unsigned int rootBoneCount;

      /* merge bones */
      if ((rootBones = glhckObjectBones(root, &rootBoneCount))) {
         if (!(allBones = _glhckCalloc(rootBoneCount + boneCount, sizeof(glhckBone*))))
            goto fail;

         for (i = 0; i < rootBoneCount; ++i) glhckBoneRef(rootBones[i]);
         memcpy(allBones, rootBones, rootBoneCount * sizeof(glhckBone*));
         memcpy(&allBones[rootBoneCount], bones, boneCount * sizeof(glhckBone*));
         NULLDO(_glhckFree, bones);
         rootBoneCount += boneCount;
      } else {
         rootBoneCount = boneCount;
      }

      glhckObjectInsertBones(root, allBones, rootBoneCount);
      for (i = 0; i < rootBoneCount; ++i) glhckBoneFree(allBones[i]);
      NULLDO(_glhckFree, allBones);
   }

   NULLDO(chckBufferFree, buf);
   return RETURN_OK;

fail:
   if (bones) {
      for (i = 0; i < boneCount; ++i)
         if (bones[i]) glhckBoneFree(bones[i]);
      _glhckFree(bones);
   }
   IFDO(_glhckFree, parents);
   IFDO(chckBufferFree, buf);
   return RETURN_FAIL;
}

/* \brief read OBD data block */
static int _glhckReadOBD(const char *file, uint8_t *version, FILE *f, glhckObject *root, glhckObject *parent,
      const glhckImportModelParameters *params, glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   kmMat4 matrix;
   unsigned char geometryType, vertexDataFlags;
   unsigned short materialCount, skinBoneCount, childCount;
   unsigned int size, vertexCount, indexCount, i, b;
   glhckImportIndexData *indices = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckObject *object = NULL;
   glhckMaterial **materials = NULL;
   glhckBone **bones = NULL;
   glhckVertexWeight *weights = NULL;
   chckBuffer *buf = NULL;
   char *str;

   enum {
      HAS_NORMALS       = 1<<0,
      HAS_UV            = 1<<1,
      HAS_VERTEX_COLORS = 1<<2
   };

   assert(version && f && root && parent && params);

   /* read size for this OBD block */
   if (_glhckReadHeader(version, f, &size) != RETURN_OK)
      goto fail;

   /* create buffer for the size and hope it fits memory \o/ */
   if (!size || !(buf = chckBufferNew(size, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   /* create object where we store the data */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* read whole block to buffer */
   if (chckBufferFillFromFile(f, 1, size, buf) != size)
      goto fail;

   /* STRING: name */
   if (chckBufferReadString(buf, 1, &str) != RETURN_OK)
      goto fail;

   if (str) {
      _glhckObjectFile(object, str);
      _glhckFree(str);
   }

   /* MATRIX4x4: transformationMatrix */
   if (chckBufferReadStringMatrix4(buf, &matrix) != RETURN_OK)
      goto fail;

   /* uint8_t: geometryType */
   if (chckBufferReadUInt8(buf, &geometryType) != RETURN_OK)
      goto fail;

   /* uint8_t: vertexDataFlags */
   if (chckBufferReadUInt8(buf, &vertexDataFlags) != RETURN_OK)
      goto fail;

   /* int32_t: indexCount (note, signed because that's how OGL likes it, we just read to uint though) */
   if (chckBufferReadUInt32(buf, &indexCount) != RETURN_OK)
      goto fail;

   /* int32_t: vertexCount (note, read above) */
   if (chckBufferReadUInt32(buf, &vertexCount) != RETURN_OK)
      goto fail;

   /* int16_t: materialCount */
   if (chckBufferReadUInt16(buf, &materialCount) != RETURN_OK)
      goto fail;

   /* int16_t: skinBoneCount */
   if (chckBufferReadUInt16(buf, &skinBoneCount) != RETURN_OK)
      goto fail;

   /* int16_t: childCount */
   if (chckBufferReadUInt16(buf, &childCount) != RETURN_OK)
      goto fail;

   /* compile time check. If you ever hit this, report a bug. */
   char is_this_part_of_code_broken[sizeof(uint32_t)-sizeof(glhckImportIndexData)];
   (void)is_this_part_of_code_broken;

   if (indexCount && !(indices = _glhckMalloc(indexCount * sizeof(glhckImportIndexData))))
      goto fail;

   /* uint32_t: indices[indexCount] */
   if (indexCount && chckBufferRead(indices, sizeof(uint32_t), indexCount, buf) != indexCount)
      goto fail;

   /* flip endian, if needed */
   if (!chckBufferIsNativeEndian(buf)) chckBufferSwap(indices, sizeof(uint32_t), indexCount);

   if (indices) {
      glhckObjectInsertIndices(object, itype, indices, indexCount);
      NULLDO(_glhckFree, indices);
   }

   if (vertexCount && !(vertexData = _glhckCalloc(vertexCount, sizeof(glhckImportVertexData))))
      goto fail;

   /* VERTEXDATA: vertices[vertexCount] */
   for (i = 0; i < vertexCount; ++i) {
      /* VECTOR3: vertex */
      if (chckBufferReadStringVector3(buf, &vertexData[i].vertex) != RETURN_OK)
         goto fail;

      /* transform vertices, actually decompose and position the objects later */
      kmVec3MultiplyMat4((kmVec3*)&vertexData[i].vertex, (kmVec3*)&vertexData[i].vertex, &matrix);

      if (vertexDataFlags & HAS_NORMALS) {
         /* VECTOR3: normal */
         if (chckBufferReadStringVector3(buf, &vertexData[i].normal) != RETURN_OK)
            goto fail;
      }

      if (vertexDataFlags & HAS_UV) {
         /* VECTOR2: uv */
         if (chckBufferReadStringVector2(buf, &vertexData[i].coord) != RETURN_OK)
            goto fail;
      }

      if (vertexDataFlags & HAS_VERTEX_COLORS) {
         /* COLOR4: color */
         if (chckBufferReadColor4(buf, &vertexData[i].color) != RETURN_OK)
            goto fail;
      }
   }

   if (vertexData) {
      glhckObjectInsertVertices(object, vtype, vertexData, vertexCount);
      NULLDO(_glhckFree, vertexData);
   }

   /* we just assume TRIANGLES for now 0.1 doesn't support anything else */
   if (object->geometry) object->geometry->type = GLHCK_TRIANGLES;

   if (materialCount && !(materials = _glhckCalloc(materialCount, sizeof(glhckMaterial*))))
      goto fail;

   /* MATERIAL: materials[materialCount] */
   for (i = 0; i < materialCount; ++i) {
      unsigned short shininess;
      unsigned char materialFlags;

      enum {
         LIGHTING = 1<<0
      };

      if (!(materials[i] = glhckMaterialNew(NULL)))
         goto fail;

      /* STRING: name */
      if (chckBufferReadString(buf, 1, &str) != RETURN_OK)
         goto fail;

      if (str) {
         /* no material names in api yet */
         _glhckFree(str);
      }

      /* COLOR3: ambient */
      if (chckBufferReadColor3(buf, &materials[i]->ambient) != RETURN_OK)
         goto fail;

      /* COLOR4: diffuse */
      if (chckBufferReadColor4(buf, &materials[i]->diffuse) != RETURN_OK)
         goto fail;

      /* COLOR4: specular */
      if (chckBufferReadColor4(buf, &materials[i]->specular) != RETURN_OK)
         goto fail;

      /* uint16_t: shininess (range 1 - 511) */
      if (chckBufferReadUInt16(buf, &shininess) != RETURN_OK)
         goto fail;

      materials[i]->shininess = (float)shininess/511;

      /* uint8_t: materialFlags */
      if (chckBufferReadUInt8(buf, &materialFlags) != RETURN_OK)
         goto fail;

      if (materialFlags & LIGHTING)
         materials[i]->flags |= GLHCK_MATERIAL_LIGHTING;

      /* LONGSTRING: diffuseTexture */
      if (chckBufferReadString(buf, sizeof(uint16_t), &str) != RETURN_OK)
         goto fail;

      if (str) {
         glhckTexture *texture;
         char *texturePath;

         if ((texturePath = _glhckImportTexturePath(str, file))) {
            if ((texture = glhckTextureNewFromFile(texturePath, NULL, NULL)))
               glhckMaterialTexture(materials[i], texture);
            _glhckFree(texturePath);
         }

         _glhckFree(str);
      }
   }

   if (materials) {
      glhckObjectMaterial(object, materials[0]);
      for (i = 0; i < materialCount; ++i) glhckMaterialFree(materials[i]);
      NULLDO(_glhckFree, materials);
   }

   if (skinBoneCount && !(bones = _glhckCalloc(skinBoneCount, sizeof(glhckBone*))))
      goto fail;

   /* SKINBONE: skinBones[skinBoneCount] */
   for (i = 0, b = 0; i < skinBoneCount; ++i) {
      unsigned int weightCount, w;

      /* STRING: name */
      if (chckBufferReadString(buf, 1, &str) != RETURN_OK)
         goto fail;

      if (str) {
         bones[b] = glhckObjectGetBone(root, str);
         _glhckFree(str);
      }

      /* MATRIX4x4: offsetMatrix */
      if (chckBufferReadStringMatrix4(buf, &matrix) != RETURN_OK)
         goto fail;

      /* int32_t: weightCount */
      if (chckBufferReadUInt32(buf, &weightCount) != RETURN_OK)
         goto fail;

      if (weightCount && !(weights = _glhckMalloc(weightCount * sizeof(glhckVertexWeight))))
         goto fail;

      /* WEIGHT: weights[weightCount] */
      for (w = 0; w < weightCount; ++w) {
         /* int32_t: vertexIndex */
         if (chckBufferReadUInt32(buf, &weights[w].vertexIndex) != RETURN_OK)
            goto fail;

         /* FLOAT: weight */
         if (chckBufferReadStringFloat(buf, &weights[w].weight) != RETURN_OK)
            goto fail;
      }

      /* FIXME: we need glhckSkinBone that points to bones[b]
       * Why? SkinBones each have own offsetMatrix and weights. */
      if (bones[b]) {
         unsigned int newWeightCount;
         glhckVertexWeight *newWeights = weights;
         const glhckVertexWeight *oldWeights;

         if ((oldWeights = glhckBoneWeights(bones[b], &newWeightCount))) {
            if (!(newWeights = _glhckCalloc(newWeightCount + weightCount, sizeof(glhckVertexWeight))))
               goto fail;


            memcpy(newWeights, oldWeights, newWeightCount * sizeof(glhckVertexWeight));
            memcpy(newWeights, weights, weightCount * sizeof(glhckVertexWeight));
            NULLDO(_glhckFree, weights);
            newWeightCount += weightCount;
         } else {
            newWeightCount = weightCount;
         }

         glhckBoneOffsetMatrix(bones[b], &matrix);
         glhckBoneInsertWeights(bones[b], newWeights, newWeightCount);
         ++b;
      }

      IFDO(_glhckFree, weights);
   }

   if (bones) {
      if (b) glhckObjectInsertBones(object, bones, b);
      NULLDO(_glhckFree, bones);
   }

   /* OB: children[childCount] */
   for (i = 0; i < childCount; ++i) {
      char block[3];

      if (fread(block, 1, sizeof(block), f) != sizeof(block))
         goto fail;

      if (block[0] != 'O' || block[1] != 'B' || block[2] != 'D')
         goto fail;

      _glhckReadOBD(file, version, f, root, object, params, itype, vtype);
   }

   glhckObjectAddChild(parent, object);
   NULLDO(glhckObjectFree, object);
   NULLDO(chckBufferFree, buf);
   return RETURN_OK;

fail:
   IFDO(glhckObjectFree, object);
   if (materials) {
      for (i = 0; i < materialCount; ++i)
         if (materials[i]) glhckMaterialFree(materials[i]);
      _glhckFree(materials);
   }
   IFDO(_glhckFree, bones);
   IFDO(_glhckFree, weights);
   IFDO(_glhckFree, vertexData);
   IFDO(_glhckFree, indices);
   IFDO(chckBufferFree, buf);
   return RETURN_FAIL;
}

/* \brief read AND data block */
static int _glhckReadAND(uint8_t *version, FILE *f, glhckObject *root, uint32_t animationCount,
      glhckAnimation **outAnimation, const glhckImportModelParameters *params)
{
   float longestTime = 0.0f;
   unsigned int i, size, nodeCount;
   glhckAnimationQuaternionKey *quaternionKeys = NULL;
   glhckAnimationVectorKey *vectorKeys = NULL;
   glhckAnimationNode **nodes = NULL;
   glhckAnimation *animation = NULL, **animations = NULL;
   chckBuffer *buf = NULL;
   char *str, block[3];
   assert(version && f && root && params);

   if (_glhckReadHeader(version, f, &size) != RETURN_OK)
      goto fail;

   if (!params->animated)
      return (fseek(f, size, SEEK_CUR) == 0 ? RETURN_OK : RETURN_FAIL);

   /* create buffer for the size and hope it fits memory \o/ */
   if (!size || !(buf = chckBufferNew(size, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   /* create animation where we store the data */
   if (!(animation = glhckAnimationNew()))
      goto fail;

   /* read whole block to buffer */
   if (chckBufferFillFromFile(f, 1, size, buf) != size)
      goto fail;

   /* STRING: name */
   if (chckBufferReadString(buf, 1, &str) != RETURN_OK)
      goto fail;

   if (str) {
      glhckAnimationName(animation, str);
      _glhckFree(str);
   }

   /* uint32_t: nodeCount */
   if (chckBufferReadUInt32(buf, &nodeCount) != RETURN_OK)
      goto fail;

   if (nodeCount && !(nodes = _glhckCalloc(nodeCount, sizeof(glhckAnimationNode*))))
      goto fail;

   /* NODE: nodes[nodeCount] */
   for (i = 0; i < nodeCount; ++i) {
      unsigned int k, rotationCount, scalingCount, translationCount, transScalingCount;

      if (!(nodes[i] = glhckAnimationNodeNew()))
         goto fail;

      /* STRING: name */
      if (chckBufferReadString(buf, 1, &str) != RETURN_OK || !str)
         goto fail;

      glhckAnimationNodeBoneName(nodes[i], str);
      _glhckFree(str);

      /* uint32_t: rotationCount */
      if (chckBufferReadUInt32(buf, &rotationCount) != RETURN_OK)
         goto fail;

      /* uint32_t: scalingCount */
      if (chckBufferReadUInt32(buf, &scalingCount) != RETURN_OK)
         goto fail;

      /* uint32_t: translationCount */
      if (chckBufferReadUInt32(buf, &translationCount) != RETURN_OK)
         goto fail;

      if (rotationCount && !(quaternionKeys = _glhckMalloc(rotationCount * sizeof(glhckAnimationQuaternionKey))))
         goto fail;

      /* QUATERNIONKEY: quaternionKeys[rotationCount] */
      for (k = 0; k < rotationCount; ++k) {
         unsigned int frame;

         /* uint32_t: frame */
         if (chckBufferReadUInt32(buf, &frame) != RETURN_OK)
            goto fail;

         /* QUATERNION: quaternion */
         if (chckBufferReadStringQuaternion(buf, &quaternionKeys[k].quaternion) != RETURN_OK)
            goto fail;

         quaternionKeys[k].time = frame;
         if (frame > longestTime) longestTime = frame;
      }

      if (quaternionKeys) {
         glhckAnimationNodeInsertRotations(nodes[i], quaternionKeys, rotationCount);
         NULLDO(_glhckFree, quaternionKeys);
      }

      transScalingCount = scalingCount + translationCount;
      if (transScalingCount && !(vectorKeys = _glhckMalloc(transScalingCount * sizeof(glhckAnimationVectorKey))))
         goto fail;

      /* VECTORKEY: scalingKeys[scalingCount] */
      /* VECTORKEY: translationKeys[scalingCount] */
      for (k = 0; k < transScalingCount; ++k) {
         unsigned int frame;
         glhckVector3f vector;

         /* uint32_t: frame */
         if (chckBufferReadUInt32(buf, &frame) != RETURN_OK)
            goto fail;

         /* VECTOR3: vector */
         if (chckBufferReadStringVector3(buf, &vector) != RETURN_OK)
            goto fail;

         vectorKeys[k].time = frame;
         vectorKeys[k].vector.x = vector.x;
         vectorKeys[k].vector.y = vector.y;
         vectorKeys[k].vector.z = vector.z;
         if (frame > longestTime) longestTime = frame;
      }

      if (vectorKeys) {
         if (scalingCount)
            glhckAnimationNodeInsertScalings(nodes[i], vectorKeys, scalingCount);
         if (translationCount)
            glhckAnimationNodeInsertTranslations(nodes[i], vectorKeys+scalingCount, translationCount);
         NULLDO(_glhckFree, vectorKeys);
      }
   }

   if (nodes) {
      if (glhckAnimationInsertNodes(animation, nodes, nodeCount) != RETURN_OK)
         goto fail;
      for (i = 0; i < nodeCount; ++i) glhckAnimationNodeFree(nodes[i]);
      NULLDO(_glhckFree, nodes);
   }

   glhckAnimationDuration(animation, longestTime);
   NULLDO(chckBufferFree, buf);

   /* read rest of AND blocks here, this won't recurse since the other reads won't enter this branch */
   if (!outAnimation) {
      if (!(animations = _glhckCalloc(animationCount, sizeof(glhckAnimation*))))
         goto fail;

      /* we are the first animation */
      animations[0] = animation;

      /* get rest of animations */
      for (i = 1; i < animationCount; ++i) {
         if (fread(block, 1, sizeof(block), f) != sizeof(block))
            goto fail;

         if (block[0] != 'A' || block[1] != 'N' || block[2] != 'D')
            goto fail;

         if (_glhckReadAND(version, f, root, animationCount, &animations[i], params) != RETURN_OK)
            goto fail;
      }

      /* phew, we are done */
      glhckObjectInsertAnimations(root, animations, animationCount);
      for (i = 0; i < animationCount; ++i) glhckAnimationFree(animations[i]);
      _glhckFree(animations);
   }

   if (outAnimation) *outAnimation = animation;
   return RETURN_OK;

fail:
   if (animations) {
      for (i = 0; i < animationCount; ++i)
         if (animations[i]) glhckAnimationFree(animations[i]);
      _glhckFree(animations);
   }
   if (nodes) {
      for (i = 0; i < nodeCount; ++i)
         if (nodes[i]) glhckAnimationNodeFree(nodes[i]);
      _glhckFree(nodes);
   }
   IFDO(_glhckFree, quaternionKeys);
   IFDO(_glhckFree, vectorKeys);
   IFDO(glhckAnimationFree, animation);
   IFDO(chckBufferFree, buf);
   return RETURN_FAIL;
}

/* \brief read glhckm/glhcka file */
static int _glhckImport(const char *file, uint8_t *version, FILE *f, glhckObject *root, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   char block[3];
   uint32_t BNH = 0, OBH = 0, ANH = 0;
   int ret = RETURN_OK;
   assert(file && version && f && root && params);

   while (ret == RETURN_OK && fread(block, 1, sizeof(block), f) == sizeof(block)) {
      if (block[0] == 'B' && block[1] == 'N') {
         if (block[2] == 'H') ret = _glhckReadHeader(version, f, &BNH);
         if (block[2] == 'D') ret = _glhckReadBND(version, f, root, params);
      }

      if (block[0] == 'O' && block[1] == 'B') {
         if (block[2] == 'H') ret = _glhckReadHeader(version, f, &OBH);
         if (block[2] == 'D') ret = _glhckReadOBD(file, version, f, root, root, params, itype, vtype);
      }

      if (block[0] == 'A' && block[1] == 'N') {
         if (block[2] == 'H') ret = _glhckReadHeader(version, f, &ANH);
         if (block[2] == 'D') ret = _glhckReadAND(version, f, root, ANH, NULL, params);
      }
   }

   if (ret != RETURN_OK) goto read_fail;
   return ret;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "glhckm read failure");
   return RETURN_FAIL;
}

/* \brief compare file header */
static int _glhckCmpHeader(FILE *f, const char *header, uint8_t *outVersion)
{
   char *buffer;
   uint8_t version[2];
   assert(f && header);

   if (outVersion) memset(outVersion, 0, sizeof(version));
   if (!(buffer = _glhckCalloc(1, strlen(header))))
      goto fail;

   fread(buffer, 1, strlen(header), f);
   if (memcmp(buffer, header, strlen(header)) != 0)
      goto fail;

   NULLDO(_glhckFree, buffer);

   fread(version, 1, 2, f);
   if ((version[0] > GLHCKM_VERSION_NEWEST[0]  ||
        version[1] > GLHCKM_VERSION_NEWEST[1]) ||
       (version[0] < GLHCKM_VERSION_OLDEST[0]  ||
        version[1] < GLHCKM_VERSION_OLDEST[1]))
      goto version_not_supported;

   if (outVersion) memcpy(outVersion, version, sizeof(version));
   return RETURN_OK;

version_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "%s version (%u.%u) not supported", header, version[0], version[1]);
fail:
   IFDO(_glhckFree, buffer);
   return 0;
}

/* \brief check if file is a either glhcka or glhckm file */
static int _glhckFormatGlhck(const char *file, const char *header)
{
   FILE *f;
   CALL(0, "%s", file);
   assert(file && header);

   /* open file for reading */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* check for correct header */
   if (!_glhckCmpHeader(f, header, NULL))
      goto fail;

   /* close file */
   NULLDO(fclose, f);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   return RETURN_FAIL;
}

int _glhckFormatGlhckm(const char *file) { return _glhckFormatGlhck(file, GLHCKM_HEADER); }
int _glhckFormatGlhcka(const char *file) { return _glhckFormatGlhck(file, GLHCKA_HEADER); }

/* \brief import glhckm/glhcka file */
static int _glhckImportGlhck(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   FILE *f;
   uint8_t version[2];
   CALL(0, "%p, %s, %p", object, file, params);
   assert(object && file && params);

   /* open file for reading */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* check that we have correct file */
   if (!_glhckCmpHeader(f, GLHCKM_HEADER, version))
      goto fail;

   /* parse the glhckm/glhcka file */
   if (!_glhckImport(file, version, f, object, params, itype, vtype))
      goto fail;

   /* mark ourself as special root object.
    * this makes most functions called on root object echo to children */
   object->flags |= GLHCK_OBJECT_ROOT;

   /* close file */
   NULLDO(fclose, f);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

int _glhckImportGlhckm(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype) {
   return _glhckImportGlhck(object, file, params, itype, vtype);
}

#if 0
int _glhckImportGlhcka(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype) {
   return _glhckImportGlhck(object, file, params, itype, vtype);
}
#endif

/* vim: set ts=8 sw=3 tw=0 :*/
