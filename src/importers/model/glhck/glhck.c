#include "importers/importer.h"

#include <stdio.h>     /* for scanf */
#include <stdint.h>    /* for standard integers */
#include <stdlib.h>

#include "trace.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

static const uint8_t GLHCKM_VERSION_NEWEST[] = {0,1};
static const uint8_t GLHCKM_VERSION_OLDEST[] = {0,1};
static const char *GLHCKM_HEADER = "glhckm";

/**
 * TODO: animation importers.
 * static const char *GLHCKA_HEADER = "glhcka";
 */


struct BND {
   chckIterPool *bones;
};

struct OBD {
   chckIterPool *meshes;
   chckIterPool *materials;
   chckIterPool *skins;
};

struct AND {
   chckIterPool *animations;
};

/* \brief read float from buffer */
static int chckBufferReadStringFloat(chckBuffer *buf, float *flt)
{
   assert(flt);

   char *str;
   if (chckBufferReadStringOfType(buf, CHCK_BUFFER_STRING_UINT8, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f", flt);
   free(str);
   return RETURN_OK;
}

/* \brief read vector2 from buffer */
static int chckBufferReadStringVector2(chckBuffer *buf, glhckVector2f *vec)
{
   assert(vec);

   char *str;
   if (chckBufferReadStringOfType(buf, CHCK_BUFFER_STRING_UINT8, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f", &vec->x, &vec->y);
   free(str);
   return RETURN_OK;
}

/* \brief read vector3 from buffer */
static int chckBufferReadStringVector3(chckBuffer *buf, glhckVector3f *vec)
{
   assert(vec);

   char *str;
   if (chckBufferReadStringOfType(buf, CHCK_BUFFER_STRING_UINT8, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f", &vec->x, &vec->y, &vec->z);
   free(str);
   return RETURN_OK;
}

/* \brief read quaternion from buffer */
static int chckBufferReadStringQuaternion(chckBuffer *buf, kmQuaternion *quat)
{
   assert(quat);

   char *str;
   if (chckBufferReadStringOfType(buf, CHCK_BUFFER_STRING_UINT8, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f,%f", &quat->x, &quat->y, &quat->z, &quat->w);
   free(str);
   return RETURN_OK;
}

/* \brief read matrix4 from buffer */
static int chckBufferReadStringMatrix4(chckBuffer *buf, kmMat4 *matrix)
{
   assert(matrix);

   char *str;
   if (chckBufferReadStringOfType(buf, CHCK_BUFFER_STRING_UINT16, &str) != RETURN_OK || !str)
      return RETURN_FAIL;

   sscanf(str, "%f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f %f,%f,%f,%f",
         &matrix->mat[0],  &matrix->mat[4],  &matrix->mat[8],  &matrix->mat[12],
         &matrix->mat[1],  &matrix->mat[5],  &matrix->mat[9],  &matrix->mat[13],
         &matrix->mat[2],  &matrix->mat[6],  &matrix->mat[10], &matrix->mat[14],
         &matrix->mat[3],  &matrix->mat[7],  &matrix->mat[11], &matrix->mat[15]);
   free(str);
   return RETURN_OK;
}

/* \brief read color3 from buffer */
static int chckBufferReadColor3(chckBuffer *buf, glhckColor *color)
{
   assert(color);

   uint8_t colors[3];
   if (chckBufferRead(colors, 1, sizeof(colors), buf) != sizeof(colors))
      return RETURN_FAIL;

   *color = (255 | (colors[2] << 8) | (colors[1] << 16) | (colors[0] << 24));
   return RETURN_OK;
}

/* \brief read color4 from buffer */
static int chckBufferReadColor4(chckBuffer *buf, glhckColor *color)
{
   assert(color);

   uint8_t colors[4];
   if (chckBufferRead(colors, 1, sizeof(colors), buf) != sizeof(colors))
      return RETURN_FAIL;

   *color = (colors[3] | (colors[2] << 8) | (colors[1] << 16) | (colors[0] << 24));
   return RETURN_OK;
}

/* \brief read header block */
static int readHeader(uint8_t *version, chckBuffer *buffer, unsigned int *outSize)
{
   assert(version && buffer && outSize);
   (void)version;

   uint32_t r;
   if (chckBufferRead(&r, sizeof(r), 1, buffer) != 1)
      return RETURN_FAIL;

   if(chckBufferIsBigEndian())
      chckBufferSwap(&r, sizeof(uint32_t), 1);

   if (outSize) *outSize = r;
   return RETURN_OK;
}

/* \brief read BND data block */
static int readBND(uint8_t *version, chckBuffer *buffer, struct BND *bnd)
{
   char *name = NULL;
   assert(version && buffer && bnd);

   /* read size for this BND block */
   unsigned int size;
   if (readHeader(version, buffer, &size) != RETURN_OK)
      goto fail;

   if (size <= 0)
      return RETURN_OK;

   /* uint16_t: boneCount */
   unsigned short boneCount;
   if (chckBufferReadUInt16(buffer, &boneCount) != RETURN_OK)
      goto fail;

   if (!bnd->bones && !(bnd->bones = chckIterPoolNew(32, boneCount, sizeof(glhckImportModelBone))))
      goto fail;

   /* BONE: bones[boneCount] */
   for (unsigned int i = 0; i < boneCount; ++i) {
      glhckImportModelBone bone;

      /* STRING: name */
      if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &name) != RETURN_OK || !name)
         goto fail;

      bone.name = name;

      /* MATRIX4x4: transformationMatrix */
      if (chckBufferReadStringMatrix4(buffer, &bone.transformationMatrix) != RETURN_OK)
         goto fail;

      /* uint16_t: parent */
      unsigned short parent;
      if (chckBufferReadUInt16(buffer, &parent) != RETURN_OK)
         goto fail;

      bone.parent = parent;

      if (!chckIterPoolAdd(bnd->bones, &bone, NULL))
         goto fail;

      name = NULL;
   }

   return RETURN_OK;

fail:
   IFDO(free, name);
   return RETURN_FAIL;
}

/* \brief read OBD data block */
static int readOBD(uint8_t *version, chckBuffer *buffer, struct OBD *obd, const size_t parent)
{
   glhckImportIndexData *indexData = NULL;
   glhckImportVertexData *vertexData = NULL;
   glhckVertexWeight *weights = NULL;
   size_t *materials = NULL, *skins = NULL;
   char *name = NULL, *mname = NULL, *sname = NULL, *texture = NULL;
   assert(version && buffer);

   /* read size for this OBD block */
   unsigned int size;
   if (readHeader(version, buffer, &size) != RETURN_OK)
      goto fail;

   if (size <= 0)
      return RETURN_OK;

   /* STRING: name */
   if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &name) != RETURN_OK)
      goto fail;

   if (!obd->meshes && !(obd->meshes = chckIterPoolNew(32, 1, sizeof(glhckImportModelMesh))))
      goto fail;

   glhckImportModelMesh mesh;
   mesh.name = name;

   /* uint8_t: geometryType */
   unsigned char geometryType;
   if (chckBufferReadUInt8(buffer, &geometryType) != RETURN_OK)
      goto fail;

   enum {
      TRIANGLES,
   };

   switch (geometryType) {
      case TRIANGLES:
      default:
         mesh.geometryType = GLHCK_TRIANGLES;
         break;
   }

   /* uint8_t: vertexDataFlags */
   unsigned char vertexDataFlags;
   if (chckBufferReadUInt8(buffer, &vertexDataFlags) != RETURN_OK)
      goto fail;

   enum {
      HAS_NORMALS       = 1<<0,
      HAS_UV            = 1<<1,
      HAS_VERTEX_COLORS = 1<<2
   };

   mesh.hasNormals = (vertexDataFlags & HAS_NORMALS);
   mesh.hasUV = (vertexDataFlags & HAS_UV);
   mesh.hasVertexColors = (vertexDataFlags & HAS_VERTEX_COLORS);

   /* int32_t: indexCount (note, signed because that's how OGL likes it, we just read to uint though) */
   int indexCount;
   if (chckBufferReadInt32(buffer, &indexCount) != RETURN_OK)
      goto fail;

   /* int32_t: vertexCount (note, read above) */
   int vertexCount;
   if (chckBufferReadInt32(buffer, &vertexCount) != RETURN_OK)
      goto fail;

   /* int16_t: materialCount */
   unsigned short materialCount;
   if (chckBufferReadUInt16(buffer, &materialCount) != RETURN_OK)
      goto fail;

   /* int16_t: skinBoneCount */
   unsigned short skinBoneCount;
   if (chckBufferReadUInt16(buffer, &skinBoneCount) != RETURN_OK)
      goto fail;

   /* int16_t: childCount */
   unsigned short childCount;
   if (chckBufferReadUInt16(buffer, &childCount) != RETURN_OK)
      goto fail;

   /* compile time check. If you ever hit this, report a bug. */
   char is_this_part_of_code_broken[sizeof(uint32_t)-sizeof(glhckImportIndexData)];
   (void)is_this_part_of_code_broken;

   if (indexCount && !(indexData = calloc(indexCount, sizeof(glhckImportIndexData))))
      goto fail;

   /* uint32_t: indices[indexCount] */
   if (indexCount && chckBufferRead(indexData, sizeof(uint32_t), indexCount, buffer) != (unsigned int)indexCount)
      goto fail;

   /* flip endian, if needed */
   if (!chckBufferIsNativeEndian(buffer))
      chckBufferSwap(indexData, sizeof(uint32_t), indexCount);

   if (vertexCount && !(vertexData = calloc(vertexCount, sizeof(glhckImportVertexData))))
      goto fail;

   /* VERTEXDATA: vertices[vertexCount] */
   for (int i = 0; i < vertexCount; ++i) {
      /* VECTOR3: vertex */
      if (chckBufferReadStringVector3(buffer, &vertexData[i].vertex) != RETURN_OK)
         goto fail;

      if (vertexDataFlags & HAS_NORMALS) {
         /* VECTOR3: normal */
         if (chckBufferReadStringVector3(buffer, &vertexData[i].normal) != RETURN_OK)
            goto fail;
      }

      if (vertexDataFlags & HAS_UV) {
         /* VECTOR2: uv */
         if (chckBufferReadStringVector2(buffer, &vertexData[i].coord) != RETURN_OK)
            goto fail;
      }

      if (vertexDataFlags & HAS_VERTEX_COLORS) {
         /* COLOR4: color */
         if (chckBufferReadColor4(buffer, &vertexData[i].color) != RETURN_OK)
            goto fail;
      }
   }

   if (!(materials = calloc(materialCount, sizeof(size_t))))
      goto fail;

   if (!obd->materials && !(obd->materials = chckIterPoolNew(32, materialCount, sizeof(glhckImportModelMaterial))))
      goto fail;

   /* MATERIAL: materials[materialCount] */
   for (int i = 0; i < materialCount; ++i) {
      glhckImportModelMaterial material;

      enum {
         LIGHTING = 1<<0
      };

      /* STRING: name */
      if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &name) != RETURN_OK)
         goto fail;

      material.name = mname;

      /* COLOR3: ambient */
      if (chckBufferReadColor3(buffer, &material.ambient) != RETURN_OK)
         goto fail;

      /* COLOR4: diffuse */
      if (chckBufferReadColor4(buffer, &material.diffuse) != RETURN_OK)
         goto fail;

      /* COLOR4: specular */
      if (chckBufferReadColor4(buffer, &material.specular) != RETURN_OK)
         goto fail;

      /* uint16_t: shininess (range 1 - 511) */
      unsigned short shininess;
      if (chckBufferReadUInt16(buffer, &shininess) != RETURN_OK)
         goto fail;

      material.shininess = (float)shininess/511;

      /* uint8_t: materialFlags */
      unsigned char materialFlags;
      if (chckBufferReadUInt8(buffer, &materialFlags) != RETURN_OK)
         goto fail;

      material.hasLighting = (materialFlags & LIGHTING);

      /* LONGSTRING: diffuseTexture */
      if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT16, &texture) != RETURN_OK)
         goto fail;

      material.diffuseTexture = texture;

      if (!chckIterPoolAdd(obd->materials, &material, &materials[i]))
         goto fail;

      mname = texture = NULL;
   }

   if (!(skins = calloc(skinBoneCount, sizeof(size_t))))
      goto fail;

   if (!obd->skins && !(obd->skins = chckIterPoolNew(32, skinBoneCount, sizeof(glhckImportModelSkin))))
      goto fail;

   /* SKINBONE: skinBones[skinBoneCount] */
   for (int i = 0; i < skinBoneCount; ++i) {
      glhckImportModelSkin skin;

      /* STRING: name */
      if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &sname) != RETURN_OK)
         goto fail;

      skin.name = sname;

      /* MATRIX4x4: offsetMatrix */
      if (chckBufferReadStringMatrix4(buffer, &skin.offsetMatrix) != RETURN_OK)
         goto fail;

      /* int32_t: weightCount */
      unsigned int weightCount;
      if (chckBufferReadUInt32(buffer, &weightCount) != RETURN_OK)
         goto fail;

      if (weightCount && !(weights = calloc(weightCount, sizeof(glhckVertexWeight))))
         goto fail;

      /* WEIGHT: weights[weightCount] */
      for (unsigned int w = 0; w < weightCount; ++w) {
         /* int32_t: vertexIndex */
         if (chckBufferReadUInt32(buffer, &weights[w].vertexIndex) != RETURN_OK)
            goto fail;

         /* FLOAT: weight */
         if (chckBufferReadStringFloat(buffer, &weights[w].weight) != RETURN_OK)
            goto fail;
      }

      skin.weights = weights;
      skin.weightCount = weightCount;

      if (!chckIterPoolAdd(obd->skins, &skin, &skins[i]))
         goto fail;

      sname = NULL;
   }

   mesh.parent = (parent > 0 ? parent - 1 : 0);
   mesh.indexData = indexData;
   mesh.vertexData = vertexData;
   mesh.indexCount = indexCount;
   mesh.vertexCount = vertexCount;

   size_t *mats = calloc(materialCount, sizeof(size_t));
   for (size_t i = 0; i < materialCount; ++i)
      mats[i] = i + chckIterPoolCount(obd->materials) - materialCount;

   mesh.materials = mats;
   mesh.materialCount = materialCount;

   size_t *skns = calloc(skinBoneCount, sizeof(size_t));
   for (size_t i = 0; i < skinBoneCount; ++i)
      skns[i] = i + chckIterPoolCount(obd->skins) - skinBoneCount;

   mesh.skins = skns;
   mesh.skinCount = skinBoneCount;

   size_t index = 0;
   if (!chckIterPoolAdd(obd->meshes, &mesh, &index))
      goto fail;

   name = NULL;

   /* OB: children[childCount] */
   for (int i = 0; i < childCount; ++i) {
      char block[3];

      if (chckBufferRead(block, 1, sizeof(block), buffer) != sizeof(block))
         goto fail;

      if (block[0] != 'O' || block[1] != 'B' || block[2] != 'D')
         goto fail;

      readOBD(version, buffer, obd, index);
   }

   return RETURN_OK;

