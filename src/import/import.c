#include "../internal.h"
#include "import.h"
#include <stdio.h>  /* for fopen    */
#include <unistd.h> /* for access   */
#include <libgen.h> /* for dirname  */

#if GLHCK_TRISTRIP
#  include <limits.h> /* for INT_MAX  */
#  include "tc.h" /* for ACTC */
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* File format enumeration here */
typedef enum _glhckFormat {
   /* models */
   FORMAT_OCTM,
   FORMAT_PMD,
   FORMAT_ASSIMP,

   /* images */
   FORMAT_PNG,
   FORMAT_JPEG,
   FORMAT_TGA,
   FORMAT_BMP
} _glhckFormat;

/* Function typedefs */
typedef int (*_glhckModelImportFunc)(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
typedef int (*_glhckImageImportFunc)(const char *file, _glhckImportImageStruct *import);
typedef int (*_glhckFormatFunc)(const char *file);

/* Model importer struct */
typedef struct _glhckModelImporter
{
   const char *str;
   _glhckFormat format;
   _glhckFormatFunc formatFunc;
   _glhckModelImportFunc importFunc;
   const char *lib;
   void *dl;
} _glhckModelImporter;

/* Image importer struct */
typedef struct _glhckImageImporter
{
   const char *str;
   _glhckFormat format;
   _glhckFormatFunc formatFunc;
   _glhckImageImportFunc importFunc;
   const char *lib;
   void *dl;
} _glhckImageImporter;

#define REGISTER_IMPORTER(format, formatFunc, importFunc, lib) \
   { __STRING(format), format, formatFunc, importFunc, lib, NULL }
#define END_IMPORTERS() \
   { NULL, 0, NULL, NULL, NULL, NULL }

/* Model importers */
static _glhckModelImporter modelImporters[] = {
#if GLHCK_IMPORT_OPENCTM
   REGISTER_IMPORTER(FORMAT_OCTM, _glhckFormatOpenCTM, _glhckImportOpenCTM, "glhckImportOpenCTM"),
#endif
#if GLHCK_IMPORT_MMD
   REGISTER_IMPORTER(FORMAT_PMD, _glhckFormatPMD, _glhckImportPMD, "glhckImportPMD"),
#endif
#if GLHCK_IMPORT_ASSIMP
   REGISTER_IMPORTER(FORMAT_ASSIMP, _glhckFormatAssimp, _glhckImportAssimp, "glhckImportAssimp"),
#endif
   END_IMPORTERS()
};

/* Image importers */
static _glhckImageImporter imageImporters[] = {
#if GLHCK_IMPORT_PNG
   REGISTER_IMPORTER(FORMAT_PNG, _glhckFormatPNG, _glhckImportPNG, "glhckImportPNG"),
#endif
#if GLHCK_IMPORT_JPEG
   REGISTER_IMPORTER(FORMAT_JPEG, _glhckFormatJPEG, _glhckImportJPEG, "glhckImportJPEG"),
#endif
#if GLHCK_IMPORT_TGA
   REGISTER_IMPORTER(FORMAT_TGA, _glhckFormatTGA, _glhckImportTGA, "glhckImportTGA"),
#endif
#if GLHCK_IMPORT_BMP
   REGISTER_IMPORTER(FORMAT_BMP, _glhckFormatBMP, _glhckImportBMP, "glhckImportBMP"),
#endif
   END_IMPORTERS()
};

#undef REGISTER_IMPORTER
#undef END_IMPORTERS

#if GLHCK_IMPORT_DYNAMIC
/* \brief load importers dynamically */
static int _glhckLoadImporters(void)
{
   CALL(0);
   /* code here */
   RET(0, RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief unload dynamically loaded importers */
static int _glhckUnloadImporters(void)
{
   CALL(0);
   /* code here */
   RET(0, RETURN_FAIL);
   return RETURN_FAIL;
}
#endif

/* \brief check against known model format headers */
static _glhckModelImporter* _glhckGetModelImporter(const char *file)
{
   unsigned int i;
   CALL(0, "%s", file);

   /* --------- FORMAT HEADER CHECKING ------------ */

   for (i = 0; modelImporters[i].formatFunc; ++i)
      if (modelImporters[i].formatFunc(file)) {
         RET(0, "%s", modelImporters[i].str);
         return &modelImporters[i];
      }

   /* ------- ^^ FORMAT HEADER CHECKING ^^ -------- */

   DEBUG(GLHCK_DBG_ERROR, "No suitable model importers found.");
   DEBUG(GLHCK_DBG_ERROR, "If the format is supported, make sure you have compiled the library with the support.");
   DEBUG(GLHCK_DBG_ERROR, "File: %s", file);

   RET(0, "%s", "FORMAT_NOT_FOUND");
   return NULL;
}

/* \brief default model import parameters */
GLHCKAPI const glhckImportModelParameters* glhckImportDefaultModelParameters(void)
{
   static glhckImportModelParameters defaultParameters = {
      .animated   = 0,
      .flatten    = 0,
   };
   return &defaultParameters;
}

/* Figure out the file type
 * Call the right importer
 * And let it fill the object structure
 * Return the object
 */

/* \brief import model file */
int _glhckImportModel(glhckObject *object, const char *file, const glhckImportModelParameters *params,
      glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   _glhckModelImporter *importer;
   int importReturn;
   CALL(0, "%p, %s, %p", object, file, params);
   DEBUG(GLHCK_DBG_CRAP, "Model: %s", file);

   /* figure out the model format */
   if (!(importer = _glhckGetModelImporter(file))) {
      RET(0, "%d", RETURN_FAIL);
      return RETURN_FAIL;
   }

   /* import */
   if (!params) params = glhckImportDefaultModelParameters();
   importReturn = importer->importFunc(object, file, params, itype, vtype);
   RET(0, "%d", importReturn);
   return importReturn;
}

/* \brief check against known image format headers */
static _glhckImageImporter* _glhckGetImageImporter(const char *file)
{
   unsigned int i;
   CALL(0, "%s", file);

   /* --------- FORMAT HEADER CHECKING ------------ */

   for (i = 0; imageImporters[i].formatFunc; ++i)
      if (imageImporters[i].formatFunc(file)) {
         RET(0, "%s", imageImporters[i].str);
         return &imageImporters[i];
      }

   /* ------- ^^ FORMAT HEADER CHECKING ^^ -------- */

   DEBUG(GLHCK_DBG_ERROR, "No suitable image importers found.");
   DEBUG(GLHCK_DBG_ERROR, "If the format is supported, make sure you have compiled the library with the support.");
   DEBUG(GLHCK_DBG_ERROR, "File: %s", file);

   RET(0, "%s", "FORMAT_NOT_FOUND");
   return NULL;
}

/* \brief default image import parameters */
GLHCKAPI const glhckImportImageParameters* glhckImportDefaultImageParameters(void)
{
   static glhckImportImageParameters defaultParameters = {
      .compression = GLHCK_COMPRESSION_NONE,
   };
   return &defaultParameters;
}

/* post-processing of image data */
int _glhckImagePostProcess(glhckTexture *texture, const glhckImportImageParameters *params, const _glhckImportImageStruct *import)
{
   int size = 0;
   void *data = NULL, *compressed = NULL;
   glhckTextureFormat format;
   glhckDataType type;
   CALL(0, "%p, %p, %p", texture, params, import);
   assert(texture);
   assert(import);

   /* use default parameters */
   if (!params) params = glhckImportDefaultImageParameters();

   /* try to compress the data if requested */
   if (params->compression != GLHCK_COMPRESSION_NONE) {
      compressed = glhckTextureCompress(params->compression, import->width, import->height,
            import->format, import->type, import->data, &size, &format, &type);
   }

   /* compression not done/failed */
   if (!(data = compressed)) {
      data = import->data;
      format = import->format;
      type = import->type;
   }

   /* pass the import flags */
   texture->importFlags =  import->flags;

   /* upload texture */
   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, import->width, import->height,
            0, 0, format, type, size, data) != RETURN_OK)
      goto fail;

   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* Figure out the file type
 * Call the right importer
 * And let it fill the import structure
 * Postprocess the texture
 */

/* \brief import image file */
int _glhckImportImage(glhckTexture *texture, const char *file, const glhckImportImageParameters *params)
{
   _glhckImageImporter *importer;
   _glhckImportImageStruct import;
   CALL(0, "%p, %s, %p", texture, file, params);
   DEBUG(GLHCK_DBG_CRAP, "Image: %s", file);
   memset(&import, 0, sizeof(_glhckImportImageStruct));

   /* figure out the image format */
   if (!(importer = _glhckGetImageImporter(file))) {
      RET(0, "%d", RETURN_FAIL);
      return RETURN_FAIL;
   }

   /* import */
   if (importer->importFunc(file, &import) != RETURN_OK)
      goto fail;

   /* post process */
   if (_glhckImagePostProcess(texture, params, &import) != RETURN_OK)
      goto fail;

   IFDO(_glhckFree, import.data);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, import.data);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* ------------------ PORTABILTY ------------------ */

/* \brief portable basename */
char *gnu_basename(char *path)
{
    char *base;
    CALL(1, "%s", path);
    base = strrchr(path, '/');
    RET(1, "%s", base ? base+1 : path);
    return base ? base+1 : path;
}

/* ------------ SHARED FUNCTIONS ---------------- */

/* \brief helper function for importers.
 * helps finding texture files.
 * maybe we could have a _default_ texture for missing files? */
char* _glhckImportTexturePath(const char* odd_texture_path, const char* model_path)
{
   char *textureFile, *modelFolder, *modelPath;
   char textureInModelFolder[2048];
   CALL(0, "%s, %s", odd_texture_path, model_path);

   /* these are must to check */
   if (!odd_texture_path || !odd_texture_path[0])
      goto fail;

   /* lets try first if it contains real path to the texture */
   textureFile = (char*)odd_texture_path;

   /* guess not, lets try basename it */
   if (access(textureFile, F_OK) != 0)
      textureFile = gnu_basename((char*)odd_texture_path);
   else {
      RET(0, "%s", textureFile);
      return _glhckStrdup(textureFile);
   }

   /* hrmm, we could not even basename it?
    * I think we have a invalid path here Watson! */
   if (!textureFile)
      goto fail; /* Sherlock, you are a genius */

   /* these are must to check */
   if (!model_path || !model_path[0])
      goto fail;

   /* copy original path */
   if (!(modelPath = _glhckStrdup(model_path)))
      goto fail;

   /* grab the folder where model resides */
   modelFolder = dirname(modelPath);

   /* ok, maybe the texture is in same folder as the model? */
   snprintf(textureInModelFolder, sizeof(textureInModelFolder)-1, "%s/%s", modelFolder, textureFile);

   /* free this */
   _glhckFree(modelPath);

   /* gah, don't give me missing textures damnit!! */
   if (access(textureInModelFolder, F_OK) != 0)
      goto fail;

   /* return, remember to free */
   RET(0, "%s", textureInModelFolder);
   return _glhckStrdup(textureInModelFolder);

fail:
   RET(0, "%p", NULL);
   return NULL;
}

#define ACTC_CHECK_SYNTAX  "%d: "__STRING(func)" returned unexpected "__STRING(c)"\n"
#define ACTC_CALL_SYNTAX   "%d: "__STRING(func)" failed with %04X\n"

#define ACTC_CHECK(func, c)               \
{ int r;                                  \
   r = (func);                            \
   if (r != c) {                          \
      fprintf(stderr, ACTC_CHECK_SYNTAX,  \
            __LINE__);                    \
      goto fail;                          \
   }                                      \
}

#define ACTC_CALL(func)                   \
{ int r;                                  \
   r = (func);                            \
   if (r < 0) {                           \
      fprintf(stderr, ACTC_CALL_SYNTAX,   \
            __LINE__, -r);                \
      goto fail;                          \
   }                                      \
}

static void _glhckTriStripReverse(glhckImportIndexData *indices, unsigned int memb)
{
   unsigned int i;
   glhckImportIndexData *original;

   if (!(original = _glhckCopy(indices, memb * sizeof(glhckImportIndexData))))
      return;

   for (i = 0; i != memb; ++i)
      indices[i] = original[memb-1-i];

   _glhckFree(original);
}

/* \brief return tristripped indecies for triangle index data */
glhckImportIndexData* _glhckTriStrip(const glhckImportIndexData *indices, unsigned int memb, unsigned int *outMemb)
{
#if !GLHCK_TRISTRIP
   (void)indices;
   (void)memb;
   (void)outMemb;

   /* stripping is disabled.
    * importers should fallback to GLHCK_TRIANGLES */
   return NULL;
#else
   int prim;
   glhckImportIndexData v1, v2, v3;
   glhckImportIndexData *outIndices = NULL, *newIndices = NULL;
   unsigned int i, primCount, tmp;
   ACTCData *tc = NULL;
   CALL(0, "%p, %u, %p", indices, memb, outMemb);

   /* check if the triangles we got are valid */
   if (memb % 3 != 0)
      goto not_valid;

   if (!(outIndices = _glhckMalloc(memb * sizeof(glhckImportIndexData))))
      goto out_of_memory;

   if (!(tc = actcNew()))
      goto actc_fail;

   /* parameters */
   ACTC_CALL(actcParami(tc, ACTC_OUT_MIN_FAN_VERTS, INT_MAX));
   ACTC_CALL(actcParami(tc, ACTC_OUT_HONOR_WINDING, ACTC_FALSE));

   /* input data */
   ACTC_CALL(actcBeginInput(tc));
   for (i = 0; i != memb; i+=3) {
      ACTC_CALL(actcAddTriangle(tc,
               indices[i+0],
               indices[i+1],
               indices[i+2]));
   }
   ACTC_CALL(actcEndInput(tc));

   /* FIXME: fix the winding of generated stiched strips */

   /* output data */
   tmp = memb, i = 0, primCount = 0;
   unsigned int flipStart = 0, length = 0;
   ACTC_CALL(actcBeginOutput(tc));
   while ((prim = actcStartNextPrim(tc, &v1, &v2)) != ACTC_DATABASE_EMPTY) {
      assert(prim == ACTC_PRIM_STRIP);
      if (i + (primCount?5:3) > memb)
         goto no_profit;

      /* degenerate to next strip */
      if (primCount && v1 != v3) {
         if (length & 1) {
            _glhckTriStripReverse(&outIndices[flipStart], i-flipStart);
            if (flipStart) outIndices[flipStart-1] = outIndices[flipStart];
         }

         v3 = outIndices[i-1];
         if (v1 != v3) {
            outIndices[i++] = v3;
            outIndices[i++] = v1;
         }
         flipStart = i;
      }

      length = 0;
      outIndices[i++] = v1;
      outIndices[i++] = v2;
      while (actcGetNextVert(tc, &v3) != ACTC_PRIM_COMPLETE) {
         if (i + 1 > memb)
            goto no_profit;
         outIndices[i++] = v3;
         ++length;
      }

      primCount++;
   }
   ACTC_CALL(actcEndOutput(tc));
   puts("");
   printf("%u alloc\n", memb);
   if (outMemb) *outMemb = i;
   memb = tmp;

   // for (i = *outMemb-12; i != *outMemb; ++i)
      // outIndices[i] = 0;

#if 0
   puts("- INDICES:");
   for (i = 0; i != *outMemb; ++i)
      printf("%u%s", outIndices[i], (!((i+1) % 3)?"\n":", "));
   puts("");
#endif

   if (!(newIndices = _glhckRealloc(outIndices, memb, i, sizeof(glhckImportIndexData))))
      goto out_of_memory;
   outIndices = newIndices;

   printf("%u indices\n", memb);
   printf("%u out indicies\n", i);
   printf("%u tristrips\n", primCount);
   printf("%u profit\n", memb - i);
   actcDelete(tc);

   RET(0, "%p", outIndices);
   return outIndices;

not_valid:
   DEBUG(GLHCK_DBG_ERROR, "Tristripper: not valid triangle indices");
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Tristripper: out of memory");
   goto fail;
actc_fail:
   DEBUG(GLHCK_DBG_ERROR, "Tristripper: init failed");
   goto fail;
no_profit:
   DEBUG(GLHCK_DBG_CRAP, "Tripstripper: no profit from stripping, fallback to triangles");
fail:
   IFDO(actcDelete, tc);
   IFDO(_glhckFree, outIndices);
   RET(0, "%p", NULL);
   return NULL;
#endif
}

#undef ACTC_CALL
#undef ACTC_CALL

/* vim: set ts=8 sw=3 tw=0 :*/
