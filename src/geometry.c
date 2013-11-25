#include "internal.h"
#include <limits.h> /* for type limits */

#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* This file contains all the methods that access
 * object's internal vertexdata, since the GLHCK object's
 * internal vertexdata structure is quite complex */

/* \brief get attributes for glhckDataType */
static void _glhckDataTypeAttributes(glhckDataType dataType, unsigned char *size, size_t *max, char *normalized)
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
static unsigned char _glhckGeometryCheckVertexType(unsigned char type)
{
   /* default to V3F on AUTO type */
   if (type == GLHCK_VTX_AUTO) type = GLHCK_VTX_V3F;

   /* fglrx and SGX on linux at least seems to fail with byte vertices.
    * workaround for now until I stumble upon why it's wrong. */
   if (GLHCKR()->driver != GLHCK_DRIVER_NVIDIA) {
      if (type == GLHCK_VTX_V2B || type == GLHCK_VTX_V3B) {
         DEBUG(GLHCK_DBG_WARNING, "Some drivers have problems with BYTE precision vertexdata.");
         DEBUG(GLHCK_DBG_WARNING, "Will use SHORT precision instead, just a friendly warning.");
      }
      if (type == GLHCK_VTX_V3B) type = GLHCK_VTX_V3S;
      if (type == GLHCK_VTX_V2B) type = GLHCK_VTX_V2S;
   }
   return type;
}

/* \brief check for valid index type  */
static unsigned char _glhckGeometryCheckIndexType(unsigned char type, const glhckImportIndexData *indices, int memb)
{
   int i;
   glhckImportIndexData max = 0;
   if (type != GLHCK_IDX_AUTO) return type;

   /* autodetect */
   for (i = 0; i < memb; ++i) if (max < indices[i]) max = indices[i];
   for (type = GLHCK_IDX_UINT, i = 0; i < GLHCKW()->numIndexTypes; ++i) {
      if (GLHCKIT(i)->max >= max && GLHCKIT(i)->max < GLHCKIT(type)->max)
         type = i;
   }
   return type;
}

/* \brief get max && min for import vertex data */
static void _glhckImportVertexDataMaxMin(const glhckImportVertexData *import, int memb, glhckVector3f *vmin, glhckVector3f *vmax)
{
   int i;
   assert(import && vmin && vmax);

   memcpy(vmin, &import[0].vertex, sizeof(glhckVector3f));
   memcpy(vmax, &import[0].vertex, sizeof(glhckVector3f));
   for (i = 1; i < memb; ++i) {
      glhckMaxV3(vmax, &import[i].vertex);
      glhckMinV3(vmin, &import[i].vertex);
   }

   /* lie about bounds by 1 point so, we don't get artifacts */
   vmax->x++; vmax->y++; vmax->z++;
   vmin->x--; vmin->y--; vmin->z--;
}

/* \brief transform V2B */
static void _glhckGeometryTransformV2B(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   glhckVertexData3b *internal = geometry->vertices;

   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData2b *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData2b);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
      }
   }
}

/* \brief transform V3B */
static void _glhckGeometryTransformV3B(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   glhckVertexData3b *internal = geometry->vertices;

   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData3b *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData3b);
         glhckSetV3(&v1, &bdata->vertex);
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
         internal[weight->vertexIndex].vertex.z += v1.z * weight->weight;
      }
   }
}

/* \brief transform V2S */
static void _glhckGeometryTransformV2S(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   glhckVertexData2s *internal = geometry->vertices;

   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData2s *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData2s);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
      }
   }
}

/* \brief transform V3S */
static void _glhckGeometryTransformV3S(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv, scale, scaleinv;
   glhckVertexData3s *internal = geometry->vertices;

   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Scaling(&scale, geometry->scale.x, geometry->scale.y, geometry->scale.z);
   kmMat4Inverse(&biasinv, &bias);
   kmMat4Inverse(&scaleinv, &scale);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&transformedMatrix, &scaleinv, &transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&offsetMatrix, &offsetMatrix, &scale);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData3s *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData3s);
         glhckSetV3(&v1, &bdata->vertex);
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
         internal[weight->vertexIndex].vertex.z += v1.z * weight->weight;
      }
   }
}

