#ifndef __glhck_h__
#define __glhck_h__

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h> /* for default typedefs */
#include <kazmath/kazmath.h> /* glhck needs kazmath */
#include <kazmath/vec4.h> /* is not include by kazmat.h */

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

/* texture format */
#define GLHCK_RED             0x1903
#define GLHCK_GREEN           0x1904
#define GLHCK_BLUE            0x1905
#define GLHCK_ALPHA           0x1906
#define GLHCK_LUMINANCE       0x1909
#define GLHCK_LUMINANCE_ALPHA 0x190A
#define GLHCK_RGB             0x1907
#define GLHCK_RGBA            0x1908

/* compressed texture format */
/* FIXME: check available formats and etc bleh stuff */
#define GLHCK_COMPRESSED_RGB_DXT1  0x83F1
#define GLHCK_COMPRESSED_RGBA_DXT5 0x83F3

/* geometry type */
#define GLHCK_POINTS          0x0000
#define GLHCK_LINES           0x0001
#define GLHCK_LINE_LOOP       0x0002
#define GLHCK_LINE_STRIP      0x0003
#define GLHCK_TRIANGLES       0x0004
#define GLHCK_TRIANGLE_STRIP  0x0005

/* renderer properties */
#define GLHCK_MAX_TEXTURE_SIZE             0x0D33
#define GLHCK_MAX_CUBE_MAP_TEXTURE_SIZE    0x8510
#define GLHCK_MAX_VERTEX_ATTRIBS           0x8869
#define GLHCK_MAX_VERTEX_UNIFORM_VECTORS   0x8859
#define GLHCK_MAX_VARYING_VECTORS          0x8DFC
#define GLHCK_MAX_COMBINED_TEXTURE_IMAGE_UNITS 0x8B4D
#define GLHCK_MAX_VERTEX_TEXTURE_IMAGE_UNITS 0x8B4C
#define GLHCK_MAX_TEXTURE_IMAGE_UNITS      0x8872
#define GLHCK_MAX_FRAGMENT_UNIFORM_VECTORS 0x8DFD
#define GLHCK_MAX_RENDERBUFFER_SIZE        0x84E8
#define GLHCK_MAX_VIEWPORT_DIMS            0x0D3A

/* debugging level */
typedef enum glhckDebugLevel {
   GLHCK_DBG_ERROR,
   GLHCK_DBG_WARNING,
   GLHCK_DBG_CRAP,
} glhckDebugLevel;

/* render type */
typedef enum glhckRenderType {
   GLHCK_RENDER_AUTO,
   GLHCK_RENDER_OPENGL,
   GLHCK_RENDER_GLES,
   GLHCK_RENDER_NULL,
} glhckRenderType;

/* renderer flags (nothing here atm) */
typedef enum glhckRenderFlags {
   GLHCK_RENDER_FLAG_NONE = 0,
} glhckRenderFlags;

/* projection type */
typedef enum glhckProjectionType {
   GLHCK_PROJECTION_PERSPECTIVE,
   GLHCK_PROJECTION_ORTHOGRAPHIC,
} glhckProjectionType;

/* frustum planes */
typedef enum glhckFrustumPlanes {
   GLHCK_FRUSTUM_PLANE_LEFT,
   GLHCK_FRUSTUM_PLANE_RIGHT,
   GLHCK_FRUSTUM_PLANE_BOTTOM,
   GLHCK_FRUSTUM_PLANE_TOP,
   GLHCK_FRUSTUM_PLANE_NEAR,
   GLHCK_FRUSTUM_PLANE_FAR,
   GLHCK_FRUSTUM_PLANE_LAST
} glhckFrustumPlanes;

/* frustum corners */
typedef enum glhckFrustumCorners {
   GLHCK_FRUSTUM_CORNER_BOTTOM_LEFT,
   GLHCK_FRUSTUM_CORNER_BOTTOM_RIGHT,
   GLHCK_FRUSTUM_CORNER_TOP_RIGHT,
   GLHCK_FRUSTUM_CORNER_TOP_LEFT,
   GLHCK_FRUSTUM_CORNER_LAST
} glhckFrustumCorners;

/* frustum struct */
typedef struct glhckFrustum {
   kmPlane planes[GLHCK_FRUSTUM_PLANE_LAST];
   kmVec3 nearCorners[GLHCK_FRUSTUM_CORNER_LAST];
   kmVec3 farCorners[GLHCK_FRUSTUM_CORNER_LAST];
} glhckFrustum;

