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
   unsigned int indicesCount;
} __OpenGLrender;
static __OpenGLrender _OpenGL;

/* helper macros */
#define DECLARE_GL_GEN_FUNC(x,y)                                     \
static void x(unsigned int count, unsigned int *objects)             \
{                                                                    \
   CALL("%d, %p", count, objects);                                   \
   y(count, objects);                                                \
}

#define DECLARE_GL_BIND_FUNC(x,y)                                    \
static void x(unsigned int object)                                   \
{                                                                    \
   CALL("%d", object);                                               \
   y;                                                                \
}

#define GL_ERROR(x)                                                  \
{                                                                    \
GLenum error;                                                        \
if (GL_DEBUG) {                                                      \
   if ((error = glGetError())                                        \
         != GL_NO_ERROR)                                             \
      DEBUG(GLHCK_DBG_ERROR, "GL @%-20s %-20s >> %s",                \
            __func__, x,                                             \
            error==GL_INVALID_ENUM?                                  \
            "GL_INVALID_ENUM":                                       \
            error==GL_INVALID_VALUE?                                 \
            "GL_INVALID_VALUE":                                      \
            error==GL_INVALID_OPERATION?                             \
            "GL_INVALID_OPERATION":                                  \
            error==GL_STACK_OVERFLOW?                                \
            "GL_STACK_OVERFLOW":                                     \
            error==GL_STACK_UNDERFLOW?                               \
            "GL_STACK_UNDERFLOW":                                    \
            error==GL_OUT_OF_MEMORY?                                 \
            "GL_OUT_OF_MEMORY":                                      \
            error==GL_TABLE_TOO_LARGE?                               \
            "GL_TABLE_TOO_LARGE":                                    \
            error==GL_INVALID_OPERATION?                             \
            "GL_INVALID_OPERATION":                                  \
            "GL_UNKNOWN_ERROR");                                     \
}                                                                    \
}

static void geometryPointer(__GLHCKobjectGeometry *geometry)
{
   /* vertex data */
   if (geometry->vertexData) {
      glVertexPointer(3, GLHCK_PRECISION_VERTEX,
            sizeof(glhckVertexData), &geometry->vertexData[0].vertex);
      glNormalPointer(GLHCK_PRECISION_VERTEX,
            sizeof(glhckVertexData), &geometry->vertexData[0].normal);
#if GLHCK_VERTEXDATA_COLOR
      glColorPointer(4, GLHCK_PRECISION_COLOR,
            sizeof(glhckVertexData), &geometry->vertexData[0].color);
#endif
      GL_ERROR("glVertexPointer()");
   }

   /* texture data */
   if (geometry->textureData) {
      glTexCoordPointer(2, GLHCK_PRECISION_COORD,
            sizeof(glhckTextureData), &geometry->textureData[0].coord);
      GL_ERROR("glVertexPointer()");
   }
}

static int renderInfo(void)
{
   char *version, *vendor, *extensions;
   unsigned int major, minor, patch;
   int num_parsed;
   TRACE();

   version = (char*)glGetString(GL_VERSION);
   vendor  = (char*)glGetString(GL_VENDOR);

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
   if ((extensions = (char*)glGetString(GL_EXTENSIONS)))
      DEBUG(3, extensions);

   RET("%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET("%d", RETURN_FAIL);
   return RETURN_FAIL;
}

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
   _OpenGL.indicesCount  = 0;
}

static void objectDraw(_glhckObject *object)
{
   kmMat4 projection;
   unsigned int i;
   CALL("%p", object);

   glMatrixMode(GL_PROJECTION);
   kmMat4PerspectiveProjection(&projection, 35, 1, 0.1f, 300);
   glLoadMatrixf((float*)&projection);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0,0,-10.0f);

   glEnableClientState(GL_VERTEX_ARRAY);
   GL_ERROR("glEnableClientState()");

   glColor3f(1,1,1);
   GL_ERROR("glColor3f()");

   geometryPointer(&object->geometry);

   glDrawElements(GL_TRIANGLE_STRIP, object->geometry.indicesCount,
         GLHCK_PRECISION_INDEX,
         &object->geometry.indices[0]);
   GL_ERROR("glDrawElements()");

   glDisableClientState(GL_VERTEX_ARRAY);
   GL_ERROR("glDisableClientState()");

   _OpenGL.indicesCount  += object->geometry.indicesCount;
}

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
   glClear(GL_COLOR_BUFFER_BIT);

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

   /* terminate function */
   GLHCK_RENDER_FUNC(terminate, terminate);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}
