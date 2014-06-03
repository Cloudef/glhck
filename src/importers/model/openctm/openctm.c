#include "importers/importer.h"

#include <openctm.h>

#include "trace.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#ifdef NDEBUG
#  define CTM_CALL(ctx, x) x
#else
#  define CTM_CALL(ctx, x) x; CTM_ERROR(ctx, __LINE__, __func__, __STRING(x));
static void CTM_ERROR(CTMcontext context, unsigned int line, const char *func, const char *ctmfunc)
{
   CTMenum error;
   if ((error = ctmGetError(context)) != CTM_NONE)
      DEBUG(GLHCK_DBG_ERROR, "OCTM @%d:%-20s %-20s >> %s", line, func, ctmfunc, ctmErrorString(error));
}
#endif

/* read callback */
static CTMuint readOCTMFile(void *buf, CTMuint toRead, void *f)
{
   return ((CTMuint)fread(buf, 1, (size_t)toRead, (FILE *)f));
}

int check(chckBuffer *buffer)
{
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

static int import(chckBuffer *buffer, glhckImportModelStruct *import)
{
   FILE *f = NULL;
   CTMcontext context = NULL;
   unsigned int *stripIndices = NULL;
   glhckImportVertexData *vertexData = NULL;
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
   const CTMfloat *vertices = CTM_CALL(context, ctmGetFloatArray(context, CTM_VERTICES));
   const CTMuint *indices = CTM_CALL(context, ctmGetIntegerArray(context, CTM_INDICES));
   CTMuint numVertices = CTM_CALL(context, ctmGetInteger(context, CTM_VERTEX_COUNT));
   CTMuint numTriangles = CTM_CALL(context, ctmGetInteger(context, CTM_TRIANGLE_COUNT));
   CTMuint numUvs = CTM_CALL(context, ctmGetInteger(context, CTM_UV_MAP_COUNT));
   CTMuint numAttribs = CTM_CALL(context, ctmGetInteger(context, CTM_ATTRIB_MAP_COUNT));

   /* indices count now */
   numTriangles *= 3;

   const CTMfloat *normals = NULL;
   if (ctmGetInteger(context, CTM_HAS_NORMALS) == CTM_TRUE) {
      normals = CTM_CALL(context, ctmGetFloatArray(context, CTM_NORMALS));
   }

   const CTMfloat *coords = NULL;
   if (numUvs) {
      const char *textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_FILE_NAME));

      if (!textureFilename)
         textureFilename = CTM_CALL(context, ctmGetUVMapString(context, CTM_UV_MAP_1, CTM_NAME));

      char *texturePath;
      if (textureFilename && (texturePath = _glhckImportTexturePath(textureFilename, file))) {
         glhckTexture *texture;
         if ((texture = glhckTextureNewFromFile(texturePath, NULL, NULL))) {
            coords = CTM_CALL(context, ctmGetFloatArray(context, CTM_UV_MAP_1));
            glhckMaterial *material;
            if ((material = glhckMaterialNew(texture)))
               glhckObjectMaterial(object, material);
            glhckTextureFree(texture);
         }
         _glhckFree(texturePath);
      }
   }

   const CTMfloat *colors = NULL;
   for (unsigned int i = 0; i < numAttribs; ++i) {
      const char *attribName = ctmGetAttribMapString(context, CTM_ATTRIB_MAP_1 + i, CTM_NAME);
      if (attribName && !strcmp(attribName, "Color"))
         colors = CTM_CALL(context, ctmGetFloatArray(context, CTM_ATTRIB_MAP_1 + i));
   }

   /* output comment to stdout */
   const char *comment = CTM_CALL(context, ctmGetString(context, CTM_FILE_COMMENT));

   if (comment)
      DEBUG(GLHCK_DBG_CRAP, "%s", comment);

   /* process vertex data to import format */
   if (!(vertexData = _glhckCalloc(numVertices, sizeof(glhckImportVertexData))))
      goto fail;

   for (unsigned int i = 0; i < numTriangles; ++i) {
      unsigned int ix = indices[i];

      vertexData[ix].vertex.x = vertices[ix*3+0];
      vertexData[ix].vertex.z = vertices[ix*3+1] * -1;
      vertexData[ix].vertex.y = vertices[ix*3+2];

      if (normals) {
         vertexData[ix].normal.x = normals[ix*3+0];
         vertexData[ix].normal.z = normals[ix*3+1] * -1;
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
   unsigned int geometryType = GLHCK_TRIANGLE_STRIP, numIndices;
   if (!(stripIndices = _glhckTriStrip(indices, numTriangles, &numIndices))) {
      /* failed, use non stripped geometry */
      geometryType = GLHCK_TRIANGLES;
      numIndices = numTriangles;
   }

   /* this object has colors */
   if (colors)
      object->flags |= GLHCK_OBJECT_VERTEX_COLOR;

   /* set geometry */
   glhckObjectInsertIndices(object, itype, stripIndices?stripIndices:indices, numIndices);
   glhckObjectInsertVertices(object, vtype, vertexData, numVertices);
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

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->modelImport = import;

   RET(0, "%s", "glhck-importer-openctm");
   return "glhck-importer-openctm";
}

/* vim: set ts=8 sw=3 tw=0 :*/
