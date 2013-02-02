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
#define GLHCK_CHANNEL_GLHCK         "GLHCK"
#define GLHCK_CHANNEL_NETWORK       "NETWORK"
#define GLHCK_CHANNEL_IMPORT        "IMPORT"
#define GLHCK_CHANNEL_OBJECT        "OBJECT"
#define GLHCK_CHANNEL_TEXT          "TEXT"
#define GLHCK_CHANNEL_FRUSTUM       "FRUSTUM"
#define GLHCK_CHANNEL_CAMERA        "CAMERA"
#define GLHCK_CHANNEL_GEOMETRY      "GEOMETRY"
#define GLHCK_CHANNEL_VDATA         "VERTEXDATA"
#define GLHCK_CHANNEL_TEXTURE       "TEXTURE"
#define GLHCK_CHANNEL_ATLAS         "ATLAS"
#define GLHCK_CHANNEL_FRAMEBUFFER   "FRAMEBUFFER"
#define GLHCK_CHANNEL_HWBUFFER      "HWBUFFER"
#define GLHCK_CHANNEL_SHADER        "SHADER"
#define GLHCK_CHANNEL_ALLOC         "ALLOC"
#define GLHCK_CHANNEL_RENDER        "RENDER"
#define GLHCK_CHANNEL_TRACE         "TRACE"
#define GLHCK_CHANNEL_TRANSFORM     "TRANSFORM"
#define GLHCK_CHANNEL_DRAW          "DRAW"
#define GLHCK_CHANNEL_ALL           "ALL"
#define GLHCK_CHANNEL_SWITCH        "DEBUG"

/* build importers as seperate dynamic libraries? */
#define GLHCK_IMPORT_DYNAMIC 0

/* disable triangle stripping? */
#define GLHCK_TRISTRIP 1

/* floating point precision text? */
#define GLHCK_TEXT_FLOAT_PRECISION 1

/* opengl mapped constants */

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
   GLHCK_TEXTURE_IMPORT_TEXT  = 2,
} _glhckTextureFlags;

/* shader attrib locations */
typedef enum _glhckShaderAttrib {
   GLHCK_ATTRIB_VERTEX,
   GLHCK_ATTRIB_NORMAL,
   GLHCK_ATTRIB_TEXTURE,
   GLHCK_ATTRIB_COLOR,
   GLHCK_ATTRIB_LAST
} _glhckShaderAttrib;

