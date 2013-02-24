/* PNG loader taken from imlib2 */

#include "../internal.h"
#include "import.h"
#include <stdio.h>
#include <png.h>
#include <setjmp.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* PNG stuff */
#define PNG_BYTES_TO_CHECK 4

/* \brief check if file is PNG */
int _glhckFormatPNG(const char *file)
{
   FILE *f;
   unsigned char buf[PNG_BYTES_TO_CHECK], isPNG = 0;
   CALL(0, "%s", file);

   /* open PNG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* read header */
   memset(buf, 0, sizeof(buf));
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, f) != PNG_BYTES_TO_CHECK)
      goto fail;

   /* check magic header */
   if (png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK) == 0)
      isPNG = 1;

   /* close file */
   NULLDO(fclose, f);

   RET(0, "%d", isPNG?RETURN_OK:RETURN_FAIL);
   return isPNG?RETURN_OK:RETURN_FAIL;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief import PNG images */
int _glhckImportPNG(_glhckTexture *texture, const char *file, unsigned int flags)
{
   FILE *f;
   char hasa = 0;
   int i, w, h, bit_depth, color_type, interlace_type;
   png_uint_32 w32, h32;
   png_structp png = NULL;
   png_infop info = NULL;
   unsigned char buf[PNG_BYTES_TO_CHECK], **lines = NULL, *import = NULL;
   CALL(0, "%p, %s, %u", texture, file, flags);

   /* open PNG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* read header */
   memset(buf, 0, sizeof(buf));
   if (fread(buf, 1, PNG_BYTES_TO_CHECK, f) != PNG_BYTES_TO_CHECK)
      goto fail;

   /* check magic header */
   if (png_sig_cmp(buf, 0, PNG_BYTES_TO_CHECK))
      goto not_possible;
   rewind(f);

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

   png_init_io(png, f);
   png_read_info(png, info);
   png_get_IHDR(png, info, (png_uint_32*)(&w32),
         (png_uint_32*)(&h32), &bit_depth, &color_type,
         &interlace_type, NULL, NULL);
   w = (int)w32; h = (int)h32;

   if (!IMAGE_DIMENSIONS_OK(w32, h32))
      goto bad_dimensions;

   /* has alpha? */
   if (png_get_valid(png, info, PNG_INFO_tRNS))
      hasa = 1;
   if (color_type == PNG_COLOR_TYPE_RGB_ALPHA)
      hasa = 1;
   if (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
      hasa = 1;

   /* Prep for transformations...  ultimately we want ARGB */
   /* expand palette -> RGB if necessary */
   if (color_type == PNG_COLOR_TYPE_PALETTE)
      png_set_palette_to_rgb(png);
   /* expand gray (w/reduced bits) -> 8-bit RGB if necessary */
   if ((color_type == PNG_COLOR_TYPE_GRAY) ||
         (color_type == PNG_COLOR_TYPE_GRAY_ALPHA)) {
      png_set_gray_to_rgb(png);
      if (bit_depth < 8)
         png_set_expand_gray_1_2_4_to_8(png);
   }
   /* expand transparency entry -> alpha channel if present */
   if (png_get_valid(png, info, PNG_INFO_tRNS))
      png_set_tRNS_to_alpha(png);
   /* reduce 16bit color -> 8bit color if necessary */
   if (bit_depth > 8)
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

   if (!(import = _glhckMalloc(w*h*4)))
      goto out_of_memory;

   if (!(lines = _glhckMalloc(h*sizeof(unsigned char*))))
      goto out_of_memory;

   for (i = 0; i != h; ++i)
      lines[h-i-1] = (unsigned char*)import+i*w*4;

   png_read_image(png, lines);

   NULLDO(_glhckFree, lines);
   png_read_end(png, info);
   png_destroy_read_struct(&png, &info, (png_infopp)NULL);

   /* close file */
   NULLDO(fclose, f);

   /* set internal texture flags */
   texture->importFlags |= (hasa==1?GLHCK_TEXTURE_IMPORT_ALPHA:0);

   /* do post processing to imported data, and assign to texture */
   _glhckImagePostProcessStruct importData;
   memset(&importData, 0, sizeof(_glhckImagePostProcessStruct));
   importData.width  = w;
   importData.height = h;
   importData.data   = import;
   importData.format = GLHCK_RGBA;
   importData.type   = GLHCK_DATA_UNSIGNED_BYTE;
   if (_glhckImagePostProcess(texture, &importData) != RETURN_OK)
      goto fail;

   /* free */
   NULLDO(_glhckFree, import);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed PNG file '%s' has too small filesize", file);
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "PNG image has invalid dimension %dx%d", w, h);
fail:
   if (png && info) png_read_end(png, info);
   if (png)         png_destroy_read_struct(&png, &info, (png_infopp)NULL);
   IFDO(_glhckFree, lines);
   IFDO(_glhckFree, import);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
