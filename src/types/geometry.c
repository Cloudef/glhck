#include "geometry.h"
#include "renderers/renderer.h"

#include <limits.h> /* for type limits */

#include "trace.h"
#include "handle.h"
#include "system/tls.h"

#ifndef __STRING
#  define __STRING(x) #x
#endif

/* assign 2D v2 vector to v1 vector regardless of datatype */
#define glhckSetV2(v1, v2) \
   (v1)->x = (v2)->x;      \
   (v1)->y = (v2)->y

/* assign 3D v2 vector to v1 vector regardless of datatype */
#define glhckSetV3(v1, v2) \
   glhckSetV2(v1, v2);     \
   (v1)->z = (v2)->z

/* assign the max units from vectors to v1 */
#define glhckMaxV2(v1, v2) \
   if ((v1)->x < (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y < (v2)->y) (v1)->y = (v2)->y
#define glhckMaxV3(v1, v2) \
   glhckMaxV2(v1, v2);     \
   if ((v1)->z < (v2)->z) (v1)->z = (v2)->z

/* assign the min units from vectors to v1 */
#define glhckMinV2(v1, v2) \
   if ((v1)->x > (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y > (v2)->y) (v1)->y = (v2)->y
#define glhckMinV3(v1, v2) \
   glhckMinV2(v1, v2);     \
   if ((v1)->z > (v2)->z) (v1)->z = (v2)->z

#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

enum pool {
   $geometry, // glhckGeometry
   $pose, // struct glhckPose
   POOL_LAST
};

static size_t pool_sizes[POOL_LAST] = {
   sizeof(glhckGeometry),
   sizeof(struct glhckPose)
};

static _GLHCK_TLS chckPool *pools[POOL_LAST];

static _GLHCK_TLS chckIterPool *indexTypes;
static _GLHCK_TLS chckIterPool *vertexTypes;

#include "handlefun.h"

/* This file contains all the methods that access
 * object's internal vertexdata, since the GLHCK object's
 * internal vertexdata structure is quite complex */

/* \brief get attributes for glhckDataType */
static void dataTypeAttributes(const glhckDataType dataType, unsigned char *size, size_t *max, char *normalized)
{
   switch (dataType) {
      case GLHCK_BYTE:
         if (max) *max = CHAR_MAX;
         if (size) *size = sizeof(char);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_UNSIGNED_BYTE:
         if (max) *max = UCHAR_MAX;
         if (size) *size = sizeof(unsigned char);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_SHORT:
         if (max) *max = SHRT_MAX;
         if (size) *size = sizeof(short);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_UNSIGNED_SHORT:
         if (max) *max = USHRT_MAX;
         if (size) *size = sizeof(unsigned short);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_INT:
         if (max) *max = INT_MAX;
         if (size) *size = sizeof(int);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_UNSIGNED_INT:
         if (max) *max = UINT_MAX;
         if (size) *size = sizeof(unsigned int);
         if (normalized) *normalized = 1;
         return;
      case GLHCK_FLOAT:
         if (max) *max = 1;
         if (size) *size = sizeof(float);
         if (normalized) *normalized = 0;
         return;
      default:break;
   }

   assert(0 && "INVALID DATATYPE");
}

/* \brief check for valid vertex type */
static glhckVertexType checkVertexType(const glhckVertexType type)
{
   glhckIndexType ntype = type;

   /* default to V3F on AUTO type */
   if (ntype == GLHCK_VTX_AUTO)
      ntype = GLHCK_VTX_V3F;

   /* fglrx and SGX on linux at least seems to fail with byte vertices.
    * workaround for now until I stumble upon why it's wrong. */
   if (glhckRendererGetFeatures()->driver != GLHCK_DRIVER_NVIDIA) {
      static unsigned char warned_once = 0;
      if (!warned_once && (type == GLHCK_VTX_V2B || type == GLHCK_VTX_V3B)) {
         DEBUG(GLHCK_DBG_WARNING, "Some drivers have problems with BYTE precision vertexdata.");
         DEBUG(GLHCK_DBG_WARNING, "Will use SHORT precision instead, just a friendly warning.");
         warned_once = 1;
      }
      if (ntype == GLHCK_VTX_V3B) ntype = GLHCK_VTX_V3S;
      if (ntype == GLHCK_VTX_V2B) ntype = GLHCK_VTX_V2S;
   }

   return ntype;
}

/* \brief check for valid index type  */
static glhckIndexType checkIndexType(const glhckIndexType type, const glhckImportIndexData *indices, const int memb)
{
#if EMSCRIPTEN
   /* emscripten works with USHRT indices atm only */
   static unsigned char warned_once = 0;
   if (!warned_once) {
      DEBUG(GLHCK_DBG_WARNING, "Emscripten currently works only with USHRT indices, forcing that!");
      warned_once = 1;
   }
   return GLHCK_IDX_USHRT;
#endif

   /* use user specific indices */
   if (type != GLHCK_IDX_AUTO)
      return type;

   /* autodetect */
   glhckImportIndexData max = 0;
   for (size_t i = 0; (int)i < memb; ++i) if (max < indices[i]) max = indices[i];

   glhckIndexType ntype = GLHCK_IDX_UINT;
   for (size_t i = 0; i < chckIterPoolCount(indexTypes); ++i) {
      const struct glhckIndexType *it = _glhckGeometryGetIndexType(i);
      if (it->max >= max && it->max < it->max)
         ntype = i;
   }

   return ntype;
}

/* \brief get max && min for import vertex data */
static void importVertexDataMaxMin(const glhckImportVertexData *import, int memb, glhckVector3f *vmin, glhckVector3f *vmax)
{
   assert(import && vmin && vmax);

   memcpy(vmin, &import[0].vertex, sizeof(glhckVector3f));
   memcpy(vmax, &import[0].vertex, sizeof(glhckVector3f));

   for (int i = 0; i < memb; ++i) {
      glhckMaxV3(vmax, &import[i].vertex);
      glhckMinV3(vmin, &import[i].vertex);
   }

   /* lie about bounds by 1 point so, we don't get artifacts */
   vmax->x++; vmax->y++; vmax->z++;
   vmin->x--; vmin->y--; vmin->z--;
}

static void transformV2B(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData3b *internal = geometry->vertices;

   kmMat4 bias, biasinv, scale, scaleinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle bone = glhckSkinBoneGetBone(skinBones[i]);
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, glhckBoneGetTransformedMatrix(bone));
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, glhckSkinBoneGetOffsetMatrix(skinBones[i]), &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      size_t numWeightsWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeightsWeights);
      for (size_t w = 0; w < numWeightsWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData2b *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData2b);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
      }
   }
}

