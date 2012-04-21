#include "../internal.h"
#include <stdio.h>  /* for fopen    */
#include <limits.h> /* for PATH_MAX */
#include <unistd.h> /* for access   */
#include <libgen.h> /* for dirname  */
#include <malloc.h> /* for free     */
#include "tc.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* I used to have own image importers,
 * but then I stumbled against SOIL which seems to do a lots
 * of stuff with it's tiny size, so why reinvent the wheel?
 *
 * Din't want to write a wrapper for it either like assimp,
 * since it would just bloat my texture structure. It's much better like this :) */
#include "../../include/SOIL.h"

/* Should be good enough,
 * dont think anyone would use this long
 * file header anyways.
 */
#define HEADER_MAX 20

/* Minimun header length.
 * I seriously hope there are no headers with only
 * 1 or 2 characters <_<
 */
#define HEADER_MIN 3

/* TO-DO :
 * If any plans to add ASCII formats,
 * then they should be handled too..
 *
 * Maybe check against common stuff on that format?
 * And increase possibility by each match, then pick the one with highest possibility.
 * */

/* Model format defines here */
#define MODEL_FORMAT_OCTM     "OCTM"
#define MODEL_FORMAT_B3D      "BB3Dd"
#define MODEL_FORMAT_PMD      "Pmd"    /* really? */

/* Model format enumeration here */
typedef enum _glhckModelFormat
{
   M_NOT_FOUND = 0,
   M_OCTM,
   M_PMD,
   M_ASSIMP,
} _glhckModelFormat;

/* Read first HEADER_MAX bytes from
 * the file, and try using that information
 * to figure out which format it is.
 *
 * returned char array must be freed */

/* \brief parse header from file */
static char* parse_header(const char *file)
{
   char *MAGIC_HEADER, bit = '\0';
   FILE *f;
   size_t bytesRead = 0, bytesTotal;
   CALL("%s", file);

   /* open file */
   if (!(f = fopen(file, "rb"))) {
      DEBUG(GLHCK_DBG_ERROR, "File: %s, could not open", file);
      goto fail;
   }

   /* allocate our header */
   if (!(MAGIC_HEADER = _glhckMalloc(HEADER_MAX + 1)))
      goto fail;

   /* read bytes */
   while (fread(&bit, 1, 1, f)) {
      /* check ascii range */
      if ((bit >= 45 && bit <= 90) ||
          (bit >= 97 && bit <= 122))
         MAGIC_HEADER[bytesRead++] = bit;
      else if (bytesRead)
         break;

      /* don't exceed the maximum */
      if (bytesRead == HEADER_MAX)
         break;
   }

   /* check that our header has minimum length */
   if (bytesRead + 1 <= HEADER_MIN) {
      bytesRead = 0;

      /* Some formats tend to be hipster and store the header/description
       * at end of the file.. I'm looking at you TGA!! */
      fseek(f, 0L, SEEK_END);
      bytesTotal = ftell(f) - HEADER_MAX;
      fseek(f, bytesTotal, SEEK_SET);

      while (fread(&bit, 1, 1, f)) {
         /* check ascii range */
         if((bit >= 45 && bit <= 90) ||
            (bit >= 97 && bit <= 122))
            MAGIC_HEADER[bytesRead++] = bit;
         else if (bytesRead)
            break;

         /* don't exceed the maximum */
         if (bytesRead == HEADER_MAX)
            break;
      }
   }

   /* close the file */
   fclose(f);

   /* if nothing */
   if (!bytesRead) {
      DEBUG(GLHCK_DBG_ERROR, "File: %s, failed to parse header", file);
      goto parse_fail;
   }

   MAGIC_HEADER[bytesRead] = '\0';

   RET("%s", MAGIC_HEADER);
   return MAGIC_HEADER;

parse_fail:
   _glhckFree(MAGIC_HEADER);
fail:
   RET("%p", NULL);
   return NULL;
}