/* \brief transform V2F */
static void _glhckGeometryTransformV2F(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv;
   glhckVertexData2f *internal = geometry->vertices;

   /* scale is always 1.0f, we can skip it */
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Inverse(&biasinv, &bias);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData2f *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData2f);
         glhckSetV2(&v1, &bdata->vertex); v1.z = 0.0f;
         kmVec3MultiplyMat4(&v1, &v1, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
      }
   }
}

/* \brief transform V3F */
static void _glhckGeometryTransformV3F(glhckGeometry *geometry, const void *bindPose, glhckSkinBone **skinBones, unsigned int memb)
{
   unsigned int i, w;
   kmMat4 bias, biasinv;
   glhckVertexData3f *internal = geometry->vertices;

   /* scale is always 1.0f, we can skip it */
   kmMat4Translation(&bias, geometry->bias.x, geometry->bias.y, geometry->bias.z);
   kmMat4Inverse(&biasinv, &bias);

   for (i = 0; i < memb; ++i) {
      kmMat4 poseMatrix, transformedMatrix, offsetMatrix;
      kmMat4Multiply(&transformedMatrix, &biasinv, &skinBones[i]->bone->transformedMatrix);
      kmMat4Multiply(&offsetMatrix, &skinBones[i]->offsetMatrix, &bias);
      kmMat4Multiply(&poseMatrix, &transformedMatrix, &offsetMatrix);

      for (w = 0; w < skinBones[i]->numWeights; ++w) {
         kmVec3 v1;
         glhckVertexWeight *weight = &skinBones[i]->weights[w];
         const glhckVertexData3f *bdata = bindPose + weight->vertexIndex * sizeof(glhckVertexData3f);
         kmVec3MultiplyMat4(&v1, (kmVec3*)&bdata->vertex, &poseMatrix);
         internal[weight->vertexIndex].vertex.x += v1.x * weight->weight;
         internal[weight->vertexIndex].vertex.y += v1.y * weight->weight;
         internal[weight->vertexIndex].vertex.z += v1.z * weight->weight;
      }
   }
}

/* \brief get min && max of V2B vertices */
static void _glhckGeometryMinMaxV2B(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData2b *internal = geometry->vertices;
   glhckSetV2(max, &internal[0].vertex);
   glhckSetV2(min, &internal[0].vertex);
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(max, &internal[i].vertex);
      glhckMinV2(min, &internal[i].vertex);
   }
}

/* \brief get min && max of V3B vertices */
static void _glhckGeometryMinMaxV3B(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData3b *internal = geometry->vertices;
   glhckSetV3(max, &internal[0].vertex);
   glhckSetV3(min, &internal[0].vertex);
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(max, &internal[i].vertex);
      glhckMinV3(min, &internal[i].vertex);
   }
}

/* \brief get min && max of V2S vertices */
static void _glhckGeometryMinMaxV2S(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData2s *internal = geometry->vertices;
   glhckSetV2(max, &internal[0].vertex);
   glhckSetV2(min, &internal[0].vertex);
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(max, &internal[i].vertex);
      glhckMinV2(min, &internal[i].vertex);
   }
}

/* \brief get min && max of V3S vertices */
static void _glhckGeometryMinMaxV3S(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData3s *internal = geometry->vertices;
   glhckSetV3(max, &internal[0].vertex);
   glhckSetV3(min, &internal[0].vertex);
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(max, &internal[i].vertex);
      glhckMinV3(min, &internal[i].vertex);
   }
}

/* \brief get min && max of V2F vertices */
static void _glhckGeometryMinMaxV2F(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData2f *internal = geometry->vertices;
   glhckSetV2(max, &internal[0].vertex);
   glhckSetV2(min, &internal[0].vertex);
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV2(max, &internal[i].vertex);
      glhckMinV2(min, &internal[i].vertex);
   }
}