static void transformV3B(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData3b *internal = geometry->vertices;

   kmMat4 bias, biasinv, scale, scaleinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle bone = glhckSkinBoneGetBone(skinBones[i]);
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, glhckBoneGetTransformedMatrix(bone));
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, glhckSkinBoneGetOffsetMatrix(skinBones[i]), &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      size_t numWeightsWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeightsWeights);
      for (size_t w = 0; w < numWeightsWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData3b *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData3b);
         glhckSetV3(&v1, &bdata->vertex);
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.z += v1.z * weights[w].weight;
      }
   }
}

static void transformV2S(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData2s *internal = geometry->vertices;

   kmMat4 bias, biasinv, scale, scaleinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle bone = glhckSkinBoneGetBone(skinBones[i]);
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, glhckBoneGetTransformedMatrix(bone));
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, glhckSkinBoneGetOffsetMatrix(skinBones[i]), &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      size_t numWeightsWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeightsWeights);
      for (size_t w = 0; w < numWeightsWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData2s *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData2s);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
      }
   }
}

static void transformV3S(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData3s *internal = geometry->vertices;

   kmMat4 bias, biasinv, scale, scaleinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle bone = glhckSkinBoneGetBone(skinBones[i]);
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, glhckBoneGetTransformedMatrix(bone));
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, glhckSkinBoneGetOffsetMatrix(skinBones[i]), &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      size_t numWeightsWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeightsWeights);
      for (size_t w = 0; w < numWeightsWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData3s *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData3s);
         glhckSetV3(&v1, &bdata->vertex);
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.z += v1.z * weights[w].weight;
      }
   }
}

