#ifndef __glhck_internal_h__
#define __glhck_internal_h__

/* thread local storage attribute macro */
#if defined(_MSC_VER)
#  define _GLHCK_TLS __declspec(thread)
#  define _GLHCK_TLS_FOUND
#elif defined(__GNUC__)
#  define _GLHCK_TLS __thread
#  define _GLHCK_TLS_FOUND
#else
#  define _GLHCK_TLS
#  warning "No Thread-local storage! Multi-context glhck applications may have unexpected behaviour!"
#endif

#include "glhck/glhck.h"
#include "helper/common.h" /* for macros, etc */
#include <string.h> /* for strrchr */

/* check compile time options
 * these are supposed to make compile
 * without cmake bit more painless */

#ifndef GLHCK_USE_GLES1
#  define GLHCK_USE_GLES1 0
#endif
#ifndef GLHCK_USE_GLES2
#  define GLHCK_USE_GLES2 0
#endif
#ifndef GLHCK_IMPORT_ASSIMP
#  define GLHCK_IMPORT_ASSIMP 0
#endif
#ifndef GLHCK_IMPORT_OPENCTM
#  define GLHCK_IMPORT_OPENCTM 0
#endif
#ifndef GLHCK_IMPORT_MMD
#  define GLHCK_IMPORT_MMD 0
#endif
#ifndef GLHCK_IMPORT_BMP
#  define GLHCK_IMPORT_BMP 0
#endif
#ifndef GLHCK_IMPORT_PNG
#  define GLHCK_IMPORT_PNG 0
#endif
#ifndef GLHCK_IMPORT_TGA
#  define GLHCK_IMPORT_TGA 0
#endif
#ifndef GLHCK_IMPORT_JPEG
#  define GLHCK_IMPORT_JPEG 0
#endif
#ifndef GLHCK_TRISTRIP
#  define GLHCK_TRISTRIP 0
#endif
#ifndef GLHCK_IMPORT_DYNAMIC
#  define GLHCK_IMPORT_DYNAMIC 0
#endif
#ifndef GLHCK_TEXT_FLOAT_PRECISION
#  define GLHCK_TEXT_FLOAT_PRECISION 1
#endif
#ifndef GLHCK_DISABLE_TRACE
#  define GLHCK_DISABLE_TRACE 0
#endif
#ifndef USE_DOUBLE_PRECISION
#  define USE_DOUBLE_PRECISION 0
#endif

/* renderer checks */

#ifndef GLHCK_HAS_OPENGL_FIXED_PIPELINE
#  if !GLHCK_USE_GLES2
#     define GLHCK_HAS_OPENGL_FIXED_PIPELINE
#  endif
#endif

#ifndef GLHCK_HAS_OPENGL
#  if !GLHCK_USE_GLES1
#     define GLHCK_HAS_OPENGL 1
#  endif
#endif

/* tracing channels */
#define GLHCK_CHANNEL_GLHCK         "GLHCK"
#define GLHCK_CHANNEL_NETWORK       "NETWORK"
#define GLHCK_CHANNEL_IMPORT        "IMPORT"
#define GLHCK_CHANNEL_OBJECT        "OBJECT"
#define GLHCK_CHANNEL_BONE          "BONE"
#define GLHCK_CHANNEL_ANIMATION     "ANIMATION"
#define GLHCK_CHANNEL_ANIMATOR      "ANIMATOR"
#define GLHCK_CHANNEL_TEXT          "TEXT"
#define GLHCK_CHANNEL_FRUSTUM       "FRUSTUM"
#define GLHCK_CHANNEL_CAMERA        "CAMERA"
#define GLHCK_CHANNEL_GEOMETRY      "GEOMETRY"
#define GLHCK_CHANNEL_VDATA         "VERTEXDATA"
#define GLHCK_CHANNEL_MATERIAL      "MATERIAL"
#define GLHCK_CHANNEL_TEXTURE       "TEXTURE"
#define GLHCK_CHANNEL_ATLAS         "ATLAS"
#define GLHCK_CHANNEL_LIGHT         "LIGHT"
#define GLHCK_CHANNEL_RENDERBUFFER  "RENDERBUFFER"
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

