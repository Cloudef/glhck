#ifndef _internal_h_
#define _internal_h_

#if defined(_init_c_)
#  define GLHCKGLOBAL
#else
#  define GLHCKGLOBAL extern
#endif

#include "../include/GL/glhck.h"
#include <string.h> /* for strrchr */

#if defined(_init_c_)
   char _glhckInitialized = 0;
#else
   GLHCKGLOBAL char _glhckInitialized;
#endif

/* tracing channels */
#define GLHCK_CHANNEL_GLHCK      "GLHCK"
#define GLHCK_CHANNEL_IMPORT     "IMPORT"
#define GLHCK_CHANNEL_OBJECT     "OBJECT"
#define GLHCK_CHANNEL_GEOMETRY   "GEOMETRY"
#define GLHCK_CHANNEL_TEXTURE    "TEXTURE"
#define GLHCK_CHANNEL_ALLOC      "ALLOC"
#define GLHCK_CHANNEL_RENDER     "RENDER"
#define GLHCK_CHANNEL_TRACE      "TRACE"
#define GLHCK_CHANNEL_DRAW       "DRAW"
#define GLHCK_CHANNEL_ALL        "ALL"
#define GLHCK_CHANNEL_SWITCH     "DEBUG"

/* return variables used throughout library */
typedef enum _glhckReturnValue {
   RETURN_FAIL    =  0,
   RETURN_OK      =  1,
   RETURN_TRUE    =  1,
   RETURN_FALSE   =  !RETURN_TRUE
} _glhckReturnValue;

/* glhck object structs */
typedef struct _glhckTexture
{
   char *file;
   unsigned int object, flags;
   unsigned char *data;
   int width, height, channels;
   size_t size;
   short refCounter;
} _glhckTexture;

typedef struct __GLHCKobjectGeometry
{
   struct glhckVertexData     *vertexData;
   GLHCK_CAST_INDEX           *indices;
   size_t                     indicesCount, vertexCount;
} __GLHCKobjectGeometry;

typedef struct _glhckObject
{
   struct __GLHCKobjectGeometry  geometry;
   short                         refCounter;
} _glhckObject;

/* library global data */
typedef struct __GLHCKtextureCache
{
   struct _glhckTexture       *texture;
   struct __GLHCKtextureCache *next;
} __GLHCKtextureCache;

typedef struct __GLHCKtexture
{
   unsigned int               bind;
   struct __GLHCKtextureCache *cache;
} __GLHCKtexture;

/* render api */
typedef void (*__GLHCKrenderAPIterminate)        (void);
typedef void (*__GLHCKrenderAPIresize)           (int width, int height);
typedef void (*__GLHCKrenderAPIrender)           (void);
typedef void (*__GLHCKrenderAPIobjectDraw)       (_glhckObject *object);

/* object generation */
typedef void (*__GLHCKrenderAPIgenerateTextures) (unsigned int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteTextures)   (unsigned int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIgenerateBuffers)  (unsigned int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteBuffers)    (unsigned int count, unsigned int *objects);

/* textures */
typedef void (*__GLHCKrenderAPIbindTexture)      (unsigned int object);
typedef int  (*__GLHCKrenderAPIuploadTexture)    (_glhckTexture *texture, unsigned int flags);

typedef unsigned int (*__GLHCKrenderAPIcreateTexture) (const unsigned char *const buffer,
                                                       int width, int height, int channels,
                                                       unsigned int reuse_texture_ID,
                                                       unsigned int flags);


/* buffers */
typedef void (*__GLHCKrenderAPIbindBuffer)       (unsigned int object);

typedef struct __GLHCKrenderAPI
{
   __GLHCKrenderAPIterminate        terminate;
   __GLHCKrenderAPIresize           resize;
   __GLHCKrenderAPIrender           render;
   __GLHCKrenderAPIobjectDraw       objectDraw;
   __GLHCKrenderAPIgenerateTextures generateTextures;
   __GLHCKrenderAPIdeleteTextures   deleteTextures;
   __GLHCKrenderAPIgenerateBuffers  generateBuffers;
   __GLHCKrenderAPIdeleteBuffers    deleteBuffers;
   __GLHCKrenderAPIbindTexture      bindTexture;
   __GLHCKrenderAPIuploadTexture    uploadTexture;
   __GLHCKrenderAPIcreateTexture    createTexture;
   __GLHCKrenderAPIbindBuffer       bindBuffer;
} __GLHCKrenderAPI;

