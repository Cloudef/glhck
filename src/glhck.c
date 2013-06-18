#define _glhck_c_
#include "internal.h"
#include "render/render.h"
#include <stdlib.h> /* for abort */
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
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GLHCK

/* dirty debug build stuff */
#ifndef NDEBUG

/* floating point exception handler */
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
static void _glhckFpeHandler(int signal)
{
   (void)signal;
   _glhckPuts("\4SIGFPE \1signal received!");
   _glhckPuts("Run the program again with DEBUG=2,+all,+trace to catch where it happens.");
   _glhckPuts("Or optionally run the program with GDB.");
   abort();
}
#endif

/* set floating point exception stuff
 * this stuff is from blender project. */
static void _glhckSetupFPE(void)
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
   (void)signal;

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

/* \brief get compile time options, can be called before context */
GLHCKAPI void glhckGetCompileFeatures(glhckCompileFeatures *features)
{
   assert(features);
   memset(features, 0, sizeof(glhckGetCompileFeatures));
   features->render.opengl = (!GLHCK_USE_GLES1 && !GLHCK_USE_GLES2);
   features->render.glesv1 = GLHCK_USE_GLES1;
   features->render.glesv2 = GLHCK_USE_GLES2;
   features->import.assimp = GLHCK_IMPORT_ASSIMP;
   features->import.openctm = GLHCK_IMPORT_OPENCTM;
   features->import.mmd = GLHCK_IMPORT_MMD;
   features->import.bmp = GLHCK_IMPORT_BMP;
   features->import.png = GLHCK_IMPORT_PNG;
   features->import.tga = GLHCK_IMPORT_TGA;
   features->import.jpeg = GLHCK_IMPORT_JPEG;
   features->math.useDoublePrecision = USE_DOUBLE_PRECISION;
}

/* \brief is glhck initialized? */
GLHCKAPI int glhckInitialized(void)
{
   return (_glhckContext?1:0);
}

/* \brief get current glhck context */
GLHCKAPI glhckContext* glhckContextGet(void)
{
   return _glhckContext;
}

/* \brief set new glhck context */
GLHCKAPI void glhckContextSet(glhckContext *ctx)
{
   _glhckContext = ctx;
}

/* \brief initialize */
GLHCKAPI glhckContext* glhckContextCreate(int argc, char **argv)
{
   glhckContext *ctx, *oldCtx;
   const char ** _argv = (const char **)argv;

   /* allocate glhck context */
   if (!(ctx = calloc(1, sizeof(glhckContext))))
      return NULL;

   /* swap current context until init done */
   oldCtx = glhckContextGet();
   glhckContextSet(ctx);

   /* enable color by default */
   glhckLogColor(1);

   /* FIXME: change the signal calls in these functions to sigaction's */
#ifndef NDEBUG
   /* set FPE handler */
   _glhckSetupFPE();

   /* setup backtrace handler
    * make this optional.. */
   _glhckSetBacktrace();
#endif

#ifndef GLHCK_DISABLE_TRACE
   /* init trace system */
   _glhckTraceInit(argc, _argv);
#endif

   /* set default global precision for glhck to use with geometry
    * NOTE: _NONE means that glhck and importers choose the best precision. */
   glhckSetGlobalPrecision(GLHCK_INDEX_NONE, GLHCK_VERTEX_NONE);

   /* pre-allocate render queues */
   GLHCKRD()->objects.queue = _glhckMalloc(GLHCK_QUEUE_ALLOC_STEP * sizeof(_glhckObject*));
   GLHCKRD()->objects.allocated += GLHCK_QUEUE_ALLOC_STEP;
   GLHCKRD()->textures.queue = _glhckMalloc(GLHCK_QUEUE_ALLOC_STEP * sizeof(_glhckTexture*));
   GLHCKRD()->textures.allocated += GLHCK_QUEUE_ALLOC_STEP;

   /* switch back to old context, if there was one */
   if (oldCtx) glhckContextSet(oldCtx);
   return ctx;
}

/* \brief destroys the current glhck context */
GLHCKAPI void glhckContextTerminate(void)
{
   if (!glhckInitialized()) return;
   TRACE(0);

   /* destroy queues */
   _glhckFree(GLHCKRD()->objects.queue);
   _glhckFree(GLHCKRD()->textures.queue);

   /* destroy world */
   glhckMassacreWorld();

   /* close display */
   glhckDisplayClose();

   /* terminate allocation tracking */
#ifndef NDEBUG
   puts("\nExit graph, this should be empty.");
   glhckMemoryGraph();
   _glhckTrackTerminate();
#endif

#ifndef GLHCK_DISABLE_TRACE
   /* terminate trace system */
   _glhckTraceTerminate();
#endif

   /* finally remove the context */
   free(_glhckContext);
   glhckContextSet(NULL);
}

/* \brief colored log? */
GLHCKAPI void glhckLogColor(char color)
{
   GLHCK_INITIALIZED();
   if (color != GLHCKM()->coloredLog && !color)
      _glhckNormal();
   GLHCKM()->coloredLog = color;
}

