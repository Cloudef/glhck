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
   static const size_t headerSize = 2 + 16 + sizeof(int32_t) + sizeof(int32_t) + sizeof(uint16_t) + sizeof(uint16_t);
   chckBuffer *buf;
   assert(f && w && h && bpp && dataSize);

   if (!(buf = chckBufferNew(headerSize, CHCK_BUFFER_ENDIAN_LITTLE)))
      goto fail;

   if (fread(buf->buffer, 1, headerSize, f) != headerSize)
      goto fail;

   if (memcmp(buf->buffer, "BM", 2) != 0)
      goto fail;

   /* FIXME: maybe do function chckBufferSeek? */
   buf->curpos += 2 + 16;

   if (chckBufferReadInt32(buf, w) != RETURN_OK)
      goto fail;

   if (chckBufferReadInt32(buf, h) != RETURN_OK)
      goto fail;

   buf->curpos += sizeof(uint16_t);

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
   size_t dataSize, i, i2;
   int w, h;
   unsigned short bpp;
   unsigned char *importData = NULL, *bgr = NULL;
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

   if (!(importData = _glhckMalloc(w*h*4)) || !(bgr =_glhckMalloc(dataSize)))
      goto out_of_memory;

   /* skip non important data */
   fseek(f, 24, SEEK_CUR);

   if (fread(bgr, 1, dataSize, f) != dataSize)
      goto not_possible;

   /* read 24 bpp BMP data */
   for (i = 0, i2 = 0; i < dataSize; i+=3, i2+=4) {
      importData[i2+0] = bgr[i+2];
      importData[i2+1] = bgr[i+1];
      importData[i2+2] = bgr[i+0];
      importData[i2+3] = 255;
   }

   NULLDO(_glhckFree, bgr);

#if 0
   /* invert */
   for (i = 0; i*2 < h; ++i) {
      int index1 = i*w*4;
      int index2 = (h-1-i)*w*4;
      for (i2 = w*4; i2 > 0; --i2) {
         unsigned char temp = importData[index1];
         importData[index1] = importData[index2];
         importData[index2] = temp;
         ++index1; ++index2;
      }
   }
#endif

   /* close file */
   NULLDO(fclose, f);

   import->width  = w;
   import->height = h;
   import->data   = importData;
   import->format = GLHCK_RGBA;
   import->type   = GLHCK_DATA_UNSIGNED_BYTE;
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
   IFDO(_glhckFree, bgr);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
