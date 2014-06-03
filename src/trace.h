#ifndef __glhck_memory_h__
#define __glhck_memory_h__

#ifndef strrchr
#  include <string.h>
#endif

/* return variables used throughout library */
enum glhckReturnValue {
   RETURN_FAIL  = 0,
   RETURN_OK    = 1,
   RETURN_TRUE  = 1,
   RETURN_FALSE = !RETURN_TRUE
};

enum glhckDebugLevel;

/* tracing channels */
#define GLHCK_CHANNEL_ALLOC         "ALLOC"
#define GLHCK_CHANNEL_ANIMATION     "ANIMATION"
#define GLHCK_CHANNEL_ANIMATOR      "ANIMATOR"
#define GLHCK_CHANNEL_ATLAS         "ATLAS"
#define GLHCK_CHANNEL_BONE          "BONE"
#define GLHCK_CHANNEL_CAMERA        "CAMERA"
#define GLHCK_CHANNEL_COLLISION     "COLLISION"
#define GLHCK_CHANNEL_DRAW          "DRAW"
#define GLHCK_CHANNEL_FRAMEBUFFER   "FRAMEBUFFER"
#define GLHCK_CHANNEL_FRUSTUM       "FRUSTUM"
#define GLHCK_CHANNEL_GEOMETRY      "GEOMETRY"
#define GLHCK_CHANNEL_GLHCK         "GLHCK"
#define GLHCK_CHANNEL_HANDLE        "HANDLE"
#define GLHCK_CHANNEL_HWBUFFER      "HWBUFFER"
#define GLHCK_CHANNEL_IMPORT        "IMPORT"
#define GLHCK_CHANNEL_LIGHT         "LIGHT"
#define GLHCK_CHANNEL_MATERIAL      "MATERIAL"
#define GLHCK_CHANNEL_NETWORK       "NETWORK"
#define GLHCK_CHANNEL_OBJECT        "OBJECT"
#define GLHCK_CHANNEL_RENDER        "RENDER"
#define GLHCK_CHANNEL_RENDERBUFFER  "RENDERBUFFER"
#define GLHCK_CHANNEL_SHADER        "SHADER"
#define GLHCK_CHANNEL_SKINBONE      "SKINBONE"
#define GLHCK_CHANNEL_TEXT          "TEXT"
#define GLHCK_CHANNEL_TEXTURE       "TEXTURE"
#define GLHCK_CHANNEL_TRANSFORM     "TRANSFORM"
#define GLHCK_CHANNEL_STRING        "STRING"

/* tracing && debug macros */
#define THIS_FILE ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#define DBG_FMT       "\2%4d\1: \5%-20s \5%s"
#define TRACE_FMT     "\2%4d\1: \5%-20s \4%s\2()"
#define CALL_FMT(fmt) "\2%4d\1: \5%-20s \4%s\2(\5"fmt"\2)"
#define RET_FMT(fmt)  "\2%4d\1: \5%-20s \4%s\2()\3 => \2(\5"fmt"\2)"

#if !GLHCK_DISABLE_TRACE
#  define DEBUG(level, fmt, ...) _glhckPassDebug(THIS_FILE, __LINE__, __func__, level, GLHCK_CHANNEL, fmt, ##__VA_ARGS__)
#  define TRACE(level) _glhckTrace(level, GLHCK_CHANNEL, __func__, TRACE_FMT,      __LINE__, THIS_FILE, __func__)
#  define CALL(level, args, ...) _glhckTrace(level, GLHCK_CHANNEL, __func__, CALL_FMT(args), __LINE__, THIS_FILE, __func__, ##__VA_ARGS__)
#  define RET(level, args, ...) _glhckTrace(level, GLHCK_CHANNEL, __func__, RET_FMT(args),  __LINE__, THIS_FILE, __func__, ##__VA_ARGS__)
#else
#  define DEBUG(level, fmt, ...) { ; }
#  define TRACE(level) ;
#  define CALL(level, args, ...) ;
#  define RET(level, args, ...) ;
#endif

/* if exists then perform function and set NULL
 * used mainly to shorten if (x) free(x); x = NULL; */
#define IFDO(f, x) { if (x) f(x); x = 0; }

/* perform function and set NULL (no checks)
 * used mainly to shorten free(x); x = NULL; */
#define NULLDO(f, x) { f(x); x = 0; }

typedef const char* (*_glhckPrintItemFunc)(const void*, size_t*);

int _glhckTraceInit(int argc, const char **argv);

#if __GNUC__
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...)
   __attribute__((format(printf, 4, 5)));
void _glhckPassDebug(const char *file, int line, const char *func, enum glhckDebugLevel level, const char *channel, const char *fmt, ...)
   __attribute__((format(printf, 6, 7)));
#else
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...);
void _glhckPassDebug(const char *file, int line, const char *func, enum glhckDebugLevel level, const char *channel, const char *fmt, ...);
#endif

const char* _glhckSprintf(size_t *outLen, const char *fmt, ...);
const char* _glhckSprintfArray(size_t *outLen, const void *data, size_t memb, size_t size, _glhckPrintItemFunc func);
int _glhckPuts(const char *buffer);

#endif /* __glhck_memory_h__ */
