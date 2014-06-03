/**
 * Very crude BMP loader, no point supporting this format more than this
 */

#include "importers/importer.h"

#include <stdint.h> /* for standard integers */
#include <string.h> /* for memcmp */
#include <stdlib.h> /* for memory */

#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

static int header(chckBuffer *buffer, int *w, int *h, unsigned short *bpp, size_t *dataSize)
{
   assert(buffer && w && h && bpp && dataSize);

   if (chckBufferGetSize(buffer) < 2 + 16)
      goto fail;

   if (memcmp(chckBufferGetPointer(buffer), "BM", 2) != 0)
      goto fail;

   chckBufferSeek(buffer, 2 + 16, SEEK_CUR);

   if (chckBufferReadInt32(buffer, w) != RETURN_OK)
      goto fail;

   if (chckBufferReadInt32(buffer, h) != RETURN_OK)
      goto fail;

   chckBufferSeek(buffer, sizeof(uint16_t), SEEK_CUR);

   if (chckBufferReadUInt16(buffer, bpp) != RETURN_OK)
      goto fail;

   *dataSize = *w * *h * 3;
   return RETURN_OK;

fail:
   return RETURN_FAIL;
}

static int check(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);
   assert(buffer);

   int w, h;
   size_t dataSize;
   unsigned short bpp;
   if (header(buffer, &w, &h, &bpp, &dataSize) != RETURN_OK)
      goto fail;

   if (!IMAGE_DIMENSIONS_OK(w, h) || bpp != 24 || dataSize <= 0)
      goto bmp_not_supported;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bmp_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "BMP format (%dx%dx%d) not supported", w, h, bpp);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static int import(chckBuffer *buffer, glhckImportImageStruct *import)
{
   unsigned char *importData = NULL;
   CALL(0, "%p, %p", buffer, import);
   assert(buffer && import);

   int w, h;
   size_t dataSize;
   unsigned short bpp;
   if (header(buffer, &w, &h, &bpp, &dataSize) != RETURN_OK)
      goto fail;

   if (bpp != 24 || dataSize <= 0)
      goto bmp_not_supported;

   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto bad_dimensions;

   if (!(importData = malloc(dataSize)))
      goto out_of_memory;

   /* skip non important data */
   chckBufferSeek(buffer, 24, SEEK_CUR);

   if (chckBufferRead(importData, 1, dataSize, buffer) != dataSize)
      goto not_possible;

   /* read 24 bpp BMP data */
   for (unsigned int i = 0; i < dataSize; i+=3) {
      unsigned char bgr[] = { importData[i+0], importData[i+1], importData[i+2] };
      importData[i+0] = bgr[2];
      importData[i+1] = bgr[1];
      importData[i+2] = bgr[0];
   }

   import->width = w;
   import->height = h;
   import->data = importData;
   import->format = GLHCK_RGB;
   import->type = GLHCK_UNSIGNED_BYTE;
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bmp_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "BMP format (%dx%dx%d) not supported", w, h, bpp);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed BMP data has too small size");
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load bmp");
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "BMP image has invalid dimension %dx%d", w, h);
fail:
   IFDO(free, importData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->imageImport = import;

   RET(0, "%s", "glhck-importer-bmp");
   return "glhck-importer-bmp";
}

/* vim: set ts=8 sw=3 tw=0 :*/
