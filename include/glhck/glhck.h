#ifndef __glhck_h__
#define __glhck_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* for default typedefs */
#include <kazmath/kazmath.h> /* glhck needs kazmath */
#include <kazmath/vec4.h> /* is not include by kazmath.h */

/* System specific defines, stolen from glfw.
 * Thanks for the nice library guys. */

#if !defined(_WIN32) && (defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))
#  define _WIN32
#endif /* _WIN32 */

/* In order for extension support to be portable, we need to define an
 * OpenGL function call method. We use the keyword APIENTRY, which is
 * defined for Win32. (Note: Windows also needs this for <GL/gl.h>)
 */
#ifndef APIENTRY
#  ifdef _WIN32
#     define APIENTRY __stdcall
#  else
#     define APIENTRY
#  endif
#endif /* APIENTRY */

/* The following three defines are here solely to make some Windows-based
 * <GL/gl.h> files happy. Theoretically we could include <windows.h>, but
 * it has the major drawback of severely polluting our namespace.
 */

/* Under Windows, we need WINGDIAPI defined */
#if !defined(WINGDIAPI) && defined(_WIN32)
#  if defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)
/* Microsoft Visual C++, Borland C++ Builder and Pelles C */
#     define WINGDIAPI __declspec(dllimport)
#  elif defined(__LCC__)
/* LCC-Win32 */
#     define WINGDIAPI __stdcall
#  else
/* Others (e.g. MinGW, Cygwin) */
#     define WINGDIAPI extern
#  endif
#     define GLHCK_WINGDIAPI_DEFINED
#endif /* WINGDIAPI */

/* Some <GL/glu.h> files also need CALLBACK defined */
#if !defined(CALLBACK) && defined(_WIN32)
#  if defined(_MSC_VER)
/* Microsoft Visual C++ */
#     if (defined(_M_MRX000) || defined(_M_IX86) || defined(_M_ALPHA) || defined(_M_PPC)) && !defined(MIDL_PASS)
#        define CALLBACK __stdcall
#     else
#     define CALLBACK
#  endif
#  else
/* Other Windows compilers */
#     define CALLBACK __stdcall
#  endif
#     define GLHCK_CALLBACK_DEFINED
#endif /* CALLBACK */

/* Microsoft Visual C++, Borland C++ and Pelles C <GL*glu.h> needs wchar_t */
#if defined(_WIN32) && (defined(_MSC_VER) || defined(__BORLANDC__) || defined(__POCC__)) && !defined(_WCHAR_T_DEFINED)
   typedef unsigned short wchar_t;
#  define _WCHAR_T_DEFINED
#endif /* _WCHAR_T_DEFINED */

/* GLHCK specific defines */

#if defined(GLHCK_DLL) && defined(_GLHCK_BUILD_DLL)
#  error "You must not have both GLHCK_DLL and _GLHCK_BUILD_DLL defined"
#endif

#if defined(_WIN32) && defined(_GLHCK_BUILD_DLL)
/* We are building a Win32 DLL */
#  define GLHCKAPI __declspec(dllexport)
#elif defined(_WIN32) && defined(GLHCK_DLL)
/* We are calling a Win32 DLL */
#  if defined(__LCC__)
#     define GLHCKAPI extern
#  else
#     define GLHCKAPI __declspec(dllimport)
#  endif
#else
/* We are either building/calling a static lib or we are non-win32 */
#  define GLHCKAPI
#endif

/***
 * public api
 ***/

/* helper for python script */
#define PYGLMAP typedef

/* clear bits */
PYGLMAP enum glhckClearBufferBits {
   GLHCK_COLOR_BUFFER_BIT = 1<<0,
   GLHCK_DEPTH_BUFFER_BIT = 1<<1,
} glhckClearBufferBits;

/* cull side face */
PYGLMAP enum glhckCullFaceType {
   GLHCK_BACK,
   GLHCK_FRONT,
} glhckCullFaceType;

/* face orientations */
PYGLMAP enum glhckFaceOrientation {
   GLHCK_CW,
   GLHCK_CCW,
} glhckFaceOrientation;

/* geometry type */
PYGLMAP enum glhckGeometryType {
   GLHCK_POINTS,
   GLHCK_LINES,
   GLHCK_LINE_LOOP,
   GLHCK_LINE_STRIP,
   GLHCK_TRIANGLES,
   GLHCK_TRIANGLE_STRIP,
} glhckGeometryType;

/* framebuffer types */
PYGLMAP enum glhckFramebufferTarget {
   GLHCK_FRAMEBUFFER,
   GLHCK_FRAMEBUFFER_READ,
   GLHCK_FRAMEBUFFER_DRAW,
   GLHCK_FRAMEBUFFER_TYPE_LAST,
} glhckFramebufferTarget;

/* framebuffer attachment types */
PYGLMAP enum glhckFramebufferAttachmentType {
   GLHCK_COLOR_ATTACHMENT0,
   GLHCK_COLOR_ATTACHMENT1,
   GLHCK_COLOR_ATTACHMENT2,
   GLHCK_COLOR_ATTACHMENT3,
   GLHCK_COLOR_ATTACHMENT4,
   GLHCK_COLOR_ATTACHMENT5,
   GLHCK_COLOR_ATTACHMENT6,
   GLHCK_COLOR_ATTACHMENT7,
   GLHCK_COLOR_ATTACHMENT8,
   GLHCK_COLOR_ATTACHMENT9,
   GLHCK_COLOR_ATTACHMENT10,
   GLHCK_COLOR_ATTACHMENT11,
   GLHCK_COLOR_ATTACHMENT12,
   GLHCK_COLOR_ATTACHMENT13,
   GLHCK_COLOR_ATTACHMENT14,
   GLHCK_COLOR_ATTACHMENT15,
   GLHCK_DEPTH_ATTACHMENT,
   GLHCK_STENCIL_ATTACHMENT,
} glhckFramebufferAttachmentType;

/* hardware buffer type */
PYGLMAP enum glhckHwBufferTarget {
   GLHCK_ARRAY_BUFFER,
   GLHCK_COPY_READ_BUFFER,
   GLHCK_COPY_WRITE_BUFFER,
   GLHCK_ELEMENT_ARRAY_BUFFER,
   GLHCK_PIXEL_PACK_BUFFER,
   GLHCK_PIXEL_UNPACK_BUFFER,
   GLHCK_TEXTURE_BUFFER,
   GLHCK_TRANSFORM_FEEDBACK_BUFFER,
   GLHCK_UNIFORM_BUFFER,
   GLHCK_SHADER_STORAGE_BUFFER,
   GLHCK_ATOMIC_COUNTER_BUFFER,
   GLHCK_HWBUFFER_TYPE_LAST,
} glhckHwBufferTarget;

/* hardware buffer storage type */
PYGLMAP enum glhckHwBufferStoreType {
   GLHCK_BUFFER_STREAM_DRAW,
   GLHCK_BUFFER_STREAM_READ,
   GLHCK_BUFFER_STREAM_COPY,
   GLHCK_BUFFER_STATIC_DRAW,
   GLHCK_BUFFER_STATIC_READ,
   GLHCK_BUFFER_STATIC_COPY,
   GLHCK_BUFFER_DYNAMIC_DRAW,
   GLHCK_BUFFER_DYNAMIC_READ,
   GLHCK_BUFFER_DYNAMIC_COPY,
} glhckHwBufferStoreType;

/* hardware buffer access type */
PYGLMAP enum glhckHwBufferAccessType {
   GLHCK_BUFFER_READ_ONLY,
   GLHCK_BUFFER_WRITE_ONLY,
   GLHCK_BUFFER_READ_WRITE,
} glhckHwBufferAccessType;