/* \brief get min && max of V3F vertices */
static void _glhckGeometryMinMaxV3F(glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max)
{
   int i = 0;
   glhckVertexData3f *internal = geometry->vertices;
   memcpy(min, &internal[0].vertex, sizeof(glhckVector3f));
   memcpy(max, &internal[0].vertex, sizeof(glhckVector3f));
   for (i = 1; i < geometry->vertexCount; ++i) {
      glhckMaxV3(max, &internal[i].vertex);
      glhckMinV3(min, &internal[i].vertex);
   }
}

/* \brief convert import vertex data to V2B */
static void _glhckGeometryConvertV2B(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   const float srange = UCHAR_MAX - 1.0f;
   const float brange = CHAR_MAX / srange;
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData2b *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      memcpy(&internal[i].color, &import[i].color, sizeof(glhckColorb));
   }

   /* fix bias after centering */
   bias->x = brange * (vmax.x - vmin.x) + vmin.x;
   bias->y = brange * (vmax.y - vmin.y) + vmin.y;
   bias->z = brange * (vmax.z - vmin.z) + vmin.z;
   scale->x = (vmax.x - vmin.x) / srange;
   scale->y = (vmax.y - vmin.y) / srange;
   scale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V3B */
static void _glhckGeometryConvertV3B(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   const float srange = UCHAR_MAX - 1.0f;
   const float brange = CHAR_MAX / srange;
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData3b *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].vertex.z = floorf(((import[i].vertex.z - vmin.z) / (vmax.z - vmin.z)) * UCHAR_MAX - CHAR_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      memcpy(&internal[i].color, &import[i].color, sizeof(glhckColorb));
   }

   /* fix bias after centering */
   bias->x = brange * (vmax.x - vmin.x) + vmin.x;
   bias->y = brange * (vmax.y - vmin.y) + vmin.y;
   bias->z = brange * (vmax.z - vmin.z) + vmin.z;
   scale->x = (vmax.x - vmin.x) / srange;
   scale->y = (vmax.y - vmin.y) / srange;
   scale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V2S */
static void _glhckGeometryConvertV2S(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   const float srange = USHRT_MAX - 1.0f;
   const float brange = SHRT_MAX / srange;
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData2s *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      memcpy(&internal[i].color, &import[i].color, sizeof(glhckColorb));
   }

   /* fix bias after centering */
   bias->x = brange * (vmax.x - vmin.x) + vmin.x;
   bias->y = brange * (vmax.y - vmin.y) + vmin.y;
   bias->z = brange * (vmax.z - vmin.z) + vmin.z;
   scale->x = (vmax.x - vmin.x) / srange;
   scale->y = (vmax.y - vmin.y) / srange;
   scale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V3S */
