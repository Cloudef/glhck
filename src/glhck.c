#include <glhck/glhck.h>

#include <stdio.h>  /* for snprintf */
#include <string.h> /* for memset */
#include <stdlib.h> /* for abort */
#include <assert.h> /* for assert */
#include <signal.h> /* for signal */
#include <unistd.h> /* for fork   */

#include "trace.h"

#include "renderers/renderer.h"
#include "renderers/render.h"

#include "importers/importer.h"

#include "system/tls.h"
#include "system/fpe.h"

#if defined(__linux__) || defined(__APPLE__)
#  include <sys/types.h>
#  include <sys/wait.h>
#endif

#if defined(__linux__)
#  include <linux/version.h>
#  if LINUX_VERSION_CODE >= KERNEL_VERSION(3,4,0)
#     include <sys/prctl.h> /* for yama */
#     define HAS_YAMA_PRCTL 1
#  endif
#endif

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GLHCK

static int initialized = 0;

/* dirty debug build stuff */
#ifndef NDEBUG

/* floating point exception handler */
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
static void fpehandler(int signal)
{
   (void)signal;
   _glhckPuts("\n\4SIGFPE \1signal received!");
   _glhckPuts("Run the program again with DEBUG=2,+all,+trace to catch where it happens.");
   _glhckPuts("Or optionally run the program with GDB.");
   abort();
}
#endif

/* set floating point exception stuff
 * this stuff is from blender project. */
static void setupFPE(void)
{
#if defined(__linux__) || defined(_WIN32) || defined(OSX_SSE_FPE)
   /* zealous but makes float issues a heck of a lot easier to find!
    * set breakpoints on fpe_handler */
   signal(SIGFPE, fpehandler);

#  if defined(__linux__) && defined(__GNUC__)
   feenableexcept(FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW);
#  endif /* defined(__linux__) && defined(__GNUC__) */
#  if defined(OSX_SSE_FPE)
   return; /* causes issues */
   /* OSX uses SSE for floating point by default, so here
    * use SSE instructions to throw floating point exceptions */
   _MM_SET_EXCEPTION_MASK(_MM_MASK_MASK & ~(_MM_MASK_OVERFLOW | _MM_MASK_INVALID | _MM_MASK_DIV_ZERO));
#  endif /* OSX_SSE_FPE */
#  if defined(_WIN32) && defined(_MSC_VER)
   _controlfp_s(NULL, 0, _MCW_EM); /* enables all fp exceptions */
   _controlfp_s(NULL, _EM_DENORMAL | _EM_UNDERFLOW | _EM_INEXACT, _MCW_EM); /* hide the ones we don't care about */
#  endif /* _WIN32 && _MSC_VER */
#endif
}

/* \brief backtrace handler of glhck */
static void backtrace(int signal)
{
   (void)signal;

   /* GDB */
#if defined(__linux__) || defined(__APPLE__)
   char buf[1024];
   pid_t dying_pid = getpid();
   pid_t child_pid = fork();

#if HAS_YAMA_PRCTL
   /* tell yama that we allow our child_pid to trace our process */
   if (child_pid > 0) prctl(PR_SET_PTRACER, child_pid);
#endif

   if (child_pid < 0) {
      _glhckPuts("\1fork failed for gdb backtrace.");
   } else if (child_pid == 0) {
      /* sed -n '/bar/h;/bar/!H;$!b;x;p' (another way, if problems) */
      _glhckPuts("\n---- gdb ----");
      snprintf(buf, sizeof(buf)-1, "gdb -p %d -batch -ex bt 2>/dev/null | sed -n '/<signal handler/{n;x;b};H;${x;p}'", dying_pid);
      const char* argv[] = { "sh", "-c", buf, NULL };
      execve("/bin/sh", (char**)argv, NULL);
      _glhckPuts("\1failed to launch gdb for backtrace.");
      _exit(EXIT_FAILURE);
   } else {
      waitpid(child_pid, NULL, 0);
   }
#endif

   /* SIGABRT || SIGSEGV */
   exit(EXIT_FAILURE);
}