/* glhck mappings for all shader uniforms (since 4.2 GL) */
typedef enum _glhckShaderVariableType {
   GLHCK_SHADER_FLOAT,
   GLHCK_SHADER_FLOAT_VEC2,
   GLHCK_SHADER_FLOAT_VEC3,
   GLHCK_SHADER_FLOAT_VEC4,
   GLHCK_SHADER_DOUBLE,
   GLHCK_SHADER_DOUBLE_VEC2,
   GLHCK_SHADER_DOUBLE_VEC3,
   GLHCK_SHADER_DOUBLE_VEC4,
   GLHCK_SHADER_INT,
   GLHCK_SHADER_INT_VEC2,
   GLHCK_SHADER_INT_VEC3,
   GLHCK_SHADER_INT_VEC4,
   GLHCK_SHADER_UNSIGNED_INT,
   GLHCK_SHADER_UNSIGNED_INT_VEC2,
   GLHCK_SHADER_UNSIGNED_INT_VEC3,
   GLHCK_SHADER_UNSIGNED_INT_VEC4,
   GLHCK_SHADER_BOOL,
   GLHCK_SHADER_BOOL_VEC2,
   GLHCK_SHADER_BOOL_VEC3,
   GLHCK_SHADER_BOOL_VEC4,
   GLHCK_SHADER_FLOAT_MAT2,
   GLHCK_SHADER_FLOAT_MAT3,
   GLHCK_SHADER_FLOAT_MAT4,
   GLHCK_SHADER_FLOAT_MAT2x3,
   GLHCK_SHADER_FLOAT_MAT2x4,
   GLHCK_SHADER_FLOAT_MAT3x2,
   GLHCK_SHADER_FLOAT_MAT3x4,
   GLHCK_SHADER_FLOAT_MAT4x2,
   GLHCK_SHADER_FLOAT_MAT4x3,
   GLHCK_SHADER_DOUBLE_MAT2,
   GLHCK_SHADER_DOUBLE_MAT3,
   GLHCK_SHADER_DOUBLE_MAT4,
   GLHCK_SHADER_DOUBLE_MAT2x3,
   GLHCK_SHADER_DOUBLE_MAT2x4,
   GLHCK_SHADER_DOUBLE_MAT3x2,
   GLHCK_SHADER_DOUBLE_MAT3x4,
   GLHCK_SHADER_DOUBLE_MAT4x2,
   GLHCK_SHADER_DOUBLE_MAT4x3,
   GLHCK_SHADER_SAMPLER_1D,
   GLHCK_SHADER_SAMPLER_2D,
   GLHCK_SHADER_SAMPLER_3D,
   GLHCK_SHADER_SAMPLER_CUBE,
   GLHCK_SHADER_SAMPLER_1D_SHADOW,
   GLHCK_SHADER_SAMPLER_2D_SHADOW,
   GLHCK_SHADER_SAMPLER_1D_ARRAY,
   GLHCK_SHADER_SAMPLER_2D_ARRAY,
   GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW,
   GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW,
   GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE,
   GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_SAMPLER_CUBE_SHADOW,
   GLHCK_SHADER_SAMPLER_BUFFER,
   GLHCK_SHADER_SAMPLER_2D_RECT,
   GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW,
   GLHCK_SHADER_INT_SAMPLER_1D,
   GLHCK_SHADER_INT_SAMPLER_2D,
   GLHCK_SHADER_INT_SAMPLER_3D,
   GLHCK_SHADER_INT_SAMPLER_CUBE,
   GLHCK_SHADER_INT_SAMPLER_1D_ARRAY,
   GLHCK_SHADER_INT_SAMPLER_2D_ARRAY,
   GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE,
   GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_INT_SAMPLER_BUFFER,
   GLHCK_SHADER_INT_SAMPLER_2D_RECT,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER,
   GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT,
   GLHCK_SHADER_IMAGE_1D,
   GLHCK_SHADER_IMAGE_2D,
   GLHCK_SHADER_IMAGE_3D,
   GLHCK_SHADER_IMAGE_2D_RECT,
   GLHCK_SHADER_IMAGE_CUBE,
   GLHCK_SHADER_IMAGE_BUFFER,
   GLHCK_SHADER_IMAGE_1D_ARRAY,
   GLHCK_SHADER_IMAGE_2D_ARRAY,
   GLHCK_SHADER_IMAGE_2D_MULTISAMPLE,
   GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_INT_IMAGE_1D,
   GLHCK_SHADER_INT_IMAGE_2D,
   GLHCK_SHADER_INT_IMAGE_3D,
   GLHCK_SHADER_INT_IMAGE_2D_RECT,
   GLHCK_SHADER_INT_IMAGE_CUBE,
   GLHCK_SHADER_INT_IMAGE_BUFFER,
   GLHCK_SHADER_INT_IMAGE_1D_ARRAY,
   GLHCK_SHADER_INT_IMAGE_2D_ARRAY,
   GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE,
   GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE,
   GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY,
   GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER,
} _glhckShaderVariableType;

/* texture container */
typedef struct _glhckTexture {
   unsigned int object;
   glhckTextureType targetType;
   glhckTextureFormat format, outFormat;
   unsigned int flags, importFlags;
   int width, height, depth, size;
   char *file;
   void *data;
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
   unsigned int blenda, blendb;
   struct glhckColorb color;
   struct _glhckTexture *texture;
   struct _glhckShader *shader;
} __GLHCKobjectMaterial;

/* object container */
typedef void (*__GLHCKobjectDraw) (const struct _glhckObject *object);
typedef struct _glhckObject {
   char *file;
   struct glhckGeometry *geometry;
   struct __GLHCKobjectView view;
   struct __GLHCKobjectMaterial material;
   __GLHCKobjectDraw drawFunc;
   size_t numChilds;
   size_t refCounter;
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
   int vertexCount;
} __GLHCKtextGeometry;

