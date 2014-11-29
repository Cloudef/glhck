#include "renderer.h"
#include "render.h"

#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>

#include "trace.h"
#include "system/tls.h"
#include "cdl/cdl.h"
#include "tinydir.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER

#ifndef __STRING
#  define __STRING(x) #x
#endif

struct renderer {
   void *handle;
   char *file, *name;
   struct glhckRendererInfo info;
};

static _GLHCK_TLS chckIterPool *renderers;
static _GLHCK_TLS chckPoolIndex active = INVALID_RENDERER;

static const char *rpath = "renderers";

static char* pstrdup(const char *str)
{
   size_t size = strlen(str);
   char *cpy = calloc(1, size + 1);
   return (cpy ? memcpy(cpy, str, size) : NULL);
}

static void apicheck(const char *name, struct glhckRendererAPI *api)
{
#define API_CHECK(x) if (!api->x) DEBUG(GLHCK_DBG_ERROR, "\4%s\1: missing render API function\5: %s", name, __STRING(x))
   API_CHECK(time);
   API_CHECK(viewport);
   API_CHECK(projectionMatrix);
   API_CHECK(modelMatrix);
   API_CHECK(clearColor);
   API_CHECK(clear);
   API_CHECK(color);
   API_CHECK(vertexPointer);
   API_CHECK(coordPointer);
   API_CHECK(normalPointer);
   API_CHECK(drawArrays);
   API_CHECK(drawElements);
   API_CHECK(bufferGetPixels);
   API_CHECK(textureGenerate);
   API_CHECK(textureDelete);
   API_CHECK(textureBind);
   API_CHECK(textureActive);
   API_CHECK(textureFill);
   API_CHECK(textureImage);
   API_CHECK(textureParameter);
   API_CHECK(renderbufferGenerate);
   API_CHECK(renderbufferDelete);
   API_CHECK(renderbufferBind);
   API_CHECK(renderbufferStorage);
   API_CHECK(framebufferGenerate);
   API_CHECK(framebufferDelete);
   API_CHECK(framebufferBind);
   API_CHECK(framebufferTexture);
   API_CHECK(framebufferRenderbuffer);
   API_CHECK(hwBufferGenerate);
   API_CHECK(hwBufferDelete);
   API_CHECK(hwBufferBind);
   API_CHECK(hwBufferCreate);
   API_CHECK(hwBufferFill);
   API_CHECK(hwBufferBindBase);
   API_CHECK(hwBufferBindRange);
   API_CHECK(hwBufferMap);
   API_CHECK(hwBufferUnmap);
   API_CHECK(programLink);
   API_CHECK(programDelete);
   API_CHECK(programUniformBufferPool);
   API_CHECK(programAttributePool);
   API_CHECK(programUniformPool);
   API_CHECK(programUniform);
   API_CHECK(programAttachUniformBuffer);
   API_CHECK(shaderCompile);
   API_CHECK(shaderDelete);
   API_CHECK(shadersPath);
   API_CHECK(shadersDirectiveToken);
#undef API_CHECK
}