/* texture flags */
typedef enum glhckTextureFlags {
   GLHCK_TEXTURE_NONE = 0,
   GLHCK_TEXTURE_DEFAULTS = 1,
   GLHCK_TEXTURE_DXT = 2,
} glhckTextureFlags;

/* material flags */
typedef enum glhckMaterialFlags {
   GLHCK_MATERIAL_NONE      = 0,
   GLHCK_MATERIAL_DRAW_AABB = 1,
   GLHCK_MATERIAL_DRAW_OBB  = 2,
   GLHCK_MATERIAL_WIREFRAME = 4,
   GLHCK_MATERIAL_ALPHA     = 8,
   GLHCK_MATERIAL_COLOR     = 16,
   GLHCK_MATERIAL_CULL      = 32,
   GLHCK_MATERIAL_DEPTH     = 64
} glhckMaterialFlags;

/* rtt modes */
typedef enum glhckRttMode {
   GLHCK_RTT_RGB,
   GLHCK_RTT_RGBA
} glhckRttMode;

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

/* internal vertex data structures,
 * if you modify the object's vertexdata directly
 * make sure you cast to the right data type. */
typedef struct glhckVertexData3b {
   glhckVector3b vertex;
   glhckVector3b normal;
   glhckVector2b coord;
   glhckColorb color;
} glhckVertexData3b;

typedef struct glhckVertexData2b {
   glhckVector2b vertex;
   glhckVector2b coord;
   glhckColorb color;
} glhckVertexData2b;

