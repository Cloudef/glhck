#include "importer.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "trace.h"
#include "cdl/cdl.h"
#include "renderers/tinydir.h"
#include "pool/pool.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_IMPORT

struct importer {
   void *handle;
   char *file, *name;
   struct glhckImporterInfo info;
};

static chckIterPool *importers;

static const char *ipath = "importers";

static char* pstrdup(const char *str)
{
   size_t size = strlen(str);
   char *cpy = calloc(1, size + 1);
   return (cpy ? memcpy(cpy, str, size) : NULL);
}

static int load(const char *file, struct importer *importer)
{
   void *handle;
   const char *error = NULL;

   memset(importer, 0, sizeof(struct importer));

   if (!(handle = chckDlLoad(file, &error)))
      goto fail;

   const char* (*regfun)(struct glhckImporterInfo*);
   if (!(regfun = chckDlLoadSymbol(handle, "glhckImporterRegister", &error)))
      goto fail;

   importer->file = pstrdup(file);
   importer->handle = handle;
   const char *name = regfun(&importer->info);
   importer->name = (name ? pstrdup(name) : NULL);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   DEBUG(GLHCK_DBG_ERROR, "%s", error);
   IFDO(chckDlUnload, handle);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

int _glhckImporterInit(void)
{
   TRACE(0);

   /* no TLS for reason, so return_ok if done already. */
   if (importers)
      return RETURN_OK;

   const char *path = getenv("GLHCK_IMPORTERS");

   if (!path || access(path, R_OK) == -1)
      path = ipath;

   tinydir_dir dir;
   if (tinydir_open(&dir, path) == -1)
      goto fail;

   size_t registered = 0;
   while (dir.has_next) {
      tinydir_file file;
      tinydir_readfile(&dir, &file);

      if (!file.is_dir && !strncmp(file.name, "glhck-importer-", strlen("glhck-importer-"))) {
         size_t len = snprintf(NULL, 0, "%s/%s", file.name, path);

         char *fpath;
         if ((fpath = calloc(1, len + 1))) {
            sprintf(fpath, "%s/%s", path, file.name);

            if (_glhckImporterRegister(fpath) == RETURN_OK)
               registered++;

            free(fpath);
         }
      }

      tinydir_next(&dir);
   }

   tinydir_close(&dir);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

void _glhckImporterTerminate(void)
{
   TRACE(0);

   struct importer *i;
   for (chckPoolIndex iter = 0; (i = chckIterPoolIter(importers, &iter));) {
      IFDO(free, i->file);
      IFDO(free, i->name);
      IFDO(chckDlUnload, i->handle);
   }

   IFDO(chckIterPoolFree, importers);
}

int _glhckImporterRegister(const char *file)
{
   CALL(0, "%s", file);

   if (!importers && !(importers = chckIterPoolNew(1, 1, sizeof(struct importer))))
      goto fail;

   struct importer i;
   if (load(file, &i) != RETURN_OK)
      goto fail;

   DEBUG(GLHCK_DBG_CRAP, "registered: %s", i.name);
   chckIterPoolAdd(importers, &i, NULL);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

const struct glhckImporterInfo* _glhckImporterGetInfo(const size_t importer, const char **outName)
{
   CALL(0, "%zu, %p", importer, outName);

   if (outName)
      *outName = NULL;

   if (!importers || importer >= chckIterPoolCount(importers))
      return NULL;

   struct importer *i = chckIterPoolGet(importers, importer);

   if (outName)
      *outName = i->name;

   RET(0, "%p", &i->info);
   return &i->info;
}

GLHCKAPI size_t glhckImporterCount(void)
{
   return (importers ? chckIterPoolCount(importers) : 0);
}

GLHCKAPI const char* glhckImporterGetName(const size_t importer)
{
   CALL(0, "%zu", importer);

   if (!importers || importer >= chckIterPoolCount(importers))
      return NULL;

   struct importer *i = chckIterPoolGet(importers, importer);

   RET(0, "%s", i->name);
   return i->name;
}

/* vim: set ts=8 sw=3 tw=0 :*/
