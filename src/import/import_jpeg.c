/* JPEG loader taken from imlib2 */

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
   errmgr = (jpegErrorStruct*)cinfo->err;
   cinfo->err->output_message(cinfo);
   siglongjmp(errmgr->setjmp_buffer, 1);
}

/* failure handler */
static void _JPEGErrorHandler(j_common_ptr cinfo)
{
}

/* failure handler */
static void _JPEGErrorHandler2(j_common_ptr cinfo, int msg_level)
{
}

/* \brief check if file is JPEG */
int _glhckFormatJPEG(const char *file)
{
   FILE *f;
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
int _glhckImportJPEG(_glhckTexture *texture, const char *file, unsigned int flags)
{
   FILE *f;
   char decompress = 0;
   unsigned int w = 0, h = 0, loc = 0, i, i2;
   unsigned char *import = NULL;
   JSAMPROW row_pointer = NULL;
   jpegErrorStruct jerr;
   struct jpeg_decompress_struct cinfo;
   CALL(0, "%p, %s, %u", texture, file, flags);

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
   if (!(import = _glhckMalloc(w * h * cinfo.output_components)))
      goto out_of_memory;

   /* allocate row pointer */
   if (!(row_pointer = _glhckMalloc(w * cinfo.output_components)))
      goto out_of_memory;

   /* read scanlines */
   while (cinfo.output_scanline != cinfo.output_height)  {
      jpeg_read_scanlines(&cinfo, &row_pointer, 1);
      for (i = 0; i != (w * cinfo.output_components); ++i) {
         import[loc++] = row_pointer[i];
      }
   }

   /* invert */
   for (i = 0; i*2 < h; ++i) {
      int index1 = i*w*cinfo.output_components;
      int index2 = (h-1-i)*w*cinfo.output_components;
      for (i2 = w*cinfo.output_components; i2 != 0; --i2) {
         unsigned char temp = import[index1];
         import[index1] = import[index2];
         import[index2] = temp;
         ++index1; ++index2;
      }
   }

   /* finish decompression */
   jpeg_finish_decompress(&cinfo);
   jpeg_destroy_decompress(&cinfo);
   decompress = 0;

   /* close file */
   NULLDO(fclose, f);

   /* do post processing to imported data, and assign to texture */
   _glhckImagePostProcessStruct importData;
   memset(&importData, 0, sizeof(_glhckImagePostProcessStruct));
   importData.width  = w;
   importData.height = h;
   importData.data   = import;
   importData.format = GLHCK_RGB;
   importData.type   = GLHCK_DATA_UNSIGNED_BYTE;
   if (_glhckImagePostProcess(texture, &importData) != RETURN_OK)
      goto fail;

   /* free */
   NULLDO(_glhckFree, row_pointer);
   NULLDO(_glhckFree, import);

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
   IFDO(_glhckFree, import);
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
