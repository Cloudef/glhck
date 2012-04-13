#include "internal.h"
#include <stdio.h>  /* for printf   */
#include <stdarg.h> /* for va_start */
#include <limits.h> /* for LINE_MAX */

#ifdef _WIN32
#  include <windows.h>
#endif

/* debug hook function */
static glhckDebugHookFunc _glhckDebugHook = NULL;

#define TRACE_CHANNEL "TRACE"
#define DRAW_CHANNEL  "DRAW"
#define ALL_CHANNEL   "ALL"
#define CLI_SWITCH    "DEBUG"

typedef struct _glhckDebugChannel
{
   const char  *name;
   char        active;
} _glhckDebugChannel;

static const char *_drawFuncs[] = {
   "glhckRender",
   "glhckObjectDraw",
   "objectDraw",
   "render",
   NULL,
};

/* list all debug channels */
static _glhckDebugChannel _channel[] =
{
   /* special DRAW channel,
    * will stop bloating output with
    * all drawing commands. */
   { DRAW_CHANNEL,      0 },

   /* TRACING */
   { TRACE_CHANNEL,     0 },

   /* <end of list> */
   { NULL,              0 },
};

static void _glhckRed(void)
{
#ifdef __unix__
   printf("\33[31m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_INTENSITY);
#endif
}

static void _glhckGreen(void)
{
#ifdef __unix__
   printf("\33[32m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN
   |FOREGROUND_INTENSITY);
#endif
}

static void _glhckYellow(void)
{
#ifdef __unix__
   printf("\33[33m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_GREEN
   |FOREGROUND_RED|FOREGROUND_INTENSITY);
#endif
}

static void _glhckBlue(void)
{
#ifdef __unix__
   printf("\33[34m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_BLUE
   |FOREGROUND_GREEN|FOREGROUND_INTENSITY);
#endif
}

static void _glhckWhite(void)
{
#ifdef __unix__
   printf("\33[37m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

static void _glhckNormal(void)
{
#ifdef __unix__
   printf("\33[0m");
#endif

#ifdef _WIN32
   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, FOREGROUND_RED
   |FOREGROUND_GREEN|FOREGROUND_BLUE);
#endif
}

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
   for (i = 0; _channel[i].name; ++i)
      if (!_glhckStrupcmp(_channel[i].name, name))
            return _channel[i].active;
   return 0;
}

/* \brief set channel active or not */
static void _glhckTraceSet(const char *name, int active)
{
   int i;
   for(i = 0; _channel[i].name ; ++i)
      if (!_glhckStrupcmp(name, _channel[i].name) ||
         (!_glhckStrupcmp(name, ALL_CHANNEL) &&
          !_glhckStrupcmp(name, TRACE_CHANNEL)))
      _channel[i].active = active;
}

/* \brief init debug system */
void _glhckTraceInit(int argc, char **argv)
{
   int i, count;
   char *match, **split;

   i = 0; match = NULL;
   for(i = 0, match = NULL; i != argc; ++i) {
      if (!_glhckStrnupcmp(argv[i], CLI_SWITCH"=", strlen(CLI_SWITCH"="))) {
         match = argv[i] + strlen(CLI_SWITCH"=");
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

/* \brief colored puts */
static void _glhckPuts(const char *buffer)
{
   int i;
   size_t len;

   len = strlen(buffer);
   for (i = 0; i != len; ++i) {
           if (buffer[i] == '\1') _glhckRed();
      else if (buffer[i] == '\2') _glhckGreen();
      else if (buffer[i] == '\3') _glhckYellow();
      else if (buffer[i] == '\4') _glhckBlue();
      else if (buffer[i] == '\5') _glhckWhite();
      else printf("%c", buffer[i]);
   }
   _glhckNormal();
   printf("\n");
   fflush(stdout);
}

/* \brief output trace info */
void _glhckTrace(const char *function, const char *fmt, ...)
{
   va_list args;
   char buffer[LINE_MAX];

   if (!_glhckTraceIsActive(TRACE_CHANNEL))
      return;

   if (!_glhckTraceIsActive(DRAW_CHANNEL) &&
        _glhckTraceIsDrawFunction(function))
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

/* \brief set debug hook */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func)
{
   _glhckDebugHook = func;
}

/* vim: set ts=8 sw=3 tw=0 :*/
