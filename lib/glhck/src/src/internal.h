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
#define GLHCK_CHANNEL_CAMERA     "CAMERA"
#define GLHCK_CHANNEL_GEOMETRY   "GEOMETRY"
#define GLHCK_CHANNEL_TEXTURE    "TEXTURE"
#define GLHCK_CHANNEL_ATLAS      "ATLAS"
#define GLHCK_CHANNEL_RTT        "RTT"
#define GLHCK_CHANNEL_ALLOC      "ALLOC"
#define GLHCK_CHANNEL_RENDER     "RENDER"
#define GLHCK_CHANNEL_TRACE      "TRACE"
#define GLHCK_CHANNEL_TRANSFORM  "TRANSFORM"
#define GLHCK_CHANNEL_DRAW       "DRAW"
#define GLHCK_CHANNEL_ALL        "ALL"
#define GLHCK_CHANNEL_SWITCH     "DEBUG"

/* temporary options */
#define GLHCK_IMPORT_PMD 1

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

typedef struct _glhckAtlasArea
{
   int x1, y1, x2, y2, rotated;
} _glhckAtlasArea;

typedef struct _glhckAtlasRect
{
   struct _glhckTexture *texture;
   unsigned short index;
   struct _glhckAtlasArea packed;
   struct _glhckAtlasRect *next;
} _glhckAtlasRect;

typedef struct _glhckAtlas
{
   struct _glhckAtlasRect *rect;
   struct _glhckTexture *texture;
   short refCounter;
} _glhckAtlas;

#define GLHCK_COLOR_ATTACHMENT   0x8CE0
#define GLHCK_DEPTH_ATTACHMENT   0x8D00
#define GLHCK_STENCIL_ATTACHMENT 0x8D20

typedef struct _glhckRtt
{
   unsigned int object;
   _glhckTexture *texture;
   short refCounter;
} _glhckRtt;

/* precision constants */
#define GLHCK_BYTE            0x1400
#define GLHCK_UNSIGNED_BYTE   0x1401
#define GLHCK_SHORT           0x1402
#define GLHCK_UNSIGNED_SHORT  0x1403
#define GLHCK_INT             0x1404
#define GLHCK_UNSIGNED_INT    0x1405
#define GLHCK_FLOAT           0x1406

/* internal data format */
#define GLHCK_PRECISION_VERTEX GLHCK_SHORT
#define GLHCK_PRECISION_COORD  GLHCK_SHORT
#define GLHCK_PRECISION_INDEX  GLHCK_UNSIGNED_SHORT
#define GLHCK_VERTEXDATA_COLOR 0

/* set C casting from internal data format spec */
#if GLHCK_PRECISION_VERTEX == GLHCK_BYTE
#  define GLHCK_CAST_VERTEX char
#elif GLHCK_PRECISION_VERTEX == GLHCK_SHORT
#  define GLHCK_CAST_VERTEX short
#else
#  define GLHCK_CAST_VERTEX kmScalar
#endif

#if GLHCK_PRECISION_COORD == GLHCK_BYTE
#  define GLHCK_CAST_COORD char
#elif GLHCK_PRECISION_COORD == GLHCK_SHORT
#  define GLHCK_CAST_COORD short
#else
#  define GLHCK_CAST_COORD kmScalar
#endif

#if GLHCK_PRECISION_INDEX == GLHCK_UNSIGNED_BYTE
#  define GLHCK_CAST_INDEX unsigned char
#elif GLHCK_PRECISION_INDEX == GLHCK_UNSIGNED_SHORT
#  define GLHCK_CAST_INDEX unsigned short
#else
#  define GLHCK_CAST_INDEX unsigned int
#endif

/* range of coordinates when not native precision */
#if GLHCK_PRECISION_COORD == GLHCK_BYTE
#  define GLHCK_RANGE_COORD 127
#elif GLHCK_PRECISION_COORD == GLHCK_SHORT
#  define GLHCK_RANGE_COORD 32767
#else
#  define GLHCK_RANGE_COORD 1
#endif

/* check if import index data format is same as internal
 * if true, we can just memcpy the import data without conversion. */
#define GLHCK_NATIVE_IMPORT_INDEXDATA 0
#if GLHCK_PRECISION_INDEX == GLHCK_UNSIGNED_INT
#  undef  GLHCK_NATIVE_IMPORT_INDEXDATA
#  define GLHCK_NATIVE_IMPORT_INDEXDATA 1
#endif