/* shader type */
PYGLMAP enum glhckShaderType {
   GLHCK_VERTEX_SHADER,
   GLHCK_FRAGMENT_SHADER,
} glhckShaderType;

/* texture wrap modes */
PYGLMAP enum glhckTextureWrap {
   GLHCK_REPEAT,
   GLHCK_MIRRORED_REPEAT,
   GLHCK_CLAMP_TO_EDGE,
   GLHCK_CLAMP_TO_BORDER,
} glhckTextureWrap;

/* texture filter modes */
PYGLMAP enum glhckTextureFilter {
   GLHCK_NEAREST,
   GLHCK_LINEAR,
   GLHCK_NEAREST_MIPMAP_NEAREST,
   GLHCK_LINEAR_MIPMAP_NEAREST,
   GLHCK_NEAREST_MIPMAP_LINEAR,
   GLHCK_LINEAR_MIPMAP_LINEAR,
} glhckTextureFilter;

/* texture compare mode */
PYGLMAP enum glhckTextureCompareMode {
   GLHCK_COMPARE_NONE,
   GLHCK_COMPARE_REF_TO_TEXTURE,
} glhckTextureCompareMode;

/* compare func */
PYGLMAP enum glhckCompareFunc {
   GLHCK_LEQUAL,
   GLHCK_GEQUAL,
   GLHCK_LESS,
   GLHCK_GREATER,
   GLHCK_EQUAL,
   GLHCK_NOTEQUAL,
   GLHCK_ALWAYS,
   GLHCK_NEVER,
} glhckCompareFunc;

/* texture types */
PYGLMAP enum glhckTextureTarget {
   GLHCK_TEXTURE_1D,
   GLHCK_TEXTURE_2D,
   GLHCK_TEXTURE_3D,
   GLHCK_TEXTURE_CUBE_MAP,
   GLHCK_TEXTURE_TYPE_LAST,
} glhckTextureTarget;

/* texture formats */
PYGLMAP enum glhckTextureFormat {
   GLHCK_RED,
   GLHCK_RG,
   GLHCK_ALPHA,
   GLHCK_LUMINANCE,
   GLHCK_LUMINANCE_ALPHA,
   GLHCK_RGB,
   GLHCK_BGR,
   GLHCK_RGBA,
   GLHCK_BGRA,
   GLHCK_DEPTH_COMPONENT,
   GLHCK_DEPTH_COMPONENT16,
   GLHCK_DEPTH_COMPONENT24,
   GLHCK_DEPTH_COMPONENT32,
   GLHCK_DEPTH_STENCIL,
   GLHCK_COMPRESSED_RGB_DXT1,
   GLHCK_COMPRESSED_RGBA_DXT5,
} glhckTextureFormat;

/* data formats */
PYGLMAP enum glhckDataType {
   GLHCK_COMPRESSED,
   GLHCK_UNSIGNED_BYTE,
   GLHCK_BYTE,
   GLHCK_UNSIGNED_SHORT,
   GLHCK_SHORT,
   GLHCK_UNSIGNED_INT,
   GLHCK_INT,
   GLHCK_FLOAT,
   GLHCK_UNSIGNED_BYTE_3_3_2,
   GLHCK_UNSIGNED_BYTE_2_3_3_REV,
   GLHCK_UNSIGNED_SHORT_5_6_5,
   GLHCK_UNSIGNED_SHORT_5_6_5_REV,
   GLHCK_UNSIGNED_SHORT_4_4_4_4,
   GLHCK_UNSIGNED_SHORT_4_4_4_4_REV,
   GLHCK_UNSIGNED_SHORT_5_5_5_1,
   GLHCK_UNSIGNED_SHORT_1_5_5_5_REV,
   GLHCK_UNSIGNED_INT_8_8_8_8,
   GLHCK_UNSIGNED_INT_8_8_8_8_REV,
   GLHCK_UNSIGNED_INT_10_10_10_2,
   GLHCK_UNSIGNED_INT_2_10_10_10_REV,
} glhckDataType;

/* blending modes */
PYGLMAP enum glhckBlendingMode {
   GLHCK_ZERO,
   GLHCK_ONE,
   GLHCK_SRC_COLOR,
   GLHCK_ONE_MINUS_SRC_COLOR,
   GLHCK_SRC_ALPHA,
   GLHCK_ONE_MINUS_SRC_ALPHA,
   GLHCK_DST_ALPHA,
   GLHCK_ONE_MINUS_DST_ALPHA,
   GLHCK_DST_COLOR,
   GLHCK_ONE_MINUS_DST_COLOR,
   GLHCK_SRC_ALPHA_SATURATE,
   GLHCK_CONSTANT_COLOR,
   GLHCK_CONSTANT_ALPHA,
   GLHCK_ONE_MINUS_CONSTANT_ALPHA,
} glhckBlendingMode;

#undef PYGLMAP

/* debugging level */
typedef enum glhckDebugLevel {
   GLHCK_DBG_ERROR,
   GLHCK_DBG_WARNING,
   GLHCK_DBG_CRAP,
} glhckDebugLevel;

/* \brief render compile time features */
typedef struct glhckCompileFeaturesRender {
   char glesv1, glesv2, opengl;
} glhckCompileFeaturesRender;

/* \brief import compile time features */
typedef struct glhckCompileFeaturesImport {
   char assimp, openctm, mmd;
   char bmp, png, tga, jpeg;
} glhckCompileFeaturesImport;

/* \brief math compile time features */
typedef struct glhckCompileFeaturesMath {
   char useDoublePrecision;
} glhckCompileFeaturesMath;

/* \brief compile time features struct */
typedef struct glhckCompileFeatures {
   glhckCompileFeaturesRender render;
   glhckCompileFeaturesImport import;
   glhckCompileFeaturesMath math;
} glhckCompileFeatures;

/* driver type */
typedef enum glhckDriverType {
   GLHCK_DRIVER_NONE,
   GLHCK_DRIVER_OTHER,
   GLHCK_DRIVER_MESA,
   GLHCK_DRIVER_NVIDIA,
   GLHCK_DRIVER_ATI,
   GLHCK_DRIVER_IMGTEC,
} glhckDriverType;

/* render type */
typedef enum glhckRenderType {
   GLHCK_RENDER_AUTO,
   GLHCK_RENDER_OPENGL,
   GLHCK_RENDER_OPENGL_FIXED_PIPELINE,
   GLHCK_RENDER_GLES1,
   GLHCK_RENDER_GLES2,
   GLHCK_RENDER_STUB,
} glhckRenderType;

/* render pass bits */
typedef enum glhckRenderPassFlags {
   GLHCK_PASS_NONE           = 0,
   GLHCK_PASS_DEPTH          = 1<<0,
   GLHCK_PASS_CULL           = 1<<1,
   GLHCK_PASS_BLEND          = 1<<2,
   GLHCK_PASS_TEXTURE        = 1<<3,
   GLHCK_PASS_DRAW_OBB       = 1<<4,
   GLHCK_PASS_DRAW_AABB      = 1<<5,
   GLHCK_PASS_DRAW_SKELETON  = 1<<6,
   GLHCK_PASS_DRAW_WIREFRAME = 1<<7,
   GLHCK_PASS_LIGHTING       = 1<<8,
   GLHCK_PASS_OVERDRAW       = 1<<9,
} glhckRenderPassFlags;

/* \brief version render features */
typedef struct glhckRenderFeaturesVersion {
   int major, minor, patch;
   int shaderMajor, shaderMinor, shaderPatch;
} glhckRenderFeaturesVersion;

/* \brief texture render features */
typedef struct glhckRenderFeaturesTexture {
   char hasNativeNpotSupport;
   int maxTextureSize;
   int maxRenderbufferSize;
} glhckRenderFeaturesTexture;

