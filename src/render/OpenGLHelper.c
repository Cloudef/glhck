#include "../internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#include "OpenGLHelper.h"

/*
 * glhck to OpenGL mappings
 */

/* \brief map glhck rtt attachment to OpenGL attachment */
unsigned int glhAttachmentTypeForGlhckType(_glhckRttAttachmentType type)
{
   switch (type) {
      case GLHCK_COLOR_ATTACHMENT0:
         return GL_COLOR_ATTACHMENT0;
      case GLHCK_COLOR_ATTACHMENT1:
         return GL_COLOR_ATTACHMENT1;
      case GLHCK_COLOR_ATTACHMENT2:
         return GL_COLOR_ATTACHMENT2;
      case GLHCK_COLOR_ATTACHMENT3:
         return GL_COLOR_ATTACHMENT3;
      case GLHCK_COLOR_ATTACHMENT4:
         return GL_COLOR_ATTACHMENT4;
      case GLHCK_COLOR_ATTACHMENT5:
         return GL_COLOR_ATTACHMENT5;
      case GLHCK_COLOR_ATTACHMENT6:
         return GL_COLOR_ATTACHMENT6;
      case GLHCK_COLOR_ATTACHMENT7:
         return GL_COLOR_ATTACHMENT7;
      case GLHCK_COLOR_ATTACHMENT8:
         return GL_COLOR_ATTACHMENT8;
      case GLHCK_COLOR_ATTACHMENT9:
         return GL_COLOR_ATTACHMENT9;
      case GLHCK_COLOR_ATTACHMENT10:
         return GL_COLOR_ATTACHMENT10;
      case GLHCK_COLOR_ATTACHMENT11:
         return GL_COLOR_ATTACHMENT11;
      case GLHCK_COLOR_ATTACHMENT12:
         return GL_COLOR_ATTACHMENT12;
      case GLHCK_COLOR_ATTACHMENT13:
         return GL_COLOR_ATTACHMENT13;
      case GLHCK_COLOR_ATTACHMENT14:
         return GL_COLOR_ATTACHMENT14;
      case GLHCK_COLOR_ATTACHMENT15:
         return GL_COLOR_ATTACHMENT15;
      case GLHCK_DEPTH_ATTACHMENT:
         return GL_DEPTH_ATTACHMENT;
      case GLHCK_STENCIL_ATTACHMENT:
         return GL_STENCIL_ATTACHMENT;
   }
   return 0;
}

/*
 * shared OpenGL renderer functions
 */

/* \brief get OpenGL parameters */
void glhGetIntegerv(unsigned int pname, int *params)
{
   CALL(1, "%u, %p", pname, params);
   GL_CALL(glGetIntegerv(pname, params));
}

/* \brief clear OpenGL buffers */
void glhClear(void)
{
   TRACE(2);

   /* clear buffers, FIXME: keep track of em */
   GL_CALL(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));
}

/* \brief set OpenGL clear color */
void glhClearColor(char r, char g, char b, char a)
{
   TRACE(1);
   float fr = (float)r/255, fg = (float)g/255, fb = (float)b/255;
   float fa = (float)a/255;

   GL_CALL(glClearColor(fr, fg, fb, fa));
   GLHCKRD()->clearColor.r = r;
   GLHCKRD()->clearColor.g = g;
   GLHCKRD()->clearColor.b = b;
   GLHCKRD()->clearColor.a = a;
}

/* \brief get pixels from OpenGL */
void glhBufferGetPixels(int x, int y, int width, int height,
      unsigned int format, unsigned char *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %p",
         x, y, width, height, format, data);
   GL_CALL(glReadPixels(x, y, width, height,
            format, GL_UNSIGNED_BYTE, data));
}

/* \brief create texture from data and upload it to OpenGL */
unsigned int glhTextureCreate(const unsigned char *buffer, size_t size,
      int width, int height, unsigned int format, unsigned int reuse_texture_ID, unsigned int flags)
{
   unsigned int object;
   CALL(0, "%p, %zu, %d, %d, %d, %d", buffer, size,
         width, height, format, reuse_texture_ID);

   /* create empty texture */
   if (!(object = reuse_texture_ID)) {
      GL_CALL(glGenTextures(1, &object));
   }

   /* fail? */
   if (!object)
      goto _return;

   glhckBindTexture(object);
   if (flags & GLHCK_TEXTURE_NEAREST) {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
   } else {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
   }

   if (flags & GLHCK_TEXTURE_REPEAT) {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT));
   } else {
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
      GL_CALL(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
   }

   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexImage2D(GL_TEXTURE_2D, 0,
               format, width, height, 0, size, buffer));
   } else {
      GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0,
               format, width, height, 0,
               format, GL_UNSIGNED_BYTE, buffer));
   }

_return:
   RET(0, "%d", object);
   return object;
}

/* \brief fill texture with data */
void glhTextureFill(unsigned int texture, const unsigned char *data, size_t size,
      int x, int y, int width, int height, unsigned int format)
{
   CALL(1, "%d, %p, %zu, %d, %d, %d, %d, %d", texture,
         data, size, x, y, width, height, format );

   glhckBindTexture(texture);
   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, format, size, data));
   } else{
      GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, format, GL_UNSIGNED_BYTE, data));
   }
}

/* \brief link FBO with texture attachmetn */
int glhFramebufferLinkWithTexture(unsigned int object,
      unsigned int texture, _glhckRttAttachmentType type)
{
   unsigned int attachment;
   CALL(0, "%d, %d, %d", object, texture, type);

   attachment = glhAttachmentTypeForGlhckType(type);
   GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, object));
   GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, attachment, GL_TEXTURE_2D, texture, 0));

   if (GL_CHECK(glCheckFramebufferStatus(GL_FRAMEBUFFER)) != GL_FRAMEBUFFER_COMPLETE)
      goto fbo_fail;

   GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, 0));
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fbo_fail:
   DEBUG(GLHCK_DBG_ERROR, "Framebuffer is not complete");
   glBindFramebuffer(GL_FRAMEBUFFER, 0);
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* helper macro for passing indices to OpenGL */
#define indicesToOpenGL(iprec, tunion)                \
   GL_CALL(glDrawElements(type, geometry->indexCount, \
            iprec, &geometry->indices.tunion[0]))

/* \brief draw interleaved geometry */
inline void glhGeometryRender(const glhckGeometry *geometry, unsigned int type)
{
   // printf("%s (%zu)\n", glhckIndexTypeString(geometry->indexType), geometry->indexCount);

   if (geometry->indexType != GLHCK_INDEX_NONE) {
      switch (geometry->indexType) {
         case GLHCK_INDEX_BYTE:
            indicesToOpenGL(GL_UNSIGNED_BYTE, ivb);
            break;

         case GLHCK_INDEX_SHORT:
            indicesToOpenGL(GL_UNSIGNED_SHORT, ivs);
            break;

         case GLHCK_INDEX_INTEGER:
            indicesToOpenGL(GL_UNSIGNED_INT, ivi);
            break;

         default:
            break;
      }
   } else {
      GL_CALL(glDrawArrays(type, 0, geometry->vertexCount));
   }
}

/* the helper is no longer needed */
#undef indicesToOpenGL

/* vim: set ts=8 sw=3 tw=0 :*/
