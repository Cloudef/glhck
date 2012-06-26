/* TGA loader taken from imlib2 */

#include "../internal.h"
#include "import.h"
#include <stdio.h>

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

#define TGA_SIGNATURE "TRUEVISION-XFILE"

/* TGA pixel formats */
#define TGA_TYPE_MAPPED      1
#define TGA_TYPE_COLOR       2
#define TGA_TYPE_GRAY        3
#define TGA_TYPE_MAPPED_RLE  9
#define TGA_TYPE_COLOR_RLE  10
#define TGA_TYPE_GRAY_RLE   11

/* TGA header flags */
#define TGA_DESC_ABITS      0x0f
#define TGA_DESC_HORIZONTAL 0x10
#define TGA_DESC_VERTICAL   0x20

typedef struct {
   unsigned char       idLength;
   unsigned char       colorMapType;
   unsigned char       imageType;
   unsigned char       colorMapIndexLo, colorMapIndexHi;
   unsigned char       colorMapLengthLo, colorMapLengthHi;
   unsigned char       colorMapSize;
   unsigned char       xOriginLo, xOriginHi;
   unsigned char       yOriginLo, yOriginHi;
   unsigned char       widthLo, widthHi;
   unsigned char       heightLo, heightHi;
   unsigned char       bpp;
   unsigned char       descriptor;
} tga_header;

typedef struct {
   unsigned int        extensionAreaOffset;
   unsigned int        developerDirectoryOffset;
   char                signature[16];
   char                dot;
   char                null;
} tga_footer;

/* \brief import TGA images */
int _glhckImportTga(_glhckTexture *texture, const char *file, const unsigned int flags)
{
   FILE *f;
   void *seg = NULL, *data;
   int bpp, vinverted = 0;
   int rle = 0, footer_present = 0;
   size_t size;
   unsigned short w, h;
   tga_header *header;
   tga_footer *footer;

   CALL(0, "%p, %s, %d", texture, file, flags);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0L, SEEK_END);
   if ((size = ftell(f)) < sizeof(tga_header) +  sizeof(tga_footer))
      goto not_possible;

   if (!(seg = malloc(size)))
      goto out_of_memory;

   fseek(f, 0L, SEEK_SET);
   if (fread(seg, 1, size, f) != size)
      goto fail;

   /* we don't need the file anymore */
   fclose(f); f = NULL;

   data = seg;
   header = (tga_header*)data;
   footer = (tga_footer*)((char*)data+size-sizeof(tga_footer));

   /* is TGA v2.0? (stop storing the headers at EOF please) */
   if (!memcmp(footer->signature, TGA_SIGNATURE, sizeof(footer->signature)))
      footer_present = 1;

   /* skip over header */
   data = (char*)data+sizeof(tga_header);

   /* skip over ID filed */
   if (header->idLength)
      data = (char*)data+header->idLength;

   vinverted = !(header->descriptor & TGA_DESC_VERTICAL);
   switch (header->imageType) {
      case TGA_TYPE_COLOR_RLE:
      case TGA_TYPE_GRAY_RLE:
         rle = 1;
         break;

      case TGA_TYPE_COLOR:
      case TGA_TYPE_GRAY:
         rle = 0;
         break;

      default:
         goto unknown_type;
   }

   bpp = header->bpp;
   if (!((bpp == 32) || (bpp == 24) || (bpp == 8)))
      goto invalid_bpp;

   /* endian safe for 16-bit dimensions */
   w = (header->widthHi  << 8) | header->widthLo;
   h = (header->heightHi << 8) | header->heightLo;

   if (!_glhckIsValidImageDimension(w, h))
      goto bad_dimensions;

   /* load image data here */
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed TGA file '%s' has too small filesize", file);
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
unknown_type:
   DEBUG(GLHCK_DBG_ERROR, "Unknown TGA image type");
   goto fail;
invalid_bpp:
   DEBUG(GLHCK_DBG_ERROR, "TGA image is not either 32, 24 or 8 bpp");
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "TGA image has invalid dimension %dx%d", w, h);
fail:
   IFDO(fclose, f);
   IFDO(free, seg);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