/* \brief renderer features */
typedef struct glhckRenderFeatures {
   glhckRenderFeaturesVersion version;
   glhckRenderFeaturesTexture texture;
} glhckRenderFeatures;

/* texture parameters struct */
typedef struct glhckTextureParameters {
   float maxAnisotropy;
   float minLod, maxLod, biasLod;
   int baseLevel, maxLevel;
   glhckTextureWrap wrapS, wrapT, wrapR;
   glhckTextureFilter minFilter, magFilter;
   glhckTextureCompareMode compareMode;
   glhckCompareFunc compareFunc;
   char mipmap;
} glhckTextureParameters;

/* projection type */
typedef enum glhckProjectionType {
   GLHCK_PROJECTION_PERSPECTIVE,
   GLHCK_PROJECTION_ORTHOGRAPHIC,
} glhckProjectionType;

/* frustum planes */
typedef enum glhckFrustumPlane {
   GLHCK_FRUSTUM_PLANE_LEFT,
   GLHCK_FRUSTUM_PLANE_RIGHT,
   GLHCK_FRUSTUM_PLANE_BOTTOM,
   GLHCK_FRUSTUM_PLANE_TOP,
   GLHCK_FRUSTUM_PLANE_NEAR,
   GLHCK_FRUSTUM_PLANE_FAR,
   GLHCK_FRUSTUM_PLANE_LAST
} glhckFrustumPlane;

/* frustum corners */
typedef enum glhckFrustumCorner {
   GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_LEFT,
   GLHCK_FRUSTUM_CORNER_NEAR_BOTTOM_RIGHT,
   GLHCK_FRUSTUM_CORNER_NEAR_TOP_RIGHT,
   GLHCK_FRUSTUM_CORNER_NEAR_TOP_LEFT,
   GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_LEFT,
   GLHCK_FRUSTUM_CORNER_FAR_BOTTOM_RIGHT,
   GLHCK_FRUSTUM_CORNER_FAR_TOP_RIGHT,
   GLHCK_FRUSTUM_CORNER_FAR_TOP_LEFT,
   GLHCK_FRUSTUM_CORNER_LAST
} glhckFrustumCorner;

typedef enum glhckFrustumTestResult {
   GLHCK_FRUSTUM_OUTSIDE,
   GLHCK_FRUSTUM_INSIDE,
   GLHCK_FRUSTUM_PARTIAL
} glhckFrustumTestResult;

/* frustum struct */
typedef struct glhckFrustum {
   kmPlane planes[GLHCK_FRUSTUM_PLANE_LAST];
   kmVec3 corners[GLHCK_FRUSTUM_CORNER_LAST];
} glhckFrustum;

/* parent affection flags */
typedef enum glhckObjectAffectionFlags {
   GLHCK_AFFECT_NONE          = 0,
   GLHCK_AFFECT_TRANSLATION   = 1<<0,
   GLHCK_AFFECT_ROTATION      = 1<<1,
   GLHCK_AFFECT_SCALING       = 1<<2,
} glhckObjectAffectionFlags;

/* material flags */
typedef enum glhckMaterialOptionsFlags {
   GLHCK_MATERIAL_NONE           = 0,
   GLHCK_MATERIAL_LIGHTING       = 1<<0,
} glhckMaterialOptionsFlags;

/* texture compression type */
typedef enum glhckTextureCompression {
   GLHCK_COMPRESSION_NONE,
   GLHCK_COMPRESSION_DXT,
} glhckTextureCompression;

/* model import parameters */
typedef struct glhckImportModelParameters {
   char animated; /* tells importer that model with animation data was requested */
   char flatten; /* tells importer to join all mesh nodes into one */
} glhckImportModelParameters;

/* texture import parameters */
typedef struct glhckImportImageParameters {
   glhckTextureCompression compression;
} glhckImportImageParameters;

/* common datatypes */
typedef struct glhckRect {
   float x, y, w, h;
} glhckRect;

/* vertex datatypes */
typedef struct glhckColorb {
   unsigned char r, g, b, a;
} glhckColorb;

typedef struct glhckVector3b {
   char x, y, z;
} glhckVector3b;

typedef struct glhckVector2b {
   char x, y;
} glhckVector2b;

typedef struct glhckVector3s {
   short x, y, z;
} glhckVector3s;

typedef struct glhckVector2s {
   short x, y;
} glhckVector2s;

typedef struct glhckVector3f {
   float x, y, z;
} glhckVector3f;

typedef struct glhckVector2f {
   float x, y;
} glhckVector2f;

typedef struct glhckVertexData3f {
   glhckVector3f vertex;
   glhckVector3f normal;
   glhckVector2f coord;
   glhckColorb color;
} glhckVertexData3f;

typedef struct glhckVertexData2f {
   glhckVector2f vertex;
   glhckVector3f normal;
   glhckVector2f coord;
   glhckColorb color;
} glhckVertexData2f;