/* geometry type */
#define GLHCK_POINTS          0x0000
#define GLHCK_LINES           0x0001
#define GLHCK_LINE_LOOP       0x0002
#define GLHCK_LINE_STRIP      0x0003
#define GLHCK_TRIANGLES       0x0004
#define GLHCK_TRIANGLE_STRIP  0x0005

/* disable triangle stripping? */
#define GLHCK_TRISTRIP 1

typedef struct _glhckVertex3d
{
   GLHCK_CAST_VERTEX x, y, z;
} _glhckVertex3d;

typedef struct _glhckCoord2d {
   GLHCK_CAST_COORD x, y;
} _glhckCoord2d;

typedef struct __GLHCKvertexData {
   struct _glhckVertex3d vertex;
   struct _glhckVertex3d normal;
   struct _glhckCoord2d coord;
#if GLHCK_VERTEXDATA_COLOR
   struct glhckColor color;
#endif
} __GLHCKvertexData;

typedef struct __GLHCKobjectGeometry
{
   struct __GLHCKvertexData *vertexData;
   GLHCK_CAST_INDEX *indices;
   size_t indicesCount, vertexCount;
   kmVec3 bias, scale;
   unsigned int type;
} __GLHCKobjectGeometry;

typedef struct __GLHCKobjectView
{
   kmVec3 translation, rotation, scaling;
   kmMat4 matrix;
   kmAABB bounding;
   char update;
} __GLHCKobjectView;

typedef struct __GLHCKobjectMaterial
{
   struct _glhckTexture *texture;
   unsigned int flags;
} __GLHCKobjectMaterial;

typedef struct _glhckObject
{
   struct __GLHCKobjectGeometry geometry;
   struct __GLHCKobjectView view;
   struct __GLHCKobjectMaterial material;
   short refCounter;
} _glhckObject;

typedef struct __GLHCKcameraView
{
   kmScalar near, far, fov;
   kmVec3 translation, target, rotation, upVector;
   kmVec4 viewport;
   kmMat4 matrix, projection;
   char update;
} __GLHCKcameraView;

typedef struct _glhckCamera
{
   struct __GLHCKcameraView view;
   short refCounter;
} _glhckCamera;

/* library global data */
/* render api */
typedef void (*__GLHCKrenderAPIterminate)        (void);
typedef void (*__GLHCKrenderAPIviewport)         (int x, int y, int width, int height);
typedef void (*__GLHCKrenderAPIsetProjection)    (const kmMat4 *m);
typedef kmMat4 (*__GLHCKrenderAPIgetProjection)  (void);
typedef void (*__GLHCKrenderAPIsetClearColor)    (const float r, const float g, const float b, const float a);
typedef void (*__GLHCKrenderAPIclear)            (void);
typedef void (*__GLHCKrenderAPIobjectDraw)       (_glhckObject *object);

/* screen control */
typedef void (*__GLHCKrenderAPIgetPixels)        (int x, int y, int width, int height,
      int channels, unsigned char *data);

/* object generation */
typedef void (*__GLHCKrenderAPIgenerateTextures) (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteTextures)   (size_t count, unsigned int *objects);

/* textures */
typedef void (*__GLHCKrenderAPIbindTexture)      (unsigned int object);
typedef int  (*__GLHCKrenderAPIuploadTexture)    (_glhckTexture *texture, unsigned int flags);

typedef unsigned int (*__GLHCKrenderAPIcreateTexture) (const unsigned char *const buffer,
                                                       int width, int height, int channels,
                                                       unsigned int reuse_texture_ID,
                                                       unsigned int flags);

/* framebuffer objects */
typedef void (*__GLHCKrenderAPIgenerateFramebuffers) (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteFramebuffers)   (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIbindFramebuffer)      (unsigned int object);
typedef int (*__GLHCKrenderAPIlinkFramebufferWithTexture) (unsigned int object, unsigned int texture, unsigned int attachment);

typedef struct __GLHCKrenderAPI
{
   __GLHCKrenderAPIterminate        terminate;
   __GLHCKrenderAPIviewport         viewport;
   __GLHCKrenderAPIsetProjection    setProjection;
   __GLHCKrenderAPIgetProjection    getProjection;
   __GLHCKrenderAPIsetClearColor    setClearColor;
   __GLHCKrenderAPIclear            clear;
   __GLHCKrenderAPIobjectDraw       objectDraw;

   __GLHCKrenderAPIgetPixels        getPixels;

   __GLHCKrenderAPIgenerateTextures generateTextures;
   __GLHCKrenderAPIdeleteTextures   deleteTextures;
   __GLHCKrenderAPIbindTexture      bindTexture;
   __GLHCKrenderAPIuploadTexture    uploadTexture;
   __GLHCKrenderAPIcreateTexture    createTexture;

   __GLHCKrenderAPIgenerateFramebuffers   generateFramebuffers;
   __GLHCKrenderAPIdeleteFramebuffers     deleteFramebuffers;
   __GLHCKrenderAPIbindFramebuffer        bindFramebuffer;
   __GLHCKrenderAPIlinkFramebufferWithTexture linkFramebufferWithTexture;
} __GLHCKrenderAPI;

