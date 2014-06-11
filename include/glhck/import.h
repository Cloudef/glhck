#ifndef __glhck_import_h__
#define __glhck_import_h__

#include <glhck/glhck.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct glhckImportAnimationNode {
   const char *name;
   const glhckAnimationQuaternionKey *rotationKeys;
   const glhckAnimationVectorKey *scalingKeys;
   const glhckAnimationVectorKey *translationKeys;
   size_t rotationCount, scalingCount, translationCount;
} glhckImportAnimationNode;

typedef struct glhckImportAnimationStruct {
   const char *name;
   const glhckImportAnimationNode *nodes;
   size_t nodeCount;
} glhckImportAnimationStruct;

typedef struct glhckImportModelMaterial {
   const char *name;
   const char *diffuseTexture;
   kmScalar shininess;
   glhckColor ambient, diffuse, specular;
   unsigned char hasLighting;
} glhckImportModelMaterial;

typedef struct glhckImportModelMesh {
   const char *name;
   const glhckImportVertexData *vertexData;
   const glhckImportIndexData *indexData;
   size_t parent;
   size_t vertexCount, indexCount;
   const size_t *materials, *skins;
   size_t materialCount, skinCount;
   glhckGeometryType geometryType;
   unsigned char hasNormals, hasUV, hasVertexColors;
} glhckImportModelMesh;

typedef struct glhckImportModelSkin {
   const char *name;
   const glhckVertexWeight *weights;
   kmMat4 offsetMatrix;
   size_t weightCount;
} glhckImportModelSkin;

typedef struct glhckImportModelBone {
   kmMat4 transformationMatrix;
   const char *name;
   size_t parent;
} glhckImportModelBone;

typedef struct glhckImportModelStruct {
   const glhckImportModelBone *bones;
   const glhckImportModelSkin *skins;
   const glhckImportModelMesh *meshes;
   const glhckImportModelMaterial *materials;
   const glhckImportAnimationStruct *animations;
   size_t bonesCount, skinCount, meshCount, materialCount, animationCount;
} glhckImportModelStruct;

typedef struct glhckImportImageStruct {
   const void *data;
   int width, height;
   glhckTextureFormat format;
   glhckDataType type;
   unsigned char hasAlpha;
} glhckImportImageStruct;

/* import */
GLHCKAPI glhckImportModelStruct* glhckImportModelData(const void *data, const size_t size);
GLHCKAPI glhckImportModelStruct* glhckImportModelFile(const char *file);
GLHCKAPI glhckImportImageStruct* glhckImportImageData(const void *data, const size_t size);
GLHCKAPI glhckImportImageStruct* glhckImportImageFile(const char *file);

GLHCKAPI glhckHandle glhckObjectNewFromImport(const glhckImportImageStruct *import);
GLHCKAPI glhckHandle glhckObjectNewFromImportEx(const glhckImportImageStruct *import, const glhckIndexType itype, const glhckVertexType vtype);
GLHCKAPI glhckHandle glhckTextureNewFromImport(const glhckImportImageStruct *import);

#ifdef __cplusplus
}
#endif

#endif /* __glhck_import_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