/* text texture container */
typedef struct __GLHCKtextTexture {
   unsigned int object, numRows;
   int size;
   glhckTextureFormat format;
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
   struct _glhckShader *shader;
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

/* framebuffer object */
typedef struct _glhckFramebuffer {
   unsigned int object;
   glhckFramebufferType targetType;
   size_t refCounter;
   struct _glhckFramebuffer *next;
} _glhckFramebuffer;

/* glhck hw buffer object type */
typedef struct _glhckHwBuffer {
   unsigned int object;
   glhckHwBufferType targetType;
   glhckHwBufferStoreType storeType;
   char created;
   size_t refCounter;
   struct _glhckHwBuffer *next;
} _glhckHwBuffer;

/* glhck shader attribute type */
typedef struct _glhckShaderAttribute {
   char *name;
   const char *typeName;
   unsigned int location;
   _glhckShaderVariableType type;
   int size;
   struct _glhckShaderAttribute *next;
} _glhckShaderAttribute;

/* glhck shader uniform type */
typedef struct _glhckShaderUniform {
   char *name;
   const char *typeName;
   unsigned int location;
   _glhckShaderVariableType type;
   int size;
   struct _glhckShaderUniform *next;
} _glhckShaderUniform;

/* glhck shader type */
typedef struct _glhckShader {
   unsigned int attrib[GLHCK_ATTRIB_NORMAL];
   unsigned int program;
   size_t refCounter;
   struct _glhckShaderAttribute *attributes;
   struct _glhckShaderUniform *uniforms;
   struct _glhckShader *next;
} _glhckShader;

/* render api */
typedef void (*__GLHCKrenderAPItime) (float time);
typedef void (*__GLHCKrenderAPIterminate) (void);
typedef void (*__GLHCKrenderAPIviewport) (int x, int y, int width, int height);
typedef void (*__GLHCKrenderAPIsetProjection) (const kmMat4 *m);
typedef const kmMat4* (*__GLHCKrenderAPIgetProjection) (void);
typedef void (*__GLHCKrenderAPIsetClearColor) (char r, char g, char b, char a);
typedef void (*__GLHCKrenderAPIclear) (void);

/* render */
typedef void (*__GLHCKrenderAPIobjectRender) (const _glhckObject *object);
typedef void (*__GLHCKrenderAPItextRender) (const _glhckText *text);
typedef void (*__GLHCKrenderAPIfrustumRender) (glhckFrustum *frustum);

/* screen control */
typedef void (*__GLHCKrenderAPIbufferGetPixels)
   (int x, int y, int width, int height, glhckTextureFormat format, void *data);

/* textures */
typedef void (*__GLHCKrenderAPItextureGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPItextureDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPItextureBind) (glhckTextureType type, unsigned int object);
typedef void (*__GLHCKrenderAPItextureFill)
   (glhckTextureType type, unsigned int texture, const void *data, int size, int x, int y, int z, int width,
    int height, int depth, glhckTextureFormat format);
typedef unsigned int (*__GLHCKrenderAPItextureCreate)
   (glhckTextureType type, const void *buffer, int size, int width, int height, int depth, glhckTextureFormat format,
    unsigned int reuseTextureObject, unsigned int flags);

/* framebuffers */
typedef void (*__GLHCKrenderAPIframebufferGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIframebufferDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPIframebufferBind) (glhckFramebufferType type, unsigned int object);
typedef int (*__GLHCKrenderAPIframebufferTexture)
   (glhckFramebufferType framebufferType, glhckTextureType textureType, unsigned int texture, glhckFramebufferAttachmentType type);

/* hardware buffer objects */
typedef void (*__GLHCKrenderAPIhwBufferGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIhwBufferDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPIhwBufferBind) (glhckHwBufferType type, unsigned int object);
typedef void (*__GLHCKrenderAPIhwBufferCreate) (glhckHwBufferType type, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage);
typedef void (*__GLHCKrenderAPIhwBufferFill) (glhckHwBufferType type, ptrdiff_t offset, ptrdiff_t size, const void *data);
typedef void (*__GLHCKrenderAPIhwBufferBindBase) (glhckHwBufferType type, unsigned int index, unsigned int object);
typedef void (*__GLHCKrenderAPIhwBufferBindRange)
   (glhckHwBufferType type, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size);
typedef void* (*__GLHCKrenderAPIhwBufferMap) (glhckHwBufferType type, glhckHwBufferAccessType access);
typedef void (*__GLHCKrenderAPIhwBufferUnmap) (glhckHwBufferType type);

/* shaders */
typedef void (*__GLHCKrenderAPIprogramBind) (unsigned int program);
typedef unsigned int (*__GLHCKrenderAPIprogramLink) (unsigned int vertexShader, unsigned int fragmentShader);
typedef void (*__GLHCKrenderAPIprogramDelete) (unsigned int program);
typedef unsigned int (*__GLHCKrenderAPIshaderCompile)
   (glhckShaderType type, const char *effectKey, const char *contentsFromMemory);
typedef void (*__GLHCKrenderAPIshaderDelete) (unsigned int shader);
typedef _glhckShaderAttribute* (*__GLHCKrenderAPIprogramAttributeList) (unsigned int program);
typedef _glhckShaderUniform* (*__GLHCKrenderAPIprogramUniformList) (unsigned int program);
typedef void (*__GLHCKrenderAPIprogramSetUniform) (unsigned int program, _glhckShaderUniform *uniform, int count, void *value);
typedef int (*__GLHCKrenderAPIshadersPath) (const char *pathPrefix, const char *pathSuffix);
typedef int (*__GLHCKrenderAPIshadersDirectiveToken) (const char* token, const char *directive);

/* parameters */
typedef void (*__GLHCKrenderAPIgetIntegerv) (unsigned int pname, int *params);

/* helper for filling the below struct */
#define GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(func) \
   __GLHCKrenderAPI##func func;

/* glhck render api */
typedef struct __GLHCKrenderAPI {
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(time);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(terminate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(viewport);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(setProjection);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(getProjection);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(setClearColor);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(clear);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(objectRender);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textRender);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(frustumRender);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(bufferGetPixels);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureCreate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureFill);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferTexture);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferCreate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferFill);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferBindBase);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferBindRange);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferMap);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(hwBufferUnmap);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programLink);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programAttributeList);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programUniformList);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programSetUniform);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shaderCompile);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shaderDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shadersPath);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shadersDirectiveToken);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(getIntegerv);
} __GLHCKrenderAPI;

