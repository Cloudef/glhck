#include "../internal.h"
#include "../../include/SOIL.h"
#include "../../include/kazmath/kazmath.h"
#include "render.h"
#include <limits.h>
#include <stdio.h>   /* for sscanf */
#include <GL/glew.h> /* for opengl */

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#define RENDER_NAME "OpenGL Renderer"
#define GL_DEBUG 1

/* global data */
typedef struct __OpenGLrender {
   size_t indicesCount;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* declare gl generation function */
#define DECLARE_GL_GEN_FUNC(x,y)                                     \
static void x(unsigned int count, unsigned int *objects)             \
{                                                                    \
   CALL("%d, %p", count, objects);                                   \
   y(count, objects);                                                \
}

/* declare gl bind function */
#define DECLARE_GL_BIND_FUNC(x,y)                                    \
static void x(unsigned int object)                                   \
{                                                                    \
   CALL("%d", object);                                               \
   y;                                                                \
}

/* check gl errors on debug build */
#ifdef NDEBUG
#  define GL_CALL(x) x
#else
#  define GL_CALL(x) x; GL_ERROR(__func__, __STRING(x));
static inline void GL_ERROR(const char *func, const char *glfunc)
{
   GLenum error;
   if ((error = glGetError())
         != GL_NO_ERROR)
      DEBUG(GLHCK_DBG_ERROR, "GL @%-20s %-20s >> %s",
            func, glfunc,
            error==GL_INVALID_ENUM?
            "GL_INVALID_ENUM":
            error==GL_INVALID_VALUE?
            "GL_INVALID_VALUE":
            error==GL_INVALID_OPERATION?
            "GL_INVALID_OPERATION":
            error==GL_STACK_OVERFLOW?
            "GL_STACK_OVERFLOW":
            error==GL_STACK_UNDERFLOW?
            "GL_STACK_UNDERFLOW":
            error==GL_OUT_OF_MEMORY?
            "GL_OUT_OF_MEMORY":
            error==GL_TABLE_TOO_LARGE?
            "GL_TABLE_TOO_LARGE":
            error==GL_INVALID_OPERATION?
            "GL_INVALID_OPERATION":
            "GL_UNKNOWN_ERROR");
}
#endif

static int uploadTexture(_glhckTexture *texture, unsigned int flags)
{
   CALL("%p, %d", texture, flags);
   texture->object = SOIL_create_OGL_texture(
         texture->data,
         texture->width, texture->height,
         texture->channels,
         texture->object?texture->object:0,
         flags);

   RET("%d", texture->object?RETURN_OK:RETURN_FAIL);
   return texture->object?RETURN_OK:RETURN_FAIL;
}

static unsigned int createTexture(const unsigned char *const buffer,
      int width, int height, int channels,
      unsigned int reuse_texture_ID,
      unsigned int flags)
{
   unsigned int object;
   CALL("%p, %d, %d, %d, %d, %d", buffer,
         width, height, channels,
         reuse_texture_ID, flags);

   object = SOIL_create_OGL_texture(
      buffer, width, height, channels,
      reuse_texture_ID,
      flags);

   RET("%d", object?RETURN_OK:RETURN_FAIL);
   return object?RETURN_OK:RETURN_FAIL;
}

static void render(void)
{
   kmMat4 projection;
   TRACE();

   /* reset stats */
   _OpenGL.indicesCount = 0;
}

/* \brief pass interleaved vertex data to OpenGL nicely. */
static inline void geometryPointer(__GLHCKobjectGeometry *geometry)
{
   /* vertex data */
   GL_CALL(glVertexPointer(3, GLHCK_PRECISION_VERTEX,
            sizeof(glhckVertexData), &geometry->vertexData[0].vertex));
   GL_CALL(glNormalPointer(GLHCK_PRECISION_VERTEX,
            sizeof(glhckVertexData), &geometry->vertexData[0].normal));
   GL_CALL(glTexCoordPointer(2, GLHCK_PRECISION_COORD,
            sizeof(glhckVertexData), &geometry->vertexData[0].coord));
#if GLHCK_VERTEXDATA_COLOR
   GL_CALL(glColorPointer(4, GLHCK_PRECISION_COLOR,
            sizeof(glhckVertexData), &geometry->vertexData[0].color));
#endif
}

/* \brief draw interleaved geometry */
static inline void geometryDraw(__GLHCKobjectGeometry *geometry)
{
   if (geometry->indices) {
      GL_CALL(glDrawElements(GL_TRIANGLE_STRIP, geometry->indicesCount,
               GLHCK_PRECISION_INDEX, &geometry->indices[0]));
   } else {
      GL_CALL(glDrawArrays(GL_TRIANGLE_STRIP, 0, geometry->vertexCount));
   }
}

/* \brief draw single object
 * TODO: In future just add to stack,
 * and handle actual drawing on glRender()?
 *
 * We also need sorting and other crap... */
static void objectDraw(_glhckObject *object)
{
   kmMat4 projection;
   unsigned int i;
   CALL("%p", object);

   /* no point drawing without vertex data */
   if (!object->geometry.vertexData)
      return;

   /* GL_CALL(glEnable(GL_DEPTH_TEST)); */
   GL_CALL(glCullFace(GL_BACK));
   GL_CALL(glEnable(GL_CULL_FACE));

   GL_CALL(glMatrixMode(GL_PROJECTION));
   kmMat4PerspectiveProjection(&projection, 35,
         (float)_GLHCKlibrary.render.width/
         (float)_GLHCKlibrary.render.height, 0.1f, 300);
   GL_CALL(glLoadMatrixf((float*)&projection));
   GL_CALL(glMatrixMode(GL_MODELVIEW));
   GL_CALL(glLoadIdentity());
   GL_CALL(glTranslatef(0,0,-10.0f));
   GL_CALL(glRotatef(25,45,15,1));

   GL_CALL(glEnable(GL_TEXTURE_2D));
   GL_CALL(glEnableClientState(GL_VERTEX_ARRAY));
   GL_CALL(glEnableClientState(GL_TEXTURE_COORD_ARRAY));

   geometryPointer(&object->geometry);
   geometryDraw(&object->geometry);

   GL_CALL(glDisableClientState(GL_TEXTURE_COORD_ARRAY));
   GL_CALL(glDisableClientState(GL_VERTEX_ARRAY));

   _OpenGL.indicesCount += object->geometry.indicesCount;
}

/* \brief get render information */
static int renderInfo(void)
{
   char *version, *vendor, *extensions;
   unsigned int major, minor, patch;
   int num_parsed;
   TRACE();

   version = (char*)GL_CALL(glGetString(GL_VERSION));
   vendor  = (char*)GL_CALL(glGetString(GL_VENDOR));

   if (!version || !vendor)
      goto fail;

   num_parsed = sscanf(version, "%u.%u.%u",
         &major, &minor, &patch);

   if (num_parsed == 1) {
      minor = 0;
      patch = 0;
   } else if (num_parsed == 2)
      patch = 0;

   DEBUG(3, "%s [%u.%u.%u] [%s]\n", RENDER_NAME, major, minor, patch, vendor);
   extensions = (char*)GL_CALL(glGetString(GL_EXTENSIONS));

   if (extensions)
      DEBUG(3, extensions);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief resize viewport */
static void resize(int width, int height)
{
   CALL("%d, %d", width, height);
   GL_CALL(glViewport(0, 0, width, height));
}

/* \brief terminate renderer */
static void terminate(void)
{
   TRACE();

   /* this tells library that we are no longer alive. */
   GLHCK_RENDER_TERMINATE(RENDER_NAME);
}

/* OpenGL bindings */
DECLARE_GL_GEN_FUNC(_glGenTextures, glGenTextures)
DECLARE_GL_GEN_FUNC(_glDeleteTextures, glDeleteTextures)
DECLARE_GL_GEN_FUNC(_glGenBuffers, glGenBuffers)
DECLARE_GL_GEN_FUNC(_glDeleteBuffers, glDeleteBuffers)
DECLARE_GL_BIND_FUNC(_glBindTexture, glBindTexture(GL_TEXTURE_2D, object))

void _glhckRenderOpenGL(void)
{
   TRACE();

   /* init OpenGL context */
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT));
   resize(_GLHCKlibrary.render.width, _GLHCKlibrary.render.height);

   /* we use GLEW */
   if (glewInit() != GLEW_OK)
      goto fail;

   /* output information */
   if (renderInfo() != RETURN_OK)
      goto fail;

   /* reset global data */
   memset(&_OpenGL, 0, sizeof(__OpenGLrender));

   /* register api functions */
   GLHCK_RENDER_FUNC(generateTextures, _glGenTextures);
   GLHCK_RENDER_FUNC(deleteTextures, _glDeleteTextures);
   GLHCK_RENDER_FUNC(generateBuffers, _glGenBuffers);
   GLHCK_RENDER_FUNC(deleteBuffers, _glDeleteBuffers);
   GLHCK_RENDER_FUNC(bindTexture, _glBindTexture);
   GLHCK_RENDER_FUNC(uploadTexture, uploadTexture);
   GLHCK_RENDER_FUNC(createTexture, createTexture);

   /* drawing functions */
   GLHCK_RENDER_FUNC(render, render);
   GLHCK_RENDER_FUNC(objectDraw, objectDraw);

   /* common */
   GLHCK_RENDER_FUNC(resize, resize);
   GLHCK_RENDER_FUNC(terminate, terminate);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}
