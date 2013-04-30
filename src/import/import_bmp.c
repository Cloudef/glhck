/* very crude BMP loader, no point supporting
 * this format more than this */

#include "../internal.h"
#include "import.h"
#include <stdio.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* \brief check if file is BMP */
int _glhckFormatBMP(const char *file)
{
   FILE *f;
   unsigned short bpp;
   unsigned char header[2];
   CALL(0, "%s", file);

   /* open BMP */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* check header */
   memset(header, 0, sizeof(header));
   if (fread(header, 1, 2, f) != 2)
      goto fail;

   if (header[0] != 'B' || header[1] != 'M')
      goto fail;

   /* check bpp */
   fseek(f, 16+4+4+2, SEEK_CUR);
   if (fread(&bpp, 2, 1, f) != 1)
      goto fail;

   if (bpp != 24)
      goto fail;

   /* close file */
   NULLDO(fclose, f);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

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
   int i, i2, w, h, dataSize;
   unsigned short bpp;
   unsigned char bgr[3], *importData = NULL;
   CALL(0, "%s, %p", file, import);

   /* open BMP */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 18, SEEK_CUR);
   if (fread(&w, 4, 1, f) != 1)
      goto not_possible;
   if (fread(&h, 4, 1, f) != 1)
      goto not_possible;
   fseek(f, 2, SEEK_CUR);
   if (fread(&bpp, 2, 1, f) != 1)
      goto not_possible;
   fseek(f, 24, SEEK_CUR);

   if (bpp != 24) goto not_24bpp;
   dataSize = w*h*3;

   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto bad_dimensions;

   if (!(importData = _glhckMalloc(w*h*4)))
      goto out_of_memory;

   /* read 24 bpp BMP data */
   for (i = 0, i2 = 0; i+3 <= dataSize; i+=3) {
      if (fread(bgr, 1, 3, f) != 3)
         goto not_possible;

      importData[i2+0] = bgr[2];
      importData[i2+1] = bgr[1];
      importData[i2+2] = bgr[0];
      importData[i2+3] = 255;
      i2+=4;
   }

#if 0
   /* invert */
   for (i = 0; i*2 < h; ++i) {
      int index1 = i*w*4;
      int index2 = (h-1-i)*w*4;
      for (i2 = w*4; i2 != 0; --i2) {
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
   import->data   = import;
   import->format = GLHCK_RGBA;
   import->type   = GLHCK_DATA_UNSIGNED_BYTE;
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed BMP file '%s' has too small filesize", file);
   goto fail;
not_24bpp:
   DEBUG(GLHCK_DBG_ERROR, "BMP loader only supports 24bpp bmps");
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "BMP image has invalid dimension %dx%d", w, h);
fail:
   IFDO(_glhckFree, import);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