static void transformV2F(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData2f *internal = geometry->vertices;

   /* scale is always 1.0f, we can skip it */
   kmMat4 bias, biasinv;
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Inverse(&biasinv, &bias);

   for (size_t i = 0; i < memb; ++i) {
      const glhckHandle bone = glhckSkinBoneGetBone(skinBones[i]);
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, glhckBoneGetTransformedMatrix(bone));
      kmMat4Multiply(&offsetMatrix, glhckSkinBoneGetOffsetMatrix(skinBones[i]), &bias);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      size_t numWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeights);
      for (size_t w = 0; w < numWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData2f *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData2f);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
      }
   }
}

static void transformV3F(glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb)
{
   glhckVertexData3f *internal = geometry->vertices;

   for (size_t i = 0; i < memb; ++i) {
      size_t numWeights;
      const glhckVertexWeight *weights = glhckSkinBoneWeights(skinBones[i], &numWeights);
      const kmMat4 *poseMatrix = glhckBoneGetPoseMatrix(glhckSkinBoneGetBone(skinBones[i]));
      for (size_t w = 0; w < numWeights; ++w) {
         kmVec3 v1;
         const glhckVertexData3f *bdata = bindPose + weights[w].vertexIndex * sizeof(glhckVertexData3f);
         kmVec3MultiplyMat4(&v1, (kmVec3*)&bdata->vertex, poseMatrix);
         internal[weights[w].vertexIndex].vertex.x += v1.x * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.y += v1.y * weights[w].weight;
         internal[weights[w].vertexIndex].vertex.z += v1.z * weights[w].weight;
      }
   }
}

/* \brief get min && max of V2B vertices */
static void minMaxV2B(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData2b *internal = geometry->vertices;
   glhckSetV2(outMax, &internal[0].vertex);
   glhckSetV2(outMin, &internal[0].vertex);

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(outMax, &internal[i].vertex);
      glhckMinV2(outMin, &internal[i].vertex);
   }
}

/* \brief get min && max of V3B vertices */
static void minMaxV3B(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData3b *internal = geometry->vertices;
   glhckSetV3(outMax, &internal[0].vertex);
   glhckSetV3(outMin, &internal[0].vertex);

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(outMax, &internal[i].vertex);
      glhckMinV3(outMin, &internal[i].vertex);
   }
}

/* \brief get min && max of V2S vertices */
static void minMaxV2S(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData2s *internal = geometry->vertices;
   glhckSetV2(outMax, &internal[0].vertex);
   glhckSetV2(outMin, &internal[0].vertex);

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(outMax, &internal[i].vertex);
      glhckMinV2(outMin, &internal[i].vertex);
   }
}

/* \brief get min && max of V3S vertices */
static void minMaxV3S(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData3s *internal = geometry->vertices;
   glhckSetV3(outMax, &internal[0].vertex);
   glhckSetV3(outMin, &internal[0].vertex);

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(outMax, &internal[i].vertex);
      glhckMinV3(outMin, &internal[i].vertex);
   }
}

/* \brief get min && max of V2F vertices */
static void minMaxV2F(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData2f *internal = geometry->vertices;
   glhckSetV2(outMax, &internal[0].vertex);
   glhckSetV2(outMin, &internal[0].vertex);

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(outMax, &internal[i].vertex);
      glhckMinV2(outMin, &internal[i].vertex);
   }
}

/* \brief get min && max of V3F vertices */
static void minMaxV3F(const glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax)
{
   const glhckVertexData3f *internal = geometry->vertices;
   memcpy(outMin, &internal[0].vertex, sizeof(glhckVector3f));
   memcpy(outMax, &internal[0].vertex, sizeof(glhckVector3f));

   for (int i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(outMax, &internal[i].vertex);
      glhckMinV3(outMin, &internal[i].vertex);
   }
}