/* \brief check against known model format headers */
static _glhckModelFormat model_format(const char *MAGIC_HEADER)
{
   CALL("%s", MAGIC_HEADER);

   /* --------- FORMAT HEADER CHECKING ------------ */

#if GLHCK_IMPORT_OPENCTM
   /* OpenCTM check */
   if (!strcmp(MODEL_FORMAT_OCTM, MAGIC_HEADER)) {
      RET("%s", "M_OCTM");
      return M_OCTM;
   }
#endif

#if GLHCK_IMPORT_PMD
   /* PMD check */
   if (!strcmp(MODEL_FORMAT_PMD, MAGIC_HEADER)) {
      RET("%s", "M_PMD");
      return M_PMD;
   }
#endif

#if GLHCK_IMPORT_ASSIMP
   /* Our importers cant handle this, let's try ASSIMP */
   RET("%s", "M_ASSIMP");
   return M_ASSIMP;
#endif

   /* ------- ^^ FORMAT HEADER CHECKING ^^ -------- */

   DEBUG(GLHCK_DBG_ERROR, "No suitable importers found");
   DEBUG(GLHCK_DBG_ERROR, "If the format is supported, make sure you have compiled the library with the support.");
   DEBUG(GLHCK_DBG_ERROR, "Magic header: %s", MAGIC_HEADER);

   RET("%s", "M_NOT_FOUND");
   return M_NOT_FOUND;
}

/* Figure out the file type
 * Call the right importer
 * And let it fill the object structure
 * Return the object
 */

/* \brief import model file */
int _glhckImportModel(_glhckObject *object, const char *file, int animated)
{
   _glhckModelFormat fileFormat;
   char *header;

   /* default for fail, as in no importer found */
   int importReturn = RETURN_FAIL;

   CALL("%p, %s, %d", object, file, animated);
   DEBUG(GLHCK_DBG_CRAP, "Model: %s", file);

   /* read file header */
   if (!(header = parse_header(file)))
      goto fail;

   /* figure out the model format */
   fileFormat = model_format(header);
   _glhckFree(header);

   /* --------- FORMAT IMPORT CALL ----------- */

   switch (fileFormat) {
      /* bail out */
      case M_NOT_FOUND:
         break;

#if GLHCK_IMPORT_OPENCTM
      /* OpenCTM */
      case M_OCTM:
         importReturn = _glhckImportOpenCTM(object, file, animated);
         break;
#endif /* WITH_OPENCTM */

#if GLHCK_IMPORT_PMD
      /* PMD */
      case M_PMD:
         importReturn = _glhckImportPMD(object, file, animated);
         break;
#endif /* WITH_PMD */

#if GLHCK_IMPORT_ASSIMP
      /* Use asssimp */
      case M_ASSIMP:
         importReturn = _glhckImportAssimp(object, file, animated);
         break;
#endif /* WITH_ASSIMP */
   }

   /* ---------- ^^ FORMAT IMPORT ^^ ---------- */

   /* can be non fail too depending on the importReturn */
fail:
   RET("%d", importReturn);
   return importReturn;
}

/* \brief import using SOIL */
int _glhckImportImage(_glhckTexture *texture, const char *file)
{
   CALL("%p, %s", texture, file);
   DEBUG(GLHCK_DBG_CRAP, "Image: %s", file);

   /* load using SOIL */
   texture->data = SOIL_load_image
      (
            file,
            &texture->width,
            &texture->height,
            &texture->channels,
            0
      );

   /* check succes */
   if (!texture->data)
      goto fail;

   texture->size = texture->width * texture->height * texture->channels;
   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to load %s", file);
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}


/* ------------------ PORTABILTY ------------------ */

/* \brief portable basename */
char *gnu_basename(char *path)
{
    char *base;
    CALL("%s", path);
    base = strrchr(path, '/');
    RET("%s", base ? base+1 : path);
    return base ? base+1 : path;
}

/* ------------ SHARED FUNCTIONS ---------------- */

/* \brief helper function for importers.
 * helps finding texture files.
 * maybe we could have a _default_ texture for missing files? */