static void _glhckGeometryConvertV3S(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   const float srange = USHRT_MAX - 1.0f;
   const float brange = SHRT_MAX / srange;
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData3s *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      internal[i].vertex.x = floorf(((import[i].vertex.x - vmin.x) / (vmax.x - vmin.x)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.y = floorf(((import[i].vertex.y - vmin.y) / (vmax.y - vmin.y)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].vertex.z = floorf(((import[i].vertex.z - vmin.z) / (vmax.z - vmin.z)) * USHRT_MAX - SHRT_MAX + 0.5f);
      internal[i].normal.x = import[i].normal.x * SHRT_MAX;
      internal[i].normal.y = import[i].normal.y * SHRT_MAX;
      internal[i].normal.z = import[i].normal.z * SHRT_MAX;
      internal[i].coord.x = import[i].coord.x * SHRT_MAX;
      internal[i].coord.y = import[i].coord.y * SHRT_MAX;
      memcpy(&internal[i].color, &import[i].color, sizeof(glhckColorb));
   }

   /* fix bias after centering */
   bias->x = brange * (vmax.x - vmin.x) + vmin.x;
   bias->y = brange * (vmax.y - vmin.y) + vmin.y;
   bias->z = brange * (vmax.z - vmin.z) + vmin.z;
   scale->x = (vmax.x - vmin.x) / srange;
   scale->y = (vmax.y - vmin.y) / srange;
   scale->z = (vmax.z - vmin.z) / srange;
}

/* \brief convert import vertex data to V2F */
static void _glhckGeometryConvertV2F(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData2f *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      memcpy(&internal[i].normal, &import[i].normal, sizeof(glhckVector3f));
      memcpy(&internal[i].coord, &import[i].coord, sizeof(glhckVector2f));
      memcpy(&internal[i].color, &import[i].color, sizeof(glhckColorb));
      internal[i].vertex.x = import[i].vertex.x + 0.5f * (vmax.x - vmin.x) + vmin.x;
      internal[i].vertex.y = import[i].vertex.y + 0.5f * (vmax.y - vmin.y) + vmin.y;
   }

   /* fix bias after centering */
   bias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
   bias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
   bias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
}

/* \brief convert import vertex data to V3F */
static void _glhckGeometryConvertV3F(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale)
{
   int i;
   glhckVector3f vmin, vmax;
   glhckVertexData3f *internal;
   CALL(0, "%p, %d, %p %p, %p", import, memb, out, bias, scale);
   assert(sizeof(glhckImportVertexData) == sizeof(glhckVertexData3f));
   internal = out;

   _glhckImportVertexDataMaxMin(import, memb, &vmin, &vmax);
   memcpy(internal, import, memb * sizeof(glhckVertexData3f));

   /* center geometry */
   for (i = 0; i < memb; ++i) {
      internal[i].vertex.x -= 0.5f * (vmax.x - vmin.x) + vmin.x;
      internal[i].vertex.y -= 0.5f * (vmax.y - vmin.y) + vmin.y;
      internal[i].vertex.z -= 0.5f * (vmax.z - vmin.z) + vmin.z;
   }

   /* fix bias after centering */
   bias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
   bias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
   bias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
}

/* \brief convert import index data to IUB */
static void _glhckGeometryConvertIUB(const glhckImportIndexData *import, int memb, void *out)
{
   int i;
   unsigned char *internal;
   CALL(0, "%p, %d, %p", import, memb, out);
   internal = out;
   for (i = 0; i < memb; ++i) internal[i] = import[i];
}

/* \brief convert import index data to IUS */
static void _glhckGeometryConvertIUS(const glhckImportIndexData *import, int memb, void *out)
{
   int i;
   unsigned short *internal;
   CALL(0, "%p, %d, %p", import, memb, out);
   internal = out;
   for (i = 0; i < memb; ++i) internal[i] = import[i];
}

/* \brief convert import index data to IUI */
static void _glhckGeometryConvertIUI(const glhckImportIndexData *import, int memb, void *out)
{
   CALL(0, "%p, %d, %p", import, memb, out);
   assert(sizeof(glhckImportIndexData) == sizeof(unsigned int));
   memcpy(out, import, memb * GLHCKIT(GLHCK_IDX_UINT)->size);
}

#define GLHCK_API_CHECK(x) \
if (!api->x) DEBUG(GLHCK_DBG_ERROR, "-!- \1missing geometry API function: %s", __STRING(x))

/* \brief add new internal vertex type */
int glhckGeometryAddVertexType(const glhckVertexTypeFunctionMap *api, const glhckDataType dataType[4], char memb[4], size_t offset[4], size_t size)
{
   void *tmp;
   unsigned int i;
   size_t oldCount = GLHCKW()->numVertexTypes;
   size_t newCount = GLHCKW()->numVertexTypes + 1;
   __GLHCKvertexType vertexType;
   CALL(0, "%p, %p, %zu", dataType, memb, size);

   GLHCK_API_CHECK(convert);
   GLHCK_API_CHECK(minMax);
   GLHCK_API_CHECK(transform);

   for (i = 0; i < GLHCKW()->numVertexTypes; ++i) {
      if (!memcmp(GLHCKW()->vertexType[i].dataType, dataType, 4 * sizeof(glhckDataType)) &&
          !memcmp(GLHCKW()->vertexType[i].memb, memb, 4)) {
         DEBUG(GLHCK_DBG_WARNING, "This vertexType already exists!");
         RET(0, "%d", i);
         return i;
      }
   }

   if (newCount >= GLHCK_VTX_AUTO)
      goto fail;

   if (!(tmp = _glhckRealloc(GLHCKW()->vertexType, oldCount, newCount, sizeof(__GLHCKvertexType))))
      goto fail;

   for (i = 0; i < 4; ++i) _glhckDataTypeAttributes(dataType[i], &vertexType.msize[i], &vertexType.max[i], &vertexType.normalized[i]);
   vertexType.normalized[0] = 0; /* vertices are never normalized */
   memcpy(&vertexType.api, api, sizeof(glhckVertexTypeFunctionMap));
   memcpy(vertexType.offset, offset, 4 * sizeof(size_t));
   memcpy(vertexType.dataType, dataType, 4 * sizeof(glhckDataType));
   memcpy(vertexType.memb, memb, 4);
   vertexType.size = size;

   GLHCKW()->vertexType = tmp;
   GLHCKW()->numVertexTypes += 1;
   memcpy(&GLHCKW()->vertexType[newCount-1], &vertexType, sizeof(__GLHCKvertexType));

   RET(0, "%d", newCount-1);
   return newCount-1;

fail:
   RET(0, "%d", -1);
   return -1;
}

/* \brief add new internal index type */
int glhckGeometryAddIndexType(const glhckIndexTypeFunctionMap *api, glhckDataType dataType)
{
   void *tmp;
   unsigned int i;
   size_t oldCount = GLHCKW()->numIndexTypes;
   size_t newCount = GLHCKW()->numIndexTypes + 1;
   __GLHCKindexType indexType;
   CALL(0, "%d", dataType);

   GLHCK_API_CHECK(convert);

   for (i = 0; i < GLHCKW()->numIndexTypes; ++i) {
      if (GLHCKW()->indexType[i].dataType == dataType) {
         DEBUG(GLHCK_DBG_WARNING, "This indexType already exists!");
         RET(0, "%d", i);
         return i;
      }
   }

   if (newCount >= GLHCK_IDX_AUTO)
      goto fail;

   if (!(tmp = _glhckRealloc(GLHCKW()->indexType, oldCount, newCount, sizeof(__GLHCKindexType))))
      goto fail;

   _glhckDataTypeAttributes(dataType, &indexType.size, &indexType.max, NULL);
   memcpy(&indexType.api, api, sizeof(glhckIndexTypeFunctionMap));
   indexType.dataType = dataType;

   GLHCKW()->indexType = tmp;
   GLHCKW()->numIndexTypes += 1;
   memcpy(&GLHCKW()->indexType[newCount-1], &indexType, sizeof(__GLHCKindexType));

   RET(0, "%d", newCount-1);
   return newCount-1;

fail:
   RET(0, "%d", -1);
   return -1;
}

#undef GLHCK_API_CHECK

/* \brief init internal vertex/index types */
int _glhckGeometryInit(void)
{
   TRACE(0);

   /* UINT */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = _glhckGeometryConvertIUI;
      if (glhckGeometryAddIndexType(&map, GLHCK_UNSIGNED_INT) != GLHCK_IDX_UINT)
         goto fail;
   }

   /* USHRT */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = _glhckGeometryConvertIUS;
      if (glhckGeometryAddIndexType(&map, GLHCK_UNSIGNED_SHORT) != GLHCK_IDX_USHRT)
         goto fail;
   }

   /* UBYTE */
   {
      glhckIndexTypeFunctionMap map;
      memset(&map, 0, sizeof(glhckIndexTypeFunctionMap));
      map.convert = _glhckGeometryConvertIUB;
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
      map.convert = _glhckGeometryConvertV3F;
      map.minMax = _glhckGeometryMinMaxV3F;
      map.transform = _glhckGeometryTransformV3F;
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
      map.convert = _glhckGeometryConvertV2F;
      map.minMax = _glhckGeometryMinMaxV2F;
      map.transform = _glhckGeometryTransformV2F;
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
      map.convert = _glhckGeometryConvertV3S;
      map.minMax = _glhckGeometryMinMaxV3S;
      map.transform = _glhckGeometryTransformV3S;
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
      map.convert = _glhckGeometryConvertV2S;
      map.minMax = _glhckGeometryMinMaxV2S;
      map.transform = _glhckGeometryTransformV2S;
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
      map.convert = _glhckGeometryConvertV3B;
      map.minMax = _glhckGeometryMinMaxV3B;
      map.transform = _glhckGeometryTransformV3B;
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
      map.convert = _glhckGeometryConvertV2B;
      map.minMax = _glhckGeometryMinMaxV2B;
      map.transform = _glhckGeometryTransformV2B;
      if (glhckGeometryAddVertexType(&map, dataType, memb, offset, sizeof(glhckVertexData2b)) != GLHCK_VTX_V2B)
         goto fail;
   }

   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   DEBUG(GLHCK_DBG_ERROR, "Not enough memory to allocate internal vertex types, going to fail now!");
   return RETURN_FAIL;
}