fail:
   IFDO(free, name);
   IFDO(free, mname);
   IFDO(free, sname);
   IFDO(free, texture);
   IFDO(free, weights);
   IFDO(free, materials);
   IFDO(free, skins);
   IFDO(free, vertexData);
   IFDO(free, indexData);
   return RETURN_FAIL;
}

/* \brief read AND data block */
static int readAND(uint8_t *version, chckBuffer *buffer, struct AND *and)
{
   glhckAnimationQuaternionKey *quaternionKeys = NULL;
   glhckAnimationVectorKey *vectorKeys[2] = { NULL, NULL };
   glhckImportAnimationNode *nodes = NULL;
   char *name = NULL, *nname = NULL;
   assert(version && buffer && and);

   unsigned int size;
   if (readHeader(version, buffer, &size) != RETURN_OK)
      goto fail;

   if (size <= 0)
      return RETURN_OK;

   /* STRING: name */
   if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &name) != RETURN_OK)
      goto fail;

   if (!and->animations && !(and->animations = chckIterPoolNew(32, 1, sizeof(glhckImportAnimationStruct))))
      goto fail;

   glhckImportAnimationStruct animation;
   animation.name = name;

   /* uint32_t: nodeCount */
   unsigned int nodeCount;
   if (chckBufferReadUInt32(buffer, &nodeCount) != RETURN_OK)
      goto fail;

   if (nodeCount && !(nodes = calloc(nodeCount, sizeof(glhckImportAnimationNode))))
      goto fail;

   /* NODE: nodes[nodeCount] */
   for (unsigned int i = 0; i < nodeCount; ++i) {
      /* STRING: name */
      if (chckBufferReadStringOfType(buffer, CHCK_BUFFER_STRING_UINT8, &nname) != RETURN_OK || !nname)
         goto fail;

      nodes[i].name = nname;

      /* uint32_t: rotationCount */
      unsigned int rotationCount;
      if (chckBufferReadUInt32(buffer, &rotationCount) != RETURN_OK)
         goto fail;

      /* uint32_t: scalingCount */
      unsigned int scalingCount;
      if (chckBufferReadUInt32(buffer, &scalingCount) != RETURN_OK)
         goto fail;

      /* uint32_t: translationCount */
      unsigned int translationCount;
      if (chckBufferReadUInt32(buffer, &translationCount) != RETURN_OK)
         goto fail;

      if (rotationCount && !(quaternionKeys = calloc(rotationCount, sizeof(glhckAnimationQuaternionKey))))
         goto fail;

      /* QUATERNIONKEY: quaternionKeys[rotationCount] */
      for (unsigned int k = 0; k < rotationCount; ++k) {
         /* uint32_t: frame */
         unsigned int frame;
         if (chckBufferReadUInt32(buffer, &frame) != RETURN_OK)
            goto fail;

         /* QUATERNION: quaternion */
         if (chckBufferReadStringQuaternion(buffer, &quaternionKeys[k].quaternion) != RETURN_OK)
            goto fail;

         quaternionKeys[k].time = frame;
      }

      if (scalingCount && !(vectorKeys[0] = calloc(scalingCount, sizeof(glhckAnimationVectorKey))))
         goto fail;

      if (translationCount && !(vectorKeys[1] = calloc(translationCount, sizeof(glhckAnimationVectorKey))))
         goto fail;

      /* VECTORKEY: scalingKeys[scalingCount] */
      /* VECTORKEY: translationKeys[scalingCount] */
      for (unsigned int k = 0; k < scalingCount + translationCount; ++k) {
         /* uint32_t: frame */
         unsigned int frame;
         if (chckBufferReadUInt32(buffer, &frame) != RETURN_OK)
            goto fail;

         /* VECTOR3: vector */
         glhckVector3f vector;
         if (chckBufferReadStringVector3(buffer, &vector) != RETURN_OK)
            goto fail;

         const unsigned int idx = (k >= scalingCount ? 1 : 0);
         const unsigned int v = (k >= scalingCount ? k - scalingCount : k);
         vectorKeys[idx][v].time = frame;
         vectorKeys[idx][v].vector.x = vector.x;
         vectorKeys[idx][v].vector.y = vector.y;
         vectorKeys[idx][v].vector.z = vector.z;
      }

      nodes[i].rotationKeys = quaternionKeys;
      nodes[i].scalingKeys = vectorKeys[0];
      nodes[i].translationKeys = vectorKeys[1];
      nodes[i].rotationCount = rotationCount;
      nodes[i].scalingCount = scalingCount;
      nodes[i].translationCount = translationCount;
      nname = NULL;
   }

   animation.nodes = nodes;
   animation.nodeCount = nodeCount;

   if (!chckIterPoolAdd(and->animations, &animation, NULL))
      goto fail;

   return RETURN_OK;

