#define _init_c_
#include "internal.h"
#include "render/render.h"
#include <stdlib.h> /* for atexit */
#include <assert.h> /* for assert */
#include <signal.h> /* for signal */

#if defined(__linux__) && defined(__GNUC__)
#  define _GNU_SOURCE
#  include <fenv.h>
int feenableexcept(int excepts);
#endif

#if (defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__)))
#  define OSX_SSE_FPE
#  include <xmmintrin.h>
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GLHCK

/* \brief macro and function for checking render api calls */
#define GLHCK_API_CHECK(x) \
if (!render->api.x) DEBUG(GLHCK_DBG_ERROR, "[%s] \1missing render API function: %s", render->name, __STRING(x))
static void _glhckCheckRenderApi(__GLHCKrender *render)
{
   GLHCK_API_CHECK(terminate);
   GLHCK_API_CHECK(viewport);
   GLHCK_API_CHECK(setProjection);
   GLHCK_API_CHECK(getProjection);
   GLHCK_API_CHECK(setClearColor);
   GLHCK_API_CHECK(clear);
   GLHCK_API_CHECK(objectDraw);
   GLHCK_API_CHECK(textDraw);
   GLHCK_API_CHECK(getPixels);
   GLHCK_API_CHECK(generateTextures);
   GLHCK_API_CHECK(deleteTextures);
   GLHCK_API_CHECK(bindTexture);
   GLHCK_API_CHECK(uploadTexture);
   GLHCK_API_CHECK(createTexture);
   GLHCK_API_CHECK(fillTexture);
   GLHCK_API_CHECK(generateFramebuffers);
   GLHCK_API_CHECK(deleteFramebuffers);
   GLHCK_API_CHECK(bindFramebuffer);
   GLHCK_API_CHECK(linkFramebufferWithTexture);
}

/* \brief builds and sends the default projection to renderer */
void _glhckDefaultProjection(int width, int height)
{
   kmMat4 projection;
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   _GLHCKlibrary.render.api.viewport(0, 0, width, height);
   kmMat4PerspectiveProjection(&projection, 35,
         (float)width/height, 0.1f, 100.0f);
   _GLHCKlibrary.render.api.setProjection(&projection);
}

/* floating point exception handler */
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
static void _glhckFpeHandler(int sig)
{
   _glhckPuts("\4SIGFPE \1signal received!");
   _glhckPuts("Run the program again with DEBUG=+++,+all,+trace to catch where it happens.");
   _glhckPuts("Or optionally run the program with GDB.");
   abort();
}
#endif

/* set floating point exception stuff
 * this stuff is from blender project. */
static int _glhckSetFPE(int argc, const char **argv)
{
#if defined(APPLE)
   /* OS X shows strange exceptions in gld... */
   return;
#endif

#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
   /* zealous but makes float issues a heck of a lot easier to find!
    * set breakpoints on fpe_handler */
   signal(SIGFPE, _glhckFpeHandler);

#  if defined(__linux__) && defined(__GNUC__)
   feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#  endif /* defined(__linux__) && defined(__GNUC__) */
#  if defined(OSX_SSE_FPE)
   /* OSX uses SSE for floating point by default, so here
    * use SSE instructions to throw floating point exceptions */
   _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK & ~
         (_MM_MASK_OVERFLOW | _MM_MASK_INVALID | _MM_MASK_DIV_ZERO));
#  endif /* OSX_SSE_FPE */
#  if defined(_WIN32) && defined(_MSC_VER)
   _controlfp_s(NULL, 0, _MCW_EM); /* enables all fp exceptions */
   _controlfp_s(NULL, _EM_DENORMAL | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM); /* hide the ones we don't care about */
#  endif /* _WIN32 && _MSC_VER */
#endif
}

/* public api */