/* \brief destroy internal vertex/index types */
void _glhckGeometryTerminate(void)
{
   TRACE(0);
   IFDO(_glhckFree, GLHCKW()->vertexType);
   GLHCKW()->numVertexTypes = 0;
   IFDO(_glhckFree, GLHCKW()->indexType);
   GLHCKW()->numIndexTypes = 0;
}

/* \brief free geometry's vertex data */
static void _glhckGeometryFreeVertices(glhckGeometry *object)
{
   IFDO(_glhckFree, object->vertices);
   object->vertexType   = GLHCK_VTX_AUTO;
   object->vertexCount  = 0;
   object->textureRange = 1;
}

/* \brief assign vertices to object internally */
static void _glhckGeometrySetVertices(glhckGeometry *object, unsigned char type, void *data, int memb)
{
   _glhckGeometryFreeVertices(object);
   object->vertices     = data;
   object->vertexType   = type;
   object->vertexCount  = memb;
   object->textureRange = GLHCKVT(type)->max[2];
}

/* \brief free indices from object */
static void _glhckGeometryFreeIndices(glhckGeometry *object)
{
   /* set index type to none */
   IFDO(_glhckFree, object->indices);
   object->indexType  = GLHCK_IDX_AUTO;
   object->indexCount = 0;
}

