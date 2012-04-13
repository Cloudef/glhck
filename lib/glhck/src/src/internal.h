#ifndef _internal_h_
#define _internal_h_

#if defined(_init_c_)
#  define GLHCKGLOBAL
#else
#  define GLHCKGLOBAL extern
#endif

#include "../include/GL/glhck.h"
#include <stdlib.h> /* for size_t  */
#include <string.h> /* for strrchr */

#if defined(_init_c_)
   char _glhckInitialized = 0;
#else
   GLHCKGLOBAL char _glhckInitialized;
#endif

/* hooks in debug mode*/
#ifndef NDEBUG
void* malloc(size_t);
#endif /* NDEBUG */

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
   glhckPrecision   verticesPrecision;
   glhckPrecision   coordsPrecision;
   glhckPrecision   normalsPrecision;
   glhckPrecision   colorsPrecision;
   glhckPrecision   indicesPrecision;
   void              *vertices;
   void              *coords;
   void              *normals;
   void              *colors;
   void              *indices;
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
   const char              *name;
   glhckRenderType         type;
   struct __GLHCKrenderAPI api;
} __GLHCKrender;

typedef struct __GLHCKlibrary
{
   struct __GLHCKtexture texture;
   struct __GLHCKrender  render;
} __GLHCKlibrary;

/* define global object */
GLHCKGLOBAL __GLHCKlibrary _GLHCKlibrary;

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

/* tracing && debug macros */
#define THIS_FILE ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#define TRACE_FMT       "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2()"
#define CALL_FMT(fmt)   "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2(\5"fmt"\2)"
#define RET_FMT(fmt)    "\2@FILE \5%-20s \2@LINE \5%-4d \5>> \3%s\2()\4 => \2(\5"fmt"\2)"

#define DEBUG(level, fmt, ...)   _glhckPassDebug(THIS_FILE, __LINE__, __func__, level, fmt, ##__VA_ARGS__)
#define TRACE()                  _glhckTrace(__func__, TRACE_FMT,      THIS_FILE, __LINE__, __func__)
#define CALL(args, ...)          _glhckTrace(__func__, CALL_FMT(args), THIS_FILE, __LINE__, __func__, ##__VA_ARGS__)
#define RET(args, ...)           _glhckTrace(__func__, RET_FMT(args),  THIS_FILE, __LINE__, __func__, ##__VA_ARGS__)

/* private api */

/* internal allocation functions */
void* _glhckMalloc(size_t size);
void* _glhckCalloc(unsigned int items, size_t size);
void* _glhckRealloc(void *ptr, unsigned int old_items, unsigned int items, size_t size);
void* _glhckCopy(void *ptr, size_t size);
void  _glhckFree(void *ptr);

/* util functions */
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
void _glhckTrace(const char *function, const char *fmt, ...);
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *fmt, ...);

#endif /* _internal_h_ */
