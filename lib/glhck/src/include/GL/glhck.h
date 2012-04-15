#ifndef __glhck_h__
#define __glhck_h__

#ifdef __cplusplus
extern "C" {
#endif

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

/* public api below */
typedef enum glhckDebugLevel {
   GLHCK_DBG_ERROR,
   GLHCK_DBG_WARNING,
   GLHCK_DBG_CRAP,
} glhckDebugLevel;

typedef enum glhckRenderType {
   GLHCK_RENDER_NONE,
   GLHCK_RENDER_OPENGL,
   GLHCK_RENDER_GLES,
   GLHCK_RENDER_NULL,
} glhckRenderType;

/* texture flags */
typedef enum glhckTextureFlags
{
   GLHCK_TEXTURE_POWER_OF_TWO    = 1,
   GLHCK_TEXTURE_MIPMAPS         = 2,
   GLHCK_TEXTURE_REPEATS         = 4,
   GLHCK_TEXTURE_MULTIPLY_ALPHA  = 8,
   GLHCK_TEXTURE_INVERT_Y        = 16,
   GLHCK_TEXTURE_DXT             = 32,
   GLHCK_TEXTURE_DDS_LOAD_DIRECT = 64,
   GLHCK_TEXTURE_NTSC_SAFE_RGB   = 128,
   GLHCK_TEXTURE_CoCg_Y          = 256,
   GLHCK_TEXTURE_RECTANGLE       = 512,
   GLHCK_TEXTURE_DEFAULTS        = 1024
} glhckTextureFlags;

/* TODO:
 * Move these to saner place,
 * and make them easier to customize */
#define GLHCK_BYTE            0x1400
#define GLHCK_UNSIGNED_BYTE   0x1401
#define GLHCK_SHORT           0x1402
#define GLHCK_UNSIGNED_SHORT  0x1403
#define GLHCK_INT             0x1404
#define GLHCK_UNSIGNED_INT    0x1405
#define GLHCK_FLOAT           0x1406

#define GLHCK_PRECISION_VERTEX GLHCK_SHORT
#define GLHCK_PRECISION_COLOR  GLHCK_UNSIGNED_BYTE
#define GLHCK_PRECISION_COORD  GLHCK_SHORT
#define GLHCK_PRECISION_INDEX  GLHCK_UNSIGNED_SHORT
#define GLHCK_VERTEXDATA_COLOR 1

#if GLHCK_PRECISION_VERTEX == GLHCK_BYTE
#  define GLHCK_CAST_VERTEX char
#elif GLHCK_PRECISION_VERTEX == GLHCK_SHORT
#  define GLHCK_CAST_VERTEX short
#else
#  define GLHCK_CAST_VERTEX float
#endif

#if GLHCK_PRECISION_COLOR == GLHCK_UNSIGNED_BYTE
#  define GLHCK_CAST_COLOR unsigned char
#else
#  define GLHCK_CAST_COLOR float
#endif

#if GLHCK_PRECISION_COORD == GLHCK_BYTE
#  define GLHCK_CAST_COORD char
#elif GLHCK_PRECISION_COORD == GLHCK_SHORT
#  define GLHCK_CAST_COORD short
#else
#  define GLHCK_CAST_COORD float
#endif

#if GLHCK_PRECISION_INDEX == GLHCK_UNSIGNED_BYTE
#  define GLHCK_CAST_INDEX unsigned char
#elif GLHCK_PRECISION_INDEX == GLHCK_UNSIGNED_SHORT
#  define GLHCK_CAST_INDEX unsigned short
#else
#  define GLHCK_CAST_INDEX unsigned int
#endif

typedef struct glhckVertex3d
{
   GLHCK_CAST_VERTEX x, y, z;
} glhckVertex3d;

typedef struct glhckColor4d
{
   GLHCK_CAST_COLOR r, g, b, a;
} glhckColor4d;

typedef struct glhckCoord2d {
   GLHCK_CAST_COORD u, v;
} glhckCoord2d;

typedef struct glhckVertexData
{
   struct glhckVertex3d vertex;
   struct glhckVertex3d normal;
   struct glhckCoord2d  coord;
#if GLHCK_VERTEXDATA_COLOR
   struct glhckColor4d  color;
#endif
} glhckVertexData;

typedef struct _glhckTexture glhckTexture;
typedef struct _glhckObject  glhckObject;

typedef void (*glhckDebugHookFunc)(const char *file, int line, const char *function, glhckDebugLevel level, const char *str);

/* init && terminate */
GLHCKAPI int glchkInit(int argc, char **argv);
GLHCKAPI void glhckTerminate(void);

/* display */
GLHCKAPI int glchkDisplayCreate(int width, int height, glhckRenderType renderType);
GLHCKAPI void glhckDisplayClose(void);
GLHCKAPI void glhckDisplayResize(int width, int height);

/* render */
GLHCKAPI void glhckRender(void);

/* objects */
GLHCKAPI glhckObject* glhckObjectNew(void);
GLHCKAPI short glhckObjectFree(glhckObject *object);
GLHCKAPI int glhckObjectInsertVertexData(glhckObject *object,
      unsigned int count, const glhckVertexData *vertexData);
GLHCKAPI int glhckObjectInsertIndices(glhckObject *object,
      unsigned int count, const GLHCK_CAST_INDEX *indices);

/* geometry */
GLHCKAPI glhckObject* glhckCubeNew(int size);

/* textures */
GLHCKAPI glhckTexture* glhckTextureNew(const char *file, unsigned int flags);
GLHCKAPI glhckTexture* glhckTextureCopy(glhckTexture *src);
GLHCKAPI glhckTexture* glhckTextureRef(glhckTexture *texture);
GLHCKAPI short glhckTextureFree(glhckTexture *texture);
GLHCKAPI int glhckTextureCreate(glhckTexture *texture, unsigned char *data,
                              int width, int height, int channels, unsigned int flags);
GLHCKAPI int glhckTextureSave(glhckTexture *texture, const char *path);
GLHCKAPI void glhckTextureBind(glhckTexture *texture);
GLHCKAPI void glhckBindTexturei(unsigned int texture);

/* trace && debug */
GLHCKAPI void glhckSetDebugHook(glhckDebugHookFunc func);
GLHCKAPI void glhckMemoryGraph(void);

/* Define cleanup */
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