/* \brief initialize */
GLHCKAPI int glhckInit(int argc, const char **argv)
{
   CALL(0, "%d, %p", argc, argv);

   if (_glhckInitialized)
      goto success;

   /* reset global state */
   memset(&_GLHCKlibrary,                 0, sizeof(__GLHCKlibrary));
   memset(&_GLHCKlibrary.trace,           0, sizeof(__GLHCKtrace));
   memset(&_GLHCKlibrary.render,          0, sizeof(__GLHCKrender));
   memset(&_GLHCKlibrary.render.api,      0, sizeof(__GLHCKrenderAPI));
   memset(&_GLHCKlibrary.render.draw,     0, sizeof(__GLHCKrenderDraw));
   memset(&_GLHCKlibrary.world,           0, sizeof(__GLHCKworld));

#ifndef NDEBUG
   /* set FPE handler */
   _glhckSetFPE(argc, argv);
#endif

   /* init trace system */
   _glhckTraceInit(argc, argv);

   atexit(glhckTerminate);
   _glhckInitialized = 1;

success:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief creates virtual display and inits renderer */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType)
{
   CALL(0, "%d, %d, %d", width, height, renderType);

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
   _GLHCKlibrary.render.type = renderType;

   /* resize display */
   glhckDisplayResize(width, height);

success:
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief close the virutal display */
GLHCKAPI void glhckDisplayClose(void)
{
   TRACE(0);

   /* nothing to terminate */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.terminate();
   _GLHCKlibrary.render.type = GLHCK_RENDER_NONE;
}

/* \brief resize virtual display */
GLHCKAPI void glhckDisplayResize(int width, int height)
{
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   /* nothing to resize */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   if (_GLHCKlibrary.render.width  == width &&
       _GLHCKlibrary.render.height == height)
      return;

   /* update all cameras */
   _glhckCameraWorldUpdate(width, height);

   /* update on library last, so functions know the old values */
   _GLHCKlibrary.render.width  = width;
   _GLHCKlibrary.render.height = height;
}

/* \brief clear scene */
GLHCKAPI void glhckClear(void)
{
   TRACE(2);

   /* can't clear */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.clear();
}

/* \brief render scene */
GLHCKAPI void glhckRender(void)
{
   unsigned int ti, oi, ts, os;
   char kt;
   _glhckTexture *t;
   _glhckObject *o;
   TRACE(2);

   /* can't render */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   /* nothing to draw */
   if (!_GLHCKlibrary.render.draw.oqueue[0])
      return;

   /* draw in sorted texture order */
   for (ti = 0, ts = 0, os = 0, kt = 0;
        t = _GLHCKlibrary.render.draw.tqueue[ti]; ++ti) {
      for (oi = 0; (o = _GLHCKlibrary.render.draw.oqueue[oi]); ++oi) {
         /* don't draw if not same texture or opaque,
          * opaque objects are drawn last */
         if (o->material.texture != t ||
            (o->material.flags & GLHCK_MATERIAL_ALPHA)) {
            if (o->material.texture == t) kt = 1; /* don't remove texture from queue */
            _GLHCKlibrary.render.draw.oqueue[os++] = o; /* assign back to list */
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         _GLHCKlibrary.render.draw.oqueue[oi] = NULL;
      }

      /* check if we need texture again or not */
      if (kt)  _GLHCKlibrary.render.draw.tqueue[ts++] = t;
      else     _GLHCKlibrary.render.draw.tqueue[ti]   = NULL;

      /* no texture, time to break
       * we do this here, so objects with no texture get drawn last */
      if (!t) break;
   }

   /* draw opaque objects next,
    * TODO: this should not be done in texture order,
    * instead draw from farthest to nearest. (I hate opaque objects) */
   for (ti = 0, os = 0;
        t = _GLHCKlibrary.render.draw.tqueue[ti]; ++ti) {
      for (oi = 0; (o = _GLHCKlibrary.render.draw.oqueue[oi]); ++oi) {
         /* don't draw if not same texture */
         if (o->material.texture != t) {
            _GLHCKlibrary.render.draw.oqueue[os++] = o; /* assign back to list */
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         _GLHCKlibrary.render.draw.oqueue[oi] = NULL;
      }

      /* no texture, time to break */
      if (!t) break;
   }

   /* good we got no leftovers \o/ */
   if (!_GLHCKlibrary.render.draw.oqueue[0])
      return;

   /* something was left un-drawn :o? */
   memset(_GLHCKlibrary.render.draw.oqueue, 0,
         GLHCK_MAX_DRAW * sizeof(_glhckObject*));
}

/* kill a list from world */
#define _massacre(list, c, n, func)             \
for (c = _GLHCKlibrary.world.list; c; c = n) {  \
   n = c->next;                                 \
   func(c);                                     \
} _GLHCKlibrary.world.list = NULL;

/* \brief frees virtual display and deinits glhck */
GLHCKAPI void glhckTerminate(void)
{
   _glhckObject *o, *on;
   _glhckCamera *c, *cn;
   _glhckTexture *t, *tn;
   _glhckAtlas *a, *an;
   _glhckRtt *r, *rn;
   _glhckText *tf, *tfn;
   TRACE(0);

   if (!_glhckInitialized) return;

   /* destroy the world */
   _massacre(olist, o, on, glhckObjectFree);
   _massacre(clist, c, cn, glhckCameraFree);
   _massacre(tlist, t, tn, glhckTextureFree);
   _massacre(alist, a, an, glhckAtlasFree);
   _massacre(rlist, r, rn, glhckRttFree);
   _massacre(tflist, tf, tfn, glhckTextFree);

#ifndef NDEBUG
   _glhckTrackTerminate();
#endif
   glhckDisplayClose();

   /* we are done */
   _glhckInitialized = 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
