#include "internal.h"
#include <stdio.h>  /* for printf   */
#include <stdarg.h> /* for va_start */
#include <limits.h> /* for LINE_MAX */

#ifdef _WIN32
#  include <windows.h>
#endif

/* debug hook function */
static glhckDebugHookFunc _glhckDebugHook = NULL;

/* list all ignored functions when
 * GLHCK_CHANNEL_DRAW switch is active */
static const char *_drawFuncs[] = {
   "glhckRender",
   "glhckObjectDraw",
   "objectDraw",
   "render",
   NULL,
};

/* list all debug channels */
__GLHCKtrace _traceChannels[] =
{
   { GLHCK_CHANNEL_GLHCK,    0 },
   { GLHCK_CHANNEL_IMPORT,   0 },
   { GLHCK_CHANNEL_OBJECT,   0 },
   { GLHCK_CHANNEL_GEOMETRY, 0 },
   { GLHCK_CHANNEL_TEXTURE,  0 },
   { GLHCK_CHANNEL_ALLOC,    0 },
   { GLHCK_CHANNEL_RENDER,   0 },

   /* special channel,
    * will stop bloating output with
    * all drawing commands. */
   { GLHCK_CHANNEL_DRAW,   0 },

   /* trace channel */
   { GLHCK_CHANNEL_TRACE,  0 },

   /* <end of list> */
   { NULL,                 0 },
};

/* \brief is a draw function? */
static int _glhckTraceIsDrawFunction(const char *function)
{
   int i;
   for (i = 0; _drawFuncs[i]; ++i)
      if (!strcmp(_drawFuncs[i], function))
         return RETURN_TRUE;
   return RETURN_FALSE;
}

/* \brief channel is active? */
static int _glhckTraceIsActive(const char *name)
{
   int i;
   for (i = 0; _GLHCKlibrary.trace[i].name; ++i)
      if (!_glhckStrupcmp(_GLHCKlibrary.trace[i].name, name))
            return _GLHCKlibrary.trace[i].active;
   return 0;
}

/* \brief set channel active or not */
static void _glhckTraceSet(const char *name, int active)
{
   int i;
   for(i = 0; _GLHCKlibrary.trace[i].name ; ++i)
      if (!_glhckStrupcmp(name, _GLHCKlibrary.trace[i].name)                  ||
         (!_glhckStrupcmp(name, GLHCK_CHANNEL_ALL)                            &&
          _glhckStrupcmp(_GLHCKlibrary.trace[i].name, GLHCK_CHANNEL_TRACE)    &&
          _glhckStrupcmp(_GLHCKlibrary.trace[i].name, GLHCK_CHANNEL_DRAW)))
      _GLHCKlibrary.trace[i].active = active;
}

/* \brief init debug system */
void _glhckTraceInit(int argc, char **argv)
{
   int i, count;
   char *match, **split;

   /* init */
   _GLHCKlibrary.trace = _traceChannels;

   i = 0; match = NULL;
   for(i = 0, match = NULL; i != argc; ++i) {
      if (!_glhckStrnupcmp(argv[i], GLHCK_CHANNEL_SWITCH"=", strlen(GLHCK_CHANNEL_SWITCH"="))) {
         match = argv[i] + strlen(GLHCK_CHANNEL_SWITCH"=");
         break;
      }
   }
   if (!match) return;
   count = _glhckStrsplit(&split, match, ",");
   if (!split) return;

   for (i = 0; i != count; ++i) {
      if (!strncmp(split[i], "+", 1))
         _glhckTraceSet(split[i] + 1, 1);
      else if (!strncmp(split[i], "-", 1))
         _glhckTraceSet(split[i] + 1, 0);
   }

   _glhckStrsplitClear(&split);
}

/* \brief output trace info */
void _glhckTrace(const char *channel, const char *function, const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   if (!_GLHCKlibrary.trace) return;
   if (!_glhckTraceIsActive(GLHCK_CHANNEL_TRACE))
      return;

   if (!_glhckTraceIsActive(GLHCK_CHANNEL_DRAW) &&
        _glhckTraceIsDrawFunction(function))
      return;

   if (!_glhckTraceIsActive(channel))
      return;

   va_start(args, fmt);
   vsnprintf(buffer, LINE_MAX-1, fmt, args);
   va_end(args);

   _glhckPuts(buffer);
}

/* \brief pass debug info */
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   va_start(args, fmt);
   vsnprintf(buffer, LINE_MAX-1, fmt, args);
   va_end(args);

   if (!_glhckDebugHook) {
      puts(buffer);
      fflush(stdout);
      return;
   }

   _glhckDebugHook(file, line, func, level, buffer);
}

/* public api */

/* \brief set debug hook */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func)
{
   _glhckDebugHook = func;
}

/* vim: set ts=8 sw=3 tw=0 :*/