/* \brief assign indices to object internally */
static void _glhckGeometrySetIndices(glhckGeometry *object, unsigned char type, void *data, int memb)
{
   _glhckGeometryFreeIndices(object);
   object->indices    = data;
   object->indexType  = type;
   object->indexCount = memb;
}

/* \brief insert vertices into object */
int _glhckGeometryInsertVertices(glhckGeometry *object, int memb, unsigned char type, const glhckImportVertexData *vertices)
{
   void *data = NULL;
   CALL(0, "%p, %d, %u, %p", object, memb, type, vertices);
   assert(object);

   /* check vertex type */
   type = _glhckGeometryCheckVertexType(type);

   /* check vertex precision conflicts */
   if (object->indexType != GLHCK_IDX_AUTO &&
      (glhckImportIndexData)memb > GLHCKW()->indexType[object->indexType].max)
      goto bad_precision;

   /* allocate vertex data */
   if (!(data = _glhckMalloc(memb * GLHCKW()->vertexType[type].size)))
      goto fail;

   /* convert and assign */
   memset(&object->bias, 0, sizeof(glhckVector3f));
   object->scale.x = object->scale.y = object->scale.z = 1.0f;
   GLHCKVT(type)->api.convert(vertices, memb, data, &object->bias, &object->scale);
   _glhckGeometrySetVertices(object, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %u, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          GLHCKIT(object->indexType)->max,
                          object);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert index data into object */
int _glhckGeometryInsertIndices(glhckGeometry *object, int memb, unsigned char type, const glhckImportIndexData *indices)
{
   void *data = NULL;
   CALL(0, "%p, %d, %u, %p", object, memb, type, indices);
   assert(object);

   /* check index type */
   type = _glhckGeometryCheckIndexType(type, indices, memb);

   /* check vertex precision conflicts */
   if ((glhckImportIndexData)object->vertexCount > GLHCKIT(type)->max)
      goto bad_precision;

   /* allocate index data */
   if (!(data = _glhckMalloc(memb * GLHCKIT(type)->size)))
      goto fail;

   /* convert and assign */
   GLHCKIT(type)->api.convert(indices, memb, data);
   _glhckGeometrySetIndices(object, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %u, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          GLHCKIT(type)->max,
                          object);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief create new geometry object */
glhckGeometry* _glhckGeometryNew(void)
{
   glhckGeometry *object;

   if (!(object = _glhckCalloc(1, sizeof(glhckGeometry))))
      return NULL;

   object->type = GLHCK_TRIANGLE_STRIP;
   object->scale.x = object->scale.y = object->scale.z = 1.0f;
   return object;
}

/* \brief copy geometry object */
glhckGeometry* _glhckGeometryCopy(glhckGeometry *src)
{
   glhckGeometry *object;
   assert(src);

   if (!(object = _glhckCopy(src, sizeof(glhckGeometry))))
      return NULL;

   if (src->vertices) object->vertices =_glhckCopy(src->vertices, src->vertexCount * GLHCKVT(object->vertexType)->size);
   if (src->indices) object->indices = _glhckCopy(src->indices, src->indexCount * GLHCKIT(object->indexType)->size);
   return object;
}

/* \brief release geometry object */
void _glhckGeometryFree(glhckGeometry *object)
{
   assert(object);
   _glhckGeometryFreeVertices(object);
   _glhckGeometryFreeIndices(object);
   _glhckFree(object);
}

/***
 * public api
 ***/

/* \brief calculate bounding box of the geometry */
GLHCKAPI void glhckGeometryCalculateBB(glhckGeometry *object, kmAABB *aabb)
{
   CALL(2, "%p, %p", object, aabb);
   assert(object && aabb);
   GLHCKVT(object->vertexType)->api.minMax(object, (glhckVector3f*)&aabb->min, (glhckVector3f*)&aabb->max);
}

/* \brief assign indices to object */
GLHCKAPI int glhckGeometryInsertIndices(glhckGeometry *object, unsigned char type, const void *data, int memb)
{
   void *idata;
   CALL(0, "%p, %u, %p, %d", object, type, data, memb);
   assert(object);

   if (data) {
      type = _glhckGeometryCheckIndexType(type, data, memb);
      if (!(idata = _glhckCopy(data, memb * GLHCKIT(type)->size)))
         goto fail;

      _glhckGeometrySetIndices(object, type, idata, memb);
   } else {
      _glhckGeometryFreeIndices(object);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief assign vertices to object */
GLHCKAPI int glhckGeometryInsertVertices(glhckGeometry *object, unsigned char type, const void *data, int memb)
{
   void *vdata;
   CALL(0, "%p, %u, %p, %d", object, type, data, memb);
   assert(object);

   if (data) {
      type = _glhckGeometryCheckVertexType(type);
      if (!(vdata = _glhckCopy(data, memb * GLHCKVT(type)->size)))
         goto fail;

      _glhckGeometrySetVertices(object, type, vdata, memb);
   } else {
      _glhckGeometryFreeVertices(object);
   }

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