/* set backtrace stuff */
static void setupBacktrace(void)
{
   signal(SIGABRT, backtrace);
   signal(SIGSEGV, backtrace);
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
}

/* \brief is glhck initialized? */
GLHCKAPI int glhckInitialized(void)
{
   return initialized;
}

/* \brief initialize */
GLHCKAPI int glhckInit(const int argc, char **argv)
{
#ifndef _GLHCK_TLS_FOUND
   fprintf(stderr, "-!- Thread-local storage support in compiler was not detected.\n");
   fprintf(stderr, "-!- Using multiple glhck contextes in different threads may cause unexpected behaviour.\n");
   fprintf(stderr, "-!- If your compiler supports TLS, file a bug report!\n");
#endif

   /* FIXME: change the signal calls in these functions to sigaction's */
#ifndef NDEBUG
   setupBacktrace();
   setupFPE();
#endif

   if (_glhckTraceInit(argc, (const char**)argv) != RETURN_OK)
      goto fail;

   if (_glhckRendererInit() != RETURN_OK)
      goto fail;

   if (_glhckImporterInit() != RETURN_OK)
      goto fail;

   _glhckGeometryInit();

#if 0
   /* set default global precision for glhck to use with geometry
    * NOTE: _NONE means that glhck and importers choose the best precision. */
   glhckSetGlobalPrecision(GLHCK_IDX_AUTO, GLHCK_IDX_AUTO);
#endif

   initialized = 1;
   return RETURN_OK;

fail:
   initialized = 0;
   return RETURN_FAIL;
}

/* \brief destroys the current glhck context */
GLHCKAPI void glhckTerminate(void)
{
   if (!glhckInitialized())
      return;

   TRACE(0);

   _glhckGeometryTerminate();

   _glhckHandleTerminate();

   glhckDisplayClose();

#ifndef NDEBUG
   puts("\nExit graph, this should be empty.");
   // glhckMemoryGraph();
#endif

   initialized = 0;
}

GLHCKAPI int glhckDisplayCreated(void)
{
   return (initialized && _glhckRendererGetActive() != INVALID_RENDERER);
}

GLHCKAPI int glhckDisplayCreate(const int width, const int height, const size_t renderer)
{
   CALL(0, "%d, %d, %zu", width, height, renderer);

   if (width <= 0 && height <= 0)
      goto fail;

   if (_glhckRendererGetActive() == renderer) {
      RET(0, "%d", RETURN_OK);
      return RETURN_OK;
   }

   _glhckRendererDeactivate();

   int ret = _glhckRendererActivate(renderer);

   if (ret == RETURN_OK) {
      glhckRenderPass(glhckRenderPassDefaults());
      glhckRenderCullFace(GLHCK_BACK);
      glhckRenderFrontFace(GLHCK_CCW);
      glhckRenderClearColoru(0, 0, 0, 255);
      glhckDisplayResize(width, height);
   }

   RET(0, "%d", ret);
   return ret;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief close the virutal display */
GLHCKAPI void glhckDisplayClose(void)
{
   TRACE(0);

   if (!glhckDisplayCreated())
      return;

   _glhckRendererDeactivate();
}

/* \brief resize virtual display */
GLHCKAPI void glhckDisplayResize(int width, int height)
{
   CALL(1, "%d, %d", width, height);
   assert(width > 0 && height > 0);
   glhckRenderResize(width, height);
}

#if 0
/* \brief set global geometry vertexdata precision to glhck */
GLHCKAPI void glhckSetGlobalPrecision(unsigned char itype, unsigned char vtype)
{
   CALL(0, "%u, %u", itype, vtype);
   GLHCKM()->globalIndexType  = itype;
   GLHCKM()->globalVertexType = vtype;
}

/* \brief get global geometry vertexdata precision */
GLHCKAPI void glhckGetGlobalPrecision(unsigned char *itype, unsigned char *vtype)
{
   GLHCK_INITIALIZED();
   CALL(0, "%p, %p", itype, vtype);
   if (itype) *itype = GLHCKM()->globalIndexType;
   if (vtype) *vtype = GLHCKM()->globalVertexType;
}
#endif

/* vim: set ts=8 sw=3 tw=0 :*/
