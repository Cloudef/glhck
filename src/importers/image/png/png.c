/* PNG loader taken from imlib2 */

#include "importers/importer.h"

#include <png.h>
#include <setjmp.h>
#include <stdlib.h>

#include "trace.h"
#include "cdl/cdl.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

struct {
   int (*png_sig_cmp) (png_const_bytep, png_size_t, png_size_t);
   png_voidp (*png_get_io_ptr) (png_const_structrp png_ptr);
   png_structp (*png_create_read_struct) (png_const_charp, png_voidp, png_error_ptr, png_error_ptr);
   png_infop (*png_create_info_struct) (png_const_structrp png_ptr);
   void (*png_set_read_fn) (png_structrp, png_voidp, png_rw_ptr);
   void (*png_read_info) (png_structrp, png_inforp);
   png_uint_32 (*png_get_IHDR) (png_const_structrp, png_const_inforp, png_uint_32*, png_uint_32*, int*, int*, int*, int*, int*);
   png_uint_32 (*png_get_valid) (png_const_structrp, png_const_inforp, png_uint_32);
   void (*png_set_palette_to_rgb) (png_structrp);
   void (*png_set_gray_to_rgb) (png_structrp);
   void (*png_set_expand_gray_1_2_4_to_8) (png_structrp);
   void (*png_set_tRNS_to_alpha) (png_structrp);
   void (*png_set_strip_16) (png_structrp);
   void (*png_set_packing) (png_structrp);
   void (*png_set_filler) (png_structrp, png_uint_32, int);
   void (*png_read_image) (png_structrp, png_bytepp image);
   void (*png_read_end) (png_structrp, png_inforp);
   void (*png_destroy_read_struct) (png_structpp, png_infopp, png_infopp);
#if (PNG_LIBPNG_VER >= 10500)
   jmp_buf* (*png_set_longjmp_fn) (png_structrp, png_longjmp_ptr, size_t);
#endif
   void *handle;
} libpng;

static void read(png_structp png, png_bytep outBytes, png_size_t readSize)
{
   chckBuffer *buffer;

   if (!(buffer = libpng.png_get_io_ptr(png)))
      return;

   chckBufferRead(outBytes, 1, readSize, buffer);
}

static int check(chckBuffer *buffer)
{
   static const size_t PNG_BYTES_TO_CHECK = 4;
   CALL(0, "%p", buffer);

   if (chckBufferGetSize(buffer) < PNG_BYTES_TO_CHECK)
      goto fail;

   if (libpng.png_sig_cmp(chckBufferGetPointer(buffer), 0, PNG_BYTES_TO_CHECK) != 0)
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

   if (!(png = libpng.png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL)))
      goto out_of_memory;

   if (!(info = libpng.png_create_info_struct(png)))
      goto out_of_memory;

#if (PNG_LIBPNG_VER < 10500)
   if (setjmp(png->jmpbuf))
      goto fail;
#else
   if (setjmp((*libpng.png_set_longjmp_fn(png, longjmp, sizeof(jmp_buf)))))
      goto fail;