typedef struct glhckVertexData3s {
   glhckVector3s vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData3s;

typedef struct glhckVertexData2s {
   glhckVector2s vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData2s;

typedef struct glhckVertexData3b {
   glhckVector3b vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData3b;

typedef struct glhckVertexData2b {
   glhckVector2b vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData2b;

/* feed glhck the highest precision */
typedef glhckVertexData3f glhckImportVertexData;
typedef unsigned int glhckImportIndexData;

typedef enum glhckBuiltinVertexType {
   GLHCK_VTX_V3F,
   GLHCK_VTX_V2F,
   GLHCK_VTX_V3S,
   GLHCK_VTX_V2S,
   GLHCK_VTX_V3B,
   GLHCK_VTX_V2B,
   GLHCK_VTX_AUTO = 255,
} glhckBuiltinVertexType;

typedef enum glhckBuiltinIndexType {
   GLHCK_IDX_UINT,
   GLHCK_IDX_USHRT,
   GLHCK_IDX_UBYTE,
   GLHCK_IDX_AUTO = 255,
} glhckBuiltinIndexType;

/* forward */
struct glhckGeometry;
struct _glhckSkinBone;

/* function map for vertexType */
typedef struct glhckVertexTypeFunctionMap {
   void (*convert)(const glhckImportVertexData *import, int memb, void *out, glhckVector3f *bias, glhckVector3f *scale);
   void (*minMax)(struct glhckGeometry *geometry, glhckVector3f *min, glhckVector3f *max);
   void (*transform)(struct glhckGeometry *geometry, const void *bindPose, struct _glhckSkinBone **bones, unsigned int memb);
} glhckVertexTypeFunctionMap;

/* function map for indexType */
typedef struct glhckIndexTypeFunctionMap {
   void (*convert)(const glhckImportIndexData *import, int memb, void *out);
} glhckIndexTypeFunctionMap;

/* geometry datatype for low-level raw access */
typedef struct glhckGeometry {
   /* geometry transformation needed
    * after precision conversion */
   glhckVector3f bias;
   glhckVector3f scale;

   /* vertices && indices*/
   void *vertices, *indices;

   /* counts for vertices && indices */
   int vertexCount, indexCount;

   /* transformed texture range
    * the texture matrix is scaled with
    * 1.0f/textureRange of geometry before
    * sending to OpenGL */
   int textureRange;

   /* vertex data && index data types */
   unsigned char vertexType, indexType;

   /* geometry type (triangles, triangle strip, etc..) */
   glhckGeometryType type;
} glhckGeometry;

/* vector animation key */
typedef struct glhckAnimationVectorKey {
   kmVec3 vector;
   float time;
} glhckAnimationVectorKey;

/* quaternion animation key */
typedef struct glhckAnimationQuaternionKey {
   kmQuaternion quaternion;
   float time;
} glhckAnimationQuaternionKey;

/* datatype that presents bone's vertex weight */
typedef struct glhckVertexWeight {
   float weight;
   glhckImportIndexData vertexIndex;
} glhckVertexWeight;

/* collision callbacks */
struct glhckCollisionInData;
struct glhckCollisionOutData;
struct _glhckCollisionPrimitive;
typedef void (*glhckCollisionResponseFunction)(const struct glhckCollisionOutData *collision);
typedef int (*glhckCollisionTestFunction)(const struct glhckCollisionInData *data, const struct _glhckCollisionPrimitive *collider);

/* collision input data */
typedef struct glhckCollisionInData {
   glhckCollisionTestFunction test;
   glhckCollisionResponseFunction response;
   const kmVec3 *velocity;
   void *userData;
} glhckCollisionInData;

/* collision output data */
typedef struct glhckCollisionOutData {
   struct _glhckCollisionWorld *world;
   const struct _glhckCollisionPrimitive *collider;
   const kmVec3 *contactPoint;
   const kmVec3 *pushVector;
   const kmVec3 *velocity;
   void *userData;
} glhckCollisionOutData;

/* typedefs for better typing */
typedef struct _glhckMaterial           glhckMaterial;
typedef struct _glhckTexture            glhckTexture;
typedef struct _glhckAtlas              glhckAtlas;
typedef struct _glhckRenderbuffer       glhckRenderbuffer;
typedef struct _glhckFramebuffer        glhckFramebuffer;
typedef struct _glhckObject             glhckObject;
typedef struct _glhckBone               glhckBone;
typedef struct _glhckSkinBone           glhckSkinBone;
typedef struct _glhckAnimation          glhckAnimation;
typedef struct _glhckAnimationNode      glhckAnimationNode;
typedef struct _glhckAnimator           glhckAnimator;
typedef struct _glhckText               glhckText;
typedef struct _glhckCamera             glhckCamera;
typedef struct _glhckLight              glhckLight;
typedef struct _glhckHwBuffer           glhckHwBuffer;
typedef struct _glhckShader             glhckShader;
typedef struct _glhckCollisionWorld     glhckCollisionWorld;
typedef struct _glhckCollisionPrimitive glhckCollisionPrimitive;
typedef struct __GLHCKcontext           glhckContext;

typedef void (*glhckDebugHookFunc)(const char *file, int line, const char *function, glhckDebugLevel level, const char *str);

/* can be called before context */
GLHCKAPI void glhckGetCompileFeatures(glhckCompileFeatures *features);

/* init && terminate */
GLHCKAPI int glhckInitialized(void);
GLHCKAPI void glhckContextSet(glhckContext *ctx);
GLHCKAPI glhckContext* glhckContextGet(void);
GLHCKAPI glhckContext* glhckContextCreate(int argc, char **argv);
GLHCKAPI void glhckContextTerminate(void);
GLHCKAPI void glhckMassacreWorld(void);
GLHCKAPI void glhckLogColor(char color);
GLHCKAPI void glhckSetGlobalPrecision(unsigned char itype, unsigned char vtype);
GLHCKAPI void glhckGetGlobalPrecision(unsigned char *itype, unsigned char *vtype);

/* import */
GLHCKAPI const glhckImportModelParameters* glhckImportDefaultModelParameters(void);
GLHCKAPI const glhckImportImageParameters* glhckImportDefaultImageParameters(void);

/* display */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType);
GLHCKAPI void glhckDisplayClose(void);
GLHCKAPI void glhckDisplayResize(int width, int height);

/* rendering */
GLHCKAPI const char* glhckRenderName(void);
GLHCKAPI glhckDriverType glhckRenderGetDriver(void);
GLHCKAPI const glhckRenderFeatures* glhckRenderGetFeatures(void);
GLHCKAPI void glhckRenderResize(int width, int height);
GLHCKAPI void glhckRenderViewport(const glhckRect *viewport);
GLHCKAPI void glhckRenderViewporti(int x, int y, int width, int height);
GLHCKAPI void glhckRenderStatePush(void);
GLHCKAPI void glhckRenderStatePush2D(int width, int height, kmScalar near, kmScalar far);
GLHCKAPI void glhckRenderStatePop(void);
GLHCKAPI unsigned int glhckRenderPassDefaults(void);
GLHCKAPI void glhckRenderPass(unsigned int flags);
GLHCKAPI unsigned int glhckRenderGetPass(void);
GLHCKAPI void glhckRenderTime(float time);
GLHCKAPI void glhckRenderClearColor(const glhckColorb *color);
GLHCKAPI void glhckRenderClearColorb(char r, char g, char b, char a);
GLHCKAPI const glhckColorb* glhckRenderGetClearColor(void);
GLHCKAPI void glhckRenderClear(unsigned int bufferBits);
GLHCKAPI void glhckRenderBlendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb);
GLHCKAPI void glhckRenderGetBlendFunc(glhckBlendingMode *blenda, glhckBlendingMode *blendb);
GLHCKAPI void glhckRenderFrontFace(glhckFaceOrientation orientation);
GLHCKAPI glhckFaceOrientation glhckRenderGetFrontFace(void);
GLHCKAPI void glhckRenderCullFace(glhckCullFaceType face);
GLHCKAPI glhckCullFaceType glhckRenderGetCullFace(void);
GLHCKAPI void glhckRenderPassShader(glhckShader *shader);
GLHCKAPI glhckShader* glhckRenderGetShader(void);
GLHCKAPI void glhckRenderFlip(int flip);
GLHCKAPI int glhckRenderGetFlip(void);
GLHCKAPI void glhckRenderProjection(kmMat4 const* mat);
GLHCKAPI void glhckRenderProjectionOnly(const kmMat4 *mat);
GLHCKAPI void glhckRenderProjection2D(int width, int height, kmScalar near, kmScalar far);
GLHCKAPI const kmMat4* glhckRenderGetProjection(void);
GLHCKAPI void glhckRenderView(kmMat4 const* mat);
GLHCKAPI const kmMat4* glhckRenderGetView(void);
GLHCKAPI void glhckRenderWorldPosition(const kmVec3 *position);
GLHCKAPI void glhckRender(void);

/* frustum */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *object, const kmMat4 *mvp);
GLHCKAPI void glhckFrustumRender(glhckFrustum *object);
GLHCKAPI int glhckFrustumContainsPoint(const glhckFrustum *object, const kmVec3 *point);
GLHCKAPI kmScalar glhckFrustumContainsSphere(const glhckFrustum *object, const kmVec3 *point, kmScalar radius);
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsSphereEx(const glhckFrustum *object, const kmVec3 *point, kmScalar radius);
GLHCKAPI int glhckFrustumContainsAABB(const glhckFrustum *object, const kmAABB *aabb);
GLHCKAPI glhckFrustumTestResult glhckFrustumContainsAABBEx(const glhckFrustum *object, const kmAABB *aabb);

/* cameras */
GLHCKAPI glhckCamera* glhckCameraNew(void);
GLHCKAPI glhckCamera* glhckCameraRef(glhckCamera *object);
GLHCKAPI unsigned int glhckCameraFree(glhckCamera *object);
GLHCKAPI void glhckCameraUpdate(glhckCamera *object);
GLHCKAPI void glhckCameraReset(glhckCamera *object);
GLHCKAPI void glhckCameraProjection(glhckCamera *object, const glhckProjectionType projectionType);
GLHCKAPI glhckFrustum* glhckCameraGetFrustum(glhckCamera *object);
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckCamera *object);
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckCamera *object);
GLHCKAPI const kmMat4* glhckCameraGetVPMatrix(const glhckCamera *object);
GLHCKAPI void glhckCameraUpVector(glhckCamera *object, const kmVec3 *upVector);
GLHCKAPI void glhckCameraFov(glhckCamera *object, const kmScalar fov);
GLHCKAPI void glhckCameraRange(glhckCamera *object, const kmScalar near, const kmScalar far);
GLHCKAPI void glhckCameraViewport(glhckCamera *object, const glhckRect *viewport);
GLHCKAPI void glhckCameraViewporti(glhckCamera *object, int x, int y, int w, int h);
GLHCKAPI glhckObject* glhckCameraGetObject(const glhckCamera *object);
GLHCKAPI kmRay3* glhckCameraCastRayFromPoint(glhckCamera *object, kmRay3* pOut, const kmVec2 *point);
GLHCKAPI kmRay3* glhckCameraCastRayFromPointf(glhckCamera *object, kmRay3* pOut, const kmScalar x, const kmScalar y);

