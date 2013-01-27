#define _glhck_c_
#include "internal.h"
#include "render/render.h"
#include <stdlib.h> /* for atexit */
#include <stdio.h>  /* for sprintf */
#include <assert.h> /* for assert */
#include <signal.h> /* for signal */
#include <unistd.h> /* for fork   */

#if defined(__linux__) && defined(__GNUC__)
#  define _GNU_SOURCE
#  include <fenv.h>
int feenableexcept(int excepts);
#endif

#if (defined(__APPLE__) && (defined(__i386__) || defined(__x86_64__)))
#  define OSX_SSE_FPE
#  include <xmmintrin.h>
#endif

#if defined(__linux__) || defined(__APPLE__)
#  include <sys/wait.h>
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
   GLHCK_API_CHECK(frustumDraw);
   GLHCK_API_CHECK(getPixels);
   GLHCK_API_CHECK(generateTextures);
   GLHCK_API_CHECK(deleteTextures);
   GLHCK_API_CHECK(bindTexture);
   GLHCK_API_CHECK(createTexture);
   GLHCK_API_CHECK(fillTexture);
   GLHCK_API_CHECK(generateFramebuffers);
   GLHCK_API_CHECK(deleteFramebuffers);
   GLHCK_API_CHECK(bindFramebuffer);
   GLHCK_API_CHECK(linkFramebufferWithTexture);
   GLHCK_API_CHECK(linkProgram);
   GLHCK_API_CHECK(deleteProgram);
   GLHCK_API_CHECK(compileShader);
   GLHCK_API_CHECK(deleteShader);
   GLHCK_API_CHECK(getIntegerv);
}
#undef GLHCK_API_CHECK

/* \brief builds and sends the default projection to renderer */
void _glhckDefaultProjection(int width, int height)
{
   kmMat4 projection;
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   _GLHCKlibrary.render.api.viewport(0, 0, width, height);
   kmMat4PerspectiveProjection(&projection, 35,
         (float)width/(float)height, 0.1f, 100.0f);
   _GLHCKlibrary.render.api.setProjection(&projection);
}

/* dirty debug build stuff */
#ifndef NDEBUG

/* floating point exception handler */
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
static void _glhckFpeHandler(int sig)
{
   _glhckPuts("\4SIGFPE \1signal received!");
   _glhckPuts("Run the program again with DEBUG=2,+all,+trace to catch where it happens.");
   _glhckPuts("Or optionally run the program with GDB.");
   abort();
}
#endif

/* set floating point exception stuff
 * this stuff is from blender project. */