typedef struct glhckVertexData3s {
   glhckVector3s vertex;
   glhckVector3s normal;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData3s;

typedef struct glhckVertexData2s {
   glhckVector2s vertex;
   glhckVector2s coord;
   glhckColorb color;
} glhckVertexData2s;

typedef struct glhckVertexData3f {
   glhckVector3f vertex;
   glhckVector3f normal;
   glhckVector2f coord;
   glhckColorb color;
} glhckVertexData3f;

typedef struct glhckVertexData2f {
   glhckVector2f vertex;
   glhckVector2f coord;
   glhckColorb color;
} glhckVertexData2f;

/* feed glhck the highest precision */
typedef glhckVertexData3f glhckImportVertexData;

typedef union glhckVertexData {
      glhckVertexData3b * v3b;
      glhckVertexData2b * v2b;
      glhckVertexData3s * v3s;
      glhckVertexData2s * v2s;
      glhckVertexData3f * v3f;
      glhckVertexData2f * v2f;
} glhckVertexData;

typedef unsigned char glhckIndexb;
typedef unsigned short glhckIndexs;
typedef unsigned int glhckIndexi;

/* feed glhck the highest precision */
typedef glhckIndexi glhckImportIndexData;

typedef union glhckIndexData {
      glhckIndexb * ivb;
      glhckIndexs * ivs;
      glhckIndexi * ivi;
} glhckIndexData;

/* geometry type enums */
typedef enum glhckGeometryVertexType {
   GLHCK_VERTEX_NONE,
   GLHCK_VERTEX_V3B,
   GLHCK_VERTEX_V2B,
   GLHCK_VERTEX_V3S,
   GLHCK_VERTEX_V2S,
   GLHCK_VERTEX_V3F,
   GLHCK_VERTEX_V2F
} glhckGeometryVertexType;

typedef enum glhckGeometryIndexType {
   GLHCK_INDEX_NONE,
   GLHCK_INDEX_BYTE,
   GLHCK_INDEX_SHORT,
   GLHCK_INDEX_INTEGER
} glhckGeometryIndexType;

/* teture coordinate transformation */
typedef struct glhckCoordTransform {
   short degrees;
   glhckRect transform;
} glhckCoordTransform;

/* datatype that presents object's geometry */
typedef struct glhckGeometry {
   /* geometry type (triangles, triangle strip, etc..) */
   unsigned int type;

   /* vertex && indices types */
   glhckGeometryVertexType vertexType;
   glhckGeometryIndexType indexType;

   /* counts for vertices && indices */
   size_t vertexCount;
   size_t indexCount;

   /* vertices */
   glhckVertexData vertices;

   /* indices */
   glhckIndexData indices;

   /* transformed coordinates */
   glhckCoordTransform *transformedCoordinates;

   /* geometry transformation needed
    * after precision conversion */
   glhckVector3f bias;
   glhckVector3f scale;

   /* transformed texture range
    * the texture matrix is scaled with
    * 1.0f/textureRange of geometry before
    * sending to OpenGL */
   unsigned short textureRange;
} glhckGeometry;

/* typedefs for better typing */
typedef struct _glhckTexture glhckTexture;
typedef struct _glhckAtlas   glhckAtlas;
typedef struct _glhckRtt     glhckRtt;
typedef struct _glhckObject  glhckObject;
typedef struct _glhckText    glhckText;
typedef struct _glhckCamera  glhckCamera;

typedef void (*glhckDebugHookFunc)(const char *file, int line, const char *function, glhckDebugLevel level, const char *str);

/* init && terminate */
GLHCKAPI int glhckInit(int argc, char **argv);
GLHCKAPI void glhckTerminate(void);
GLHCKAPI void glhckMassacreWorld(void);
GLHCKAPI int glhckInitialized(void);

/* display */
GLHCKAPI int glhckDisplayCreate(int width, int height, glhckRenderType renderType);
GLHCKAPI void glhckDisplayClose(void);
GLHCKAPI void glhckDisplayResize(int width, int height);

/* rendering */
GLHCKAPI void glhckSetGlobalPrecision(glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
GLHCKAPI void glhckRenderGetIntegerv(unsigned int pname, int *params);
GLHCKAPI void glhckRenderSetFlags(unsigned int flags);
GLHCKAPI void glhckRenderSetProjection(kmMat4 const* mat);
GLHCKAPI void glhckRender(void);
GLHCKAPI void glhckClear(void);

/* frustum */
GLHCKAPI void glhckFrustumBuild(glhckFrustum *frustum, const kmMat4 *mvp);
GLHCKAPI void glhckFrustumRender(glhckFrustum *frustum, const kmMat4 *model);

/* cameras */
GLHCKAPI glhckCamera* glhckCameraNew(void);
GLHCKAPI glhckCamera* glhckCameraRef(glhckCamera *camera);
GLHCKAPI short glhckCameraFree(glhckCamera *camera);
GLHCKAPI void glhckCameraUpdate(glhckCamera *camera);
GLHCKAPI void glhckCameraReset(glhckCamera *camera);
GLHCKAPI void glhckCameraProjection(glhckCamera *camera, const glhckProjectionType projectionType);
GLHCKAPI glhckFrustum* glhckCameraGetFrustum(glhckCamera *camera);
GLHCKAPI const kmMat4* glhckCameraGetViewMatrix(const glhckCamera *camera);
GLHCKAPI const kmMat4* glhckCameraGetProjectionMatrix(const glhckCamera *camera);
GLHCKAPI const kmMat4* glhckCameraGetMVPMatrix(const glhckCamera *camera);
GLHCKAPI void glhckCameraFov(glhckCamera *camera, const kmScalar fov);
GLHCKAPI void glhckCameraRange(glhckCamera *camera,
      const kmScalar near, const kmScalar far);
GLHCKAPI void glhckCameraViewport(glhckCamera *camera, const glhckRect *viewport);
GLHCKAPI void glhckCameraViewportf(glhckCamera *camera,
      int x, int y, int w, int h);
GLHCKAPI glhckObject* glhckCameraGetObject(const glhckCamera *camera);

/* objects */
GLHCKAPI glhckObject* glhckObjectNew(void);
GLHCKAPI glhckObject* glhckObjectCopy(const glhckObject *src);
GLHCKAPI glhckObject* glhckObjectRef(glhckObject *object);
GLHCKAPI short glhckObjectFree(glhckObject *object);
GLHCKAPI void glhckObjectAddChildren(glhckObject *object, glhckObject *child);
GLHCKAPI void glhckObjectRemoveChildren(glhckObject *object, glhckObject *child);
GLHCKAPI void glhckObjectRemoveAllChildren(glhckObject *object);
GLHCKAPI glhckTexture* glhckObjectGetTexture(const glhckObject *object);
GLHCKAPI void glhckObjectSetTexture(glhckObject *object, glhckTexture *texture);
GLHCKAPI void glhckObjectDraw(glhckObject *object);
GLHCKAPI void glhckObjectRender(glhckObject *object);
GLHCKAPI int glhckObjectInsertVertices(
      glhckObject *object, glhckGeometryVertexType type,
      const glhckImportVertexData *vertices, size_t memb);
GLHCKAPI int glhckObjectInsertIndices(
      glhckObject *object, glhckGeometryIndexType type,
      const glhckImportIndexData *indices, size_t memb);
GLHCKAPI void glhckObjectUpdate(glhckObject *object);

/* material */
GLHCKAPI const glhckColorb* glhckObjectGetColor(const glhckObject *object);
GLHCKAPI void glhckObjectColor(glhckObject *object, const glhckColorb *color);
GLHCKAPI void glhckObjectColorb(glhckObject *object,
      unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI unsigned int glhckObjectGetMaterialFlags(const glhckObject *object);
GLHCKAPI void glhckObjectMaterialFlags(glhckObject *object, unsigned int flags);

/* object control */
GLHCKAPI const kmAABB* glhckObjectGetOBB(const glhckObject *object);
GLHCKAPI const kmAABB* glhckObjectGetAABB(const glhckObject *object);
GLHCKAPI const kmVec3* glhckObjectGetPosition(const glhckObject *object);
GLHCKAPI void glhckObjectPosition(glhckObject *object, const kmVec3 *position);
GLHCKAPI void glhckObjectPositionf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectMove(glhckObject *object, const kmVec3 *move);
GLHCKAPI void glhckObjectMovef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetRotation(const glhckObject *object);
GLHCKAPI void glhckObjectRotation(glhckObject *object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotationf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI void glhckObjectRotate(glhckObject *object, const kmVec3 *rotate);
GLHCKAPI void glhckObjectRotatef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetTarget(const glhckObject *object);
GLHCKAPI void glhckObjectTarget(glhckObject *object, const kmVec3 *target);
GLHCKAPI void glhckObjectTargetf(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);
GLHCKAPI const kmVec3* glhckObjectGetScale(const glhckObject *object);
GLHCKAPI void glhckObjectScale(glhckObject *object, const kmVec3 *scale);
GLHCKAPI void glhckObjectScalef(glhckObject *object,
      const kmScalar x, const kmScalar y, const kmScalar z);

GLHCKAPI glhckGeometry* glhckObjectNewGeometry(glhckObject *object);
GLHCKAPI glhckGeometry* glhckObjectGetGeometry(const glhckObject *object);

/* pre-defined geometry */
GLHCKAPI glhckObject* glhckModelNew(const char *file, kmScalar size);
GLHCKAPI glhckObject* glhckModelNewEx(const char *file, kmScalar size,
         glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
GLHCKAPI glhckObject* glhckCubeNew(kmScalar size);
GLHCKAPI glhckObject* glhckCubeNewEx(kmScalar size,
            glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
GLHCKAPI glhckObject* glhckPlaneNew(kmScalar size);
GLHCKAPI glhckObject* glhckPlaneNewEx(kmScalar size,
            glhckGeometryIndexType itype, glhckGeometryVertexType vtype);
GLHCKAPI glhckObject* glhckSpriteNew(glhckTexture* texture, size_t x, size_t y);
GLHCKAPI glhckObject* glhckSpriteNewFromFile(const char *file,
      size_t width, size_t height, unsigned int flags);

/* text */
GLHCKAPI glhckText* glhckTextNew(int cachew, int cacheh);
GLHCKAPI short glhckTextFree(glhckText *text);
GLHCKAPI void glhckTextGetMetrics(glhckText *text, unsigned int font_id,
      float size, float *ascender, float *descender, float *lineh);
GLHCKAPI void glhckTextGetMinMax(glhckText *text, int font_id, float size,
      const char *s, kmVec2 *min, kmVec2 *max);
GLHCKAPI void glhckTextColor(glhckText *text,
      unsigned char r, unsigned char g, unsigned char b, unsigned char a);
GLHCKAPI const glhckColorb* glhckGetTextColor(glhckText *text);
GLHCKAPI unsigned int glhckTextNewFontFromMemory(glhckText *text,
      unsigned char *data);
GLHCKAPI unsigned int glhckTextNewFont(glhckText *text, const char *file);
GLHCKAPI unsigned int glhckTextNewFontFromBitmap(glhckText *text,
      const char *file, int ascent, int descent, int line_gap);
GLHCKAPI void glhckTextNewGlyph(glhckText *text,
      unsigned int font_id, const char *s,
      short size, short base, int x, int y, int w, int h,
      float xoff, float yoff, float xadvance);
GLHCKAPI void glhckTextRender(glhckText *text);
GLHCKAPI void glhckTextDraw(glhckText *text, unsigned int font_id,
      float size, float x, float y, const char *s, float *dx);

/* textures */
GLHCKAPI glhckTexture* glhckTextureNew(const char *file, unsigned int flags);
GLHCKAPI glhckTexture* glhckTextureCopy(glhckTexture *src);
GLHCKAPI glhckTexture* glhckTextureRef(glhckTexture *texture);
GLHCKAPI short glhckTextureFree(glhckTexture *texture);
GLHCKAPI int glhckTextureCreate(glhckTexture *texture, unsigned char *data,
      int width, int height, unsigned int format, unsigned int outFormat, unsigned int flags);
GLHCKAPI int glhckTextureRecreate(glhckTexture *texture, unsigned char *data, unsigned int format);
GLHCKAPI int glhckTextureSave(glhckTexture *texture, const char *path);
GLHCKAPI void glhckTextureBind(glhckTexture *texture);
GLHCKAPI void glhckBindTexture(unsigned int texture);

/* texture atlases */
GLHCKAPI glhckAtlas* glhckAtlasNew(void);
GLHCKAPI short glhckAtlasFree(glhckAtlas *atlas);
GLHCKAPI int glhckAtlasInsertTexture(glhckAtlas *atlas, glhckTexture *texture);
GLHCKAPI glhckTexture* glhckAtlasGetTexture(glhckAtlas *atlas);
GLHCKAPI int glhckAtlasPack(glhckAtlas *atlas, const int power_of_two,
      const int border);
GLHCKAPI glhckTexture* glhckAtlasGetTextureByIndex(const glhckAtlas *atlas,
      unsigned short index);
GLHCKAPI int glhckAtlasGetTransform(const glhckAtlas *atlas, glhckTexture *texture,
      glhckRect *transformed, short *degrees);
GLHCKAPI int glhckAtlasTransformCoordinates(const glhckAtlas *atlas, glhckTexture *texture,
      const kmVec2 *in, kmVec2 *out);

/* render to texture */
GLHCKAPI glhckRtt* glhckRttNew(int width, int height, glhckRttMode mode);
GLHCKAPI short glhckRttFree(glhckRtt *rtt);
GLHCKAPI int glhckRttFillData(glhckRtt *rtt);
GLHCKAPI glhckTexture* glhckRttGetTexture(const glhckRtt *rtt);
GLHCKAPI void glhckRttBegin(const glhckRtt *rtt);
GLHCKAPI void glhckRttEnd(const glhckRtt *rtt);

/* trace && debug */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func);
GLHCKAPI void glhckMemoryGraph(void);
GLHCKAPI void glhckPrintObjectQueue(void);
GLHCKAPI void glhckPrintTextureQueue(void);

/* vertexdata geometry */
GLHCKAPI size_t glhckIndexTypeElementSize(glhckGeometryIndexType type);
GLHCKAPI glhckIndexi glhckIndexTypeMaxPrecision(glhckGeometryIndexType type);
GLHCKAPI const char* glhckIndexTypeString(glhckGeometryIndexType type);
GLHCKAPI int glhckIndexTypeWithinRange(unsigned int value, glhckGeometryIndexType type);
GLHCKAPI glhckGeometryIndexType glhckIndexTypeForValue(unsigned int value);

GLHCKAPI size_t glhckVertexTypeElementSize(glhckGeometryVertexType type);
GLHCKAPI float glhckVertexTypeMaxPrecision(glhckGeometryVertexType type);
GLHCKAPI const char* glhckVertexTypeString(glhckGeometryVertexType type);
GLHCKAPI glhckGeometryVertexType glhckVertexTypeGetV2Counterpart(glhckGeometryVertexType type);
GLHCKAPI int glhckVertexTypeWithinRange(float value, glhckGeometryVertexType type);
GLHCKAPI glhckGeometryVertexType glhckVertexTypeForSize(size_t width, size_t height);
GLHCKAPI int glhckVertexTypeHasNormal(glhckGeometryVertexType type);
GLHCKAPI int glhckVertexTypeHasColor(glhckGeometryVertexType type);

GLHCKAPI void glhckGeometryVertexDataForIndex(
      glhckGeometry *geometry, glhckIndexi ix,
      glhckVector3f *vertex, glhckVector3f *normal,
      glhckVector2f *coord, glhckColorb *color,
      unsigned int *vmagic);
GLHCKAPI void glhckGeometrySetVertexDataForIndex(
      glhckGeometry *geometry, glhckIndexi ix,
      glhckVector3f *vertex, glhckVector3f *normal,
      glhckVector2f *coord, glhckColorb *color);
GLHCKAPI void glhckGeometryTransformCoordinates(
      glhckGeometry *geometry, const glhckRect *transformed, short degrees);
GLHCKAPI void glhckGeometryCalculateBB(glhckGeometry *geometry, kmAABB *bb);
GLHCKAPI int glhckGeometrySetVertices(glhckGeometry *geometry,
      glhckGeometryVertexType type, void *data, size_t memb);
GLHCKAPI int glhckGeometrySetIndices(glhckGeometry *geometry,
      glhckGeometryIndexType type, void *data, size_t memb);

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
