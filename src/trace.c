#include "trace.h"

#include <glhck/glhck.h>
#include <stdlib.h> /* for malloc */
#include <stdio.h>  /* for printf   */
#include <stdarg.h> /* for va_start */
#include <ctype.h>  /* for toupper  */
#include <stdint.h> /* for intptr_t */

#include "handle.h"
#include "lut/lut.h"

#include "system/tls.h"

/* Tracing levels:
 * 0: Non frequent calls (creation, freeing, etc..)
 * 1: Average call (setters, getters)
 * 2: Spam call (render, draw, transformation, ref/deref, etc..)
 * 3: Alloc.c allocations */

static _GLHCK_TLS chckHashTable *channels;
static _GLHCK_TLS int coloredLog = 1;
static _GLHCK_TLS int traceLevel = 0;
static _GLHCK_TLS int traceEnabled = 0;
static _GLHCK_TLS int allChannels = 0;

static void defaultDebugHook(const char *file, int line, const char *func, glhckDebugLevel level, const char *channel, const char *message);
static _GLHCK_TLS glhckDebugHookFunc debugHook = defaultDebugHook;

/* somewhat crappy assumptation that we won't have 32 inline calls filling these buffers. */
static _GLHCK_TLS char *staticBuffer[32];
static _GLHCK_TLS size_t staticBufferLen[32];
static _GLHCK_TLS int staticBufferIdx = 0;

static const char* ssprintfArray(size_t *outLen, const void *items, size_t memb, size_t size, _glhckPrintItemFunc func)
{
   char **buffer = &staticBuffer[staticBufferIdx];
   size_t *bufferLen = &staticBufferLen[staticBufferIdx];
   staticBufferIdx = (staticBufferIdx + 1) % 32;

   size_t flen = 0;

   for (size_t i = 0; i < memb; ++i) {
      const void *data = items + i * size;

      size_t len;
      const char *r = func(data, &len);

      if (len <= 0)
         continue;

      if (flen + len + 3 > *bufferLen) {
         void *ptr;
         if (!(ptr = realloc(*buffer, flen + len + 3)))
            return NULL;

         *buffer = ptr;
         *bufferLen = flen + len + 3;
      }

      (*buffer)[flen] = (i == 0 ? '[' : ',');
      memcpy(*buffer + flen + 1, r, len);
      flen += len + 1;
   }

   if (outLen)
      *outLen = flen;

   (*buffer)[flen] = ']';
   (*buffer)[flen + 1] = 0;
   return *buffer;
}

static const char* ssprintf(size_t *outLen, const char *fmt, va_list args)
{
   assert(fmt);

   char **buffer = &staticBuffer[staticBufferIdx];
   size_t *bufferLen = &staticBufferLen[staticBufferIdx];
   staticBufferIdx = (staticBufferIdx + 1) % 32;

   va_list orig;
   va_copy(orig, args);

   size_t len = vsnprintf(NULL, 0, fmt, args);

   if (len <= 0)
      return NULL;

   if (len > *bufferLen) {
      void *ptr;
      if (!(ptr = realloc(*buffer, len + 1)))
         return NULL;

      memset(ptr + *bufferLen, 0, len - *bufferLen);

      *buffer = ptr;
      *bufferLen = len;
   }

   len = vsprintf(*buffer, fmt, orig);

   if (outLen)
      *outLen = len;

   return *buffer;
}

#ifdef __WIN32__
#  include <windows.h>
#endif

enum glhckLogColor {
   GLHCK_LOG_COLOR_NORMAL,
   GLHCK_LOG_COLOR_RED,
   GLHCK_LOG_COLOR_GREEN,
   GLHCK_LOG_COLOR_BLUE,
   GLHCK_LOG_COLOR_YELLOW,
   GLHCK_LOG_COLOR_WHITE
};

static int strnupcmp(const char *hay, const char *needle, size_t len)
{
   const unsigned char *p1 = (const unsigned char*)hay;
   const unsigned char *p2 = (const unsigned char*)needle;

   unsigned char a = 0, b = 0;
   for (size_t i = 0; len > 0; --len, ++i) {
      if ((a = toupper(*p1++)) != (b = toupper(*p2++)))
         return a - b;
   }

   return a - b;
}

