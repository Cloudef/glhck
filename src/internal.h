#ifndef __glhck_internal_h__
#define __glhck_internal_h__

#if defined(_glhck_c_)
#  define GLHCKGLOBAL
#else
#  define GLHCKGLOBAL extern
#endif

#include "glhck/glhck.h"
#include "helper/common.h" /* for macros, etc */
#include <float.h>  /* for float   */
#include <string.h> /* for strrchr */

/* FIXME: Most structures here and especially in glhck.h
 * need to be reordered and padded so they align better memory.
 * Especially the vertexdata structs. */

#if defined(_glhck_c_)
   char _glhckInitialized = 0;
#else
   GLHCKGLOBAL char _glhckInitialized;
#endif

/* tracing channels */
#define GLHCK_CHANNEL_GLHCK      "GLHCK"
#define GLHCK_CHANNEL_NETWORK    "NETWORK"
#define GLHCK_CHANNEL_IMPORT     "IMPORT"
#define GLHCK_CHANNEL_OBJECT     "OBJECT"
#define GLHCK_CHANNEL_TEXT       "TEXT"
#define GLHCK_CHANNEL_FRUSTUM    "FRUSTUM"
#define GLHCK_CHANNEL_CAMERA     "CAMERA"
#define GLHCK_CHANNEL_GEOMETRY   "GEOMETRY"
#define GLHCK_CHANNEL_VDATA      "VERTEXDATA"
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

/* build importers as seperate dynamic libraries? */
#define GLHCK_IMPORT_DYNAMIC 0

/* disable triangle stripping? */
#define GLHCK_TRISTRIP 1

/* floating point precision text? */
#define GLHCK_TEXT_FLOAT_PRECISION 1

/* opengl mapped constants */
#define GLHCK_COLOR_ATTACHMENT   0x8CE0
#define GLHCK_DEPTH_ATTACHMENT   0x8D00
#define GLHCK_STENCIL_ATTACHMENT 0x8D20

/* return variables used throughout library */
typedef enum _glhckReturnValue {
   RETURN_FAIL    =  0,
   RETURN_OK      =  1,
   RETURN_TRUE    =  1,
   RETURN_FALSE   =  !RETURN_TRUE
} _glhckReturnValue;

/* internal object flags */
typedef enum _glhckObjectFlags
{
   GLHCK_OBJECT_NONE = 0,
   GLHCK_OBJECT_ROOT = 1,
} _glhckObjectFlags;

/* internal texture flags */
typedef enum _glhckTextureFlags
{
   GLHCK_TEXTURE_IMPORT_NONE  = 0,
   GLHCK_TEXTURE_IMPORT_ALPHA = 1,
} _glhckTextureFlags;

/* texture container */
typedef struct _glhckTexture {
   char *file;
   unsigned int object, format, outFormat;
   unsigned int flags, importFlags;
   unsigned char *data;
   int width, height;
   size_t size;
   size_t refCounter;
   struct _glhckTexture *next;
} _glhckTexture;

/* texture packer container */
typedef struct _glhckTexturePacker {
   unsigned short debug_count, texture_index, texture_count;
   int longest_edge, total_area;
   struct tpNode *free_list;
   struct tpTexture *textures;
} _glhckTexturePacker;

/* representation of packed area */
typedef struct _glhckAtlasArea {
   int x1, y1, x2, y2, rotated;
} _glhckAtlasArea;

/* representation of packed texture */
typedef struct _glhckAtlasRect {
   struct _glhckTexture *texture;
   unsigned short index;
   struct _glhckAtlasArea packed;
   struct _glhckAtlasRect *next;
} _glhckAtlasRect;

/* atlas container */
typedef struct _glhckAtlas {
   struct _glhckAtlasRect *rect;
   struct _glhckTexture *texture;
   size_t refCounter;
   struct _glhckAtlas *next;
} _glhckAtlas;

/* rtt container */
typedef struct _glhckRtt {
   unsigned int object;
   struct _glhckTexture *texture;
   size_t refCounter;
   struct _glhckRtt *next;
} _glhckRtt;

/* object's view container */
typedef struct __GLHCKobjectView {
   kmVec3 translation, target, rotation, scaling;
   kmMat4 matrix;
   kmAABB bounding;
   kmAABB aabb; /* transformed bounding (axis aligned) */
   kmAABB obb;  /* transformed bounding (oriented) */
   char update;
} __GLHCKobjectView;

/* material container */
typedef struct __GLHCKobjectMaterial {
   unsigned int flags;
   struct glhckColorb color;
   struct _glhckTexture *texture;
} __GLHCKobjectMaterial;

