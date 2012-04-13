#include "../internal.h"
#include "../../include/SOIL.h"
#include "render.h"
#include <stdio.h>   /* for sscanf */
#include <GL/glew.h> /* for opengl */

#define RENDER_NAME "OpenGL Renderer"

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

   /* register api functions */
   GLHCK_RENDER_FUNC(generateTextures, _glGenTextures);
   GLHCK_RENDER_FUNC(deleteTextures, _glDeleteTextures);
   GLHCK_RENDER_FUNC(generateBuffers, _glGenBuffers);
   GLHCK_RENDER_FUNC(deleteBuffers, _glDeleteBuffers);
   GLHCK_RENDER_FUNC(bindTexture, _glBindTexture);
   GLHCK_RENDER_FUNC(uploadTexture, uploadTexture);
   GLHCK_RENDER_FUNC(createTexture, createTexture);

   /* terminate function */
   GLHCK_RENDER_FUNC(terminate, terminate);

   /* this also tells library that everything went OK,
    * so do it last */
   GLHCK_RENDER_INIT(RENDER_NAME);

fail:
   return;
}