static int strupcmp(const char *hay, const char *needle)
{
   return strnupcmp(hay, needle, strlen(hay));
}

static char* strupstr(const char *hay, const char *needle)
{
   size_t len, len2;
   if ((len = strlen(hay)) < (len2 = strlen(needle)))
      return NULL;

   if (!strnupcmp(hay, needle, len2))
      return (char*)hay;

   size_t r = 0, p = 0;
   for (size_t i = 0; i < len; ++i) {
      if (p == len2)
         return (char*)hay + r;

      if (toupper(hay[i]) == toupper(needle[p++])) {
         if (!r)
            r = i;
      } else {
         if (r)
            i = r;
         r = p = 0;
      }
   }

   return (p == len2 ? (char*)hay + r : NULL);
}

static void outColor(enum glhckLogColor color)
{
   if (color > GLHCK_LOG_COLOR_WHITE || !coloredLog)
      return;

#if defined(__unix__) || defined(__APPLE__)
   static const char *ctable[GLHCK_LOG_COLOR_WHITE + 1] = {
      "\33[0m", /* GLHCK_LOG_COLOR_NORMAL */
      "\33[31m", /* GLHCK_LOG_COLOR_RED */
      "\33[32m", /* GLHCK_LOG_COLOR_GREEN */
      "\33[34m", /* GLHCK_LOG_COLOR_BLUE */
      "\33[33m", /* GLHCK_LOG_COLOR_YELLOW */
      "\33[37m" /* GLHCK_LOG_COLOR_WHITE */
   };

   printf("%s", ctable[color]);
#endif

#ifdef _WIN32
   static const unsigned int ctable[GLHCK_LOG_COLOR_WHITE + 1] = {
      FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE, /* GLHCK_LOG_COLOR_NORMAL */
      FOREGROUND_RED | FOREGROUND_INTENSITY, /* GLHCK_LOG_COLOR_RED */
      FOREGROUND_GREEN | FOREGROUND_INTENSITY, /* GLHCK_LOG_COLOR_GREEN */
      FOREGROUND_BLUE | FOREGROUND_GREEN | FOREGROUND_INTENSITY, /* GLHCK_LOG_COLOR_BLUE */
      FOREGROUND_YELLOW | FOREGROUND_RED | FOREGROUND_INTENSITY, /* GLHCK_LOG_COLOR_YELLOW */
      FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE /* GLHCK_LOG_COLOR_WHITE */
   };

   HANDLE hStdout = GetStdHandle(STD_OUTPUT_HANDLE);
   SetConsoleTextAttribute(hStdout, ctable[color]);
#endif
}

static int printfFun(int (*func)(const char*), const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   const char *buffer = ssprintf(NULL, fmt, args);
   va_end(args);

   if (buffer)
      return func(buffer);

   return 0;
}

static void defaultDebugHook(const char *file, int line, const char *func, glhckDebugLevel level, const char *channel, const char *message)
{
   (void)func, (void)level;

   /* by default, we assume debug prints are useless if tracing. */
   int *enabled;
   if (traceEnabled || (!allChannels && (!channels || ((enabled = chckHashTableStrGet(channels, channel)) && *enabled))))
      return;

   printfFun(_glhckPuts, DBG_FMT, line, file, message);
}

/* \brief set channel active or not */
static void traceSet(const char *name, int active)
{
   if (!strupcmp(name, "all")) {
      allChannels = active;
      return;
   } else if (!strupcmp(name, "trace")) {
      traceEnabled = active;
      return;
   }

   chckHashTableStrSet(channels, name, &active, sizeof(int));
}

#if EMSCRIPTEN
static const char *EMSCRIPTEN_URL = NULL;
__attribute__((used, noinline)) extern void _glhckTraceEmscriptenURL(const char *url) {
   if (EMSCRIPTEN_URL) free((char*)EMSCRIPTEN_URL);
   EMSCRIPTEN_URL = (url ? strdup(url) : NULL);
}
#endif