/* return variables used throughout library */
typedef enum _glhckReturnValue {
   RETURN_FAIL    =  0,
   RETURN_OK      =  1,
   RETURN_TRUE    =  1,
   RETURN_FALSE   =  !RETURN_TRUE
} _glhckReturnValue;

/* internal texture flags */
typedef enum _glhckTextureFlags {
   GLHCK_TEXTURE_IMPORT_NONE  = 0,
   GLHCK_TEXTURE_IMPORT_ALPHA = 1,
   GLHCK_TEXTURE_IMPORT_TEXT  = 2,
} _glhckTextureFlags;

/* internal object flags */
typedef enum _glhckObjectFlags {
   GLHCK_OBJECT_NONE           = 0,
   GLHCK_OBJECT_ROOT           = 1,
   GLHCK_OBJECT_CULL           = 2,
   GLHCK_OBJECT_DEPTH          = 4,
   GLHCK_OBJECT_VERTEX_COLOR   = 8,
   GLHCK_OBJECT_DRAW_AABB      = 16,
   GLHCK_OBJECT_DRAW_OBB       = 32,
   GLHCK_OBJECT_DRAW_SKELETON  = 64,
   GLHCK_OBJECT_DRAW_WIREFRAME = 128,
} _glhckObjectFlags;

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

/* mark internal type reference counted.
 * takes variable names refCounter and next in struct.
 * all reference counted objects should be defined in __GLHCKworld world as well. */
#define REFERENCE_COUNTED(type) \
   struct type *next;           \
   unsigned int refCounter

/* texture container */
typedef struct _glhckTexture {
   struct glhckTextureParameters params;
   char *file;
   REFERENCE_COUNTED(_glhckTexture);
   kmVec3 internalScale;
   unsigned int object;
   unsigned int flags, importFlags;
   int width, height, depth;
   int internalWidth, internalHeight, internalDepth;
   char border;
   glhckTextureTarget target;
} _glhckTexture;

/* texture packer container */
typedef struct _glhckTexturePacker {
   struct tpNode *free_list;
   struct tpTexture *textures;
   int longest_edge, total_area;
   unsigned short debug_count, texture_index, texture_count;
} _glhckTexturePacker;

/* representation of packed area */
typedef struct _glhckAtlasArea {
   int x1, y1, x2, y2, rotated;
} _glhckAtlasArea;

/* representation of packed texture */
typedef struct _glhckAtlasRect {
   struct _glhckAtlasRect *next;
   struct _glhckTexture *texture;
   struct _glhckAtlasArea packed;
   unsigned short index;
} _glhckAtlasRect;

/* atlas container */
typedef struct _glhckAtlas {
   struct _glhckAtlasRect *rect;
   struct _glhckTexture *texture;
   REFERENCE_COUNTED(_glhckAtlas);
} _glhckAtlas;

/* material container */
typedef struct _glhckMaterial {
   kmVec2 textureScale;
   kmVec2 textureOffset;
   kmScalar textureRotation; /* in degrees */
   struct _glhckShader *shader;
   struct _glhckTexture *texture;
   struct glhckColorb ambient;
   struct glhckColorb diffuse;
   struct glhckColorb emissive;
   struct glhckColorb specular;
   REFERENCE_COUNTED(_glhckMaterial);
   float shininess;
   glhckBlendingMode blenda, blendb;
   unsigned int flags;
} _glhckMaterial;

/* object's view container */
typedef struct __GLHCKobjectView {
   kmMat4 matrix; /* model matrix */
   kmAABB bounding; /* bounding box (non-transformed) */
   kmAABB aabb; /* transformed bounding (axis aligned) */
   kmAABB obb; /* transformed bounding (oriented) */
   kmVec3 translation, target, rotation, scaling;
   char update; /* does the model matrix need recalculation? */
   char wasFlipped; /* was drawn last time using flipped projection? */
} __GLHCKobjectView;

/* object container */
typedef void (*__GLHCKobjectDraw) (const struct _glhckObject *object);
typedef struct _glhckObject {
   struct __GLHCKobjectView view;
   struct _glhckMaterial *material;
   struct _glhckObject *parent;
   struct _glhckObject **childs;
   struct _glhckBone **bones;
   struct _glhckAnimation **animations;
   struct glhckGeometry *geometry;
   struct glhckGeometry *bindGeometry; /* used on CPU transformation path, has the original geometry copied */
   __GLHCKobjectDraw drawFunc;
   char *file, *name;
   REFERENCE_COUNTED(_glhckObject);
   float transformedGeometryTime; /* stores skeletal animated transformation time (to avoid retransform) */
   unsigned int numChilds;
   unsigned int numBones;
   unsigned int numAnimations;
   unsigned char affectionFlags; /* flags how parent affects us */
   unsigned char flags;
} _glhckObject;