fail:
   IFDO(free, quaternionKeys);
   IFDO(free, vectorKeys[0]);
   IFDO(free, vectorKeys[1]);
   IFDO(free, nodes);
   IFDO(free, name);
   IFDO(free, nname);
   return RETURN_FAIL;
}

/* \brief compare file header */
static int cmpHeader(chckBuffer *buffer, const char *header, uint8_t *outVersion)
{
   uint8_t version[2];
   assert(buffer && header);

   if (outVersion)
      memset(outVersion, 0, sizeof(version));

   char *hdr;
   if (!(hdr = calloc(1, strlen(header))))
      goto fail;

   chckBufferRead(hdr, 1, strlen(header), buffer);
   if (memcmp(hdr, header, strlen(header)) != 0)
      goto fail;

   NULLDO(free, hdr);

   chckBufferRead(version, 1, 2, buffer);
   if ((version[0] > GLHCKM_VERSION_NEWEST[0]  ||
        version[1] > GLHCKM_VERSION_NEWEST[1]) ||
       (version[0] < GLHCKM_VERSION_OLDEST[0]  ||
        version[1] < GLHCKM_VERSION_OLDEST[1]))
      goto version_not_supported;

   if (outVersion)
      memcpy(outVersion, version, sizeof(version));

   return RETURN_OK;

version_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "%s version (%u.%u) not supported", header, version[0], version[1]);
fail:
   IFDO(free, buffer);
   return 0;
}