static void _glhckSetFPE(int argc, const char **argv)
{
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
   /* zealous but makes float issues a heck of a lot easier to find!
    * set breakpoints on fpe_handler */
   signal(SIGFPE, _glhckFpeHandler);

#  if defined(__linux__) && defined(__GNUC__)
   feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#  endif /* defined(__linux__) && defined(__GNUC__) */
#  if defined(OSX_SSE_FPE)
   return; /* causes issues */
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

/* \brief backtrace handler of glhck */
static void _glhckBacktrace(int signal)
{
   /* GDB */
#if defined(__linux__) || defined(__APPLE__)
   char buf[1024];
   pid_t dying_pid = getpid();
   pid_t child_pid = fork();

   if (child_pid < 0) {
      _glhckPuts("\1fork failed for gdb backtrace.");
   } else if (child_pid == 0) {
      sprintf(buf, "gdb -p %d -batch -ex bt 2>/dev/null | "
                   "sed '0,/<signal handler/d'", dying_pid);
      const char* argv[] = { "sh", "-c", buf, NULL };
      execve("/bin/sh", (char**)argv, NULL);
      _exit(EXIT_FAILURE);
   } else {
      waitpid(child_pid, NULL, 0);
   }
#endif

   /* SIGABRT || SIGSEGV */
   exit(EXIT_FAILURE);
}

/* set backtrace stuff */
static void _glhckSetBacktrace(void)
{
   signal(SIGABRT, _glhckBacktrace);
   signal(SIGSEGV, _glhckBacktrace);
}

#endif /* NDEBUG */

/***
 * public api
 ***/

/* \brief initialize */
GLHCKAPI int glhckInit(int argc, char **argv)
{
   const char ** _argv = (const char **)argv;

   if (_glhckInitialized)
      return RETURN_OK;

   /* reset global state */
   memset(&_GLHCKlibrary,                 0, sizeof(__GLHCKlibrary));
   memset(&_GLHCKlibrary.trace,           0, sizeof(__GLHCKtrace));
   memset(&_GLHCKlibrary.render,          0, sizeof(__GLHCKrender));
   memset(&_GLHCKlibrary.render.api,      0, sizeof(__GLHCKrenderAPI));
   memset(&_GLHCKlibrary.render.draw,     0, sizeof(__GLHCKrenderDraw));
   memset(&_GLHCKlibrary.world,           0, sizeof(__GLHCKworld));

   _GLHCKlibrary.render.draw.objects.queue =
      _glhckMalloc(GLHCK_QUEUE_ALLOC_STEP * sizeof(_glhckObject*));
   _GLHCKlibrary.render.draw.objects.allocated += GLHCK_QUEUE_ALLOC_STEP;

   _GLHCKlibrary.render.draw.textures.queue =
      _glhckMalloc(GLHCK_QUEUE_ALLOC_STEP * sizeof(_glhckTexture*));
   _GLHCKlibrary.render.draw.textures.allocated += GLHCK_QUEUE_ALLOC_STEP;

   /* our data structure are intialized now */

   /* FIXME(?):
    * Maybe get rid of the _GLHCKlibrary global variable.
    * Instead make init return pointer to allocated context variable,
    * and let user pass it around.
    *
    * This would make the library a lot easier for concurrency as well. */
   atexit(glhckTerminate);
   _glhckInitialized = 1;

   /* enable color by default */
   glhckLogColor(1);

   /* FIXME: change the signal calls in these functions to sigaction's */
#ifndef NDEBUG
#if 0
   /* set FPE handler */
   _glhckSetFPE(argc, _argv);
#endif

   /* setup backtrace handler
    * make this optional.. */
   _glhckSetBacktrace();
#endif

   /* init trace system */
   _glhckTraceInit(argc, _argv);

   /* set default global precision for glhck to use with geometry
    * NOTE: _NONE means that glhck and importers choose the best precision. */
   glhckSetGlobalPrecision(GLHCK_INDEX_NONE, GLHCK_VERTEX_NONE);
   return RETURN_OK;
}

/* \brief is glhck initialized? (tracing here is pointless) */
GLHCKAPI int glhckInitialized(void)
{
   return _glhckInitialized;
}

GLHCKAPI void glhckLogColor(char color)
{
   GLHCK_INITIALIZED();
   if (color != _GLHCKlibrary.misc.coloredLog && !color)
      _glhckNormal();
   _GLHCKlibrary.misc.coloredLog = color;
}

/* \brief creates virtual display and inits renderer */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType)
{
   GLHCK_INITIALIZED();
   CALL(0, "%d, %d, %d", width, height, renderType);

   if (width <= 0 && height <= 0)
      goto fail;

   /* autodetect */
   if (renderType == GLHCK_RENDER_AUTO)
      renderType = GLHCK_RENDER_OPENGL_FIXED_PIPELINE;

   /* close display if created already */
   if (_GLHCKlibrary.render.type == renderType) goto success;
   else glhckDisplayClose();

   /* init renderer */
   switch (renderType) {
      case GLHCK_RENDER_OPENGL:
      case GLHCK_RENDER_GLES2:
         _glhckRenderOpenGL();
         break;
      case GLHCK_RENDER_OPENGL_FIXED_PIPELINE:
      case GLHCK_RENDER_GLES1:
         _glhckRenderOpenGLFixedPipeline();
         break;
   }

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
   GLHCK_INITIALIZED();
   TRACE(0);

   /* nothing to terminate */
   if (!_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.terminate();
   _GLHCKlibrary.render.type = GLHCK_RENDER_AUTO;
}

/* \brief resize virtual display */
GLHCKAPI void glhckDisplayResize(int width, int height)
{
   GLHCK_INITIALIZED();
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   /* nothing to resize */
   if (!_GLHCKlibrary.render.name)
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
   GLHCK_INITIALIZED();
   TRACE(2);

   /* can't clear */
   if (!_GLHCKlibrary.render.name)
      return;

   _GLHCKlibrary.render.api.clear();
}

/* \brief return detected driver type */
GLHCKAPI glhckDriverType glhckRenderGetDriver(void)
{
   GLHCK_INITIALIZED();
   TRACE(0);
   return _GLHCKlibrary.render.driver;
}

/* \brief set global geometry vertexdata precision to glhck */
GLHCKAPI void glhckSetGlobalPrecision(glhckGeometryIndexType itype,
      glhckGeometryVertexType vtype)
{
   GLHCK_INITIALIZED();
   CALL(0, "%u, %u", itype, vtype);
   if (itype != GLHCK_INDEX_NONE)
      _GLHCKlibrary.render.globalIndexType  = itype;
   if (vtype != GLHCK_VERTEX_NONE)
      _GLHCKlibrary.render.globalVertexType = vtype;
}

/* \brief get parameter from renderer */
GLHCKAPI void glhckRenderGetIntegerv(unsigned int pname, int *params) {
   GLHCK_INITIALIZED();
   CALL(1, "%u, %p", pname, params);
   _GLHCKlibrary.render.api.getIntegerv(pname, params);
}

/* \brief set render flags */
GLHCKAPI void glhckRenderSetFlags(unsigned int flags)
{
   GLHCK_INITIALIZED();
   CALL(1, "%u", flags);
   _GLHCKlibrary.render.flags = flags;
}

/* \brief set projection matrix */
GLHCKAPI void glhckRenderProjection(kmMat4 const* mat)
{
   GLHCK_INITIALIZED();
   CALL(1, "%p", mat);
   _GLHCKlibrary.render.api.setProjection(mat);
}

/* \brief output queued objects */
GLHCKAPI void glhckPrintObjectQueue(void)
{
   unsigned int i;
   __GLHCKobjectQueue *objects;
   GLHCK_INITIALIZED();

   objects = &_GLHCKlibrary.render.draw.objects;
   _glhckPuts("\n--- Object Queue ---");
   for (i = 0; i != objects->count; ++i)
      _glhckPrintf("%u. %p", i, objects->queue[i]);
   _glhckPuts("--------------------");
   _glhckPrintf("count/alloc: %u/%u", objects->count, objects->allocated);
   _glhckPuts("--------------------\n");
}

 /* \brief output queued textures */
GLHCKAPI void glhckPrintTextureQueue(void)
{
   unsigned int i;
   __GLHCKtextureQueue *textures;
   GLHCK_INITIALIZED();

   textures = &_GLHCKlibrary.render.draw.textures;
   _glhckPuts("\n--- Texture Queue ---");
   for (i = 0; i != textures->count; ++i)
      _glhckPrintf("%u. %p", i, textures->queue[i]);
   _glhckPuts("---------------------");
   _glhckPrintf("count/alloc: %u/%u", textures->count, textures->allocated);
   _glhckPuts("--------------------\n");
}

/* \brief render scene */
GLHCKAPI void glhckRender(void)
{
   unsigned int ti, oi, ts, os, tc, oc;
   char kt;
   _glhckTexture *t;
   _glhckObject *o;
   __GLHCKobjectQueue *objects;
   __GLHCKtextureQueue *textures;
   GLHCK_INITIALIZED();
   TRACE(2);

   /* can't render */
   if (!_glhckInitialized || !_GLHCKlibrary.render.name)
      return;

   objects  = &_GLHCKlibrary.render.draw.objects;
   textures = &_GLHCKlibrary.render.draw.textures;

   /* store counts for enumeration, +1 for untexture objects */
   tc = textures->count+1;
   oc = objects->count;

   /* nothing to draw */
   if (!oc)
      return;

   /* draw in sorted texture order */
   for (ti = 0, ts = 0; ti != tc; ++ti) {
      if (ti < tc-1) {
         if (!(t = textures->queue[ti])) continue;
      } else t = NULL; /* untextured object */

      for (oi = 0, os = 0, kt = 0; oi != oc; ++oi) {
         if (!(o = objects->queue[oi])) continue;

         /* don't draw if not same texture or opaque,
          * opaque objects are drawn last */
         if (o->material.texture != t ||
            (o->material.blenda != GLHCK_ZERO && o->material.blendb != GLHCK_ZERO)) {
            if (o->material.texture == t) kt = 1; /* don't remove texture from queue */
            if (os != oi) objects->queue[oi] = NULL;
            objects->queue[os++] = o;
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         glhckObjectFree(o); /* referenced on draw call */
         objects->queue[oi] = NULL;
         --objects->count;
      }

      /* check if we need texture again or not */
      if (kt) {
         if (ts != ti) textures->queue[ti] = NULL;
         textures->queue[ts++] = t;
      } else {
         if (t) {
            glhckTextureFree(t); /* ref is increased on draw call! */
            textures->queue[ti] = NULL;
            --textures->count;
         }
      }
   }

   /* store counts for enumeration, +1 for untextured objects */
   tc = textures->count+1;
   oc = objects->count;

   /* FIXME: shift queue here */
   if (oc) {
   }

   /* draw opaque objects next,
    * FIXME: this should not be done in texture order,
    * instead draw from farthest to nearest. (I hate opaque objects) */
   for (ti = 0; ti != tc && oc; ++ti) {
      if (ti < tc-1) {
         if (!(t = textures->queue[ti])) continue;
      } else t = NULL; /* untextured object */

      for (oi = 0, os = 0; oi != oc; ++oi) {
         if (!(o = objects->queue[oi])) continue;

         /* don't draw if not same texture */
         if (o->material.texture != t) {
            if (os != oi) objects->queue[oi] = NULL;
            objects->queue[os++] = o;
            continue;
         }

         /* render object */
         glhckObjectRender(o);
         glhckObjectFree(o); /* referenced on draw call */
         objects->queue[oi] = NULL;
         --objects->count;
      }

      /* this texture is done for */
      if (t) {
         glhckTextureFree(t); /* ref is increased on draw call! */
         textures->queue[ti] = NULL;
         --textures->count;
      }

      /* no texture, time to break */
      if (!t) break;
   }

   /* FIXME: shift queue here if leftovers */

   /* good we got no leftovers \o/ */
   if (objects->count) {
      /* something was left un-drawn :o? */
      for (oi = 0; oi != objects->count; ++oi) glhckObjectFree(objects->queue[oi]);
      memset(objects->queue, 0, objects->count * sizeof(_glhckObject*));
      objects->count = 0;
   }

   if (textures->count) {
      /* something was left un-drawn :o? */
      DEBUG(GLHCK_DBG_CRAP, "COUNT UNLEFT %u", textures->count);
      for (ti = 0; ti != textures->count; ++ti) glhckTextureFree(textures->queue[ti]);
      memset(textures->queue, 0, textures->count * sizeof(_glhckTexture*));
      textures->count = 0;
   }
}

/* kill a list from world */
#define _massacre(list, c, func)                \
   while ((c = _GLHCKlibrary.world.list))       \
   { while (func(c)); }                         \
   DEBUG(GLHCK_DBG_CRAP, "Slaughter: %p");      \
   _GLHCKlibrary.world.list = NULL;

/* \brief frees all objects that are handled by glhck */
GLHCKAPI void glhckMassacreWorld(void)
{
   _glhckObject *o;
   _glhckCamera *c;
   _glhckTexture *t;
   _glhckAtlas *a;
   _glhckRtt *r;
   _glhckText *tf;
   GLHCK_INITIALIZED();
   TRACE(0);

   /* destroy the world */
   _massacre(clist, c, glhckCameraFree);
   _massacre(alist, a, glhckAtlasFree);
   _massacre(rlist, r, glhckRttFree);
   _massacre(tflist, tf, glhckTextFree);
   _massacre(olist, o, glhckObjectFree);
   _massacre(tlist, t, glhckTextureFree);
}

/* \brief frees virtual display and deinits glhck */
GLHCKAPI void glhckTerminate(void)
{
   /* just return if library not initialized.
    * we avoid assert for atExit call */
   if (!_glhckInitialized) return;
   TRACE(0);

   /* destroy world */
   glhckMassacreWorld();

   /* destroy queues */
   _glhckFree(_GLHCKlibrary.render.draw.objects.queue);
   _glhckFree(_GLHCKlibrary.render.draw.textures.queue);

#ifndef NDEBUG
   _glhckTrackTerminate();
#endif
   glhckDisplayClose();

   /* we are done */
   _glhckInitialized = 0;
}

/* vim: set ts=8 sw=3 tw=0 :*/