/* bone container */
typedef struct _glhckBone {
   kmMat4 offsetMatrix; /* bone -> mesh space transformation */
   kmMat4 transformationMatrix; /* local transformation of bone  */
   kmMat4 transformedMatrix; /* final bone space transformation */
   struct glhckVertexWeight *weights;
   struct _glhckBone *parent;
   char *name;
   REFERENCE_COUNTED(_glhckBone);
   unsigned int numWeights;
} _glhckBone;

/* animation node container
 * contains keys for each bone (translation, rotation, scaling) */
typedef struct _glhckAnimationNode {
   struct glhckAnimationQuaternionKey *rotations;
   struct glhckAnimationVectorKey *translations;
   struct glhckAnimationVectorKey *scalings;
   char *boneName;
   REFERENCE_COUNTED(_glhckAnimationNode);
   unsigned int numTranslations;
   unsigned int numRotations;
   unsigned int numScalings;
} _glhckAnimationNode;

/* animation container
 * stores information for single animation */
typedef struct _glhckAnimation {
   struct _glhckAnimationNode **nodes;
   char *name;
   REFERENCE_COUNTED(_glhckAnimation);
   float ticksPerSecond;
   float duration;
   unsigned int numNodes;
} _glhckAnimation;

/* stores history of glhckAnimatioNode */
typedef struct _glhckAnimatorState {
   struct _glhckBone *bone;
   unsigned int translationFrame;
   unsigned int rotationFrame;
   unsigned int scalingFrame;
} _glhckAnimatorState;

/* animator object
 * animates glhckObject, using glhckBones and glhckAnimation */
typedef struct _glhckAnimator {
   struct _glhckAnimation *animation;
   struct _glhckAnimatorState *previousNodes;
   struct _glhckBone **bones;
   REFERENCE_COUNTED(_glhckAnimator);
   float lastTime;
   unsigned int numBones;
   char dirty;
} _glhckAnimator;

/* row data of text texture */
typedef struct __GLHCKtextTextureRow {
   unsigned short x, y, h;
} __GLHCKtextTextureRow;

/* representation of text geometry */
typedef struct __GLHCKtextGeometry {
#if GLHCK_TEXT_FLOAT_PRECISION
   struct glhckVertexData2f *vertexData;
#else
   struct glhckVertexData2s *vertexData;
#endif
   int vertexCount, allocatedCount;
} __GLHCKtextGeometry;

/* text texture container */
typedef struct __GLHCKtextTexture {
   struct __GLHCKtextGeometry geometry;
   struct __GLHCKtextTextureRow *rows;
   struct __GLHCKtextTexture *next;
   glhckTexture *texture;
   float internalWidth, internalHeight;
   int rowsCount, allocatedCount;
} __GLHCKtextTexture;

/* text container */
typedef struct _glhckText {
   struct _glhckShader *shader;
   struct __GLHCKtextFont *fontCache;
   struct __GLHCKtextTexture *textureCache;
   REFERENCE_COUNTED(_glhckText);
   unsigned int textureRange;
   int cacheWidth, cacheHeight;
   struct glhckColorb color;
} _glhckText;

/* camera's view container */
typedef struct __GLHCKcameraView {
   kmMat4 view, projection, viewProj;
   glhckRect viewport;
   kmVec3 upVector;
   kmScalar near, far, fov;
   glhckProjectionType projectionType;
   char update, updateViewport;
} __GLHCKcameraView;

/* camera container */
typedef struct _glhckCamera {
   struct __GLHCKcameraView view;
   struct glhckFrustum frustum;
   struct _glhckObject *object;
   REFERENCE_COUNTED(_glhckCamera);
} _glhckCamera;

/* \brief light's internal camera && object state */
typedef struct __GLHCKlightState {
   __GLHCKcameraView cameraView;
   __GLHCKobjectView objectView;
} __GLHCKlightCameraOldState;