/* \brief check if file is a either glhcka or glhckm file */
static int check(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);
   assert(buffer);
   int ret = cmpHeader(buffer, GLHCKM_HEADER, NULL);
   RET(0, "%d", ret);
   return ret;
}

static int import(chckBuffer *buffer, glhckImportModelStruct *import)
{
   CALL(0, "%p, %p", buffer, import);
   assert(buffer && import);

   uint8_t version[2];
   if (!cmpHeader(buffer, GLHCKM_HEADER, version))
      goto fail;

   struct BND bnd;
   struct OBD obd;
   struct AND and;
   memset(&bnd, 0, sizeof(bnd));
   memset(&obd, 0, sizeof(obd));
   memset(&and, 0, sizeof(and));

   char block[3];
   int ret = RETURN_OK;
   uint32_t ANH = 0, BNH = 0, OBH = 0;
   while (ret == RETURN_OK && chckBufferRead(block, 1, sizeof(block), buffer) == sizeof(block)) {
      if (block[0] == 'B' && block[1] == 'N') {
         if (block[2] == 'H') ret = readHeader(version, buffer, &BNH);
         if (block[2] == 'D') ret = readBND(version, buffer, &bnd);
      }

      if (block[0] == 'O' && block[1] == 'B') {
         if (block[2] == 'H') ret = readHeader(version, buffer, &OBH);
         if (block[2] == 'D') ret = readOBD(version, buffer, &obd, 0);
      }

      if (block[0] == 'A' && block[1] == 'N') {
         if (block[2] == 'H') ret = readHeader(version, buffer, &ANH);
         if (block[2] == 'D') ret = readAND(version, buffer, &and);
      }
   }

   if (ret != RETURN_OK)
      goto fail;

   if (bnd.bones)
      import->bones = chckIterPoolToCArray(bnd.bones, &import->bonesCount);
   if (obd.materials)
      import->materials = chckIterPoolToCArray(obd.materials, &import->materialCount);
   if (obd.skins)
      import->skins = chckIterPoolToCArray(obd.skins, &import->skinCount);
   if (obd.meshes)
      import->meshes = chckIterPoolToCArray(obd.meshes, &import->meshCount);
   if (and.animations)
      import->animations = chckIterPoolToCArray(and.animations, &import->animationCount);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->modelImport = import;

   RET(0, "%s", "glhck-importer-glhck");
   return "glhck-importer-glhck";
}

/* vim: set ts=8 sw=3 tw=0 :*/