/* object container */
typedef void (*__GLHCKobjectDraw) (const struct _glhckObject *object);
typedef struct _glhckObject {
   char *file;
   struct glhckGeometry *geometry;
   struct __GLHCKobjectView view;
   struct __GLHCKobjectMaterial material;
   __GLHCKobjectDraw drawFunc;
   size_t refCounter;
   size_t numChilds;
   unsigned char affectionFlags;
   unsigned char flags;
   struct _glhckObject *parent;
   struct _glhckObject **childs;
   struct _glhckObject *next;
} _glhckObject;

#define GLHCK_TEXT_MAX_ROWS   128
#define GLHCK_TEXT_VERT_COUNT (6*128)

/* row data of text texture */
typedef struct __GLHCKtextTextureRow {
   unsigned short x, y, h;
} __GLHCKtextTextureRow;

/* representation of text geometry */
typedef struct __GLHCKtextGeometry {
#if GLHCK_TEXT_FLOAT_PRECISION
   struct glhckVertexData2f vertexData[GLHCK_TEXT_VERT_COUNT];
#else
   struct glhckVertexData2s vertexData[GLHCK_TEXT_VERT_COUNT];
#endif
   size_t vertexCount;
} __GLHCKtextGeometry;

/* text texture container */
typedef struct __GLHCKtextTexture {
   unsigned int object, numRows, format;
   size_t size;
   struct __GLHCKtextGeometry geometry;
   struct __GLHCKtextTextureRow rows[GLHCK_TEXT_MAX_ROWS];
   struct __GLHCKtextTexture *next;
} __GLHCKtextTexture;

/* text container */
typedef struct _glhckText {
   int tw, th;
   float itw, ith;
   struct glhckColorb color;
   unsigned int textureRange;
   struct __GLHCKtextFont *fcache;
   struct __GLHCKtextTexture *tcache;
   struct _glhckText *next;
} _glhckText;

/* camera's view container */
typedef struct __GLHCKcameraView {
   glhckProjectionType projectionType;
   kmScalar near, far, fov;
   kmVec3 upVector;
   kmMat4 view, projection, mvp;
   glhckRect viewport;
   char update;
} __GLHCKcameraView;

/* camera container */
typedef struct _glhckCamera {
   struct _glhckObject *object;
   struct __GLHCKcameraView view;
   struct glhckFrustum frustum;
   size_t refCounter;
   struct _glhckCamera *next;
} _glhckCamera;

/* render api */
typedef void (*__GLHCKrenderAPIterminate)        (void);
typedef void (*__GLHCKrenderAPIviewport)         (int x, int y, int width, int height);
typedef void (*__GLHCKrenderAPIsetProjection)    (const kmMat4 *m);
typedef const kmMat4* (*__GLHCKrenderAPIgetProjection)  (void);
typedef void (*__GLHCKrenderAPIsetClearColor)    (char r, char g, char b, char a);
typedef void (*__GLHCKrenderAPIclear)            (void);
typedef void (*__GLHCKrenderAPIobjectDraw)       (const _glhckObject *object);
typedef void (*__GLHCKrenderAPItextDraw)         (const _glhckText *text);
typedef void (*__GLHCKrenderAPIfrustumDraw)      (glhckFrustum *frustum);

/* screen control */
typedef void (*__GLHCKrenderAPIgetPixels)        (int x, int y, int width, int height,
                                                  unsigned int format, unsigned char *data);

/* object generation */
typedef void (*__GLHCKrenderAPIgenerateTextures) (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteTextures)   (size_t count, unsigned int *objects);

/* textures */
typedef void (*__GLHCKrenderAPIbindTexture)      (unsigned int object);
typedef void (*__GLHCKrenderAPIfillTexture)      (unsigned int texture, const unsigned char *data, size_t size,
                                                  int x, int y, int width, int height, unsigned int format);

typedef unsigned int (*__GLHCKrenderAPIcreateTexture) (const unsigned char *buffer, size_t size,
                                                       int width, int height, unsigned int format,
                                                       unsigned int reuse_texture_ID, unsigned int flags);

/* framebuffer objects */
typedef void (*__GLHCKrenderAPIgenerateFramebuffers) (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIdeleteFramebuffers)   (size_t count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIbindFramebuffer)      (unsigned int object);
typedef int (*__GLHCKrenderAPIlinkFramebufferWithTexture) (unsigned int object, unsigned int texture,
                                                           unsigned int attachment);

/* parameters */
typedef void (*__GLHCKrenderAPIgetIntegerv) (unsigned int pname, int *params);