/* \brief light's options */
typedef struct __GLHCKlightOptions {
   kmVec3 atten;
   kmVec2 cutout;
   kmScalar pointLightFactor;
} __GLHCKlightOptions;

/* light container */
typedef struct _glhckLight {
   struct __GLHCKlightState old;
   struct __GLHCKlightState current;
   struct __GLHCKlightOptions options;
   struct _glhckObject *object;
   REFERENCE_COUNTED(_glhckLight);
} _glhckLight;

/* renderbuffer object */
typedef struct _glhckRenderbuffer {
   REFERENCE_COUNTED(_glhckRenderbuffer);
   unsigned int object;
   int width, height;
   glhckTextureFormat format;
} _glhckRenderbuffer;

/* framebuffer object */
typedef struct _glhckFramebuffer {
   glhckRect rect;
   REFERENCE_COUNTED(_glhckFramebuffer);
   unsigned int object;
   glhckFramebufferTarget target;
} _glhckFramebuffer;

/* glhck hw buffer uniform location type */
typedef struct _glhckHwBufferShaderUniform {
   struct _glhckHwBufferShaderUniform *next;
   char *name;
   const char *typeName;
   int offset, size;
   _glhckShaderVariableType type;
} _glhckHwBufferShaderUniform;

/* glhck hw buffer object type */
typedef struct _glhckHwBuffer {
   struct _glhckHwBufferShaderUniform *uniforms;
   char *name;
   REFERENCE_COUNTED(_glhckHwBuffer);
   unsigned int object;
   int size;
   glhckHwBufferTarget target;
   glhckHwBufferStoreType storeType;
   char created;
} _glhckHwBuffer;

/* glhck shader attribute type */
typedef struct _glhckShaderAttribute {
   struct _glhckShaderAttribute *next;
   char *name;
   const char *typeName;
   unsigned int location;
   int size;
   _glhckShaderVariableType type;
} _glhckShaderAttribute;

/* glhck shader uniform type */
typedef struct _glhckShaderUniform {
   struct _glhckShaderUniform *next;
   char *name;
   const char *typeName;
   unsigned int location;
   int size;
   _glhckShaderVariableType type;
} _glhckShaderUniform;

/* glhck shader type */
typedef struct _glhckShader {
   struct _glhckShaderAttribute *attributes;
   struct _glhckShaderUniform *uniforms;
   REFERENCE_COUNTED(_glhckShader);
   unsigned int program;
} _glhckShader;

#undef REFERENCE_COUNTED

/* render api */
typedef void (*__GLHCKrenderAPIterminate) (void);

typedef void (*__GLHCKrenderAPItime) (float time);
typedef void (*__GLHCKrenderAPIclearColor) (const glhckColorb *color);
typedef void (*__GLHCKrenderAPIclear) (unsigned int bufferBits);

/* view */
typedef void (*__GLHCKrenderAPIviewport) (int x, int y, int width, int height);
typedef void (*__GLHCKrenderAPIsetOrthographic) (const kmMat4 *m);
typedef void (*__GLHCKrenderAPIsetProjection) (const kmMat4 *m);
typedef void (*__GLHCKrenderAPIsetView) (const kmMat4 *m);

/* render */
typedef void (*__GLHCKrenderAPIobjectRender) (const _glhckObject *object);
typedef void (*__GLHCKrenderAPItextRender) (const _glhckText *text);
typedef void (*__GLHCKrenderAPIfrustumRender) (glhckFrustum *frustum);

/* screen control */
typedef void (*__GLHCKrenderAPIbufferGetPixels) (int x, int y, int width, int height, glhckTextureFormat format, glhckDataType type, void *data);

/* textures */
typedef void (*__GLHCKrenderAPItextureGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPItextureDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPItextureBind) (glhckTextureTarget target, unsigned int object);
typedef void (*__GLHCKrenderAPItextureActive) (unsigned int index);
typedef void (*__GLHCKrenderAPItextureFill) (glhckTextureTarget target, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, int datatype, int size, const void *data);
typedef void (*__GLHCKrenderAPItextureImage) (glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, int datatype, int size, const void *data);
typedef void (*__GLHCKrenderAPItextureParameter) (glhckTextureTarget target, const glhckTextureParameters *params);
typedef void (*__GLHCKrenderAPItextureGenerateMipmap) (glhckTextureTarget target);

