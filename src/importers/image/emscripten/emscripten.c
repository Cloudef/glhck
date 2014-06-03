/* Emscripten importer */

#include "importers/importer.h"

#include <stdlib.h> /* for memory */

/* Doesn't actually need SDL_Image, this is wrapped by emscripten */
#include <SDL/SDL_image.h>

#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

static int check(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);
   assert(buffer);

   /**
    * We assume that emscripten is the only importer when build with emscripten support.
    * So the loading phase is also our "check".
    */

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

static int import(chckBuffer *buffer, glhckImportImageStruct *import)
{
   SDL_Surface *surface = NULL;
   unsigned char *importData = NULL;
   CALL(0, "%p, %p", buffer, import);

   size_t size = chckBufferGetSize(buffer);

   if (!(surface = IMG_Load_RW(SDL_RWFromConstMem(chckBufferGetPointer(buffer), size), 1)))
      goto fail;

   unsigned char hasa = 0;
   glhckTextureFormat format;
   if (surface->format->BytesPerPixel == 4) {
      hasa = 1;
      format = (surface->format->Rmask == 0x000000ff ? GLHCK_RGBA : GLHCK_BGRA);
   } else if (surface->format->BytesPerPixel == 3) {
      format = (surface->format->Rmask == 0x000000ff ? GLHCK_RGB : GLHCK_BGR);
   } else {
      goto not_truecolor;
   }

   if (!(importData = malloc(surface->w * surface->h * 4)))
      goto fail;

   memcpy(importData, surface->pixels, surface->w * surface->h * 4);
   _glhckInvertPixels(importData, surface->w, surface->h, surface->format->BytesPerPixel);

   import->width  = surface->w;
   import->height = surface->h;
   import->data   = importData;
   import->format = format;
   import->type   = GLHCK_UNSIGNED_BYTE;
   import->hasAlpha = hasa;

   SDL_FreeSurface(surface);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

not_truecolor:
   DEBUG(GLHCK_DBG_ERROR, "SDL_Surface not truecolor, will bail out!");
fail:
   IFDO(SDL_FreeSurface, surface);
   IFDO(free, importData);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

GLHCKAPI const char* glhckImporterRegister(struct glhckImporterInfo *info)
{
   CALL(0, "%p", info);

   info->check = check;
   info->imageImport = import;

   RET(0, "%s", "glhck-importer-emscripten");
   return "glhck-importer-emscripten";
}

/* vim: set ts=8 sw=3 tw=0 :*/
