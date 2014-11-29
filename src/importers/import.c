#include "importer.h"

#include <stdio.h>  /* for fopen */
#include <stdlib.h>

#include "trace.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

enum importerType {
   MODEL,
   IMAGE
};

/* \brief check against known model format headers */
static const struct glhckImporterInfo* modelImporter(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);

   for (size_t i = 0; i < glhckImporterCount(); ++i) {
      const char *name = NULL;
      const struct glhckImporterInfo *info = _glhckImporterGetInfo(i, &name);

      if (info->modelImport && info->check(buffer)) {
         RET(0, "%s", name);
         return info;
      }

      chckBufferSeek(buffer, 0L, SEEK_SET);
   }

   RET(0, "%p", NULL);
   return NULL;
}

/* \brief check against known image format headers */
static const struct glhckImporterInfo* imageImporter(chckBuffer *buffer)
{
   CALL(0, "%p", buffer);

   for (size_t i = 0; i < glhckImporterCount(); ++i) {
      const char *name = NULL;
      const struct glhckImporterInfo *info = _glhckImporterGetInfo(i, &name);

      if (info->imageImport && info->check(buffer)) {
         RET(0, "%s", name);
         return info;
      }

      chckBufferSeek(buffer, 0L, SEEK_SET);
   }

   RET(0, "%p", NULL);
   return NULL;
}

static void* import(chckBuffer *buffer, const enum importerType type)
{
   void *importData = NULL;
   CALL(0, "%p", buffer);

   static const size_t size[] = {
      sizeof(glhckImportModelStruct), // MODEL
      sizeof(glhckImportImageStruct), // IMAGE
   };

   static const struct glhckImporterInfo* (*importer[])(chckBuffer*) = {
      modelImporter, // MODEL
      imageImporter, // IMAGE
   };

   const struct glhckImporterInfo *info;
   if (!(info = importer[type](buffer)))
      goto no_importer;

   if (!(importData = calloc(1, size[type])))
      goto fail;

   int (*importFun[])(chckBuffer*, void*) = {
      (void*)info->modelImport, // MODEL
      (void*)info->imageImport, // IMAGE
   };

   chckBufferSeek(buffer, 0L, SEEK_SET);
   if (importFun[type](buffer, importData) != RETURN_OK)
      goto fail;

   RET(0, "%p", importData);
   return importData;

no_importer:
   DEBUG(GLHCK_DBG_ERROR, "No suitable importers found.");
   DEBUG(GLHCK_DBG_ERROR, "If the format is supported, make sure you have compiled the library with the support.");
fail:
   IFDO(free, importData);
   RET(0, "%p", NULL);
   return NULL;
}

static void* importData(const void *data, const size_t size, const enum importerType type)
{
   chckBuffer *buffer;
   CALL(0, "%p, %zu", data, size);

   if (!(buffer = chckBufferNewFromPointer(data, size, CHCK_BUFFER_ENDIAN_NATIVE)))
      goto fail;

   void *importData = import(buffer, type);
   chckBufferFree(buffer);

   RET(0, "%p", importData);
   return importData;

fail:
   RET(0, "%p", NULL);
   return NULL;
}

static void* importFile(const char *file, const enum importerType type)
{
   FILE *f = NULL;
   CALL(0, "%s", file);

   if (!(f = fopen(file, "rb")))
      goto read_fail;

   fseek(f, 0L, SEEK_END);
   size_t size = ftell(f);

   chckBuffer *buffer;
   if (!(buffer = chckBufferNew(size, CHCK_BUFFER_ENDIAN_NATIVE)))
      goto fail;

   fseek(f, 0L, SEEK_SET);
   chckBufferFillFromFile(f, 1, size, buffer);
   fclose(f);

   void *importData = import(buffer, type);
   chckBufferFree(buffer);

   RET(0, "%p", importData);
   return importData;

read_fail:
   DEBUG(GLHCK_DBG_ERROR, "Could not access file: %s", file);
fail:
   IFDO(fclose, f);
   RET(0, "%p", NULL);
   return NULL;
}

GLHCKAPI glhckImportModelStruct* glhckImportModelData(const void *data, const size_t size)
{
   return importData(data, size, MODEL);
}

GLHCKAPI glhckImportModelStruct* glhckImportModelFile(const char *file)
{
   return importFile(file, MODEL);
}

GLHCKAPI glhckImportImageStruct* glhckImportImageData(const void *data, const size_t size)
{
   return importData(data, size, IMAGE);
}

GLHCKAPI glhckImportImageStruct* glhckImportImageFile(const char *file)
{
   return importFile(file, IMAGE);
}

/* vim: set ts=8 sw=3 tw=0 :*/
