/* TGA loader taken from imlib2 */

#include "../internal.h"
#include "import.h"
#include <stdio.h>

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
   unsigned char idLength;
   unsigned char colorMapType;
   unsigned char imageType;
   unsigned char colorMapIndexLo, colorMapIndexHi;
   unsigned char colorMapLengthLo, colorMapLengthHi;
   unsigned char colorMapSize;
   unsigned char xOriginLo, xOriginHi;
   unsigned char yOriginLo, yOriginHi;
   unsigned char widthLo, widthHi;
   unsigned char heightLo, heightHi;
   unsigned char bpp;
   unsigned char descriptor;
} tga_header;

typedef struct {
   unsigned int extensionAreaOffset;
   unsigned int developerDirectoryOffset;
   char signature[16];
   char dot;
   char null;
} tga_footer;

/* \brief check if file is TGA */
int _glhckFormatTGA(const char *file)
{
   FILE *f = NULL;
   void *data = NULL;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0L, SEEK_END);

   size_t size;
   if ((size = ftell(f)) < sizeof(tga_header) + sizeof(tga_footer))
      goto fail;

   if (!(data = _glhckMalloc(size)))
      goto out_of_memory;

   fseek(f, 0L, SEEK_SET);
   if (fread(data, 1, size, f) != size)
      goto fail;

   /* we don't need the file anymore */
   NULLDO(fclose, f);
   tga_header *header = (tga_header*)data;
   tga_footer *footer = (tga_footer*)((char*)data+size-sizeof(tga_footer));

   /* is TGA v2.0? (stop storing the headers at EOF please) */
   int isTga = 0;
   if (memcmp(footer->signature, TGA_SIGNATURE, sizeof(footer->signature)))
      isTga = 1;
   else if ((header->bpp == 32) || (header->bpp == 24) || (header->bpp == 8))
      isTga = 1;

   NULLDO(_glhckFree, data);

   RET(0, "%d", (isTga ? RETURN_OK : RETURN_FAIL));
   return (isTga ? RETURN_OK : RETURN_FAIL);

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
fail:
   IFDO(fclose, f);
   IFDO(_glhckFree, data);
   return RETURN_FAIL;
}

/* \brief import TGA images */
int _glhckImportTGA(const char *file, _glhckImportImageStruct *import)
{
   FILE *f = NULL;
   void *seg = NULL;
   unsigned char *importData = NULL;
   CALL(0, "%s, %p", file, import);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0L, SEEK_END);

   size_t size;
   if ((size = ftell(f)) < sizeof(tga_header) + sizeof(tga_footer))
      goto not_possible;

   if (!(seg = _glhckMalloc(size)))
      goto out_of_memory;

   fseek(f, 0L, SEEK_SET);
   if (fread(seg, 1, size, f) != size)
      goto fail;

   /* we don't need the file anymore */
   NULLDO(fclose, f);

   void *data = seg;
   tga_header *header = (tga_header*)data;
   tga_footer *footer = (tga_footer*)((char*)data + size - sizeof(tga_footer));

   /* is TGA v2.0? (stop storing the headers at EOF please) */
   int footerPresent = 0;
   if (!memcmp(footer->signature, TGA_SIGNATURE, sizeof(footer->signature)))
      footerPresent = 1;

   /* skip over header */
   data = (char*)data + sizeof(tga_header);

   /* skip over ID filed */
   if (header->idLength)
      data = (char*)data + header->idLength;

   /* inverted TGA? */
   int vinverted = !(header->descriptor & TGA_DESC_VERTICAL);

   int rle = 0;
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

   unsigned int bpp = header->bpp;
   if (!((bpp == 32) || (bpp == 24) || (bpp == 8)))
      goto invalid_bpp;

   /* endian safe for 16-bit dimensions */
   unsigned short w = (header->widthHi  << 8) | header->widthLo;
   unsigned short h = (header->heightHi << 8) | header->heightLo;

   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto bad_dimensions;

   /* allocate destination buffer */
   if (!(importData = _glhckMalloc(w * h * 4)))
      goto out_of_memory;

   /* find out how much data to be read from file
    * (this is NOT simply width * height * 4, due to compression) */
   size_t datasize = size - sizeof(tga_header) - header->idLength - (footerPresent ? sizeof(tga_footer) : 0);

   /* bufptr is the next byte to be read from the buffer */
   unsigned char *bufptr = data;
   unsigned char *bufend = data + datasize;

   /* non RLE compressed data */
   if (!rle) {
      for (int i = 0; i < h * w && bufptr + bpp / 8 <= bufend; ++i) {
         switch (bpp) {
            /* 32-bit BGRA */
            case 32:
               importData[i*4+0] = bufptr[2];
               importData[i*4+1] = bufptr[1];
               importData[i*4+2] = bufptr[0];
               importData[i*4+3] = bufptr[3];
               bufptr += 4;
            break;

            /* 24-bit BGR */
            case 24:
               importData[i*4+0] = bufptr[2];
               importData[i*4+1] = bufptr[1];
               importData[i*4+2] = bufptr[0];
               importData[i*4+3] = 255;
               bufptr += 3;
            break;

            /* 8-bit grayscale */
            case 8:
               importData[i*4+0] = bufptr[0];
               importData[i*4+1] = bufptr[0];
               importData[i*4+2] = bufptr[0];
               importData[i*4+3] = 255;
               bufptr += 1;
            break;
         }
      }
   }

   /* RLE compressed data */
   if (rle) {
      DEBUG(GLHCK_DBG_ERROR, "RLE compressed import not yet implemented.");
      goto fail;
   }

   /* some TGA's are upside-down */
   if (vinverted)
      _glhckInvertPixels(importData, w, h, 4);

   /* free */
   NULLDO(_glhckFree, seg);

   /* fill import struct */
   import->width = w;
   import->height = h;
   import->data = importData;
   import->format = GLHCK_RGBA;
   import->type = GLHCK_UNSIGNED_BYTE;
   import->flags |= (bpp == 32 ? GLHCK_TEXTURE_IMPORT_ALPHA : 0);
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
   IFDO(_glhckFree, importData);
   IFDO(_glhckFree, seg);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