/* \brief convert import vertex data to V2B */
static void convertV2B(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   const float srange = UCHAR_MAX - 1.0f;
   const float brange = CHAR_MAX / srange;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      glhckVertexData2b *internal = out;
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      internal[i].color = import[i].color;
   }

   /* fix outBias after centering */
   outBias->x = brange * (vmax.x - vmin.x) + vmin.x;
   outBias->y = brange * (vmax.y - vmin.y) + vmin.y;
   outBias->z = brange * (vmax.z - vmin.z) + vmin.z;
   outScale->x = (vmax.x - vmin.x) / srange;
   outScale->y = (vmax.y - vmin.y) / srange;
   outScale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V3B */
static void convertV3B(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   const float srange = UCHAR_MAX - 1.0f;
   const float brange = CHAR_MAX / srange;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      glhckVertexData3b *internal = out;
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.z = floorf(((import[i].vertex.z - vmin.z) / (vmax.z - vmin.z)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      internal[i].color = import[i].color;
   }

   /* fix outBias after centering */
   outBias->x = brange * (vmax.x - vmin.x) + vmin.x;
   outBias->y = brange * (vmax.y - vmin.y) + vmin.y;
   outBias->z = brange * (vmax.z - vmin.z) + vmin.z;
   outScale->x = (vmax.x - vmin.x) / srange;
   outScale->y = (vmax.y - vmin.y) / srange;
   outScale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V2S */
static void convertV2S(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   const float srange = USHRT_MAX - 1.0f;
   const float brange = SHRT_MAX / srange;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      glhckVertexData2s *internal = out;
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      internal[i].color = import[i].color;
   }

   /* fix outBias after centering */
   outBias->x = brange * (vmax.x - vmin.x) + vmin.x;
   outBias->y = brange * (vmax.y - vmin.y) + vmin.y;
   outBias->z = brange * (vmax.z - vmin.z) + vmin.z;
   outScale->x = (vmax.x - vmin.x) / srange;
   outScale->y = (vmax.y - vmin.y) / srange;
   outScale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V3S */
static void convertV3S(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   const float srange = USHRT_MAX - 1.0f;
   const float brange = SHRT_MAX / srange;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      glhckVertexData3s *internal = out;
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.z = floorf(((import[i].vertex.z - vmin.z) / (vmax.z - vmin.z)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      internal[i].color = import[i].color;
   }

   /* fix outBias after centering */
   outBias->x = brange * (vmax.x - vmin.x) + vmin.x;
   outBias->y = brange * (vmax.y - vmin.y) + vmin.y;
   outBias->z = brange * (vmax.z - vmin.z) + vmin.z;
   outScale->x = (vmax.x - vmin.x) / srange;
   outScale->y = (vmax.y - vmin.y) / srange;
   outScale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V2F */
static void convertV2F(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      glhckVertexData2f *internal = out;
      memcpy(&internal[i].normal, &import[i].normal, sizeof(glhckVector3f));
      memcpy(&internal[i].coord, &import[i].coord, sizeof(glhckVector2f));
      internal[i].color = import[i].color;
      internal[i].vertex.x = import[i].vertex.x + 0.5f * (vmax.x - vmin.x) + vmin.x;
      internal[i].vertex.y = import[i].vertex.y + 0.5f * (vmax.y - vmin.y) + vmin.y;
   }

   /* fix bias after centering */
   outBias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
   outBias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
   outBias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
}

/* \brief convert import vertex data to V3F */
static void convertV3F(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale)
{
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, outBias, outScale);
   assert(sizeof(glhckImportVertexData) == sizeof(glhckVertexData3f));

   glhckVector3f vmin, vmax;
   importVertexDataMaxMin(import, memb, &vmin, &vmax);

   glhckVertexData3f *internal = out;
   memcpy(internal, import, memb * sizeof(glhckVertexData3f));

   /* center geometry */
   for (int i = 0; i < memb; ++i) {
      internal[i].vertex.x -= 0.5f * (vmax.x - vmin.x) + vmin.x;
      internal[i].vertex.y -= 0.5f * (vmax.y - vmin.y) + vmin.y;
      internal[i].vertex.z -= 0.5f * (vmax.z - vmin.z) + vmin.z;
   }

   /* fix bias after centering */
   outBias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
   outBias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
   outBias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
}

/* \brief convert import index data to IUB */
static void convertIUB(const glhckImportIndexData *import, const int memb, void *out)
{
   CALL(0, "%p, %d, %p", import, memb, out);
   unsigned char *internal = out;
   for (int i = 0; i < memb; ++i) internal[i] = import[i];
}

/* \brief convert import index data to IUS */
static void convertIUS(const glhckImportIndexData *import, const int memb, void *out)
{
   CALL(0, "%p, %d, %p", import, memb, out);
   unsigned short *internal = out;
   for (int i = 0; i < memb; ++i) internal[i] = import[i];
}

/* \brief convert import index data to IUI */
static void convertIUI(const glhckImportIndexData *import, const int memb, void *out)
{
   CALL(0, "%p, %d, %p", import, memb, out);
   assert(sizeof(glhckImportIndexData) == sizeof(unsigned int));
   memcpy(out, import, memb * sizeof(unsigned int));
}

#define GLHCK_API_CHECK(x) \
if (!api->x) DEBUG(GLHCK_DBG_ERROR, "-!- \1missing geometry API function: %s", __STRING(x))

/* \brief add new internal vertex type */
int glhckGeometryAddVertexType(const glhckVertexTypeFunctionMap *api, const glhckDataType dataType[4], const char memb[4], const size_t offset[4], const size_t size)
{
   CALL(0, "%p, %p, %zu", dataType, memb, size);

   struct glhckVertexType *vertexType;
   for (chckPoolIndex iter = 0; (vertexType = chckIterPoolIter(vertexTypes, &iter));) {
      if (!memcmp(vertexType->dataType, dataType, 4 * sizeof(glhckDataType)) &&
          !memcmp(vertexType->memb, memb, 4)) {
         DEBUG(GLHCK_DBG_WARNING, "This vertexType already exists!");
         RET(0, "%zu", iter - 1);
         return iter - 1;
      }
   }

   GLHCK_API_CHECK(convert);
   GLHCK_API_CHECK(minMax);
   GLHCK_API_CHECK(transform);

   if (!(vertexType = chckIterPoolAdd(vertexTypes, NULL, NULL)))
      goto fail;

   for (unsigned int i = 0; i < 4; ++i) dataTypeAttributes(dataType[i], &vertexType->msize[i], &vertexType->max[i], &vertexType->normalized[i]);
   vertexType->normalized[0] = 0; /* vertices are never normalized */
   memcpy(&vertexType->api, api, sizeof(glhckVertexTypeFunctionMap));
   memcpy(vertexType->offset, offset, 4 * sizeof(size_t));
   memcpy(vertexType->dataType, dataType, 4 * sizeof(glhckDataType));
   memcpy(vertexType->memb, memb, 4);
   vertexType->size = size;

   int i = chckIterPoolCount(vertexTypes) - 1;
   RET(0, "%d", i);
   return i;

fail:
   RET(0, "%d", -1);
   return -1;
}

/* \brief add new internal index type */
int glhckGeometryAddIndexType(const glhckIndexTypeFunctionMap *api, const glhckDataType dataType)
{
   CALL(0, "%d", dataType);

   GLHCK_API_CHECK(convert);

   struct glhckIndexType *indexType;
   for (chckPoolIndex iter = 0; (indexType = chckIterPoolIter(indexTypes, &iter));) {
      if (indexType->dataType == dataType) {
         DEBUG(GLHCK_DBG_WARNING, "This indexType already exists!");
         RET(0, "%zu", iter - 1);
         return iter - 1;
      }
   }

   if (!(indexType = chckIterPoolAdd(indexTypes, NULL, NULL)))
      goto fail;

   dataTypeAttributes(dataType, &indexType->size, &indexType->max, NULL);
   memcpy(&indexType->api, api, sizeof(glhckIndexTypeFunctionMap));
   indexType->dataType = dataType;

   int i =  chckIterPoolCount(indexTypes) - 1;
   RET(0, "%d", i);
   return i;

fail:
   RET(0, "%d", -1);
   return -1;
}

#undef GLHCK_API_CHECK

/* \brief init internal vertex/index types */
int _glhckGeometryInit(void)
{
   TRACE(0);

   indexTypes = chckIterPoolNew(8, 8, sizeof(struct glhckIndexType));
   vertexTypes = chckIterPoolNew(8, 8, sizeof(struct glhckVertexType));

   /* UINT */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = convertIUI;
      if (glhckGeometryAddIndexType(&map, GLHCK_UNSIGNED_INT) != GLHCK_IDX_UINT)
         goto fail;
   }

   /* USHRT */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = convertIUS;
      if (glhckGeometryAddIndexType(&map, GLHCK_UNSIGNED_SHORT) != GLHCK_IDX_USHRT)
         goto fail;
   }

   /* UBYTE */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = convertIUB;
      if (glhckGeometryAddIndexType(&map, GLHCK_UNSIGNED_BYTE) != GLHCK_IDX_UBYTE)
         goto fail;
   }

   /* V3F */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData3f, vertex),
         offsetof(glhckVertexData3f, normal),
         offsetof(glhckVertexData3f, coord),
         offsetof(glhckVertexData3f, color)
      };
      char memb[4] = { 3, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_FLOAT, GLHCK_FLOAT, GLHCK_FLOAT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV3F;
      map.minMax = minMaxV3F;
      map.transform = transformV3F;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData3f)) != GLHCK_VTX_V3F)
         goto fail;
   }

   /* V2F */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData2f, vertex),
         offsetof(glhckVertexData2f, normal),
         offsetof(glhckVertexData2f, coord),
         offsetof(glhckVertexData2f, color)
      };
      char memb[4] = { 2, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_FLOAT, GLHCK_FLOAT, GLHCK_FLOAT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV2F;
      map.minMax = minMaxV2F;
      map.transform = transformV2F;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData2f)) != GLHCK_VTX_V2F)
         goto fail;
   }

   /* V3S */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData3s, vertex),
         offsetof(glhckVertexData3s, normal),
         offsetof(glhckVertexData3s, coord),
         offsetof(glhckVertexData3s, color)
      };
      char memb[4] = { 3, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_SHORT, GLHCK_SHORT, GLHCK_SHORT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV3S;
      map.minMax = minMaxV3S;
      map.transform = transformV3S;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData3s)) != GLHCK_VTX_V3S)
         goto fail;
   }

   /* V2S */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData2s, vertex),
         offsetof(glhckVertexData2s, normal),
         offsetof(glhckVertexData2s, coord),
         offsetof(glhckVertexData2s, color)
      };
      char memb[4] = { 2, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_SHORT, GLHCK_SHORT, GLHCK_SHORT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV2S;
      map.minMax = minMaxV2S;
      map.transform = transformV2S;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData2s)) != GLHCK_VTX_V2S)
         goto fail;
   }

   /* V3B */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData3b, vertex),
         offsetof(glhckVertexData3b, normal),
         offsetof(glhckVertexData3b, coord),
         offsetof(glhckVertexData3b, color)
      };
      char memb[4] = { 3, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_BYTE, GLHCK_SHORT, GLHCK_SHORT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV3B;
      map.minMax = minMaxV3B;
      map.transform = transformV3B;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData3b)) != GLHCK_VTX_V3B)
         goto fail;
   }

   /* V2B */
   {
      size_t offset[4] = {
         offsetof(glhckVertexData2b, vertex),
         offsetof(glhckVertexData2b, normal),
         offsetof(glhckVertexData2b, coord),
         offsetof(glhckVertexData2b, color)
      };
      char memb[4] = { 2, 3, 2, 4 };
      glhckDataType dataType[4] = { GLHCK_BYTE, GLHCK_SHORT, GLHCK_SHORT, GLHCK_UNSIGNED_BYTE };
      glhckVertexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckVertexTypeFunctionMap));
      map.convert = convertV2B;
      map.minMax = minMaxV2B;
      map.transform = transformV2B;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData2b)) != GLHCK_VTX_V2B)
         goto fail;
   }

   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   DEBUG(GLHCK_DBG_ERROR, "Not enough memory to allocate internal vertex types, going to fail now!");
   return RETURN_FAIL;
}

