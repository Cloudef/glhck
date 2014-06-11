#include <glhck/glhck.h>
#include <glhck/import.h>

#include <libgen.h> /* for dirname */
#include <unistd.h> /* for access */
#include <stdlib.h>
#include <stdio.h>

#if GLHCK_TRISTRIP
#  include <limits.h> /* for INT_MAX */
#  include "tc.h" /* for ACTC */
#endif

#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

static const char* gnubasename(const char *path)
{
    CALL(1, "%s", path);
    const char *base = strrchr(path, '/');
    RET(1, "%s", (base ? base + 1 : path));
    return (base ? base + 1 : path);
}

/* ------------ SHARED FUNCTIONS ---------------- */

/* \brief helper function for importers.
 * helps finding texture files.
 * maybe we could have a _default_ texture for missing files? */
char* findTexturePath(const char* oddTexturePath, const char* modelPath)
{
   char *textureInModelFolder = NULL, *modelPathCpy = NULL;
   CALL(0, "%s, %s", oddTexturePath, modelPath);

   /* these are must to check */
   if (!oddTexturePath || !oddTexturePath[0])
      goto fail;

   const char *textureFile = oddTexturePath;

   /* guess not, lets try basename it */
   if (access(textureFile, F_OK) != 0) {
      textureFile = gnubasename(textureFile);
   } else {
      RET(0, "%s", textureFile);
      return strdup(textureFile);
   }

   /* hrmm, we could not even basename it?
    * I think we have a invalid path here Watson! */
   if (!textureFile)
      goto fail; /* Sherlock, you are a genius */

   /* these are must to check */
   if (!modelPath || !modelPath[0])
      goto fail;

   /* copy original path */
   if (!(modelPathCpy = strdup(modelPath)))
      goto fail;

   /* grab the folder where model resides */
   char *modelFolder = dirname(modelPathCpy);

   /* ok, maybe the texture is in same folder as the model? */
   size_t len = snprintf(NULL, 0, "%s/%s", modelFolder, textureFile) + 1;

   if (!(textureInModelFolder = calloc(1, len + 1)))
      goto fail;

   snprintf(textureInModelFolder, len, "%s/%s", modelFolder, textureFile);

   /* free this */
   free(modelPathCpy);

   /* gah, don't give me missing textures damnit!! */
   if (access(textureInModelFolder, F_OK) != 0)
      goto fail;

   /* return, remember to free */
   RET(0, "%s", textureInModelFolder);
   return textureInModelFolder;

fail:
   IFDO(free, modelPathCpy);
   IFDO(free, textureInModelFolder);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief invert pixel data */
static void invertPixels(unsigned char *pixels, unsigned int w, unsigned int h, unsigned int components)
{
   for (unsigned int i = 0; i * 2 < h; ++i) {
      unsigned int index1 = i * w * components;
      unsigned int index2 = (h - 1 - i) * w * components;
      for (unsigned int i2 = w * components; i2 > 0; --i2) {
         unsigned char temp = pixels[index1];
         pixels[index1] = pixels[index2];
         pixels[index2] = temp;
         ++index1; ++index2;
      }
   }
}

#ifndef __STRING
#  define __STRING(x) #x
#endif

#define ACTC_CHECK_SYNTAX __STRING(func)" returned unexpected "__STRING(c)"\n"
#define ACTC_CALL_SYNTAX  __STRING(func)" failed with %04X\n"

#define ACTC_CHECK(func, c)                      \
{                                                \
   if ((func) != c) {                            \
      DEBUG(GLHCK_DBG_ERROR, ACTC_CHECK_SYNTAX); \
      goto fail;                                 \
   }                                             \
}

#define ACTC_CALL(func)                             \
{                                                   \
   if ((func) < 0) {                                \
      DEBUG(GLHCK_DBG_ERROR, ACTC_CALL_SYNTAX, -r); \
      goto fail;                                    \
   }                                                \
}

#if GLHCK_TRISTRIP
static void tristripReverse(glhckImportIndexData *indices, unsigned int memb)
{
   glhckImportIndexData *original;
   if (!(original = _glhckCopy(indices, memb * sizeof(glhckImportIndexData))))
      return;

   for (unsigned int i = 0; i != memb; ++i)
      indices[i] = original[memb - 1 - i];

   _glhckFree(original);
}
#endif

/* \brief return tristripped indecies for triangle index data */
glhckImportIndexData* tristrip(const glhckImportIndexData *indices, unsigned int memb, unsigned int *outMemb)
{
#if !GLHCK_TRISTRIP
   (void)indices, (void)memb, (void)outMemb;

   /* stripping is disabled.
    * importers should fallback to GLHCK_TRIANGLES */
   return NULL;
#else
   ACTCData *tc = NULL;
   glhckImportIndexData *outIndices = NULL, *newIndices = NULL;
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
   for (unsigned int i = 0; i != memb; i += 3)
      ACTC_CALL(actcAddTriangle(tc, indices[i+0], indices[i+1], indices[i+2]));
   ACTC_CALL(actcEndInput(tc));

   /* FIXME: fix the winding of generated stiched strips */

   /* output data */
   ACTC_CALL(actcBeginOutput(tc));

   int prim;
   unsigned int i = 0, primCount = 0, length = 0, flipStart = 0;
   glhckImportIndexData v1, v2, v3;
   while ((prim = actcStartNextPrim(tc, &v1, &v2)) != ACTC_DATABASE_EMPTY) {
      assert(prim == ACTC_PRIM_STRIP);
      if (i + (primCount ? 5 : 3) > memb)
         goto no_profit;

      /* degenerate to next strip */
      if (primCount && v1 != v3) {
         if (length & 1) {
            _glhckTriStripReverse(&outIndices[flipStart], i - flipStart);

            if (flipStart)
               outIndices[flipStart - 1] = outIndices[flipStart];
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

   if (outMemb)
      *outMemb = i;

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

#if 0
static int _glhckPostProcessImage(const glhckPostProcessImageParameters *params, glhckImportImageStruct *import)
{
   void *data = NULL, *compressed = NULL;
   CALL(0, "%p, %p", params, import);
   assert(import);

   /* use default parameters */
   if (!params)
      params = glhckImportDefaultImageParameters();

   /* try to compress the data if requested */
   int size = 0;
   glhckDataType type;
   glhckTextureFormat format;
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
   texture->importFlags = import->flags;

   /* upload texture */
   if (glhckTextureCreate(texture, GLHCK_TEXTURE_2D, 0, import->width, import->height, 0, 0, format, type, size, data) != RETURN_OK)
      goto fail;

   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   IFDO(_glhckFree, compressed);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}
#endif
