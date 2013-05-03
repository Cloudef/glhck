#include "../internal.h"
#include "import.h"
#include <openctm.h>
#include <stdio.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#ifdef NDEBUG
#  define CTM_CALL(x) x
#else
#  define CTM_CALL(ctx, x) x; CTM_ERROR(ctx, __LINE__, __func__, __STRING(x));
static inline void CTM_ERROR(CTMcontext context, unsigned int line, const char *func, const char *ctmfunc)
{
   CTMenum error;
   if ((error = ctmGetError(context)) != CTM_NONE)
      DEBUG(GLHCK_DBG_ERROR, "OCTM @%d:%-20s %-20s >> %s",
            line, func, ctmfunc, ctmErrorString(error));
}
#endif

/* read callback */
static CTMuint readOCTMFile(void *buf, CTMuint toRead, void *f)
{
   return ((CTMuint)fread(buf, 1, (size_t)toRead, (FILE *)f));
}

/* \brief check if file is a OpenCTM file */
int _glhckFormatOpenCTM(const char *file)
{
   FILE *f;
   CTMcontext context = NULL;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* create import OpenCTM context */
   if (!(context = ctmNewContext(CTM_IMPORT)))
      goto fail;

   /* load OpenCTM file */
   ctmLoadCustom(context, (CTMreadfn)readOCTMFile, f);

   /* close file */
   NULLDO(fclose, f);

   if (ctmGetError(context) != CTM_NONE)
      goto fail;

   NULLDO(ctmFreeContext, context);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   IFDO(ctmFreeContext, context);
   return RETURN_FAIL;
}

/* \brief import OpenCTM file */
int _glhckImportOpenCTM(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   FILE *f;
   unsigned int i, ix, *stripIndices = NULL, numIndices = 0;
   CTMcontext context = NULL;
   CTMuint num_vertices, numTriangles, numUvs, numAttribs;
   const CTMuint *indices;
   const CTMfloat *vertices = NULL, *normals = NULL, *coords = NULL, *colors = NULL;
   const char *attribName, *comment, *textureFilename;
   char *texturePath;
   glhckImportVertexData *vertexData = NULL;
   glhckMaterial *material;
   glhckTexture *texture;
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP;
   CALL(0, "%p, %s, %p", object, file, params);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* create import OpenCTM context */
   if (!(context = ctmNewContext(CTM_IMPORT)))
      goto fail;

   /* load OpenCTM file */
   CTM_CALL(context, ctmLoadCustom(context, (CTMreadfn)readOCTMFile, f));

   /* close file */
   NULLDO(fclose, f);

   /* read geometry data */
   vertices      = CTM_CALL(context, ctmGetFloatArray(context, CTM_VERTICES));
   indices       = CTM_CALL(context, ctmGetIntegerArray(context, CTM_INDICES));
   num_vertices  = CTM_CALL(context, ctmGetInteger(context, CTM_VERTEX_COUNT));
   numTriangles = CTM_CALL(context, ctmGetInteger(context, CTM_TRIANGLE_COUNT));
   numUvs       = CTM_CALL(context, ctmGetInteger(context, CTM_UV_MAP_COUNT));
   numAttribs   = CTM_CALL(context, ctmGetInteger(context, CTM_ATTRIB_MAP_COUNT));

   /* indices count now */
   numTriangles *= 3;

   if (ctmGetInteger(context, CTM_HAS_NORMALS) == CTM_TRUE) {
      normals = CTM_CALL(context, ctmGetFloatArray(context, CTM_NORMALS));
   }

   if (numUvs) {
      textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_FILE_NAME));
      if (!textureFilename) {
         textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_NAME));
      }

      if ((texturePath = _glhckImportTexturePath(textureFilename, file))) {
         if ((texture = glhckTextureNew(texturePath, NULL, NULL))) {
            coords = CTM_CALL(context, ctmGetFloatArray(context, CTM_UV_MAP_1));
            if ((material = glhckMaterialNew(texture)))
               glhckObjectMaterial(object, material);
            glhckTextureFree(texture);
         }
         _glhckFree(texturePath);
      }
   }

   for (i = 0; i != numAttribs; ++i) {
      attribName = ctmGetAttribMapString(context, CTM_ATTRIB_MAP_1 + i, CTM_NAME);
      if (!strcmp(attribName, "Color")) {
         colors = CTM_CALL(context, ctmGetFloatArray(context, CTM_ATTRIB_MAP_1 + i));
      }
   }

   /* output comment to stdout */
   comment = CTM_CALL(context, ctmGetString(context, CTM_FILE_COMMENT));
   if (comment) DEBUG(GLHCK_DBG_CRAP, "%s", comment);

   /* process vertex data to import format */
   if (!(vertexData = _glhckCalloc(num_vertices, sizeof(glhckImportVertexData))))
      goto fail;

   for (i = 0; i != numTriangles; ++i) {
      ix = indices[i];

      vertexData[ix].vertex.x = vertices[ix*3+0];
      vertexData[ix].vertex.z = vertices[ix*3+1]*-1; /* flip Z */
      vertexData[ix].vertex.y = vertices[ix*3+2];

      if (normals) {
         vertexData[ix].normal.x = normals[ix*3+0];
         vertexData[ix].normal.z = normals[ix*3+1]-1; /* flip Z */
         vertexData[ix].normal.y = normals[ix*3+2];
      }

      if (coords) {
         vertexData[ix].coord.x = coords[ix*2+0];
         vertexData[ix].coord.y = coords[ix*2+1];
      }

      if (colors) {
         vertexData[ix].color.r = colors[ix*4+0]*255.0f;
         vertexData[ix].color.g = colors[ix*4+1]*255.0f;
         vertexData[ix].color.b = colors[ix*4+2]*255.0f;
         vertexData[ix].color.a = colors[ix*4+3]*255.0f;
      }
   }

   /* triangle strip geometry */
   if (!(stripIndices = _glhckTriStrip(indices, numTriangles, &numIndices))) {
      /* failed, use non stripped geometry */
      geometryType = GLHCK_TRIANGLES;
      numIndices  = numTriangles;
   }

   /* this object has colors */
   if (colors) object->flags |= GLHCK_OBJECT_VERTEX_COLOR;

   /* set geometry */
   glhckObjectInsertIndices(object, itype, stripIndices?stripIndices:indices, numIndices);
   glhckObjectInsertVertices(object, vtype, vertexData, num_vertices);
   object->geometry->type = geometryType;

   /* finish */
   IFDO(_glhckFree, stripIndices);
   NULLDO(_glhckFree, vertexData);
   NULLDO(ctmFreeContext, context);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
fail:
   IFDO(_glhckFree, stripIndices);
   IFDO(_glhckFree, vertexData);
   IFDO(fclose, f);
   IFDO(ctmFreeContext, context);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