void _glhckGeometryTerminate(void)
{
   TRACE(0);
   IFDO(chckIterPoolFree, indexTypes);
   IFDO(chckIterPoolFree, vertexTypes);
}

const struct glhckVertexType* _glhckGeometryGetVertexType(size_t index)
{
   return chckIterPoolGet(vertexTypes, index);
}

const struct glhckIndexType* _glhckGeometryGetIndexType(size_t index)
{
   return chckIterPoolGet(indexTypes, index);
}

static void geometryFreeVertices(glhckGeometry *geometry)
{
   IFDO(free, geometry->vertices);
   geometry->vertexType = GLHCK_VTX_AUTO;
   geometry->vertexCount = 0;
   geometry->textureRange = 1;
}

static void geometrySetVertices(glhckGeometry *geometry, const glhckVertexType type, void *data, const int memb)
{
   geometryFreeVertices(geometry);
   geometry->vertices = data;
   geometry->vertexType = type;
   geometry->vertexCount = memb;

   const struct glhckVertexType *vt = _glhckGeometryGetVertexType(type);
   geometry->textureRange = vt->max[2];
}

static void geometryFreeIndices(glhckGeometry *geometry)
{
   IFDO(free, geometry->indices);
   geometry->indexType = GLHCK_IDX_AUTO;
   geometry->indexCount = 0;
}