static int load(const char *file, struct renderer *renderer)
{
   void *handle;
   const char *error = NULL;

   memset(renderer, 0, sizeof(struct renderer));

   if (!(handle = chckDlLoad(file, &error)))
      goto fail;

   const char* (*regfun)(struct glhckRendererInfo*);
   if (!(regfun = chckDlLoadSymbol(handle, "glhckRendererRegister", &error)))
      goto fail;

   renderer->file = pstrdup(file);
   renderer->handle = handle;
   const char *name = regfun(&renderer->info);
   renderer->name = (name ? pstrdup(name) : NULL);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   DEBUG(GLHCK_DBG_ERROR, "%s", error);
   IFDO(chckDlUnload, handle);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

int _glhckRendererInit(void)
{
   TRACE(0);

   const char *path = getenv("GLHCK_RENDERER");

   if (path) {
      int ret = _glhckRendererRegister(path);
      RET(0, "%d", ret);
      return ret;
   }

   path = getenv("GLHCK_RENDERERS");

   if (!path || access(path, R_OK) == -1)
      path = rpath;

   tinydir_dir dir;
   if (tinydir_open(&dir, path) == -1)
      goto fail;

   size_t registered = 0;
   while (dir.has_next) {
      tinydir_file file;
      memset(&file, 0, sizeof(file));
      tinydir_readfile(&dir, &file);

      if (!file.is_dir && !strncmp(file.name, "glhck-renderer-", strlen("glhck-renderer-"))) {
         size_t len = snprintf(NULL, 0, "%s/%s", file.name, path);

         char *fpath;
         if ((fpath = calloc(1, len + 1))) {
            sprintf(fpath, "%s/%s", path, file.name);

            if (_glhckRendererRegister(fpath) == RETURN_OK)
               registered++;

            free(fpath);
         }
      }

      tinydir_next(&dir);
   }

   tinydir_close(&dir);

   RET(0, "%d", (registered > 0 ? RETURN_OK : RETURN_FAIL));
   return (registered > 0 ? RETURN_OK : RETURN_FAIL);

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

void _glhckRendererTerminate(void)
{
   TRACE(0);

   _glhckRendererDeactivate();

   struct renderer *r;
   for (chckPoolIndex iter = 0; (r = chckIterPoolIter(renderers, &iter));) {
      IFDO(free, r->file);
      IFDO(free, r->name);
   }

   IFDO(chckIterPoolFree, renderers);
}

int _glhckRendererRegister(const char *file)
{
   CALL(0, "%s", file);

   if (!renderers && !(renderers = chckIterPoolNew(1, 1, sizeof(struct renderer))))
      goto fail;

   struct renderer r;
   if (load(file, &r) != RETURN_OK)
      goto fail;

   DEBUG(GLHCK_DBG_CRAP, "registered: %s", r.name);
   chckIterPoolAdd(renderers, &r, NULL);
   IFDO(chckDlUnload, r.handle);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

void _glhckRendererDeactivate(void)
{
   TRACE(0);

   if (active == INVALID_RENDERER)
      return;

   struct renderer *r;
   if ((r = chckIterPoolGet(renderers, active))) {
      if (r->info.destructor)
         r->info.destructor();
      IFDO(chckDlUnload, r->handle);
   }

   active = INVALID_RENDERER;
   _glhckRenderAPI(NULL);
}

int _glhckRendererActivate(size_t renderer)
{
   struct renderer *r = NULL;
   CALL(0, "%zu", renderer);

   if (renderer == INVALID_RENDERER)
      goto fail;

   if (active == renderer)
      goto ok;

   _glhckRendererDeactivate();

   if (!(r = chckIterPoolGet(renderers, renderer)))
      goto fail;

   if (load(r->file, r) != RETURN_OK)
      goto fail;

   if (r->info.constructor && r->info.constructor(&r->info) != RETURN_OK)
      goto fail;

   apicheck(r->name, &r->info.api);
   DEBUG(GLHCK_DBG_CRAP, "activated: %s\n", r->name);

   active = renderer;
   _glhckRenderAPI(&r->info.api);

ok:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   if (r)
      IFDO(chckDlUnload, r->handle);

   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

size_t _glhckRendererGetActive(void)
{
   return active;
}

GLHCKAPI size_t glhckRendererCount(void)
{
   return (renderers ? chckIterPoolCount(renderers) : 0);
}

GLHCKAPI const glhckRendererContextInfo* glhckRendererGetContextInfo(size_t renderer, const char **outName)
{
   CALL(0, "%zu, %p", renderer, outName);

   if (outName)
      *outName = NULL;

   if (!renderers || renderer >= chckIterPoolCount(renderers))
      return NULL;

   struct renderer *r = chckIterPoolGet(renderers, renderer);

   if (outName)
      *outName = r->name;

   RET(0, "%p", &r->info.context);
   return &r->info.context;
}

GLHCKAPI const glhckRendererFeatures* glhckRendererGetFeatures(void)
{
   TRACE(0);

   if (active == INVALID_RENDERER)
      return NULL;

   struct renderer *r = chckIterPoolGet(renderers, active);

   RET(0, "%p", &r->info.features);
   return &r->info.features;
}

/* vim: set ts=8 sw=3 tw=0 :*/