#endif

   libpng.png_set_read_fn(png, buffer, read);
   libpng.png_read_info(png, info);

   png_uint_32 w32, h32;
   int bitDepth, colorType, interlaceType;
   libpng.png_get_IHDR(png, info, (png_uint_32*)(&w32), (png_uint_32*)(&h32), &bitDepth, &colorType, &interlaceType, NULL, NULL);

   if (!IMAGE_DIMENSIONS_OK(w32, h32))
      goto bad_dimensions;

   /* has alpha? */
   int hasa = 0;
   if (libpng.png_get_valid(png, info, PNG_INFO_tRNS) || colorType == PNG_COLOR_TYPE_RGB_ALPHA || colorType == PNG_COLOR_TYPE_GRAY_ALPHA)
      hasa = 1;

   /* Prep for transformations...  ultimately we want ARGB */
   /* expand palette -> RGB if necessary */
   if (colorType == PNG_COLOR_TYPE_PALETTE)
      libpng.png_set_palette_to_rgb(png);

   /* expand gray (w/reduced bits) -> 8-bit RGB if necessary */
   if (colorType == PNG_COLOR_TYPE_GRAY || colorType == PNG_COLOR_TYPE_GRAY_ALPHA) {
      libpng.png_set_gray_to_rgb(png);
      if (bitDepth < 8)
         libpng.png_set_expand_gray_1_2_4_to_8(png);
   }

   /* expand transparency entry -> alpha channel if present */
   if (libpng.png_get_valid(png, info, PNG_INFO_tRNS))
      libpng.png_set_tRNS_to_alpha(png);

   /* reduce 16bit color -> 8bit color if necessary */
   if (bitDepth > 8)
      libpng.png_set_strip_16(png);

   /* pack all pixels to byte boundaries */
   libpng.png_set_packing(png);

   /* note from raster:                                                         */
   /* thanks to mustapha for helping debug this on PPC Linux remotely by        */
   /* sending across screenshots all the time and me figuring out from them     */
   /* what the hell was up with the colors                                      */
   /* now png loading should work on big-endian machines nicely                 */
#ifdef WORDS_BIGENDIAN
   if (!hasa) libpng.png_set_filler(png, 0xff, PNG_FILLER_BEFORE);
#else
   if (!hasa) libpng.png_set_filler(png, 0xff, PNG_FILLER_AFTER);
#endif

   if (!(importData = malloc(w32 * h32 * 4)))
      goto out_of_memory;

   if (!(lines = malloc(h32 * sizeof(unsigned char*))))
      goto out_of_memory;

   for (unsigned int i = 0; i < h32; ++i)
      lines[h32 - i - 1] = (unsigned char*)importData + i * w32 * 4;

   libpng.png_read_image(png, lines);

   NULLDO(free, lines);
   libpng.png_read_end(png, info);
   libpng.png_destroy_read_struct(&png, &info, (png_infopp)NULL);

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
      libpng.png_read_end(png, info);

   if (png)
      libpng.png_destroy_read_struct(&png, &info, (png_infopp)NULL);

   IFDO(free, lines);
   IFDO(free, importData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   const char *error = NULL, *func = NULL;
   if (!(libpng.handle = chckDlLoad("libpng.so", &error)))
      goto fail;

#define load(x) (libpng.x = chckDlLoadSymbol(libpng.handle, (func = #x), &error))
   if (!load(png_sig_cmp))
      goto fail;
   if (!load(png_get_io_ptr))
      goto fail;
   if (!load(png_create_read_struct))
      goto fail;
   if (!load(png_create_info_struct))
      goto fail;
   if (!load(png_set_read_fn))
      goto fail;
   if (!load(png_read_info))
      goto fail;
   if (!load(png_get_IHDR))
      goto fail;
   if (!load(png_get_valid))
      goto fail;
   if (!load(png_set_palette_to_rgb))
      goto fail;
   if (!load(png_set_gray_to_rgb))
      goto fail;
   if (!load(png_set_expand_gray_1_2_4_to_8))
      goto fail;
   if (!load(png_set_tRNS_to_alpha))
      goto fail;
   if (!load(png_set_strip_16))
      goto fail;
   if (!load(png_set_packing))
      goto fail;
   if (!load(png_set_filler))
      goto fail;
   if (!load(png_read_image))
      goto fail;
   if (!load(png_read_end))
      goto fail;
   if (!load(png_destroy_read_struct))
      goto fail;
#if (PNG_LIBPNG_VER >= 10500)
   if (!load(png_set_longjmp_fn))
      goto fail;
#endif
#undef load

   info->check = check;
   info->imageImport = import;

   RET(0, "%s", "glhck-importer-png");
   return "glhck-importer-png";

fail:
   DEBUG(GLHCK_DBG_ERROR, "%s: %s", (func ? func : ""), error);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