/* lights */
typedef void (*__GLHCKrenderAPIlightBind) (glhckLight *light);

/* renderbuffers */
typedef void (*__GLHCKrenderAPIrenderbufferGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIrenderbufferDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPIrenderbufferBind) (unsigned int object);
typedef void (*__GLHCKrenderAPIrenderbufferStorage) (int width, int height, glhckTextureFormat format);

/* framebuffers */
typedef void (*__GLHCKrenderAPIframebufferGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIframebufferDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPIframebufferBind) (glhckFramebufferTarget target, unsigned int object);
typedef int (*__GLHCKrenderAPIframebufferTexture) (glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, unsigned int texture, glhckFramebufferAttachmentType type);
typedef int (*__GLHCKrenderAPIframebufferRenderbuffer) (glhckFramebufferTarget framebufferTarget, unsigned int buffer, glhckFramebufferAttachmentType type);

/* hardware buffer objects */
typedef void (*__GLHCKrenderAPIhwBufferGenerate) (int count, unsigned int *objects);
typedef void (*__GLHCKrenderAPIhwBufferDelete) (int count, const unsigned int *objects);
typedef void (*__GLHCKrenderAPIhwBufferBind) (glhckHwBufferTarget target, unsigned int object);
typedef void (*__GLHCKrenderAPIhwBufferCreate) (glhckHwBufferTarget target, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage);
typedef void (*__GLHCKrenderAPIhwBufferFill) (glhckHwBufferTarget target, ptrdiff_t offset, ptrdiff_t size, const void *data);
typedef void (*__GLHCKrenderAPIhwBufferBindBase) (glhckHwBufferTarget target, unsigned int index, unsigned int object);
typedef void (*__GLHCKrenderAPIhwBufferBindRange) (glhckHwBufferTarget target, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size);
typedef void* (*__GLHCKrenderAPIhwBufferMap) (glhckHwBufferTarget target, glhckHwBufferAccessType access);
typedef void (*__GLHCKrenderAPIhwBufferUnmap) (glhckHwBufferTarget target);

/* shaders */
typedef void (*__GLHCKrenderAPIprogramBind) (unsigned int program);
typedef unsigned int (*__GLHCKrenderAPIprogramLink) (unsigned int vertexShader, unsigned int fragmentShader);
typedef void (*__GLHCKrenderAPIprogramDelete) (unsigned int program);
typedef unsigned int (*__GLHCKrenderAPIshaderCompile) (glhckShaderType type, const char *effectKey, const char *contentsFromMemory);
typedef void (*__GLHCKrenderAPIshaderDelete) (unsigned int shader);
typedef _glhckHwBufferShaderUniform* (*__GLHCKrenderAPIprogramUniformBufferList) (unsigned int program, const char *uboName, int *size);
typedef _glhckShaderAttribute* (*__GLHCKrenderAPIprogramAttributeList) (unsigned int program);
typedef _glhckShaderUniform* (*__GLHCKrenderAPIprogramUniformList) (unsigned int program);
typedef void (*__GLHCKrenderAPIprogramUniform) (unsigned int program, _glhckShaderUniform *uniform, int count, const void *value);
typedef unsigned int (*__GLHCKrenderAPIprogramAttachUniformBuffer) (unsigned int program, const char *name, unsigned int location);
typedef int (*__GLHCKrenderAPIshadersPath) (const char *pathPrefix, const char *pathSuffix);
typedef int (*__GLHCKrenderAPIshadersDirectiveToken) (const char* token, const char *directive);

/* helper for filling the below struct */
#define GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(func) \
   __GLHCKrenderAPI##func func;

/* glhck render api */
typedef struct __GLHCKrenderAPI {
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(terminate);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(time);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(clearColor);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(clear);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(viewport);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(setOrthographic);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(setProjection);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(setView);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(objectRender);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textRender);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(frustumRender);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(bufferGetPixels);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureActive);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureFill);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureImage);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureParameter);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(textureGenerateMipmap);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(lightBind);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(renderbufferGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(renderbufferDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(renderbufferBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(renderbufferStorage);

   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferGenerate);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferBind);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferTexture);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(framebufferRenderbuffer);

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
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programUniformBufferList);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programAttributeList);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programUniformList);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programUniform);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(programAttachUniformBuffer);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shaderCompile);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shaderDelete);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shadersPath);
   GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION(shadersDirectiveToken);
} __GLHCKrenderAPI;