/* objects */
GLHCKAPI glhckObject* glhckObjectNew(void);
GLHCKAPI glhckObject* glhckObjectCopy(const glhckObject *src);
GLHCKAPI glhckObject* glhckObjectRef(glhckObject *object);
GLHCKAPI unsigned int glhckObjectFree(glhckObject *object);
GLHCKAPI int glhckObjectIsRoot(const glhckObject *object);
GLHCKAPI void glhckObjectMakeRoot(glhckObject *object, int root);
GLHCKAPI unsigned char glhckObjectGetParentAffection(const glhckObject *object);
GLHCKAPI void glhckObjectParentAffection(glhckObject *object, unsigned char affectionFlags);
GLHCKAPI glhckObject* glhckObjectParent(glhckObject *object);
GLHCKAPI glhckObject** glhckObjectChildren(glhckObject *object, unsigned int *memb);
GLHCKAPI void glhckObjectAddChild(glhckObject *object, glhckObject *child);
GLHCKAPI void glhckObjectRemoveChild(glhckObject *object, glhckObject *child);
GLHCKAPI void glhckObjectRemoveChildren(glhckObject *object);
GLHCKAPI void glhckObjectRemoveFromParent(glhckObject *object);
GLHCKAPI void glhckObjectMaterial(glhckObject *object, glhckMaterial *material);
GLHCKAPI glhckMaterial* glhckObjectGetMaterial(const glhckObject *object);
GLHCKAPI void glhckObjectDraw(glhckObject *object);
GLHCKAPI void glhckObjectRender(glhckObject *object);
GLHCKAPI void glhckObjectRenderAll(glhckObject *object);

/* object animation */
GLHCKAPI int glhckObjectInsertBones(glhckObject *object, glhckBone **bones, unsigned int memb);
GLHCKAPI glhckBone** glhckObjectBones(glhckObject *object, unsigned int *memb);
GLHCKAPI glhckBone* glhckObjectGetBone(glhckObject *object, const char *name);
GLHCKAPI int glhckObjectInsertSkinBones(glhckObject *object, glhckSkinBone **skinBones, unsigned int memb);
GLHCKAPI glhckSkinBone** glhckObjectSkinBones(glhckObject *object, unsigned int *memb);
GLHCKAPI glhckSkinBone* glhckObjectGetSkinBone(glhckObject *object, const char *name);
GLHCKAPI int glhckObjectInsertAnimations(glhckObject *object, glhckAnimation **animations, unsigned int memb);
GLHCKAPI glhckAnimation** glhckObjectAnimations(glhckObject *object, unsigned int *memb);

/* object drawing options */
GLHCKAPI void glhckObjectVertexColors(glhckObject *object, int vertexColors);
GLHCKAPI int glhckObjectGetVertexColors(glhckObject *object);
GLHCKAPI void glhckObjectCull(glhckObject *object, int cull);
GLHCKAPI int glhckObjectGetCull(const glhckObject *object);
GLHCKAPI void glhckObjectDepth(glhckObject *object, int depth);
GLHCKAPI int glhckObjectGetDepth(const glhckObject *object);
GLHCKAPI void glhckObjectDrawAABB(glhckObject *object, int drawAABB);
GLHCKAPI int glhckObjectGetDrawAABB(const glhckObject *object);
GLHCKAPI void glhckObjectDrawOBB(glhckObject *object, int drawOBB);
GLHCKAPI int glhckObjectGetDrawOBB(const glhckObject *object);
GLHCKAPI void glhckObjectDrawSkeleton(glhckObject *object, int drawSkeleton);
GLHCKAPI int glhckObjectGetDrawSkeleton(const glhckObject *object);
GLHCKAPI void glhckObjectDrawWireframe(glhckObject *object, int drawWireframe);
GLHCKAPI int glhckObjectGetDrawWireframe(const glhckObject *object);