char* _glhckImportTexturePath(const char* odd_texture_path, const char* model_path)
{
   char *textureFile, *modelFolder;
   char textureInModelFolder[PATH_MAX];
   CALL("%s, %s", odd_texture_path, model_path);

   /* these are must to check */
   if (!odd_texture_path || !odd_texture_path[0])
      goto fail;

   /* lets try first if it contains real path to the texture */
   textureFile = (char*)odd_texture_path;

   /* guess not, lets try basename it */
   if (access(textureFile, F_OK) != 0)
      textureFile = gnu_basename((char*)odd_texture_path);
   else {
      RET("%s", textureFile);
      return strdup(textureFile);
   }

   /* hrmm, we could not even basename it?
    * I think we have a invalid path here Watson! */
   if (!textureFile)
      goto fail; /* Sherlock, you are a genius */

   /* these are must to check */
   if (!model_path || !model_path[0])
      goto fail;

   /* grab the folder where model resides */
   modelFolder = dirname(strdup(model_path));

   /* ok, maybe the texture is in same folder as the model? */
   snprintf(textureInModelFolder, PATH_MAX-1, "%s/%s",
            modelFolder, textureFile );

   /* free this */
   free(modelFolder);

   /* gah, don't give me missing textures damnit!! */
   if (access(textureInModelFolder, F_OK) != 0)
      goto fail;

   /* return, remember to free */
   RET("%s", textureInModelFolder);
   return strdup(textureInModelFolder);

fail:
   RET("%p", NULL);
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

/* \brief return tristripped indecies for triangle index data */
unsigned int* _glhckTriStrip(unsigned int *indices, size_t num_indices, size_t *out_num_indices)
{
#if GLHCK_TRISTRIP
   unsigned int v1, v2, v3;
   unsigned int *out_indices = NULL;
   size_t i, strips, prim_count, tmp;
   ACTCData *tc = NULL;
   CALL("%p, %zu, %p", indices, num_indices, out_num_indices);
   assert(num_indices / 3);

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
               indices[i++],
               indices[i++],
               indices[i++]));
   }
   ACTC_CALL(actcEndInput(tc));

   /* output data */
   tmp = num_indices; i = 0; prim_count = 0;
   ACTC_CALL(actcBeginOutput(tc));
   while (actcStartNextPrim(tc, &v1, &v2) != ACTC_DATABASE_EMPTY) {
      if (i + (prim_count?5:3) > num_indices) {
         out_indices = _glhckRealloc(out_indices, num_indices,
               (num_indices += prim_count?5:3), sizeof(unsigned int));
         if (!out_indices) goto out_of_memory;
      }
      if (i > 2) {
         out_indices[i++] = v3;
         out_indices[i++] = v1;
      }
      out_indices[i++] = v1;
      out_indices[i++] = v2;
      while (actcGetNextVert(tc, &v3) != ACTC_PRIM_COMPLETE) {
         if (i + 1 > num_indices) {
            out_indices = _glhckRealloc(out_indices, num_indices,
                  ++num_indices, sizeof(unsigned int));
            if (!out_indices) goto out_of_memory;
         }
         out_indices[i++] = v3;
      }
      prim_count++;
   }
   ACTC_CALL(actcEndOutput(tc));
   puts("");
   printf("%d alloc\n", num_indices);
   *out_num_indices = i; num_indices = tmp;

   printf("%d indices\n", num_indices);
   printf("%d out indicies\n", i);
   printf("%d tristrips\n", prim_count);
   printf("%d profit\n", num_indices - i);
   actcDelete(tc);

   RET("%p", out_indices);
   return out_indices;

out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Tristripper: out of memory");
   goto fail;
actc_fail:
   DEBUG(GLHCK_DBG_ERROR, "Tristripper: init failed");
   goto fail;
fail:
   if (tc) actcDelete(tc);
   if (out_indices) free(out_indices);
   RET("%p", NULL);
   return NULL;
#else
   /* stripping is disabled.
    * importers should fallback to GLHCK_TRIANGLES */
   return NULL;
#endif
}