#undef GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION

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
   unsigned int drawCount;
   glhckColorb clearColor;
   struct _glhckTexture *texture[GLHCK_TEXTURE_TYPE_LAST];
   struct _glhckCamera *camera;
   struct _glhckFramebuffer *framebuffer[GLHCK_FRAMEBUFFER_TYPE_LAST];
   struct _glhckHwBuffer *hwBuffer[GLHCK_HWBUFFER_TYPE_LAST];
   struct _glhckShader *shader;
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
   struct _glhckObject        *olist;
   struct _glhckCamera        *clist;
   struct _glhckAtlas         *alist;
   struct _glhckFramebuffer   *flist;
   struct _glhckTexture       *tlist;
   struct _glhckText          *tflist;
   struct _glhckHwBuffer      *hlist;
   struct _glhckShader        *slist;
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

/* macros for faster typing */
#define GLHCKR() (&_GLHCKlibrary.render)
#define GLHCKRA() (&_GLHCKlibrary.render.api)
#define GLHCKRD() (&_GLHCKlibrary.render.draw)

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
void* __glhckCopy(const char *channel, const void *ptr, size_t nmemb);
void* _glhckRealloc(void *ptr, size_t omemb, size_t nmemb, size_t size);
void _glhckFree(void *ptr);

#ifndef NDEBUG
/* tracking functions */
void _glhckTrackFake(void *ptr, size_t size);
void _glhckTrackTerminate(void);
#endif

/* util functions */
void _glhckRed(void);
void _glhckGreen(void);
void _glhckBlue(void);
void _glhckYellow(void);
void _glhckWhite(void);
void _glhckNormal(void);
void _glhckPuts(const char *buffer);
void _glhckPrintf(const char *fmt, ...);
size_t _glhckStrsplit(char ***dst, const char *str, const char *token);
void _glhckStrsplitClear(char ***dst);
char* _glhckStrupstr(const char *hay, const char *needle);
int _glhckStrupcmp(const char *hay, const char *needle);
int _glhckStrnupcmp(const char *hay, const char *needle, size_t len);

/* texture packing functions */
void _glhckTexturePackerSetCount(_glhckTexturePacker *tp, short textureCount);
short _glhckTexturePackerAdd(_glhckTexturePacker *tp, int width, int height);
int _glhckTexturePackerPack(_glhckTexturePacker *tp, int *width, int *height, const int force_power_of_two, const int one_pixel_border);
int _glhckTexturePackerGetLocation(const _glhckTexturePacker *tp, int index, int *x, int *y, int *width, int *height);
_glhckTexturePacker* _glhckTexturePackerNew(void);
void _glhckTexturePackerFree(_glhckTexturePacker *tp);

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
      glhckGeometry *geometry, int memb,
      glhckGeometryVertexType type,
      const glhckImportVertexData *vertices);
int _glhckGeometryInsertIndices(
      glhckGeometry *geometry, int memb,
      glhckGeometryIndexType type,
      const glhckImportIndexData *indices);

#endif /* __glhck_internal_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
