/* JPEG loader taken from imlib2 */

#include "../internal.h"
#include "../helper/imagedata.h"
#include "import.h"
#include <stdio.h>

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

/* \brief check if file is JPEG */
int _glhckFormatJPEG(const char *file)
{
   FILE *f;
   char isJPEG = 0;
   CALL(0, "%s", file);

   /* open PNG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   DEBUG(GLHCK_DBG_WARNING, "JPEG loader not implemented");

   /* close file */
   NULLDO(fclose, f);

   RET(0, "%d", isJPEG?RETURN_OK:RETURN_FAIL);
   return isJPEG?RETURN_OK:RETURN_FAIL;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief import JPEG images */
int _glhckImportJPEG(_glhckTexture *texture, const char *file, unsigned int flags)
{
   FILE *f;
   unsigned int w = 0, h = 0;
   CALL(0, "%p, %s, %u", texture, file, flags);

   /* open JPEG */
   if (!(f = fopen(file, "rb")))
      goto read_fail;

   DEBUG(GLHCK_DBG_WARNING, "JPEG loader not implemented");

   /* close file */
   NULLDO(fclose, f);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to open: %s", file);
   goto fail;
not_possible:
   DEBUG(GLHCK_DBG_ERROR, "Assumed JPEG file '%s' has too small filesize", file);
   goto fail;
out_of_memory:
   DEBUG(GLHCK_DBG_ERROR, "Out of memory, won't load file: %s", file);
   goto fail;
bad_dimensions:
   DEBUG(GLHCK_DBG_ERROR, "PNG image has invalid dimension %dx%d", w, h);
fail:
   IFDO(fclose, f);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}
