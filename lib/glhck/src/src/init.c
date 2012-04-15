#define _init_c_
#include "internal.h"
#include "render/render.h"
#include <assert.h> /* for assert */

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GLHCK

/* \brief macro and function for checking render api calls */
#define GLHCK_API_CHECK(x) \
if (!render->api.x) DEBUG(GLHCK_DBG_ERROR, "[%s] missing render API function: %s", render->name, __STRING(x))
static void _glhckCheckRenderApi(__GLHCKrender *render)
{
   GLHCK_API_CHECK(terminate);
   GLHCK_API_CHECK(resize);
   GLHCK_API_CHECK(render);
   GLHCK_API_CHECK(objectDraw);
   GLHCK_API_CHECK(generateTextures);
   GLHCK_API_CHECK(deleteTextures);
   GLHCK_API_CHECK(bindTexture);
   GLHCK_API_CHECK(uploadTexture);
   GLHCK_API_CHECK(createTexture);
   GLHCK_API_CHECK(bindBuffer);
}

/* public api */

/* \brief initialize */
GLHCKAPI int glhckInit(int argc, char **argv)
{
   CALL("%d, %p", argc, argv);

   if (_glhckInitialized)
      goto success;

   /* reset global state */
   memset(&_GLHCKlibrary,                 0, sizeof(__GLHCKlibrary));
   memset(&_GLHCKlibrary.render,          0, sizeof(__GLHCKrender));
   memset(&_GLHCKlibrary.render.api,      0, sizeof(__GLHCKrenderAPI));
   memset(&_GLHCKlibrary.texture,         0, sizeof(__GLHCKtexture));

   /* init trace system */
   _glhckTraceInit(argc, argv);
   atexit(glhckTerminate);
   _glhckInitialized = 1;

success:
   RET("%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief creates virtual display and inits renderer */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType)
{
   CALL("%d, %d, %d", width, height, renderType);

   if (!_glhckInitialized ||
       width <= 0 && height <= 0)
      goto fail;

   /* autodetect */
   if (renderType == GLHCK_RENDER_NONE)
      renderType = GLHCK_RENDER_OPENGL;

   /* close display if created already */
   if (_GLHCKlibrary.render.type == renderType) goto success;
   else glhckDisplayClose();

   /* init renderer */
   if (renderType == GLHCK_RENDER_OPENGL)
      _glhckRenderOpenGL();

   /* check that initialization was success */
   if (!_GLHCKlibrary.render.name)
      goto fail;

   /* check render api and output warnings,
    * if any function is missing */
   _glhckCheckRenderApi(&_GLHCKlibrary.render);

   /* resize display */
   glhckDisplayResize(width, height);
   _GLHCKlibrary.render.type = renderType;

success:
   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief close the virutal display */
GLHCKAPI void glhckDisplayClose(void)
{
   TRACE();

   /* nothing to terminate */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.terminate();
   _GLHCKlibrary.render.type = GLHCK_RENDER_NONE;
}

/* \brief resize virtual display */
GLHCKAPI void glhckDisplayResize(int width, int height)
{
   assert(width > 0 && height > 0);
   CALL("%d, %d", width, height);

   /* nothing to resize */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.width  = width;
   _GLHCKlibrary.render.height = height;
   _GLHCKlibrary.render.api.resize(width, height);
}

/* \brief render scene using renderer */
GLHCKAPI void glhckRender(void)
{
   TRACE();

   /* can't render */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.render();
}

/* \brief frees virtual display and deinits glue framework */
GLHCKAPI void glhckTerminate(void)
{
   TRACE();
   if (!_glhckInitialized) return;
   glhckDisplayClose();
   _glhckTextureCacheRelease();
#ifndef NDEBUG
   _glhckTrackTerminate();
#endif
   _glhckInitialized = 0;
}