typedef struct __GLHCKtextureCache
{
   struct _glhckTexture *texture;
   struct __GLHCKtextureCache *next;
} __GLHCKtextureCache;

typedef struct __GLHCKtexture
{
   unsigned int bind;
   struct __GLHCKtextureCache *cache;
} __GLHCKtexture;

typedef struct __GLHCKcameraStack
{
   struct _glhckCamera *camera;
   struct __GLHCKcameraStack *next;
} __GLHCKcameraStack;

typedef struct __GLHCKcamera
{
   struct _glhckCamera *bind;
   struct __GLHCKcameraStack *stack;
} __GLHCKcamera;

typedef struct __GLHCKrender
{
   int width, height;
   const char *name;
   glhckRenderType type;
   struct __GLHCKrenderAPI api;
} __GLHCKrender;

typedef struct __GLHCKtrace
{
   const char *name;
   char active;
} __GLHCKtrace;

#ifndef NDEBUG
typedef struct __GLHCKalloc {
   const char *channel;
   void *ptr;
   size_t size;
   struct __GLHCKalloc *next;
} __GLHCKalloc;
#endif

typedef struct __GLHCKlibrary
{
   struct __GLHCKrender render;
   struct __GLHCKtexture texture;
   struct __GLHCKcamera camera;
   struct __GLHCKtrace *trace;
#ifndef NDEBUG
   struct __GLHCKalloc *alloc;
#endif
} __GLHCKlibrary;

/* define global object */
GLHCKGLOBAL struct __GLHCKlibrary _GLHCKlibrary;

typedef struct _glhckTexturePacker
{
   unsigned short debug_count, texture_index, texture_count;
   int longest_edge, total_area;
   struct tpNode *free_list;
   struct tpTexture *textures;
} _glhckTexturePacker;

/* helpful macros */
#define VEC2(v)   v?v->x:-1, v?v->y:-1
#define VEC2S     "vec2[%f, %f]"
#define VEC3(v)   v?v->x:-1, v?v->y:-1, v?v->z:-1
#define VEC3S     "vec3[%f, %f, %f]"
#define VEC4(v)   v?v->x:-1, v?v->y:-1, v?v->z:-1, v?v->w:-1
#define VEC4S     "vec3[%f, %f, %f, %f]"

/* if exists then free and set NULL */
#define IFDO(f, x) if (x) f(x); x = NULL;

/* tracking allocation macros */
#define _glhckMalloc(x)    __glhckMalloc(GLHCK_CHANNEL, x)
#define _glhckCalloc(x,y)  __glhcKCalloc(GLHCK_CHANNEL, x, y)
#define _glhckStrdup(x)    __glhckStrdup(GLHCK_CHANNEL, x)
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
char* __glhckStrdup(const char *channel, const char *s);
void* __glhckCopy(const char *channel, void *ptr, size_t nmemb);
void* _glhckRealloc(void *ptr, size_t omemb, size_t nmemb, size_t size);
void  _glhckFree(void *ptr);

#ifndef NDEBUG
/* tracking functions */
void _glhckTrackFake(void *ptr, size_t size);
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
int   _glhckTexturePackerPack(_glhckTexturePacker *tp, int *width, int *height, const int force_power_of_two, const int one_pixel_border);
int   _glhckTexturePackerGetLocation(_glhckTexturePacker *tp, int index, int *x, int *y, int *width, int *height);
_glhckTexturePacker*  _glhckTexturePackerNew(void);
void                  _glhckTexturePackerFree(_glhckTexturePacker *tp);

/* misc */
void _glhckDefaultProjection(void);

/* camera */
void _glhckCameraStackUpdate(int width, int height);

/* textures */
void _glhckTextureSetData(_glhckTexture *texture, unsigned char *data);

/* texture cache */
void _glhckTextureCacheRelease(void);

/* tracing && debug functions */
void _glhckTraceInit(int argc, char **argv);
void _glhckTrace(const char *channel, const char *function, const char *fmt, ...);
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *fmt, ...);

#endif /* _internal_h_ */

/* vim: set ts=8 sw=3 tw=0 :*/