/* \brief creates virtual display and inits renderer */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType)
{
   GLHCK_INITIALIZED();
   CALL(0, "%d, %d, %d", width, height, renderType);

   if (width <= 0 && height <= 0)
      goto fail;

   /* close display if created already */
   if (GLHCKR()->type == renderType && renderType != GLHCK_RENDER_AUTO) goto success;
   else glhckDisplayClose();

   /* init renderer */
   switch (renderType) {
      case GLHCK_RENDER_AUTO:
#ifdef GLHCK_HAS_OPENGL
         _glhckRenderOpenGL();
         if (_glhckRenderInitialized()) break;
#endif
#ifdef GLHCK_HAS_OPENGL_FIXED_PIPELINE
         _glhckRenderOpenGLFixedPipeline();
         if (_glhckRenderInitialized()) break;
#endif
         _glhckRenderStub();
         break;

      case GLHCK_RENDER_OPENGL:
      case GLHCK_RENDER_GLES2:
#ifdef GLHCK_HAS_OPENGL
         _glhckRenderOpenGL();
#else
         DEBUG(GLHCK_DBG_ERROR, "OpenGL support was not compiled in!");
#endif
         break;
      case GLHCK_RENDER_OPENGL_FIXED_PIPELINE:
      case GLHCK_RENDER_GLES1:
#ifdef GLHCK_HAS_OPENGL_FIXED_PIPELINE
         _glhckRenderOpenGLFixedPipeline();
#else
         DEBUG(GLHCK_DBG_ERROR, "OpenGL Fixed Pipeline support was not compiled in!");
#endif
         break;
      case GLHCK_RENDER_STUB:
      default:
         _glhckRenderStub();
         break;
   }

   /* check that initialization was success */
   if (!_glhckRenderInitialized()) goto fail;

   /* check render api and output warnings,
    * if any function is missing */
   _glhckRenderCheckApi();
   GLHCKR()->type = renderType;

   /* default render pass bits */
   glhckRenderPass(GLHCK_PASS_DEFAULTS);

   /* default cull face */
   glhckRenderCullFace(GLHCK_CULL_BACK);

   /* counter-clockwise are front face by default */
   glhckRenderFrontFace(GLHCK_FACE_CCW);

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
   if (!_glhckRenderInitialized()) return;
   memset(&GLHCKR()->features, 0, sizeof(glhckRenderFeatures));
   GLHCKRA()->terminate();
   GLHCKR()->type = GLHCK_RENDER_AUTO;
}

/* \brief resize virtual display */
GLHCKAPI void glhckDisplayResize(int width, int height)
{
   GLHCK_INITIALIZED();
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);

   /* pass resize event to render interface */
   glhckRenderResize(width, height);
}

/* \brief set global geometry vertexdata precision to glhck */
GLHCKAPI void glhckSetGlobalPrecision(glhckGeometryIndexType itype, glhckGeometryVertexType vtype)
{
   GLHCK_INITIALIZED();
   CALL(0, "%u, %u", itype, vtype);
   GLHCKM()->globalIndexType  = itype;
   GLHCKM()->globalVertexType = vtype;
}

/* \brief get global geometry vertexdata precision */
GLHCKAPI void glhckGetGlobalPrecision(glhckGeometryIndexType *itype, glhckGeometryVertexType *vtype)
{
   GLHCK_INITIALIZED();
   CALL(0, "%p, %p", itype, vtype);
   if (itype) *itype = GLHCKM()->globalIndexType;
   if (vtype) *vtype = GLHCKM()->globalVertexType;
}

#define _massacre(list, func) {                       \
   void *c;                                           \
   while ((c = GLHCKW()->list)) { while (func(c)); }  \
   DEBUG(GLHCK_DBG_CRAP, "Slaughter: %p");            \
   GLHCKW()->list = NULL;                             \
}

/* \brief frees all objects that are handled by glhck */
GLHCKAPI void glhckMassacreWorld(void)
{
   GLHCK_INITIALIZED();
   TRACE(0);

   /* destroy the world */
   _massacre(atlas, glhckAtlasFree);
   _massacre(camera, glhckCameraFree);
   _massacre(framebuffer, glhckFramebufferFree);
   _massacre(hwbuffer, glhckHwBufferFree);
   _massacre(light, glhckLightFree);
   _massacre(object, glhckObjectFree);
   _massacre(bone, glhckBoneFree);
   _massacre(animation, glhckAnimationFree);
   _massacre(animationNode, glhckAnimationNodeFree);
   _massacre(animator, glhckAnimatorFree);
   _massacre(renderbuffer, glhckRenderbufferFree);
   _massacre(shader,  glhckShaderFree);
   _massacre(text, glhckTextFree);
   _massacre(material, glhckMaterialFree);
   _massacre(texture, glhckTextureFree);
}

#undef _massacre

/* vim: set ts=8 sw=3 tw=0 :*/
