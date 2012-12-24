#include "internal.h"
#include <stdio.h>  /* for printf   */
#include <stdarg.h> /* for va_start */
#include <limits.h> /* for LINE_MAX */

#ifdef _WIN32
#  include <windows.h>
#endif

/* debug hook function */
static glhckDebugHookFunc _glhckDebugHook = NULL;

/* Tracing levels:
 * 0: Non frequent calls (creation, freeing, etc..)
 * 1: Average call (setters, getters)
 * 2: Spam call (render, draw, transformation, ref/deref, etc..) */

/* list all debug channels */
__GLHCKtraceChannel _traceChannels[] =
{
   /* glhck's channels */
   { GLHCK_CHANNEL_GLHCK,    0 },
   { GLHCK_CHANNEL_IMPORT,   0 },
   { GLHCK_CHANNEL_OBJECT,   0 },
   { GLHCK_CHANNEL_TEXT,     0 },
   { GLHCK_CHANNEL_CAMERA,   0 },
   { GLHCK_CHANNEL_GEOMETRY, 0 },
   { GLHCK_CHANNEL_VDATA,    0 },
   { GLHCK_CHANNEL_TEXTURE,  0 },
   { GLHCK_CHANNEL_ATLAS,    0 },
   { GLHCK_CHANNEL_RTT,      0 },
   { GLHCK_CHANNEL_ALLOC,    0 },
   { GLHCK_CHANNEL_RENDER,   0 },

   /* trace channel */
   { GLHCK_CHANNEL_TRACE,  0 },

   /* <end of list> */
   { NULL,                 0 },
};

/* \brief channel is active? */
static int _glhckTraceIsActive(const char *name)
{
   int i;
   for (i = 0; _GLHCKlibrary.trace.channel[i].name; ++i)
      if (!_glhckStrupcmp(_GLHCKlibrary.trace.channel[i].name, name))
            return _GLHCKlibrary.trace.channel[i].active;
   return 0;
}

/* \brief set channel active or not */
static void _glhckTraceSet(const char *name, int active)
{
   int i;
   for(i = 0; _GLHCKlibrary.trace.channel[i].name ; ++i)
      if (!_glhckStrupcmp(name, _GLHCKlibrary.trace.channel[i].name)           ||
         (!_glhckStrupcmp(name, GLHCK_CHANNEL_ALL)                             &&
          _glhckStrupcmp(_GLHCKlibrary.trace.channel[i].name, GLHCK_CHANNEL_TRACE)))
      _GLHCKlibrary.trace.channel[i].active = (char)active;
}

/* \brief init debug system */
void _glhckTraceInit(int argc, const char **argv)
{
   size_t i, count;
   const char *match;
   char **split;

   /* init */
   _GLHCKlibrary.trace.channel = _traceChannels;

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
      if (!strncmp(split[i], "-color", 6))
         _GLHCKlibrary.misc.coloredLog = 0;
      else if (!strncmp(split[i], "2", 1))
         _GLHCKlibrary.trace.level = 2;
      else if (!strncmp(split[i], "1", 1))
         _GLHCKlibrary.trace.level = 1;
      else if (!strncmp(split[i], "+", 1))
         _glhckTraceSet(split[i] + 1, 1);
      else if (!strncmp(split[i], "-", 1))
         _glhckTraceSet(split[i] + 1, 0);
   }

   _glhckStrsplitClear(&split);
}

/* \brief output trace info */
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   if (!_GLHCKlibrary.trace.channel) return;

   if (level > _GLHCKlibrary.trace.level)
      return;

   if (!_glhckTraceIsActive(GLHCK_CHANNEL_TRACE))
      return;

   if (!_glhckTraceIsActive(channel))
      return;

   memset(buffer, 0, LINE_MAX);
   va_start(args, fmt);
   vsnprintf(buffer, LINE_MAX-1, fmt, args);
   va_end(args);

   _glhckPuts(buffer);
}

/* \brief pass debug info */
void _glhckPassDebug(const char *file, int line, const char *func,
      glhckDebugLevel level, const char *channel, const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   memset(buffer, 0, LINE_MAX);
   va_start(args, fmt);
   vsnprintf(buffer, LINE_MAX-1, fmt, args);
   va_end(args);

   if (!_glhckDebugHook) {
      /* by default, we assume debug prints are
       * useless if tracing. */
      if (_glhckTraceIsActive(GLHCK_CHANNEL_TRACE) ||
            !_glhckTraceIsActive(channel)) return;
      _glhckPrintf(DBG_FMT, line, file, buffer);
      return;
   }

   _glhckDebugHook(file, line, func, level, buffer);
}

/***
 * public api
 ***/

/* \brief set debug hook */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func)
{
   GLHCK_INITIALIZED();
   _glhckDebugHook = func;
}

/* vim: set ts=8 sw=3 tw=0 :*/