static void geometrySetIndices(glhckGeometry *geometry, const glhckIndexType type, void *data, const int memb)
{
   geometryFreeIndices(geometry);
   geometry->indices = data;
   geometry->indexType = type;
   geometry->indexCount = memb;
}

static int geometryInsertVertices(glhckGeometry *geometry, const glhckVertexType type, const glhckImportVertexData *vertices, const int memb)
{
   void *data = NULL;
   CALL(0, "%p, %u, %p, %d", geometry, type, vertices, memb);
   assert(geometry);

   /* check vertex type */
   const glhckVertexType vtype = checkVertexType(type);

   /* check vertex precision conflicts */
   const struct glhckIndexType *it = _glhckGeometryGetIndexType(geometry->indexType);
   if (geometry->indexType != GLHCK_IDX_AUTO && (glhckImportIndexData)memb > it->max)
      goto bad_precision;

   /* allocate vertex data */
   const struct glhckVertexType *vt = _glhckGeometryGetVertexType(vtype);
   if (!(data = malloc(memb * vt->size)))
      goto fail;

   /* convert and assign */
   memset(&geometry->bias, 0, sizeof(glhckVector3f));
   geometry->scale.x = geometry->scale.y = geometry->scale.z = 1.0f;
   vt->api.convert(vertices, memb, data, &geometry->bias, &geometry->scale);
   geometrySetVertices(geometry, vtype, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %zu, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                           it->max,
                           geometry);
fail:
   IFDO(free, data);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static int geometryInsertIndices(glhckGeometry *geometry, const glhckIndexType type, const glhckImportIndexData *indices, const int memb)
{
   void *data = NULL;
   CALL(0, "%p, %u, %p, %d", geometry, type, indices, memb);
   assert(geometry);

   /* check index type */
   const glhckIndexType itype = checkIndexType(type, indices, memb);

   /* check vertex precision conflicts */
   const struct glhckIndexType *it = _glhckGeometryGetIndexType(itype);
   if ((glhckImportIndexData)geometry->vertexCount > it->max)
      goto bad_precision;

   /* allocate index data */
   if (!(data = malloc(memb * it->size)))
      goto fail;

   /* convert and assign */
   it->api.convert(indices, memb, data);
   geometrySetIndices(geometry, itype, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %zu, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          it->max,
                          geometry);
fail:
   IFDO(free, data);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI glhckGeometry* glhckGeometryGetStruct(const glhckHandle handle)
{
   return (glhckGeometry*)get($geometry, handle);
}

GLHCKAPI struct glhckPose* glhckGeometryGetPose(const glhckHandle handle)
{
   return (struct glhckPose*)get($pose, handle);
}

const struct glhckVertexType* geometryVertexType(const glhckHandle handle)
{
   const glhckGeometry *geometry = glhckGeometryGetStruct(handle);
   return chckIterPoolGet(vertexTypes, geometry->vertexType);
}

static void destructor(const glhckHandle handle)
{
   CALL(0, "%s", glhckHandleRepr(handle));
   assert(handle > 0);

   glhckGeometry *geometry = glhckGeometryGetStruct(handle);
   geometryFreeVertices(geometry);
   geometryFreeIndices(geometry);
}

GLHCKAPI glhckHandle glhckGeometryNew(void)
{
   TRACE(0);

   glhckHandle handle = 0;
   if (!(handle = _glhckInternalHandleCreateFrom(GLHCK_TYPE_GEOMETRY, pools, pool_sizes, POOL_LAST, destructor)))
      goto fail;

   glhckGeometry *geometry = glhckGeometryGetStruct(handle);
   geometry->type = GLHCK_TRIANGLE_STRIP;
   geometry->scale.x = geometry->scale.y = geometry->scale.z = 1.0f;

   RET(0, "%s", glhckHandleRepr(handle));
   return handle;

fail:
   RET(0, "%s", glhckHandleRepr(0));
   return 0;
}

GLHCKAPI void glhckGeometryCalculateBB(const glhckHandle handle, kmAABB *outAABB)
{
   CALL(2, "%s, %p", glhckHandleRepr(handle), outAABB);
   assert(handle > 0 && outAABB);

   glhckGeometry *geometry = glhckGeometryGetStruct(handle);
   geometryVertexType(handle)->api.minMax(geometry, (glhckVector3f*)&outAABB->min, (glhckVector3f*)&outAABB->max);
}

GLHCKAPI int glhckGeometryInsertIndices(const glhckHandle handle, const glhckIndexType type, const void *data, const int memb)
{
   CALL(0, "%s, %u, %p, %d", glhckHandleRepr(handle), type, data, memb);
   assert(handle > 0);

   glhckGeometry *geometry = glhckGeometryGetStruct(handle);

   if (data) {
      if (geometryInsertIndices(geometry, type, data, memb) != RETURN_OK)
         goto fail;
   } else {
      geometryFreeIndices(geometry);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI int glhckGeometryInsertVertices(const glhckHandle handle, const glhckVertexType type, const void *data, const int memb)
{
   CALL(0, "%s, %u, %p, %d", glhckHandleRepr(handle), type, data, memb);
   assert(handle > 0);

   glhckGeometry *geometry = glhckGeometryGetStruct(handle);

   if (data) {
      if (geometryInsertVertices(geometry, type, data, memb) != RETURN_OK)
         goto fail;

      struct glhckPose *pose = glhckGeometryGetPose(handle);
      const struct glhckVertexType *vt = geometryVertexType(handle);
      if (pose->bind) free(pose->bind);
      pose->bind = malloc(geometry->vertexCount * vt->size);
      memcpy(pose->bind, geometry->vertices, geometry->vertexCount * vt->size);
      if (pose->zero) free(pose->zero);
      pose->zero = malloc(geometry->vertexCount * vt->size);
      memcpy(pose->zero, geometry->vertices, geometry->vertexCount * vt->size);
      for (int v = 0; v < geometry->vertexCount; ++v)
         memset(pose->zero + vt->size * v + vt->offset[0], 0, vt->msize[0] * vt->memb[0]);

   } else {
      geometryFreeVertices(geometry);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