/* \brief init debug system */
int _glhckTraceInit(int argc, const char **argv)
{
   const char *match = NULL;
   for(int i = 0; i < argc; ++i) {
      if (!strnupcmp(argv[i], "DEBUG=", strlen("DEBUG="))) {
         match = argv[i] + strlen("DEBUG=");
         break;
      }
   }

#if EMSCRIPTEN
   if (!match) {
      asm("if (typeof __glhckTraceEmscriptenURL != 'undefined')"
          "__glhckTraceEmscriptenURL(allocate(intArrayFromString(document.URL), 'i8', ALLOC_STACK));"
          "else Module.print('__glhckTraceEmscriptenURL was not exported for some reason.. Skipping!');");
      if (EMSCRIPTEN_URL) match = strupstr(EMSCRIPTEN_URL, GLHCK_CHANNEL_SWITCH"=") + strlen(GLHCK_CHANNEL_SWITCH"=");
   }
#endif

   if (!match)
      return RETURN_OK;

   if (!(channels = chckHashTableNew(32)))
      goto fail;

   size_t pos = 0;
   while ((pos = strcspn(match, ",")) != 0) {
      char buf[64];
      memcpy(buf, match, (pos < sizeof(buf) ? pos : sizeof(buf)));
      buf[pos] = 0;

      if (!strncmp(buf, "-color", 6))
         coloredLog = 0;
      else if (!strncmp(buf, "2", 1))
         traceLevel = 2;
      else if (!strncmp(buf, "1", 1))
         traceLevel = 1;
      else if (!strncmp(buf, "+", 1))
         traceSet(buf + 1, 1);
      else if (!strncmp(buf, "-", 1))
         traceSet(buf + 1, 0);

      match += pos + (match[pos] != 0);
   }

   return RETURN_OK;

fail:
   IFDO(chckHashTableFree, channels);
   return RETURN_FAIL;
}

const char* _glhckSprintf(size_t *outLen, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   const char *buffer = ssprintf(outLen, fmt, args);
   va_end(args);
   return buffer;
}

const char* _glhckSprintfArray(size_t *outLen, const void *items, size_t memb, size_t size, _glhckPrintItemFunc func)
{
   return ssprintfArray(outLen, items, memb, size, func);
}

int _glhckPuts(const char *buffer)
{
   size_t len = strlen(buffer);
   for (size_t i = 0; i != len; ++i) {
      if (buffer[i] >= 1 && buffer[i] <= 5) {
         outColor(buffer[i]);
      } else {
         printf("%c", buffer[i]);
      }
   }

   outColor(GLHCK_LOG_COLOR_NORMAL);
   puts("");
   fflush(stdout);
   return len;
}

/* \brief output trace info */
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...)
{
   (void)function;

   if (!channels || !traceEnabled || level > traceLevel)
      return;

   int *enabled;
   if (!allChannels && (!channels || ((enabled = chckHashTableStrGet(channels, channel)) && *enabled)))
      return;

   va_list args;
   va_start(args, fmt);
   const char *buffer = ssprintf(NULL, fmt, args);
   va_end(args);

   if (buffer)
      _glhckPuts(buffer);
}

/* \brief pass debug info */
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *channel, const char *fmt, ...)
{
   va_list args;
   va_start(args, fmt);
   const char *buffer = ssprintf(NULL, fmt, args);
   va_end(args);

   if (buffer)
      debugHook(file, line, func, level, channel, buffer);
}

/***
 * public api
 ***/

/* \brief set debug hook */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func)
{
   debugHook = (func ? func : defaultDebugHook);
}

GLHCKAPI void glhckLogColor(int color)
{
#if EMSCRIPTEN
   color = 0;
#endif

   if (color != coloredLog && !color)
      outColor(GLHCK_LOG_COLOR_NORMAL);

   coloredLog = color;
}

/* vim: set ts=8 sw=3 tw=0 :*/