#undef GLHCK_INCLUDE_INTERNAL_RENDER_API_FUNCTION

#define GLHCK_QUEUE_ALLOC_STEP 15

/* context object queue */
typedef struct __GLHCKobjectQueue {
   struct _glhckObject **queue;
   unsigned int allocated, count;
} __GLHCKobjectQueue;

/* context texture queue */
typedef struct __GLHCKtextureQueue {
   struct _glhckTexture **queue;
   unsigned int allocated, count;
} __GLHCKtextureQueue;

/* view matrices */
typedef struct __GLHCKrenderView {
   kmMat4 projection, view, orthographic;
   char flippedProjection;
} __GLHCKrenderView;

#define GLHCK_MAX_ACTIVE_TEXTURE 8

/* context render draw state */
typedef struct __GLHCKrenderDraw {
   struct __GLHCKobjectQueue objects;
   struct __GLHCKtextureQueue textures;
   struct __GLHCKrenderView view;
   struct _glhckTexture *texture[GLHCK_MAX_ACTIVE_TEXTURE][GLHCK_TEXTURE_TYPE_LAST];
   struct _glhckFramebuffer *framebuffer[GLHCK_FRAMEBUFFER_TYPE_LAST];
   struct _glhckHwBuffer *hwBuffer[GLHCK_HWBUFFER_TYPE_LAST];
   struct _glhckLight *light;
   struct _glhckShader *shader;
   struct _glhckCamera *camera;
   unsigned int activeTexture;
   unsigned int drawCount;
} __GLHCKrenderDraw;

/* context render pass state
 * the pass state overrides object state */
typedef struct __GLHCKrenderPass {
   struct _glhckShader *shader;
   glhckRect viewport;
   unsigned int blenda, blendb;
   unsigned int flags;
   glhckCullFaceType cullFace;
   glhckFaceOrientation frontFace;
   glhckColorb clearColor;
} __GLHCKrenderPass;

/* render state */
typedef struct __GLHCKrenderState {
   struct __GLHCKrenderView view;
   struct __GLHCKrenderPass pass;
   struct __GLHCKrenderState *next;
   int width, height;
} __GLHCKrenderState;

/* context render properties */
typedef struct __GLHCKrender {
   struct __GLHCKrenderState *stack;
   struct __GLHCKrenderAPI api;
   struct __GLHCKrenderPass pass;
   struct __GLHCKrenderDraw draw;
   struct glhckRenderFeatures features;
   void *renderPointer; /* this pointer is reserved for renderer */
   const char *name;
   int width, height;
   glhckRenderType type;
   glhckDriverType driver;
} __GLHCKrender;

/* context's world */
typedef struct __GLHCKworld {
   struct _glhckObject           *object;
   struct _glhckBone             *bone;
   struct _glhckAnimationNode    *animationNode;
   struct _glhckAnimation        *animation;
   struct _glhckAnimator         *animator;
   struct _glhckCamera           *camera;
   struct _glhckLight            *light;
   struct _glhckAtlas            *atlas;
   struct _glhckRenderbuffer     *renderbuffer;
   struct _glhckFramebuffer      *framebuffer;
   struct _glhckMaterial         *material;
   struct _glhckTexture          *texture;
   struct _glhckText             *text;
   struct _glhckHwBuffer         *hwbuffer;
   struct _glhckShader           *shader;
} __GLHCKworld;

/* context trace channel */
typedef struct __GLHCKtraceChannel {
   const char *name;
   char active;
} __GLHCKtraceChannel;

/* context tracing */
typedef struct __GLHCKtrace {
   struct __GLHCKtraceChannel *channel;
   glhckDebugHookFunc debugHook;
   unsigned char level;
} __GLHCKtrace;

#ifndef NDEBUG
/* context allocation tracing */
typedef struct __GLHCKalloc {
   size_t size;
   struct __GLHCKalloc *next;
   void *ptr;
   const char *channel;
} __GLHCKalloc;
#endif

