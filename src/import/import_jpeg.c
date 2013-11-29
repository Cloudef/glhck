/* JPEG loader taken from imlib2 */

#define _XOPEN_SOURCE
#include "../internal.h"
#include "import.h"
#include <stdio.h>
#include <jpeglib.h>
#include <setjmp.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

typedef struct jpegErrorStruct {
   struct jpeg_error_mgr pub;
   sigjmp_buf setjmp_buffer;
} jpegErrorStruct;

/* fatal failure handler */
static void _JPEGFatalErrorHandler(j_common_ptr cinfo)
{
   jpegErrorStruct *errmgr;
   (void)cinfo;
   errmgr = (jpegErrorStruct*)cinfo->err;
   cinfo->err->output_message(cinfo);
   siglongjmp(errmgr->setjmp_buffer, 1);
}

/* failure handler */
static void _JPEGErrorHandler(j_common_ptr cinfo)
{
   (void)cinfo;
}

/* failure handler */
static void _JPEGErrorHandler2(j_common_ptr cinfo, int msg_level)
{
   (void)cinfo; (void)msg_level;
}

/* \brief check if file is JPEG */
int _glhckFormatJPEG(const char *file)
{
   FILE *f = NULL;
   jpegErrorStruct jerr;
   struct jpeg_decompress_struct cinfo;
   CALL(0, "%s", file);

   /* JPEG error handlers, uh... */
   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit     = _JPEGFatalErrorHandler;
   jerr.pub.emit_message   = _JPEGErrorHandler2;
   jerr.pub.output_message = _JPEGErrorHandler;

   /* execute this on error */
   if (sigsetjmp(jerr.setjmp_buffer, 1)) {
      DEBUG(GLHCK_DBG_WARNING, "JPEG format check fails");
      goto fail;
   }

   /* open JPEG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* start the reading */
   jpeg_create_decompress(&cinfo);
   jpeg_stdio_src(&cinfo, f);
   jpeg_read_header(&cinfo, TRUE);
   jpeg_start_decompress(&cinfo);

   /* check headers */
   if (!IMAGE_DIMENSIONS_OK(cinfo.output_width, cinfo.output_height))
      goto fail;

   /* end decompress */
   jpeg_destroy_decompress(&cinfo);

   /* close file */
   NULLDO(fclose, f);

   /* no failures, prob JPEG */
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   if (f) {
      jpeg_destroy_decompress(&cinfo);
   }
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief import JPEG images */
int _glhckImportJPEG(const char *file, _glhckImportImageStruct *import)
{
   FILE *f = NULL;
   char decompress = 0;
   unsigned int w = 0, h = 0, loc = 0, i;
   unsigned char *importData = NULL;
   JSAMPROW row_pointer = NULL;
   jpegErrorStruct jerr;
   struct jpeg_decompress_struct cinfo;
   CALL(0, "%s, %p", file, import);

   /* JPEG error handlers, uh... */
   cinfo.err = jpeg_std_error(&(jerr.pub));
   jerr.pub.error_exit     = _JPEGFatalErrorHandler;
   jerr.pub.emit_message   = _JPEGErrorHandler2;
   jerr.pub.output_message = _JPEGErrorHandler;

   /* execute this on error */
   if (sigsetjmp(jerr.setjmp_buffer, 1)) {
      DEBUG(GLHCK_DBG_WARNING, "JPEG import fails");
      goto fail;
   }

   /* open JPEG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   /* start the reading */
   jpeg_create_decompress(&cinfo);
   jpeg_stdio_src(&cinfo, f);
   jpeg_read_header(&cinfo, TRUE);

   /* start decompression */
   decompress = 1;
   cinfo.do_fancy_upsampling = FALSE;
   cinfo.do_block_smoothing  = FALSE;
   jpeg_start_decompress(&cinfo);

   /* check dimensions */
   w = cinfo.output_width;
   h = cinfo.output_height;
   if (!IMAGE_DIMENSIONS_OK(w, h))
      goto bad_dimensions;

   /* check for bad JPEG */
   if (cinfo.rec_outbuf_height > 16 || cinfo.output_components <= 0)
      goto bad_jpeg;

   /* allocate import buffer */
   if (!(importData = _glhckMalloc(w * h * cinfo.output_components)))
      goto out_of_memory;

   /* allocate row pointer */
   if (!(row_pointer = _glhckMalloc(w * cinfo.output_components)))
      goto out_of_memory;

   /* read scanlines */
   while (cinfo.output_scanline < cinfo.output_height)  {
      jpeg_read_scanlines(&cinfo, &row_pointer, 1);
      for (i = 0; i < (w * cinfo.output_components); ++i) {
         importData[loc++] = row_pointer[i];
      }
   }

   /* invert */
   _glhckInvertPixels(importData, w, h, cinfo.output_components);

   /* finish decompression */
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);
   decompress = 0;

   /* close file */
   NULLDO(fclose, f);

   /* free */
   NULLDO(_glhckFree, row_pointer);

   /* fill import struct */
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
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "JPEG image has invalid dimension %dx%d", w, h);
bad_jpeg:
   DEBUG(GLHCK_DBG_ERROR, "Bad JPEG file, won't handle it: %s", file);
fail:
   if (f) {
      if (decompress) jpeg_finish_decompress(&cinfo);
      jpeg_destroy_decompress(&cinfo);
   }
   IFDO(_glhckFree, row_pointer);
   IFDO(_glhckFree, importData);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
