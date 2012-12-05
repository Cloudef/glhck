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

static int buildModel(const char *file, _glhckObject *object,
      size_t numIndices, size_t numVertices,
      const glhckImportIndexData *indices, const glhckImportVertexData *vertexData,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype, int animated)
{
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   size_t numStrippedIndices = 0;
   glhckImportIndexData *stripIndices = NULL;

   if (!numVertices)
      return RETURN_FAIL;

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
   NULLDO(_glhckFree, stripIndices);
   return RETURN_OK;
}

static void joinMesh(const char *file, const struct aiScene *sc,
      const struct aiNode *nd, const struct aiMesh *mesh, int animated,
      glhckImportIndexData *indices, glhckImportVertexData *vertexData)
{
   unsigned int f, i, skip;
   glhckImportIndexData index;
   const struct aiFace *face;

   for (f = 0, skip = 0; f != mesh->mNumFaces; ++f) {
      face = &mesh->mFaces[f];
      if (!face) continue;

      for (i = 0; i != face->mNumIndices; ++i) {
         index = face->mIndices[i];
         vertexData[index].vertex.x = mesh->mVertices[index].x;
         vertexData[index].vertex.y = mesh->mVertices[index].y;
         vertexData[index].vertex.z = mesh->mVertices[index].z;

         indices[skip+i] = index;
      } skip += face->mNumIndices;
   }
}

static int processModel(const char *file, _glhckObject *object,
      const struct aiScene *sc, const struct aiNode *nd,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype, int animated)
{
   unsigned int m, f;
   size_t numVertices = 0, numIndices = 0;
   size_t ioffset, voffset;
   glhckImportIndexData *indices = NULL;
   glhckImportVertexData *vertexData = NULL;
   const struct aiMesh *mesh;
   const struct aiFace *face;

   printf("%d\n",nd->mNumMeshes);
   printf("%d\n",nd->mNumChildren);

   for (m = 0; m != nd->mNumMeshes; ++m) {
      mesh = sc->mMeshes[nd->mMeshes[m]];
      if (!mesh->mVertices) continue;

      for (f = 0; f != mesh->mNumFaces; ++f) {
         face = &mesh->mFaces[f];
         if (!face) goto fail;
         numIndices += face->mNumIndices;
      }
      numVertices += mesh->mNumVertices;
   }

   DEBUG(GLHCK_DBG_CRAP, "I: %d", numIndices);
   DEBUG(GLHCK_DBG_CRAP, "V: %d", numVertices);

   if (!(vertexData = _glhckCalloc(numVertices, sizeof(glhckImportVertexData))))
      goto assimp_no_memory;

   if (!(indices = _glhckMalloc(numIndices * sizeof(glhckImportIndexData))))
      goto assimp_no_memory;

   for (m = 0, ioffset = 0, voffset = 0; m != nd->mNumMeshes; ++m) {
      mesh = sc->mMeshes[nd->mMeshes[m]];
      if (!mesh->mVertices) continue;

      joinMesh(file, sc, nd, mesh, animated, indices+ioffset, vertexData+voffset);
      for (f = 0; f != mesh->mNumFaces; ++f) {
         face = &mesh->mFaces[f];
         if (!face) goto fail;
         ioffset += face->mNumIndices;
      }
      voffset += mesh->mNumVertices;
   }

   if (buildModel(file, object, numIndices,  numVertices,
            indices, vertexData, itype, vtype, animated) == RETURN_OK) {
      NULLDO(_glhckFree, vertexData);
      NULLDO(_glhckFree, indices);
      return RETURN_OK;
   }

   for (m = 0; m != nd->mNumChildren; ++m) {
      if (processModel(file, object, sc, nd->mChildren[m],
               itype, vtype, animated) == RETURN_OK)
         return RETURN_OK;
   }

   /* TODO: child import.. (from nd)
    * glhck first needs child objects */

   return RETURN_FAIL;

assimp_no_memory:
   DEBUG(GLHCK_DBG_ERROR, "Assimp not enough memory.");
fail:
   IFDO(_glhckFree, vertexData);
   IFDO(_glhckFree, indices);
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
int _glhckImportAssimp(_glhckObject *object, const char *file, int animated,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   const struct aiScene *scene;
   CALL(0, "%p, %s, %d", object, file, animated);

   /* import the model using assimp
    * TODO: make import hints tunable?
    * Needs changes to import protocol! */
   scene = aiImportFile(file, aiProcessPreset_TargetRealtime_Fast);
   if (!scene) goto assimp_fail;

   /* process the model */
   if (processModel(file, object, scene, scene->mRootNode,
            itype, vtype, animated) != RETURN_OK)
      goto fail;

   /* close file */
   NULLDO(aiReleaseImport, scene);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

assimp_fail:
   DEBUG(GLHCK_DBG_ERROR, aiGetErrorString());
fail:
   IFDO(aiReleaseImport, scene);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