/* object control */
GLHCKAPI const kmAABB* glhckObjectGetOBB(glhckObject *object);
GLHCKAPI const kmAABB* glhckObjectGetOBBWithChildren(glhckObject *object);
GLHCKAPI const kmAABB* glhckObjectGetAABB(glhckObject *object);
GLHCKAPI const kmAABB* glhckObjectGetAABBWithChildren(glhckObject *object);
GLHCKAPI const kmMat4* glhckObjectGetMatrix(glhckObject *object);
GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckObject *object);
GLHCKAPI void glhckObjectPosition(glhckObject *object, const kmVec3 *position);
GLHCKAPI void glhckObjectPositionf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectMove(glhckObject *object, const kmVec3 *move);
GLHCKAPI void glhckObjectMovef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetRotation(const glhckObject *object);
GLHCKAPI void glhckObjectRotation(glhckObject *object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotationf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectRotate(glhckObject *object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotatef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetTarget(const glhckObject *object);
GLHCKAPI void glhckObjectTarget(glhckObject *object, const kmVec3 *target);
GLHCKAPI void glhckObjectTargetf(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetScale(const glhckObject *object);
GLHCKAPI void glhckObjectScale(glhckObject *object, const kmVec3 *scale);
GLHCKAPI void glhckObjectScalef(glhckObject *object, const kmScalar x, const kmScalar y, const kmScalar z);

/* object geometry */
GLHCKAPI int glhckObjectInsertVertices(glhckObject *object, unsigned char type, const glhckImportVertexData *vertices, int memb);
GLHCKAPI int glhckObjectInsertIndices(glhckObject *object, unsigned char type, const glhckImportIndexData *indices, int memb);
GLHCKAPI void glhckObjectUpdate(glhckObject *object);
GLHCKAPI glhckGeometry* glhckObjectNewGeometry(glhckObject *object);
GLHCKAPI glhckGeometry* glhckObjectGetGeometry(const glhckObject *object);
GLHCKAPI int glhckObjectPickTextureCoordinatesWithRay(const glhckObject *object, const kmRay3 *ray, kmVec2 *outCoords);

/* pre-defined geometry */
GLHCKAPI glhckObject* glhckModelNew(const char *file, kmScalar size, const glhckImportModelParameters *importParams);
GLHCKAPI glhckObject* glhckModelNewEx(const char *file, kmScalar size, const glhckImportModelParameters *importParams, unsigned char itype, unsigned char vtype);
GLHCKAPI glhckObject* glhckCubeNew(kmScalar size);
GLHCKAPI glhckObject* glhckCubeNewEx(kmScalar size, unsigned char itype, unsigned char vtype);
GLHCKAPI glhckObject* glhckSphereNew(kmScalar size);
GLHCKAPI glhckObject* glhckSphereNewEx(kmScalar size, unsigned char itype, unsigned char vtype);
GLHCKAPI glhckObject* glhckEllipsoidNew(kmScalar rx, kmScalar ry, kmScalar rz);
GLHCKAPI glhckObject* glhckEllipsoidNewEx(kmScalar rx, kmScalar ry, kmScalar rz, unsigned char itype, unsigned char vtype);
GLHCKAPI glhckObject* glhckPlaneNew(kmScalar width, kmScalar height);
GLHCKAPI glhckObject* glhckPlaneNewEx(kmScalar width, kmScalar height, unsigned char itype, unsigned char vtype);
GLHCKAPI glhckObject* glhckSpriteNew(glhckTexture* texture, kmScalar width, kmScalar height);
GLHCKAPI glhckObject* glhckSpriteNewFromFile(const char *file, kmScalar width, kmScalar height, const glhckImportImageParameters *importParams, const glhckTextureParameters *params);

/* bones */
GLHCKAPI glhckBone* glhckBoneNew(void);
GLHCKAPI glhckBone* glhckBoneRef(glhckBone *object);
GLHCKAPI unsigned int glhckBoneFree(glhckBone *object);
GLHCKAPI void glhckBoneName(glhckBone *object, const char *name);
GLHCKAPI const char* glhckBoneGetName(glhckBone *object);
GLHCKAPI void glhckBoneParentBone(glhckBone *object, glhckBone *parentBone);
GLHCKAPI glhckBone* glhckBoneGetParentBone(glhckBone *object);
GLHCKAPI void glhckBoneTransformationMatrix(glhckBone *object, const kmMat4 *transformationMatrix);
GLHCKAPI const kmMat4* glhckBoneGetTransformationMatrix(glhckBone *object);
GLHCKAPI const kmMat4* glhckBoneGetTransformedMatrix(glhckBone *object);
GLHCKAPI void glhckBoneGetPositionRelativeOnObject(glhckBone *object, glhckObject *gobject, kmVec3 *outPosition);
GLHCKAPI void glhckBoneGetPositionAbsoluteOnObject(glhckBone *object, glhckObject *gobject, kmVec3 *outPosition);

/* skin bones (for skinning \o/) */
GLHCKAPI glhckSkinBone* glhckSkinBoneNew(void);
GLHCKAPI glhckSkinBone* glhckSkinBoneRef(glhckSkinBone *object);
GLHCKAPI unsigned int glhckSkinBoneFree(glhckSkinBone *object);
GLHCKAPI void glhckSkinBoneBone(glhckSkinBone *object, glhckBone *bone);
GLHCKAPI glhckBone* glhckSkinBoneGetBone(glhckSkinBone *object);
GLHCKAPI const char* glhckSkineBoneGetName(glhckSkinBone *object);
GLHCKAPI int glhckSkinBoneInsertWeights(glhckSkinBone *object, const glhckVertexWeight *weights, unsigned int memb);
GLHCKAPI const glhckVertexWeight* glhckSkinBoneWeights(glhckSkinBone *object, unsigned int *memb);
GLHCKAPI void glhckSkinBoneOffsetMatrix(glhckSkinBone *object, const kmMat4 *offsetMatrix);
GLHCKAPI const kmMat4* glhckSkinBoneGetOffsetMatrix(glhckSkinBone *object);

/* animation nodes */
GLHCKAPI glhckAnimationNode* glhckAnimationNodeNew(void);
GLHCKAPI glhckAnimationNode* glhckAnimationNodeRef(glhckAnimationNode *object);
GLHCKAPI unsigned int glhckAnimationNodeFree(glhckAnimationNode *object);
GLHCKAPI void glhckAnimationNodeBoneName(glhckAnimationNode *object, const char *name);
GLHCKAPI const char* glhckAnimationNodeGetBoneName(glhckAnimationNode *object);
GLHCKAPI int glhckAnimationNodeInsertTranslations(glhckAnimationNode *object, const glhckAnimationVectorKey *keys, unsigned int memb);
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeTranslations(glhckAnimationNode *object, unsigned int *memb);
GLHCKAPI int glhckAnimationNodeInsertRotations(glhckAnimationNode *object, const glhckAnimationQuaternionKey *keys, unsigned int memb);
GLHCKAPI const glhckAnimationQuaternionKey* glhckAnimationNodeRotations(glhckAnimationNode *object, unsigned int *memb);
GLHCKAPI int glhckAnimationNodeInsertScalings(glhckAnimationNode *object, const glhckAnimationVectorKey *keys, unsigned int memb);
GLHCKAPI const glhckAnimationVectorKey* glhckAnimationNodeScalings(glhckAnimationNode *object, unsigned int *memb);

/* animations */
GLHCKAPI glhckAnimation* glhckAnimationNew(void);
GLHCKAPI glhckAnimation* glhckAnimationRef(glhckAnimation *object);
GLHCKAPI unsigned int glhckAnimationFree(glhckAnimation *object);
GLHCKAPI void glhckAnimationName(glhckAnimation *object, const char *name);
GLHCKAPI const char* glhckAnimationGetName(glhckAnimation *object);
GLHCKAPI void glhckAnimationTicksPerSecond(glhckAnimation *object, float ticksPerSecond);
GLHCKAPI float glhckAnimationGetTicksPerSecond(glhckAnimation *object);
GLHCKAPI void glhckAnimationDuration(glhckAnimation *object, float duration);
GLHCKAPI float glhckAnimationGetDuration(glhckAnimation *object);
GLHCKAPI int glhckAnimationInsertNodes(glhckAnimation *object, glhckAnimationNode **nodes, unsigned int memb);
GLHCKAPI glhckAnimationNode** glhckAnimationNodes(glhckAnimation *object, unsigned int *memb);

/* animator */
GLHCKAPI glhckAnimator* glhckAnimatorNew(void);
GLHCKAPI glhckAnimator* glhckAnimatorRef(glhckAnimator *object);
GLHCKAPI unsigned int glhckAnimatorFree(glhckAnimator *object);
GLHCKAPI void glhckAnimatorAnimation(glhckAnimator *object, glhckAnimation *animation);
GLHCKAPI int glhckAnimatorInsertBones(glhckAnimator *object, glhckBone **bones, unsigned int memb);
GLHCKAPI glhckBone** glhckAnimatorBones(glhckAnimator *object, unsigned int *memb);
GLHCKAPI void glhckAnimatorTransform(glhckAnimator *object, glhckObject *gobject);
GLHCKAPI void glhckAnimatorUpdate(glhckAnimator *object, float playTime);

/* text */
GLHCKAPI glhckText* glhckTextNew(int cacheWidth, int cacheHeight);
GLHCKAPI glhckText* glhckTextRef(glhckText *object);
GLHCKAPI unsigned int glhckTextFree(glhckText *object);
GLHCKAPI void glhckTextFontFree(glhckText *object, unsigned int font_id);
GLHCKAPI void glhckTextFlushCache(glhckText *object);
GLHCKAPI void glhckTextGetMetrics(glhckText *object, unsigned int font_id, float size, float *ascender, float *descender, float *lineHeight);
GLHCKAPI void glhckTextGetMinMax(glhckText *object, unsigned int font_id, float size, const char *s, kmVec2 *min, kmVec2 *max);
GLHCKAPI void glhckTextColor(glhckText *object, const glhckColorb *color);
GLHCKAPI void glhckTextColorb(glhckText *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckTextGetColor(glhckText *object);
GLHCKAPI unsigned int glhckTextFontNewKakwafont(glhckText *object, int *nativeSize);
GLHCKAPI unsigned int glhckTextFontNewFromMemory(glhckText *object, const void *data, size_t size);
GLHCKAPI unsigned int glhckTextFontNew(glhckText *object, const char *file);
GLHCKAPI unsigned int glhckTextFontNewFromTexture(glhckText *object, glhckTexture *texture, int ascent, int descent, int lineGap);
GLHCKAPI unsigned int glhckTextFontNewFromBitmap(glhckText *object, const char *file, int ascent, int descent, int lineGap);
GLHCKAPI void glhckTextGlyphNew(glhckText *object, unsigned int font_id, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance);
GLHCKAPI void glhckTextStash(glhckText *object, unsigned int font_id, float size, float x, float y, const char *s, float *width);
GLHCKAPI void glhckTextClear(glhckText *object);
GLHCKAPI void glhckTextRender(glhckText *object);
GLHCKAPI void glhckTextShader(glhckText *object, glhckShader *shader);
GLHCKAPI glhckShader* glhckTextGetShader(const glhckText *object);
GLHCKAPI glhckTexture* glhckTextRTT(glhckText *object, unsigned int font_id, float size, const char *s, const glhckTextureParameters *params);
GLHCKAPI glhckObject* glhckTextPlane(glhckText *object, unsigned int font_id, float size, const char *s, const glhckTextureParameters *params);

/* materials */
GLHCKAPI glhckMaterial* glhckMaterialNew(glhckTexture *texture);
GLHCKAPI glhckMaterial* glhckMaterialRef(glhckMaterial *object);
GLHCKAPI unsigned int glhckMaterialFree(glhckMaterial *object);
GLHCKAPI void glhckMaterialOptions(glhckMaterial *object, unsigned int flags);
GLHCKAPI unsigned int glhckMaterialGetOptions(const glhckMaterial *object);
GLHCKAPI void glhckMaterialShader(glhckMaterial *object, glhckShader *shader);
GLHCKAPI glhckShader* glhckMaterialGetShader(const glhckMaterial *object);
GLHCKAPI void glhckMaterialTexture(glhckMaterial *object, glhckTexture *texture);
GLHCKAPI glhckTexture* glhckMaterialGetTexture(const glhckMaterial *object);
GLHCKAPI void glhckMaterialTextureScale(glhckMaterial *object, const kmVec2 *scale);
GLHCKAPI void glhckMaterialTextureScalef(glhckMaterial *object, kmScalar x, kmScalar y);
GLHCKAPI const kmVec2* glhckMaterialGetTextureScale(glhckMaterial *object);
GLHCKAPI void glhckMaterialTextureOffset(glhckMaterial *object, const kmVec2 *offset);
GLHCKAPI void glhckMaterialTextureOffsetf(glhckMaterial *object, kmScalar x, kmScalar y);
GLHCKAPI const kmVec2* glhckMaterialGetTextureOffset(glhckMaterial *object);
GLHCKAPI void glhckMaterialTextureRotationf(glhckMaterial *object, kmScalar degrees);
GLHCKAPI kmScalar glhckMaterialGetTextureRotation(glhckMaterial *object);
GLHCKAPI void glhckMaterialTextureTransform(glhckMaterial *object, const glhckRect *transformed, short degrees);
GLHCKAPI void glhckMaterialShininess(glhckMaterial *object, float shininess);
GLHCKAPI float glhckMaterialGetShininess(const glhckMaterial *object);
GLHCKAPI void glhckMaterialBlendFunc(glhckMaterial *object, glhckBlendingMode blenda, glhckBlendingMode blendb);
GLHCKAPI void glhckMaterialGetBlendFunc(const glhckMaterial *object, glhckBlendingMode *blenda, glhckBlendingMode *blendb);
GLHCKAPI void glhckMaterialAmbient(glhckMaterial *object, const glhckColorb *ambient);
GLHCKAPI void glhckMaterialAmibentb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckMaterialGetAmbient(const glhckMaterial *object);
GLHCKAPI void glhckMaterialDiffuse(glhckMaterial *object, const glhckColorb *diffuse);
GLHCKAPI void glhckMaterialDiffuseb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckMaterialGetDiffuse(const glhckMaterial *object);
GLHCKAPI void glhckMaterialEmissive(glhckMaterial *object, const glhckColorb *emissive);
GLHCKAPI void glhckMaterialEmissiveb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckMaterialGetEmissive(const glhckMaterial *object);
GLHCKAPI void glhckMaterialSpecular(glhckMaterial *object, const glhckColorb *specular);
GLHCKAPI void glhckMaterialSpecularb(glhckMaterial *object, unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckMaterialGetSpecular(const glhckMaterial *object);

/* textures */
GLHCKAPI glhckTexture* glhckTextureNew(void);
GLHCKAPI glhckTexture* glhckTextureNewFromFile(const char *file, const glhckImportImageParameters *importParams, const glhckTextureParameters *params);
GLHCKAPI glhckTexture* glhckTextureRef(glhckTexture *object);
GLHCKAPI unsigned int glhckTextureFree(glhckTexture *object);
GLHCKAPI void glhckTextureGetInformation(glhckTexture *object, glhckTextureTarget *target, int *width, int *height, int *depth, int *border, glhckTextureFormat *format, glhckDataType *type);
GLHCKAPI int glhckTextureCreate(glhckTexture *object, glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, glhckDataType type, int size, const void *data);
GLHCKAPI int glhckTextureRecreate(glhckTexture *object, glhckTextureFormat format, glhckDataType type, int size, const void *data);
GLHCKAPI void glhckTextureFill(glhckTexture *object, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data);
GLHCKAPI void glhckTextureFillFrom(glhckTexture *object, int level, int sx, int sy, int sz, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, glhckDataType type, int size, const void *data);
GLHCKAPI void glhckTextureParameter(glhckTexture *object, const glhckTextureParameters *params);
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultParameters(void);
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultLinearParameters(void);
GLHCKAPI const glhckTextureParameters* glhckTextureDefaultSpriteParameters(void);
GLHCKAPI int glhckTextureSave(glhckTexture *object, const char *path);
GLHCKAPI glhckTexture* glhckTextureCurrentForTarget(glhckTextureTarget target);
GLHCKAPI void glhckTextureActive(unsigned int index);
GLHCKAPI void glhckTextureBind(glhckTexture *object);
GLHCKAPI void glhckTextureUnbind(glhckTextureTarget target);
GLHCKAPI void* glhckTextureCompress(glhckTextureCompression compression, int width, int height, glhckTextureFormat format, glhckDataType type, const void *data, int *size, glhckTextureFormat *outFormat, glhckDataType *outType);

/* texture atlases */
GLHCKAPI glhckAtlas* glhckAtlasNew(void);
GLHCKAPI unsigned int glhckAtlasFree(glhckAtlas *object);
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *object, glhckTexture *texture);
GLHCKAPI glhckTexture* glhckAtlasGetTexture(glhckAtlas *object);
GLHCKAPI int glhckAtlasPack(glhckAtlas *object, glhckTextureFormat format, int powerOfTwo, int border, const glhckTextureParameters *params);
GLHCKAPI glhckTexture* glhckAtlasGetTextureByIndex(const glhckAtlas *object, unsigned short index);
GLHCKAPI int glhckAtlasGetTransform(const glhckAtlas *object, glhckTexture *texture, glhckRect *transformed, short *degrees);
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckAtlas *object, glhckTexture *texture, const kmVec2 *in, kmVec2 *out);

/* light objects */
GLHCKAPI glhckLight* glhckLightNew(void);
GLHCKAPI glhckLight* glhckLightRef(glhckLight *object);
GLHCKAPI unsigned int glhckLightFree(glhckLight *object);
GLHCKAPI glhckObject* glhckLightGetObject(const glhckLight *object);
GLHCKAPI void glhckLightBind(glhckLight *object);
GLHCKAPI void glhckLightBeginProjectionWithCamera(glhckLight *object, glhckCamera *camera);
GLHCKAPI void glhckLightEndProjectionWithCamera(glhckLight *object, glhckCamera *camera);
GLHCKAPI void glhckLightColor(glhckLight *object, const glhckColorb *color);
GLHCKAPI void glhckLightColorb(glhckLight *object, char r, char g, char b, char a);
GLHCKAPI void glhckLightPointLightFactor(glhckLight *object, kmScalar factor);
GLHCKAPI kmScalar glhckLightGetPointLightFactor(glhckLight *object);
GLHCKAPI void glhckLightAtten(glhckLight *object, const kmVec3 *atten);
GLHCKAPI void glhckLightAttenf(glhckLight *object, kmScalar constant, kmScalar linear, kmScalar quadratic);
GLHCKAPI const kmVec3* glhckLightGetAtten(glhckLight *object);
GLHCKAPI void glhckLightCutout(glhckLight *object, const kmVec2 *cutout);
GLHCKAPI void glhckLightCutoutf(glhckLight *object, kmScalar outer, kmScalar inner);
GLHCKAPI const kmVec2* glhckLightGetCutout(glhckLight *object);

/* renderbuffer objects */
GLHCKAPI glhckRenderbuffer* glhckRenderbufferNew(int width, int height, glhckTextureFormat format);
GLHCKAPI glhckRenderbuffer* glhckRenderbufferRef(glhckRenderbuffer *object);
GLHCKAPI unsigned int glhckRenderbufferFree(glhckRenderbuffer *object);
GLHCKAPI void glhckRenderbufferBind(glhckRenderbuffer *object);

/* framebuffer objects */
GLHCKAPI glhckFramebuffer* glhckFramebufferNew(glhckFramebufferTarget target);
GLHCKAPI glhckFramebuffer* glhckFramebufferRef(glhckFramebuffer *object);
GLHCKAPI unsigned int glhckFramebufferFree(glhckFramebuffer *object);
GLHCKAPI glhckFramebuffer* glhckFramebufferCurrentForTarget(glhckFramebufferTarget target);
GLHCKAPI void glhckFramebufferBind(glhckFramebuffer *object);
GLHCKAPI void glhckFramebufferUnbind(glhckFramebufferTarget target);
GLHCKAPI void glhckFramebufferBegin(glhckFramebuffer *object);
GLHCKAPI void glhckFramebufferEnd(glhckFramebuffer *object);
GLHCKAPI void glhckFramebufferRect(glhckFramebuffer *object, glhckRect *rect);
GLHCKAPI void glhckFramebufferRecti(glhckFramebuffer *object, int x, int y, int w, int h);
GLHCKAPI int glhckFramebufferAttachTexture(glhckFramebuffer *object, glhckTexture *texture, glhckFramebufferAttachmentType attachment);
GLHCKAPI int glhckFramebufferAttachRenderbuffer(glhckFramebuffer *object, glhckRenderbuffer *buffer, glhckFramebufferAttachmentType attachment);

/* hardware buffer objects */
GLHCKAPI glhckHwBuffer* glhckHwBufferNew(void);
GLHCKAPI glhckHwBuffer* glhckHwBufferRef(glhckHwBuffer *object);
GLHCKAPI unsigned int glhckHwBufferFree(glhckHwBuffer *object);
GLHCKAPI glhckHwBuffer* glhckHwBufferCurrentForTarget(glhckHwBufferTarget target);
GLHCKAPI void glhckHwBufferBind(glhckHwBuffer *object);
GLHCKAPI void glhckHwBufferUnbind(glhckHwBufferTarget target);
GLHCKAPI glhckHwBufferTarget glhckHwBufferGetType(glhckHwBuffer *object);
GLHCKAPI glhckHwBufferStoreType glhckHwBufferGetStoreType(glhckHwBuffer *object);
GLHCKAPI void glhckHwBufferCreate(glhckHwBuffer *object, glhckHwBufferTarget target, int size, const void *data, glhckHwBufferStoreType usage);
GLHCKAPI void glhckHwBufferBindBase(glhckHwBuffer *object, unsigned int index);
GLHCKAPI void glhckHwBufferBindRange(glhckHwBuffer *object, unsigned int index, int offset, int size);
GLHCKAPI void glhckHwBufferFill(glhckHwBuffer *object, int offset, int size, const void *data);
GLHCKAPI void* glhckHwBufferMap(glhckHwBuffer *object, glhckHwBufferAccessType access);
GLHCKAPI void glhckHwBufferUnmap(glhckHwBuffer *object);

/* uniform buffer specific */
GLHCKAPI int glhckHwBufferCreateUniformBufferFromShader(glhckHwBuffer *object, glhckShader *shader, const char *uboName, glhckHwBufferStoreType usage);
GLHCKAPI int glhckHwBufferAppendInformationFromShader(glhckHwBuffer *object, glhckShader *shader);
GLHCKAPI void glhckHwBufferFillUniform(glhckHwBuffer *object, const char *uniform, int size, const void *data);

/* shaders */
GLHCKAPI unsigned int glhckCompileShaderObject(glhckShaderType type, const char *effectKey, const char *contentsFromMemory);
GLHCKAPI void glhckDeleteShaderObject(unsigned int shaderObject);
GLHCKAPI glhckShader* glhckShaderNew(const char *vertexEffect, const char *fragmentEffect, const char *contentsFromMemory);
GLHCKAPI glhckShader* glhckShaderNewWithShaderObjects(unsigned int vertexShader, unsigned int fragmentShader);
GLHCKAPI glhckShader* glhckShaderRef(glhckShader *object);
GLHCKAPI unsigned int glhckShaderFree(glhckShader *object);
GLHCKAPI void glhckShaderBind(glhckShader *object);
GLHCKAPI void glhckShaderUniform(glhckShader *object, const char *uniform, int count, void *value);
GLHCKAPI int glhckShaderAttachHwBuffer(glhckShader *object, glhckHwBuffer *buffer, const char *name, unsigned int index);

/* trace && debug */
GLHCKAPI void glhckDebugHook(glhckDebugHookFunc func);
GLHCKAPI void glhckMemoryGraph(void);
GLHCKAPI void glhckRenderPrintObjectQueue(void);
GLHCKAPI void glhckRenderPrintTextureQueue(void);

/* vertexdata geometry */
GLHCKAPI void glhckGeometryCalculateBB(glhckGeometry *geometry, kmAABB *bb);
GLHCKAPI int glhckGeometryInsertVertices(glhckGeometry *geometry, unsigned char type, const void *data, int memb);
GLHCKAPI int glhckGeometryInsertIndices(glhckGeometry *geometry, unsigned char type, const void *data, int memb);

/* collisions
 * XXX: incomplete */
GLHCKAPI glhckCollisionWorld* glhckCollisionWorldNew(void *userData);
GLHCKAPI void glhckCollisionWorldFree(glhckCollisionWorld *object);
GLHCKAPI void* glhckCollisionPrimitiveGetUserData(const glhckCollisionPrimitive *object);
GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddAABBRef(glhckCollisionWorld *object, const kmAABB *aabb, void *userData);
GLHCKAPI glhckCollisionPrimitive* glhckCollisionWorldAddAABB(glhckCollisionWorld *object, const kmAABB *aabb, void *userData);
GLHCKAPI unsigned int glhckCollisionWorldCollideAABB(glhckCollisionWorld *object, const kmAABB *aabb, const glhckCollisionInData *data);

/* define cleanup */
#ifdef GLHCK_WINGDIAPI_DEFINED
#  undef WINGDIAPI
#  undef GLHCK_WINGDIAPI_DEFINED
#endif

#ifdef GLHCK_CALLBACK_DEFINED
#  undef CALLBACK
#  undef GLHCK_CALLBACK_DEFINED
#endif

#ifdef __cplusplus
}
#endif

#endif /* __glhck_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