/* glhck render api */
typedef struct __GLHCKrenderAPI {
   __GLHCKrenderAPIterminate        terminate;
   __GLHCKrenderAPIviewport         viewport;
   __GLHCKrenderAPIsetProjection    setProjection;
   __GLHCKrenderAPIgetProjection    getProjection;
   __GLHCKrenderAPIsetClearColor    setClearColor;
   __GLHCKrenderAPIclear            clear;
   __GLHCKrenderAPIobjectDraw       objectDraw;
   __GLHCKrenderAPItextDraw         textDraw;
   __GLHCKrenderAPIfrustumDraw      frustumDraw;

   __GLHCKrenderAPIgetPixels        getPixels;

   __GLHCKrenderAPIgenerateTextures generateTextures;
   __GLHCKrenderAPIdeleteTextures   deleteTextures;
   __GLHCKrenderAPIbindTexture      bindTexture;
   __GLHCKrenderAPIcreateTexture    createTexture;
   __GLHCKrenderAPIfillTexture      fillTexture;

   __GLHCKrenderAPIgenerateFramebuffers   generateFramebuffers;
   __GLHCKrenderAPIdeleteFramebuffers     deleteFramebuffers;
   __GLHCKrenderAPIbindFramebuffer        bindFramebuffer;
   __GLHCKrenderAPIlinkFramebufferWithTexture linkFramebufferWithTexture;

   __GLHCKrenderAPIgetIntegerv getIntegerv;
} __GLHCKrenderAPI;

/* glhck texture state */
typedef struct __GLHCKtexture {
   unsigned int bind;
} __GLHCKtexture;

/* glhck camera state */
typedef struct __GLHCKcamera {
   struct _glhckCamera *bind;
} __GLHCKcamera;

#define GLHCK_QUEUE_ALLOC_STEP 15

/* glhck object queue */
typedef struct __GLHCKobjectQueue {
   unsigned int allocated, count;
   struct _glhckObject **queue;
} __GLHCKobjectQueue;

/* glhck texture queue */
typedef struct __GLHCKtextureQueue {
   unsigned int allocated, count;
   struct _glhckTexture **queue;
} __GLHCKtextureQueue;

/* glhck render draw state */
typedef struct __GLHCKrenderDraw {
   glhckColorb clearColor;
   unsigned int texture, drawCount;
   struct _glhckCamera  *camera;
   struct __GLHCKobjectQueue objects;
   struct __GLHCKtextureQueue textures;
} __GLHCKrenderDraw;

/* glhck render properties */
typedef struct __GLHCKrender {
   int width, height;
   const char *name;
   glhckRenderType type;
   glhckDriverType driver;
   unsigned int flags;
   struct __GLHCKrenderAPI api;
   struct __GLHCKrenderDraw draw;
   glhckGeometryIndexType globalIndexType;
   glhckGeometryVertexType globalVertexType;
} __GLHCKrender;

/* glhck world */
typedef struct __GLHCKworld {
   struct _glhckObject  *olist;
   struct _glhckCamera  *clist;
   struct _glhckAtlas   *alist;
   struct _glhckRtt     *rlist;
   struct _glhckTexture *tlist;
   struct _glhckText    *tflist;
} __GLHCKworld;

/* glhck trace channel */
typedef struct __GLHCKtraceChannel {
   const char *name;
   char active;
} __GLHCKtraceChannel;

/* glhck tracing */
typedef struct __GLHCKtrace {
   unsigned char level;
   struct __GLHCKtraceChannel *channel;
} __GLHCKtrace;

#ifndef NDEBUG
/* glhck allocation tracing */
typedef struct __GLHCKalloc {
   const char *channel;
   void *ptr;
   size_t size;
   struct __GLHCKalloc *next;
} __GLHCKalloc;
#endif

/* misc glhck options */
typedef struct __GLHCKmisc {
   char coloredLog;
} __GLHCKmisc;

/* glhck global state */
typedef struct __GLHCKlibrary {
   struct __GLHCKrender render;
   struct __GLHCKworld world;
   struct __GLHCKtrace trace;
#ifndef NDEBUG
   struct __GLHCKalloc *alloc;
#endif
   struct __GLHCKmisc misc;
} __GLHCKlibrary;

/* define global object */
#if defined(_glhck_c_)
   struct __GLHCKlibrary _GLHCKlibrary;
#else
   GLHCKGLOBAL struct __GLHCKlibrary _GLHCKlibrary;
#endif

/* api check macro
 * don't use with internal api
 * should be first call in GLHCKAPI public functions that access _GLHCKlibrary */
#define GLHCK_INITIALIZED() assert(_glhckInitialized && "call glhckInit() first");

