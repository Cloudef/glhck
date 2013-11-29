#include "internal.h"
#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for printf   */
#include <stdarg.h> /* for va_start */

/* Tracing levels:
 * 0: Non frequent calls (creation, freeing, etc..)
 * 1: Average call (setters, getters)
 * 2: Spam call (render, draw, transformation, ref/deref, etc..)
 * 3: Alloc.c allocations */

/* list all debug channels */
static __GLHCKtraceChannel _traceChannels[] =
{
   /* glhck's channels */
   { GLHCK_CHANNEL_GLHCK,           0 },
   { GLHCK_CHANNEL_IMPORT,          0 },
   { GLHCK_CHANNEL_OBJECT,          0 },
   { GLHCK_CHANNEL_BONE,            0 },
   { GLHCK_CHANNEL_SKINBONE,        0 },
   { GLHCK_CHANNEL_ANIMATION,       0 },
   { GLHCK_CHANNEL_ANIMATOR,        0 },
   { GLHCK_CHANNEL_TEXT,            0 },
   { GLHCK_CHANNEL_CAMERA,          0 },
   { GLHCK_CHANNEL_GEOMETRY,        0 },
   { GLHCK_CHANNEL_MATERIAL,        0 },
   { GLHCK_CHANNEL_TEXTURE,         0 },
   { GLHCK_CHANNEL_ATLAS,           0 },
   { GLHCK_CHANNEL_ALLOC,           0 },
   { GLHCK_CHANNEL_RENDER,          0 },
   { GLHCK_CHANNEL_LIGHT,           0 },
   { GLHCK_CHANNEL_RENDERBUFFER,    0 },
   { GLHCK_CHANNEL_FRAMEBUFFER,     0 },
   { GLHCK_CHANNEL_HWBUFFER,        0 },
   { GLHCK_CHANNEL_SHADER,          0 },
   { GLHCK_CHANNEL_COLLISION,       0 },

   /* trace channel */
   { GLHCK_CHANNEL_TRACE,  0 },

   /* <end of list> */
   { NULL,                 0 },
};

/* \brief channel is active? */
static int _glhckTraceIsActive(const char *name)
{
   int i;
   for (i = 0; GLHCKT()->channel[i].name && strcmp(GLHCKT()->channel[i].name, name); ++i);
   return GLHCKT()->channel[i].active;
}

/* \brief set channel active or not */
static void _glhckTraceSet(const char *name, int active)
{
   int i;
   for(i = 0; GLHCKT()->channel[i].name ; ++i)
      if (!_glhckStrupcmp(name, GLHCKT()->channel[i].name)  ||
         (!_glhckStrupcmp(name, GLHCK_CHANNEL_ALL)          &&
          _glhckStrupcmp(GLHCKT()->channel[i].name, GLHCK_CHANNEL_TRACE)))
      GLHCKT()->channel[i].active = active;
}

/* \brief init debug system */
void _glhckTraceInit(int argc, const char **argv)
{
   int i, count;
   const char *match;
   char **split;
   __GLHCKtraceChannel *channels;

   /* init */
   if (!(channels = malloc(sizeof(_traceChannels))))
      return;

   /* copy the template */
   memcpy(channels, _traceChannels, sizeof(_traceChannels));
   GLHCKT()->channel = channels;

#if EMSCRIPTEN
   // _glhckTraceSet("2", 1);
   // _glhckTraceSet("trace", 1);
   _glhckTraceSet("all", 1);
#endif

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
         GLHCKM()->coloredLog = 0;
      else if (!strncmp(split[i], "2", 1))
         GLHCKT()->level = 2;
      else if (!strncmp(split[i], "1", 1))
         GLHCKT()->level = 1;
      else if (!strncmp(split[i], "+", 1))
         _glhckTraceSet(split[i] + 1, 1);
      else if (!strncmp(split[i], "-", 1))
         _glhckTraceSet(split[i] + 1, 0);
   }

   _glhckStrsplitClear(&split);
}

/* \brief destroy debug system */
void _glhckTraceTerminate(void)
{
   IFDO(free, GLHCKT()->channel);
}

/* \brief output trace info */
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...)
{
   va_list args;
   char buffer[2048];
   (void)function;

   if (!GLHCKT()->channel) return;

   if (level > GLHCKT()->level)
      return;

   if (!_glhckTraceIsActive(GLHCK_CHANNEL_TRACE))
      return;

   if (!_glhckTraceIsActive(channel))
      return;

   memset(buffer, 0, sizeof(buffer));
   va_start(args, fmt);
   vsnprintf(buffer, sizeof(buffer)-1, fmt, args);
   va_end(args);

   _glhckPuts(buffer);
}

/* \brief pass debug info */
void _glhckPassDebug(const char *file, int line, const char *func,
      glhckDebugLevel level, const char *channel, const char *fmt, ...)
{
   va_list args;
   char buffer[2048];

   memset(buffer, 0, sizeof(buffer));
   va_start(args, fmt);
   vsnprintf(buffer, sizeof(buffer)-1, fmt, args);
   va_end(args);

   if (!GLHCKT()->debugHook) {
      /* by default, we assume debug prints are
       * useless if tracing. */
      if (_glhckTraceIsActive(GLHCK_CHANNEL_TRACE) || !_glhckTraceIsActive(channel))
         return;

      _glhckPrintf(DBG_FMT, line, file, buffer);
      return;
   }

   GLHCKT()->debugHook(file, line, func, level, buffer);
}

/***
 * public api
 ***/

/* \brief set debug hook */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func)
{
   GLHCK_INITIALIZED();
   GLHCKT()->debugHook = func;
}

/* vim: set ts=8 sw=3 tw=0 :*/