/* misc context options */
typedef struct __GLHCKmisc {
   glhckGeometryIndexType globalIndexType;
   glhckGeometryVertexType globalVertexType;
   char coloredLog;
} __GLHCKmisc;

/* glhck context state */
typedef struct __GLHCKcontext {
   struct __GLHCKrender render;
   struct __GLHCKworld world;
   struct __GLHCKtrace trace;
   struct __GLHCKmisc misc;
#ifndef NDEBUG
   struct __GLHCKalloc *alloc;
#endif
} __GLHCKcontext;

/* thread-local storage glhck context, allows in theory to use opengl on different threads.
 * (as long as each thread has own gl context as well) */
extern _GLHCK_TLS struct __GLHCKcontext *_glhckContext;

/* internal glhck flip matrix
 * every model matrix is multiplied with this when glhckRenderFlip(1) is used */
extern const kmMat4 _glhckFlipMatrix;

/* api check macro
 * don't use with internal api
 * should be first call in GLHCKAPI public functions that access _GLHCKlibrary */
#define GLHCK_INITIALIZED() assert(_glhckContext != NULL && "call glhckInit() first");

/* macros for faster typing */
#define GLHCKR() (&glhckContextGet()->render)
#define GLHCKRA() (&glhckContextGet()->render.api)
#define GLHCKRD() (&glhckContextGet()->render.draw)
#define GLHCKRP() (&glhckContextGet()->render.pass)
#define GLHCKRF() (&glhckContextGet()->render.features)
#define GLHCKW() (&glhckContextGet()->world)
#define GLHCKT() (&glhckContextGet()->trace)
#define GLHCKM() (&glhckContextGet()->misc)

/* tracking allocation macros */
#define _glhckMalloc(x)       __glhckMalloc(GLHCK_CHANNEL, x)
#define _glhckCalloc(x,y)     __glhckCalloc(GLHCK_CHANNEL, x, y)
#define _glhckStrdup(x)       __glhckStrdup(GLHCK_CHANNEL, x)
#define _glhckCopy(x,y)       __glhckCopy(GLHCK_CHANNEL, x, y)
#define _glhckTrackSteal(x)   __glhckTrackSteal(GLHCK_CHANNEL, x)

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
void __glhckTrackSteal(const char *channel, void *ptr);

#ifndef NDEBUG
/* tracking functions */
void _glhckTrackFake(void *ptr, size_t size);
void _glhckTrackTerminate(void);
#else
#define _glhckTrackFake(x,y) ;
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
void _glhckTexturePackerCount(_glhckTexturePacker *tp, short textureCount);
short _glhckTexturePackerAdd(_glhckTexturePacker *tp, int width, int height);
int _glhckTexturePackerPack(_glhckTexturePacker *tp, int *width, int *height, const int forcePowerOfTwo, const int onePixelBorder);
int _glhckTexturePackerGetLocation(const _glhckTexturePacker *tp, int index, int *x, int *y, int *width, int *height);
_glhckTexturePacker* _glhckTexturePackerNew(void);
void _glhckTexturePackerFree(_glhckTexturePacker *tp);

/* render functions */
int _glhckRenderInitialized(void);
void _glhckRenderDefaultProjection(int width, int height);
void _glhckRenderCheckApi(void);

/* objects */
void _glhckObjectFile(_glhckObject *object, const char *file);
void _glhckObjectInsertToQueue(_glhckObject *object);
void _glhckObjectUpdateBoxes(glhckObject *object);

/* bones */
void _glhckBoneTransformObject(glhckObject *object, int updateBones);

/* camera */
void _glhckCameraWorldUpdate(int width, int height);

/* textures */
int _glhckHasAlpha(glhckTextureFormat format);
unsigned int _glhckNumChannels(unsigned int format);
int _glhckSizeForTexture(glhckTextureTarget target, int width, int height, int depth, glhckTextureFormat format, glhckDataType type);
int _glhckIsCompressedFormat(unsigned int format);
void _glhckNextPow2(int width, int height, int depth, int *outWidth, int *outHeight, int *outDepth, int limitToSize);
void _glhckTextureInsertToQueue(_glhckTexture *texture);

/* tracing && debug functions */
void _glhckTraceInit(int argc, const char **argv);
void _glhckTraceTerminate(void);
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