/* tracking allocation macros */
#define _glhckMalloc(x)    __glhckMalloc(GLHCK_CHANNEL, x)
#define _glhckCalloc(x,y)  __glhckCalloc(GLHCK_CHANNEL, x, y)
#define _glhckStrdup(x)    __glhckStrdup(GLHCK_CHANNEL, x)
#define _glhckCopy(x,y)    __glhckCopy(GLHCK_CHANNEL, x, y)

/* tracing && debug macros */
#define THIS_FILE ((strrchr(__FILE__, '/') ?: __FILE__ - 1) + 1)
#define DBG_FMT         "\2%4d\1: \5%-20s \5%s"
#define TRACE_FMT       "\2%4d\1: \5%-20s \4%s\2()"
#define CALL_FMT(fmt)   "\2%4d\1: \5%-20s \4%s\2(\5"fmt"\2)"
#define RET_FMT(fmt)    "\2%4d\1: \5%-20s \4%s\2()\3 => \2(\5"fmt"\2)"
#define DEBUG(level, fmt, ...)   _glhckPassDebug(THIS_FILE, __LINE__, __func__, level, GLHCK_CHANNEL, fmt, ##__VA_ARGS__)
#define TRACE(level)             _glhckTrace(level, GLHCK_CHANNEL, __func__, TRACE_FMT,      __LINE__, THIS_FILE, __func__)
#define CALL(level, args, ...)   _glhckTrace(level, GLHCK_CHANNEL, __func__, CALL_FMT(args), __LINE__, THIS_FILE, __func__, ##__VA_ARGS__)
#define RET(level, args, ...)    _glhckTrace(level, GLHCK_CHANNEL, __func__, RET_FMT(args),  __LINE__, THIS_FILE, __func__, ##__VA_ARGS__)

/***
 * private api
 ***/

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
void   _glhckRed(void);
void   _glhckGreen(void);
void   _glhckBlue(void);
void   _glhckYellow(void);
void   _glhckWhite(void);
void   _glhckNormal(void);
void   _glhckPuts(const char *buffer);
void   _glhckPrintf(const char *fmt, ...);
size_t _glhckStrsplit(char ***dst, const char *str, const char *token);
void   _glhckStrsplitClear(char ***dst);
char*  _glhckStrupstr(const char *hay, const char *needle);
int    _glhckStrupcmp(const char *hay, const char *needle);
int    _glhckStrnupcmp(const char *hay, const char *needle, size_t len);

/* texture packing functions */
void  _glhckTexturePackerSetCount(_glhckTexturePacker *tp, short textureCount);
short _glhckTexturePackerAdd(_glhckTexturePacker *tp, int width, int height);
int   _glhckTexturePackerPack(_glhckTexturePacker *tp, int *width, int *height, const int force_power_of_two, const int one_pixel_border);
int   _glhckTexturePackerGetLocation(const _glhckTexturePacker *tp, int index, int *x, int *y, int *width, int *height);
_glhckTexturePacker*  _glhckTexturePackerNew(void);
void                  _glhckTexturePackerFree(_glhckTexturePacker *tp);

/* misc */
void _glhckDefaultProjection(int width, int height);

/* objects */
void _glhckObjectSetFile(_glhckObject *object, const char *file);
void _glhckObjectInsertToQueue(_glhckObject *object);
void _glhckObjectUpdateMatrix(_glhckObject *object);

/* camera */
void _glhckCameraWorldUpdate(int width, int height);

/* textures */
unsigned int _glhckNumChannels(unsigned int format);
int _glhckIsCompressedFormat(unsigned int format);
void _glhckTextureInsertToQueue(_glhckTexture *texture);

/* tracing && debug functions */
void _glhckTraceInit(int argc, const char **argv);
void _glhckTrace(int level, const char *channel, const char *function, const char *fmt, ...);
void _glhckPassDebug(const char *file, int line, const char *func, glhckDebugLevel level, const char *channel, const char *fmt, ...);

/* internal geometry vertexdata */
glhckGeometry *_glhckGeometryNew(void);
glhckGeometry *_glhckGeometryCopy(glhckGeometry *src);
void _glhckGeometryFree(glhckGeometry *geometry);
int _glhckGeometryInsertVertices(
      glhckGeometry *geometry, size_t memb,
      glhckGeometryVertexType type,
      const glhckImportVertexData *vertices);
int _glhckGeometryInsertIndices(
      glhckGeometry *geometry, size_t memb,
      glhckGeometryIndexType type,
      const glhckImportIndexData *indices);

#endif /* __glhck_internal_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
