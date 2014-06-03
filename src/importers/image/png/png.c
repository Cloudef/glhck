/* PNG loader taken from imlib2 */

#include "importers/importer.h"

#include <png.h>
#include <setjmp.h>
#include <stdlib.h>

#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#define PNG_BYTES_TO_CHECK 4

static void read(png_structp png, png_bytep outBytes, png_size_t readSize)
{
   chckBuffer *buffer;

   if (!(buffer = png_get_io_ptr(png)))
      return;

   chckBufferRead(outBytes, 1, readSize, buffer);
}

static int check(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);

   if (chckBufferGetSize(buffer) < PNG_BYTES_TO_CHECK)
      goto fail;

   if (png_sig_cmp(chckBufferGetPointer(buffer), 0, PNG_BYTES_TO_CHECK) != 0)
      goto fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

static int import(chckBuffer *buffer, glhckImportImageStruct *import)
{
   png_structp png = NULL;
   png_infop info = NULL;
   unsigned char **lines = NULL, *importData = NULL;
   CALL(0, "%p, %p", buffer, import);

   if (!check(buffer))
      goto not_possible;

   if (!(png = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
      goto out_of_memory;

   if (!(info = png_create_info_struct(png)))
      goto out_of_memory;

#if (PNG_LIBPNG_VER < 10500)
   if (setjmp(png->jmpbuf))
      goto fail;
#else
   if (setjmp(png_jmpbuf(png)))
      goto fail;
#endif

   png_set_read_fn(png, buffer, read);
   png_read_info(png, info);

   png_uint_32 w32, h32;
   int bitDepth, colorType, interlaceType;
   png_get_IHDR(png, info, (png_uint_32*)(&w32), (png_uint_32*)(&h32), &bitDepth, &colorType, &interlaceType, NULL, NULL);

   if (!IMAGE_DIMENSIONS_OK(w32, h32))
      goto bad_dimensions;

   /* has alpha? */
   int hasa = 0;
   if (png_get_valid(png, info, PNG_INFO_tRNS) || colorType == PNG_COLOR_TYPE_RGB_ALPHA || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
      hasa = 1;

   /* Prep for transformations...  ultimately we want ARGB */
   /* expand palette -> RGB if necessary */
   if (colorType == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png);

   /* expand gray (w/reduced bits) -> 8-bit RGB if necessary */
   if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
      png_set_gray_to_rgb(png);
      if (bitDepth < 8)
         png_set_expand_gray_1_2_4_to_8(png);
   }

   /* expand transparency entry -> alpha channel if present */
   if (png_get_valid(png, info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png);

   /* reduce 16bit color -> 8bit color if necessary */
   if (bitDepth > 8)
      png_set_strip_16(png);

   /* pack all pixels to byte boundaries */
   png_set_packing(png);

   /* note from raster:                                                         */
   /* thanks to mustapha for helping debug this on PPC Linux remotely by        */
   /* sending across screenshots all the time and me figuring out from them     */
   /* what the hell was up with the colors                                      */
   /* now png loading should work on big-endian machines nicely                 */
#ifdef WORDS_BIGENDIAN
   if (!hasa) png_set_filler(png, 0xff, PNG_FILLER_BEFORE);
#else
   if (!hasa) png_set_filler(png, 0xff, PNG_FILLER_AFTER);
#endif

   if (!(importData = malloc(w32 * h32 * 4)))
      goto out_of_memory;

   if (!(lines = malloc(h32 * sizeof(unsigned char*))))
      goto out_of_memory;

   for (unsigned int i = 0; i < h32; ++i)
      lines[h32 - i - 1] = (unsigned char*)importData + i * w32 * 4;

   png_read_image(png, lines);

   NULLDO(free, lines);
   png_read_end(png, info);
   png_destroy_read_struct(&png, &info, (png_infopp)NULL);

   /* fill import struct */
   import->width = w32;
   import->height = h32;
   import->data = importData;
   import->format = GLHCK_RGBA;
   import->type = GLHCK_UNSIGNED_BYTE;
   import->hasAlpha = hasa;
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed PNG data has too small size");
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load png");
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "PNG image has invalid dimension %dx%d", w32, h32);
fail:
   if (png && info)
      png_read_end(png, info);

   if (png)
      png_destroy_read_struct(&png, &info, (png_infopp)NULL);

   IFDO(free, lines);
   IFDO(free, importData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->imageImport = import;

   RET(0, "%s", "glhck-importer-png");
   return "glhck-importer-png";
}

/* vim: set ts=8 sw=3 tw=0 :*/
