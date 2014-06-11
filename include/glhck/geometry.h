#ifndef __glhck_geometry_h__
#define __glhck_geometry_h__

#include <glhck/glhck.h>

#ifdef __cplusplus
extern "C" {
#endif

/* vertex datatypes */
typedef struct glhckVector3b {
   char x, y, z;
} glhckVector3b;

typedef struct glhckVector2b {
   char x, y;
} glhckVector2b;

typedef struct glhckVector3s {
   short x, y, z;
} glhckVector3s;

typedef struct glhckVector2s {
   short x, y;
} glhckVector2s;

typedef struct glhckVector3f {
   float x, y, z;
} glhckVector3f;

typedef struct glhckVector2f {
   float x, y;
} glhckVector2f;

typedef struct glhckVertexData3f {
   glhckVector3f vertex;
   glhckVector3f normal;
   glhckVector2f coord;
   unsigned short color;
} glhckVertexData3f;

typedef struct glhckVertexData2f {
   glhckVector2f vertex;
   glhckVector3f normal;
   glhckVector2f coord;
   unsigned short color;
} glhckVertexData2f;

typedef struct glhckVertexData3s {
   glhckVector3s vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   unsigned short color;
} glhckVertexData3s;

typedef struct glhckVertexData2s {
   glhckVector2s vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   unsigned short color;
} glhckVertexData2s;

typedef struct glhckVertexData3b {
   glhckVector3b vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   unsigned short color;
} glhckVertexData3b;

typedef struct glhckVertexData2b {
   glhckVector2b vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   unsigned short color;
} glhckVertexData2b;

/* feed glhck the highest precision */
typedef glhckVertexData3f glhckImportVertexData;
typedef unsigned int glhckImportIndexData;

typedef enum glhckBuiltinVertexType {
   GLHCK_VTX_V3F,
   GLHCK_VTX_V2F,
   GLHCK_VTX_V3S,
   GLHCK_VTX_V2S,
   GLHCK_VTX_V3B,
   GLHCK_VTX_V2B,
   GLHCK_VTX_AUTO = 255,
} glhckBuiltinVertexType;

typedef enum glhckBuiltinIndexType {
   GLHCK_IDX_UINT,
   GLHCK_IDX_USHRT,
   GLHCK_IDX_UBYTE,
   GLHCK_IDX_AUTO = 255,
} glhckBuiltinIndexType;

struct glhckGeometry;
typedef struct glhckVertexTypeFunctionMap {
   void (*convert)(const glhckImportVertexData *import, const int memb, void *out, glhckVector3f *outBias, glhckVector3f *outScale);
   void (*minMax)(const struct glhckGeometry *geometry, glhckVector3f *outMin, glhckVector3f *outMax);
   void (*transform)(struct glhckGeometry *geometry, const void *bindPose, const glhckHandle *skinBones, const size_t memb);
} glhckVertexTypeFunctionMap;

/* function map for indexType */
typedef struct glhckIndexTypeFunctionMap {
   void (*convert)(const glhckImportIndexData *import, const int memb, void *out);
} glhckIndexTypeFunctionMap;

/* geometry datatype for low-level raw access */
typedef struct glhckGeometry {
   /* geometry transformation needed
    * after precision conversion */
   glhckVector3f bias;
   glhckVector3f scale;

   /* vertices && indices*/
   void *vertices, *indices;

   /* counts for vertices && indices */
   int vertexCount, indexCount;

   /* transformed texture range
    * the texture matrix is scaled with
    * 1.0f/textureRange of geometry before
    * sending to OpenGL */
   int textureRange;

   /* vertex data && index data types */
   glhckVertexType vertexType;
   glhckIndexType indexType;

   size_t *weightBone;
   void *weights;
   int weightCount;

   /* geometry type (triangles, triangle strip, etc..) */
   glhckGeometryType type;
} glhckGeometry;

#ifdef __cplusplus
}
#endif

#endif /* __glhck_geometry_h__ */
