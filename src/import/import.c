#include "../internal.h"
#include "import.h"
#include <stdio.h>  /* for fopen    */
#include <limits.h> /* for PATH_MAX */
#include <unistd.h> /* for access   */
#include <libgen.h> /* for dirname  */
#include "tc.h"

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* File format enumeration here */
typedef enum _glhckFormat
{
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
typedef int (*_glhckModelImportFunc)(_glhckObject *object, const char *file, int animated);
typedef int (*_glhckImageImportFunc)(_glhckTexture *texture, const char *file, unsigned int flags);
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
   REGISTER_IMPORTER(FORMAT_OCTM, _glhckFormatOpenCTM, _glhckImportOpenCTM, "glhckImportOpenCTM"),
   REGISTER_IMPORTER(FORMAT_PMD, _glhckFormatPMD, _glhckImportPMD, "glhckImportPMD"),
   //REGISTER_IMPORTER(FORMAT_ASSIMP, _glhckFormatAssimp, _glhckImportAssimp, "glhckImportAssimp"),
   END_IMPORTERS()
};

/* Image importers */
static _glhckImageImporter imageImporters[] = {
   REGISTER_IMPORTER(FORMAT_PNG, _glhckFormatPNG, _glhckImportPNG, "glhckImportPNG"),
   REGISTER_IMPORTER(FORMAT_JPEG, _glhckFormatJPEG, _glhckImportJPEG, "glhckImportJPEG"),
   REGISTER_IMPORTER(FORMAT_TGA, _glhckFormatTGA, _glhckImportTGA, "glhckImportTGA"),
   REGISTER_IMPORTER(FORMAT_BMP, _glhckFormatBMP, _glhckImportBMP, "glhckImportBMP"),
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

/* Figure out the file type
 * Call the right importer
 * And let it fill the object structure
 * Return the object
 */

/* \brief import model file */
int _glhckImportModel(_glhckObject *object, const char *file, int animated)
{
   _glhckModelImporter *importer;
   int importReturn;
   CALL(0, "%p, %s, %d", object, file, animated);
   DEBUG(GLHCK_DBG_CRAP, "Model: %s", file);

   /* figure out the model format */
   if (!(importer = _glhckGetModelImporter(file))) {
      RET(0, "%d", RETURN_FAIL);
      return RETURN_FAIL;
   }

   /* import */
   importReturn = importer->importFunc(object, file, animated);
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

/* Figure out the file type
 * Call the right importer
 * And let it fill the texture structure
 * Return the texture
 */

/* \brief import image file */
int _glhckImportImage(_glhckTexture *texture, const char *file, unsigned int flags)
{
   _glhckImageImporter *importer;
   int importReturn;
   CALL(0, "%p, %s, %u", texture, file, flags);
   DEBUG(GLHCK_DBG_CRAP, "Image: %s", file);

   /* figure out the image format */
   if (!(importer = _glhckGetImageImporter(file))) {
      RET(0, "%d", RETURN_FAIL);
      return RETURN_FAIL;
   }

   /* import */
   importReturn = importer->importFunc(texture, file, flags);
   RET(0, "%d", importReturn);
   return importReturn;
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
   char textureInModelFolder[PATH_MAX];
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
      return strdup(textureFile);
   }

   /* hrmm, we could not even basename it?
    * I think we have a invalid path here Watson! */
   if (!textureFile)
      goto fail; /* Sherlock, you are a genius */

   /* these are must to check */
   if (!model_path || !model_path[0])
      goto fail;

   /* copy original path */
   if (!(modelPath = strndup(model_path, PATH_MAX-1)))
      goto fail;

   /* grab the folder where model resides */
   modelFolder = dirname(modelPath);

   /* ok, maybe the texture is in same folder as the model? */
   snprintf(textureInModelFolder, PATH_MAX-1, "%s/%s",
            modelFolder, textureFile );

   /* free this */
   free(modelPath);

   /* gah, don't give me missing textures damnit!! */
   if (access(textureInModelFolder, F_OK) != 0)
      goto fail;

   /* return, remember to free */
   RET(0, "%s", textureInModelFolder);
   return strdup(textureInModelFolder);

fail:
   RET(0, "%p", NULL);
   return NULL;
}

/* post-processing of image data */
int _glhckImagePostProcess(_glhckTexture *texture, _glhckImagePostProcessStruct *data, unsigned int flags)
{
   unsigned char *outData = NULL;
   unsigned int outFormat, inFormat;
   CALL(0, "%p, %p, %u", texture, data, flags);

   /* assign default format for texture,
    * most imports are in RGBA and is preferred. */
   inFormat = outFormat = data->format;

   /* post processing below */

   /* format manipulations here */

   /* upload texture */
   glhckTextureCreate(texture, outData?outData:data->data, data->width, data->height,
         inFormat, outFormat, flags);

   IFDO(_glhckFree, outData);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory");
fail:
   IFDO(_glhckFree, outData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
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

/* TODO: fix winding of strips (allow using backface culling without artifacts)
 * - if the length of the first part of the strip is odd, the strip must be reversed
 * - to reverse the strip, write it in reverse order. If the position of the original face in this new reversed strip is odd, you’re done. Else replicate the first index.
 */

/* \brief return tristripped indecies for triangle index data */
unsigned int* _glhckTriStrip(const unsigned int *indices, size_t num_indices, size_t *out_num_indices)
{
#if GLHCK_TRISTRIP
   unsigned int v1, v2, v3;
   unsigned int *out_indices = NULL, test;
   size_t i, prim_count, tmp;
   ACTCData *tc = NULL;
   CALL(0, "%p, %zu, %p", indices, num_indices, out_num_indices);

   /* check if the triangles we got are valid */
   test = num_indices;
   while ((test-=3)>3);
   if (test != 3 && test != 0) goto not_valid;

   if (!(out_indices = _glhckMalloc(num_indices * sizeof(unsigned int))))
      goto out_of_memory;

   if (!(tc = actcNew()))
      goto actc_fail;

   /* paramaters */
   ACTC_CALL(actcParami(tc, ACTC_OUT_MIN_FAN_VERTS, 2147483647));

   /* input data */
   i = 0;
   ACTC_CALL(actcBeginInput(tc));
   while (i != num_indices) {
      ACTC_CALL(actcAddTriangle(tc,
               indices[i+0],
               indices[i+1],
               indices[i+2]));
      i+=3;
   }
   ACTC_CALL(actcEndInput(tc));

   /* output data */
   tmp = num_indices; i = 0; prim_count = 0;
   ACTC_CALL(actcBeginOutput(tc));
   while (actcStartNextPrim(tc, &v1, &v2) != ACTC_DATABASE_EMPTY) {
      if (i + (prim_count?5:3) > num_indices)
         goto no_profit;
      if (prim_count) {
         out_indices[i++] = v3;
         out_indices[i++] = v1;
      }
      out_indices[i++] = v1;
      out_indices[i++] = v2;
      while (actcGetNextVert(tc, &v3) != ACTC_PRIM_COMPLETE) {
         if (i + 1 > num_indices)
            goto no_profit;
         out_indices[i++] = v3;
      }
      prim_count++;
   }
   ACTC_CALL(actcEndOutput(tc));
   puts("");
   printf("%zu alloc\n", num_indices);
   *out_num_indices = i; num_indices = tmp;

   printf("%zu indices\n", num_indices);
   printf("%zu out indicies\n", i);
   printf("%zu tristrips\n", prim_count);
   printf("%zu profit\n", num_indices - i);
   actcDelete(tc);

   RET(0, "%p", out_indices);
   return out_indices;

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
   IFDO(_glhckFree, out_indices);
   RET(0, "%p", NULL);
   return NULL;
#else
   /* stripping is disabled.
    * importers should fallback to GLHCK_TRIANGLES */
   return NULL;
#endif
}

#undef ACTC_CALL
#undef ACTC_CALL

/* vim: set ts=8 sw=3 tw=0 :*/
