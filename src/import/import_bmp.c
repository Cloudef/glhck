/* very crude BMP loader, no point supporting
 * this format more than this */

#include "../internal.h"
#include "import.h"
#include "buffer/buffer.h"
#include <stdio.h>  /* for FILE */
#include <stdint.h> /* for standard integers */

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* \brief read BMP header */
int _readBMPHeader(FILE *f, int *w, int *h, unsigned short *bpp, size_t *dataSize)
{
   chckBuffer *buf;
   static const size_t headerSize = 2 + 16 + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint16_t) + sizeof(uint16_t);
   assert(f && w && h && bpp && dataSize);

   if (!(buf = chckBufferNew(headerSize, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (chckBufferFillFromFile(f, 1, headerSize, buf) != headerSize)
      goto fail;

   if (memcmp(chckBufferGetPointer(buf), "BM", 2) != 0)
      goto fail;

   chckBufferSeek(buf, 2 + 16, SEEK_CUR);

   if (chckBufferReadInt32(buf, w) != RETURN_OK)
      goto fail;

   if (chckBufferReadInt32(buf, h) != RETURN_OK)
      goto fail;

   chckBufferSeek(buf, sizeof(uint16_t), SEEK_CUR);

   if (chckBufferReadUInt16(buf, bpp) != RETURN_OK)
      goto fail;

   *dataSize = *w * *h * 3;
   NULLDO(chckBufferFree, buf);
   return RETURN_OK;

fail:
   IFDO(chckBufferFree, buf);
   return RETURN_FAIL;
}

/* \brief check if file is BMP */
int _glhckFormatBMP(const char *file)
{
   FILE *f;
   int w, h;
   unsigned short bpp;
   size_t dataSize;
   CALL(0, "%s", file);

   /* open BMP */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (_readBMPHeader(f, &w, &h, &bpp, &dataSize) != RETURN_OK)
      goto fail;

   if (!IMAGE_DIMENSIONS_OK(w, h) || bpp != 24 || dataSize <= 0)
      goto bmp_not_supported;

   /* close file */
   NULLDO(fclose, f);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bmp_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "BMP format (%dx%dx%d) not supported", w, h, bpp);
   goto fail;
read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief import BMP images */
int _glhckImportBMP(const char *file, _glhckImportImageStruct *import)
{
   FILE *f;
   size_t dataSize, i;
   int w, h;
   unsigned short bpp;
   unsigned char *importData = NULL;
   CALL(0, "%s, %p", file, import);

   /* open BMP */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   if (_readBMPHeader(f, &w, &h, &bpp, &dataSize) != RETURN_OK)
      goto fail;

   if (bpp != 24 || dataSize <= 0)
      goto bmp_not_supported;

   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto bad_dimensions;

   if (!(importData = _glhckMalloc(dataSize)))
      goto out_of_memory;

   /* skip non important data */
   fseek(f, 24, SEEK_CUR);

   if (fread(importData, 1, dataSize, f) != dataSize)
      goto not_possible;

   /* read 24 bpp BMP data */
   for (i = 0; i < dataSize; i+=3) {
      unsigned char bgr[] = { importData[i+0], importData[i+1], importData[i+2] };
      importData[i+0] = bgr[2];
      importData[i+1] = bgr[1];
      importData[i+2] = bgr[0];
   }

   /* close file */
   NULLDO(fclose, f);

   import->width  = w;
   import->height = h;
   import->data   = importData;
   import->format = GLHCK_RGB;
   import->type   = GLHCK_UNSIGNED_BYTE;
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
bmp_not_supported:
   DEBUG(GLHCK_DBG_ERROR, "BMP format (%dx%dx%d) not supported", w, h, bpp);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed BMP file '%s' has too small filesize", file);
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "BMP image has invalid dimension %dx%d", w, h);
fail:
   IFDO(_glhckFree, importData);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