typedef struct __GLHCKrender
{
   int width, height;
   const char              *name;
   glhckRenderType         type;
   struct __GLHCKrenderAPI api;
} __GLHCKrender;

typedef struct __GLHCKtrace
{
   const char  *name;
   char        active;
} __GLHCKtrace;

#ifndef NDEBUG
typedef struct __GLHCKalloc {
   const char           *channel;
   void                 *ptr;
   size_t               size;
   struct __GLHCKalloc  *next;
} __GLHCKalloc;
#endif

typedef struct __GLHCKlibrary
{
   struct __GLHCKtexture   texture;
   struct __GLHCKrender    render;
   struct __GLHCKtrace     *trace;
#ifndef NDEBUG
   struct __GLHCKalloc     *alloc;
#endif
} __GLHCKlibrary;

/* define global object */
GLHCKGLOBAL struct __GLHCKlibrary _GLHCKlibrary;

typedef struct _glhckTexturePacker
{
   short             debug_count;
   struct tpNode     *free_list;
   short             texture_index;
   short             texture_count;
   struct tpTexture  *textures;
   short             longest_edge;
   short             total_area;
} _glhckTexturePacker;

/* tracking allocation macros */
#define _glhckMalloc(x)    __glhckMalloc(GLHCK_CHANNEL, x)
#define _glhckCalloc(x,y)  __glhcKCalloc(GLHCK_CHANNEL, x, y)
#define _glhckCopy(x,y)    __glhckCopy(GLHCK_CHANNEL, x, y)

/* tracing && debug macros */
#define THIS_FILE ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#define TRACE_FMT       "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2()"
#define CALL_FMT(fmt)   "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2(\5"fmt"\2)"
#define RET_FMT(fmt)    "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2()\4 => \2(\5"fmt"\2)"

#define DEBUG(level, fmt, ...)   _glhckPassDebug(THIS_FILE, __LINE__, __func__, level, fmt, ##__VA_ARGS__)
#define TRACE()                  _glhckTrace(GLHCK_CHANNEL, __func__, TRACE_FMT,      THIS_FILE, __LINE__, __func__)
#define CALL(args, ...)          _glhckTrace(GLHCK_CHANNEL, __func__, CALL_FMT(args), THIS_FILE, __LINE__, __func__, ##__VA_ARGS__)
#define RET(args, ...)           _glhckTrace(GLHCK_CHANNEL, __func__, RET_FMT(args),  THIS_FILE, __LINE__, __func__, ##__VA_ARGS__)

/* private api */

/* internal allocation functions */
void* __glhckMalloc(const char *channel, size_t size);
void* __glhckCalloc(const char *channel, size_t nmemb, size_t size);
void* __glhckCopy(const char *channel, void *ptr, size_t nmemb);
void* _glhckRealloc(void *ptr, size_t omemb, size_t nmemb, size_t size);
void  _glhckFree(void *ptr);

#ifndef NDEBUG
/* tracking functions */
void _glhckTrackTerminate(void);
#endif

/* util functions */
void  _glhckRed(void);
void  _glhckGreen(void);
void  _glhckBlue(void);
void  _glhckYellow(void);
void  _glhckWhite(void);
void  _glhckNormal(void);
void  _glhckPuts(const char *buffer);
int   _glhckStrsplit(char ***dst, char *str, char *token);
void  _glhckStrsplitClear(char ***dst);
char* _glhckStrupstr(const char *hay, const char *needle);
int   _glhckStrupcmp(const char *hay, const char *needle);
int   _glhckStrnupcmp(const char *hay, const char *needle, size_t len);

/* texture packing functions */
void  _glhckTexturePackerSetCount(_glhckTexturePacker *tp, short textureCount);
short _glhckTexturePackerAdd(_glhckTexturePacker *tp, int width, int height);
int   _glhckTexturePackerPack(_glhckTexturePacker *tp, int *width, int *height, int forcePowerOfTwo, int onePixelBorder);
int   _glhckTexturePackerGetLocation(_glhckTexturePacker *tp, int index, int *x, int *y, int *width, int *height);
_glhckTexturePacker*  glhckTexturePackerNew(void);
void                  glhckTexturePackerFree(_glhckTexturePacker *tp);

/* texture cache*/
void _glhckTextureCacheRelease(void);

/* tracing && debug functions */
void _glhckTraceInit(int argc, char **argv);
void _glhckTrace(const char *channel, const char *function, const char *fmt, ...);
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *fmt, ...);

#endif /* _internal_h_ */
