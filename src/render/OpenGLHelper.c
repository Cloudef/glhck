#include "../internal.h"

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#include "OpenGLHelper.h"

/*
 * glhck to OpenGL mappings
 */

/* \brief map glhck render property to OpenGL render property */
GLenum glhRenderPropertyForGlhckProperty(glhckRenderProperty property)
{
   switch (property) {
      case GLHCK_MAX_TEXTURE_SIZE:
         return GL_MAX_TEXTURE_SIZE;
      case GLHCK_MAX_CUBE_MAP_TEXTURE_SIZE:
         return GL_MAX_CUBE_MAP_TEXTURE_SIZE;
      case GLHCK_MAX_VERTEX_ATTRIBS:
         return GL_MAX_VERTEX_ATTRIBS;
      case GLHCK_MAX_VERTEX_UNIFORM_VECTORS:
         return GL_MAX_VERTEX_UNIFORM_VECTORS;
      case GLHCK_MAX_VARYING_VECTORS:
         return GL_MAX_VARYING_VECTORS;
      case GLHCK_MAX_COMBINED_TEXTURE_IMAGE_UNITS:
         return GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS;
      case GLHCK_MAX_VERTEX_TEXTURE_IMAGE_UNITS:
         return GL_MAX_VERTEX_TEXTURE_IMAGE_UNITS;
      case GLHCK_MAX_TEXTURE_IMAGE_UNITS:
         return GL_MAX_TEXTURE_IMAGE_UNITS;
      case GLHCK_MAX_FRAGMENT_UNIFORM_VECTORS:
         return GL_MAX_FRAGMENT_UNIFORM_VECTORS;
      case GLHCK_MAX_RENDERBUFFER_SIZE:
         return GL_MAX_RENDERBUFFER_SIZE;
      case GLHCK_MAX_VIEWPORT_DIMS:
         return GL_MAX_VIEWPORT_DIMS;
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck geometry constant to OpenGL geometry constant */
GLenum glhGeometryTypeForGlhckType(glhckGeometryType type)
{
   switch (type) {
      case GLHCK_POINTS:
         return GL_POINTS;
      case GLHCK_LINES:
         return GL_LINES;
      case GLHCK_LINE_LOOP:
         return GL_LINE_LOOP;
      case GLHCK_LINE_STRIP:
         return GL_LINE_STRIP;
      case GLHCK_TRIANGLES:
         return GL_TRIANGLES;
      case GLHCK_TRIANGLE_STRIP:
         return GL_TRIANGLE_STRIP;
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck texture format to OpenGL texture format */
GLenum glhTextureFormatForGlhckFormat(glhckTextureFormat format)
{
   switch (format) {
      case GLHCK_RED:
         return GL_RED;
      case GLHCK_GREEN:
         return GL_GREEN;
      case GLHCK_BLUE:
         return GL_BLUE;
      case GLHCK_ALPHA:
         return GL_ALPHA;
      case GLHCK_LUMINANCE:
         return GL_LUMINANCE;
      case GLHCK_LUMINANCE_ALPHA:
         return GL_LUMINANCE_ALPHA;
      case GLHCK_RGB:
         return GL_RGB;
      case GLHCK_RGBA:
         return GL_RGBA;
      case GLHCK_COMPRESSED_RGBA_DXT5:
         return GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
      case GLHCK_COMPRESSED_RGB_DXT1:
         return GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck blending mode to OpenGL blending mode */
GLenum glhBlendingModeForGlhckMode(glhckBlendingMode mode)
{
   switch (mode) {
      case GLHCK_ZERO:
         return GL_ZERO;
      case GLHCK_ONE:
         return GL_ONE;
      case GLHCK_SRC_COLOR:
         return GL_SRC_COLOR;
      case GLHCK_ONE_MINUS_SRC_COLOR:
         return GL_ONE_MINUS_SRC_COLOR;
      case GLHCK_SRC_ALPHA:
         return GL_SRC_ALPHA;
      case GLHCK_ONE_MINUS_SRC_ALPHA:
         return GL_ONE_MINUS_SRC_ALPHA;
      case GLHCK_DST_ALPHA:
         return GL_DST_ALPHA;
      case GLHCK_ONE_MINUS_DST_ALPHA:
         return GL_ONE_MINUS_DST_ALPHA;
      case GLHCK_DST_COLOR:
         return GL_DST_COLOR;
      case GLHCK_ONE_MINUS_DST_COLOR:
         return GL_ONE_MINUS_DST_COLOR;
      case GLHCK_SRC_ALPHA_SATURATE:
         return GL_SRC_ALPHA_SATURATE;
      case GLHCK_CONSTANT_COLOR:
         return GL_CONSTANT_COLOR;
      case GLHCK_CONSTANT_ALPHA:
         return GL_CONSTANT_ALPHA;
      case GLHCK_ONE_MINUS_CONSTANT_ALPHA:
         return GL_ONE_MINUS_CONSTANT_ALPHA;
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck fbo attachment constants to OpenGL attachment constants */
GLenum glhAttachmentTypeForGlhckType(_glhckRttAttachmentType type)
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
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck shader type to OpenGL shader type */
GLenum glhShaderTypeForGlhckType(glhckShaderType type)
{
   switch (type) {
      case GLHCK_VERTEX_SHADER:
         return GL_VERTEX_SHADER;
      case GLHCK_FRAGMENT_SHADER:
         return GL_FRAGMENT_SHADER;
      default:
         return 0;
   }
   return 0;
}

/* \brief map glhck shader constants to OpenGL shader constants */
GLenum glhShaderVariableTypeForGlhckType(_glhckShaderVariableType type)
{
   switch (type) {
      case GLHCK_SHADER_FLOAT:
         return GL_FLOAT;
      case GLHCK_SHADER_FLOAT_VEC2:
         return GL_FLOAT_VEC2;
      case GLHCK_SHADER_FLOAT_VEC3:
         return GL_FLOAT_VEC3;
      case GLHCK_SHADER_FLOAT_VEC4:
         return GL_FLOAT_VEC4;
      case GLHCK_SHADER_DOUBLE:
         return GL_DOUBLE;
      case GLHCK_SHADER_DOUBLE_VEC2:
         return GL_DOUBLE_VEC2;
      case GLHCK_SHADER_DOUBLE_VEC3:
         return GL_DOUBLE_VEC3;
      case GLHCK_SHADER_DOUBLE_VEC4:
         return GL_DOUBLE_VEC4;
      case GLHCK_SHADER_INT:
         return GL_INT;
      case GLHCK_SHADER_INT_VEC2:
         return GL_INT_VEC2;
      case GLHCK_SHADER_INT_VEC3:
         return GL_INT_VEC3;
      case GLHCK_SHADER_INT_VEC4:
         return GL_INT_VEC4;
      case GLHCK_SHADER_UNSIGNED_INT:
         return GL_UNSIGNED_INT;
      case GLHCK_SHADER_UNSIGNED_INT_VEC2:
         return GL_UNSIGNED_INT_VEC2;
      case GLHCK_SHADER_UNSIGNED_INT_VEC3:
         return GL_UNSIGNED_INT_VEC3;
      case GLHCK_SHADER_UNSIGNED_INT_VEC4:
         return GL_UNSIGNED_INT_VEC4;
      case GLHCK_SHADER_BOOL:
         return GL_BOOL;
      case GLHCK_SHADER_BOOL_VEC2:
         return GL_BOOL_VEC2;
      case GLHCK_SHADER_BOOL_VEC3:
         return GL_BOOL_VEC3;
      case GLHCK_SHADER_BOOL_VEC4:
         return GL_BOOL_VEC4;
      case GLHCK_SHADER_FLOAT_MAT2:
      case GLHCK_SHADER_FLOAT_MAT3:
      case GLHCK_SHADER_FLOAT_MAT4:
      case GLHCK_SHADER_FLOAT_MAT2x3:
      case GLHCK_SHADER_FLOAT_MAT2x4:
      case GLHCK_SHADER_FLOAT_MAT3x2:
      case GLHCK_SHADER_FLOAT_MAT3x4:
      case GLHCK_SHADER_FLOAT_MAT4x2:
      case GLHCK_SHADER_FLOAT_MAT4x3:
      case GLHCK_SHADER_DOUBLE_MAT2:
      case GLHCK_SHADER_DOUBLE_MAT3:
      case GLHCK_SHADER_DOUBLE_MAT4:
      case GLHCK_SHADER_DOUBLE_MAT2x3:
      case GLHCK_SHADER_DOUBLE_MAT2x4:
      case GLHCK_SHADER_DOUBLE_MAT3x2:
      case GLHCK_SHADER_DOUBLE_MAT3x4:
      case GLHCK_SHADER_DOUBLE_MAT4x2:
      case GLHCK_SHADER_DOUBLE_MAT4x3:
      case GLHCK_SHADER_SAMPLER_1D:
      case GLHCK_SHADER_SAMPLER_2D:
      case GLHCK_SHADER_SAMPLER_3D:
      case GLHCK_SHADER_SAMPLER_CUBE:
      case GLHCK_SHADER_SAMPLER_1D_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_SHADOW:
      case GLHCK_SHADER_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW:
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_SAMPLER_CUBE_SHADOW:
      case GLHCK_SHADER_SAMPLER_BUFFER:
      case GLHCK_SHADER_SAMPLER_2D_RECT:
      case GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW:
      case GLHCK_SHADER_INT_SAMPLER_1D:
      case GLHCK_SHADER_INT_SAMPLER_2D:
      case GLHCK_SHADER_INT_SAMPLER_3D:
      case GLHCK_SHADER_INT_SAMPLER_CUBE:
      case GLHCK_SHADER_INT_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_INT_SAMPLER_BUFFER:
      case GLHCK_SHADER_INT_SAMPLER_2D_RECT:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER:
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT:
      case GLHCK_SHADER_IMAGE_1D:
      case GLHCK_SHADER_IMAGE_2D:
      case GLHCK_SHADER_IMAGE_3D:
      case GLHCK_SHADER_IMAGE_2D_RECT:
      case GLHCK_SHADER_IMAGE_CUBE:
      case GLHCK_SHADER_IMAGE_BUFFER:
      case GLHCK_SHADER_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_1D:
      case GLHCK_SHADER_INT_IMAGE_2D:
      case GLHCK_SHADER_INT_IMAGE_3D:
      case GLHCK_SHADER_INT_IMAGE_2D_RECT:
      case GLHCK_SHADER_INT_IMAGE_CUBE:
      case GLHCK_SHADER_INT_IMAGE_BUFFER:
      case GLHCK_SHADER_INT_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:
      case GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER:
      default:
         return 0;
   }

   return 0;
}

/*
 * shared OpenGL renderer functions
 */

/* \brief get OpenGL parameters */
void glhGetIntegerv(GLenum pname, int *params)
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
void glhBufferGetPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      glhckTextureFormat format, GLvoid *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %p", x, y, width, height, format, data);
   GL_CALL(glReadPixels(x, y, width, height,
            glhTextureFormatForGlhckFormat(format), GL_UNSIGNED_BYTE, data));
}

/* \brief blend func wrapper */
void glhBlendFunc(GLenum blenda, GLenum blendb)
{
   GL_CALL(glBlendFunc(
            glhBlendingModeForGlhckMode(blenda),
            glhBlendingModeForGlhckMode(blendb)));
}

/* \brief texture bind wrapper */
void glhTextureBind(GLenum type, GLuint texture)
{
   if (GLHCKRD()->texture && GLHCKRD()->texture->object == texture)
      return;

   GL_CALL(glBindTexture(GL_TEXTURE_2D, texture));
}

/* \brief restore glhck texture state */
void glhTextureBindRestore(void)
{
   glhckTextureBind(GLHCKRD()->texture);
}

/* \brief create texture from data and upload it to OpenGL */
GLuint glhTextureCreate(const GLvoid *buffer, GLsizei size,
      GLsizei width, GLsizei height, glhckTextureFormat format,
      GLuint reuseTextureObject, unsigned int flags)
{
   GLuint object;
   GLenum glFormat;
   CALL(0, "%p, %d, %d, %d, %d, %u, %u", buffer, size,
         width, height, format, reuseTextureObject, flags);

   /* create empty texture */
   if (!(object = reuseTextureObject)) {
      GL_CALL(glGenTextures(1, &object));
   }

   /* fail? */
   if (!object)
      goto _return;

   glhTextureBind(GL_TEXTURE_2D, object);

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

   glFormat = glhTextureFormatForGlhckFormat(format);
   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexImage2D(GL_TEXTURE_2D, 0,
               glFormat, width, height, 0, size, buffer));
   } else {
      GL_CALL(glTexImage2D(GL_TEXTURE_2D, 0,
               glFormat, width, height, 0, glFormat,
               GL_UNSIGNED_BYTE, buffer));
   }

   glhTextureBindRestore();

_return:
   RET(0, "%u", object);
   return object;
}

/* \brief fill texture with data */
void glhTextureFill(GLuint texture, const GLvoid *data, GLsizei size,
      GLint x, GLint y, GLsizei width, GLsizei height, glhckTextureFormat format)
{
   GLenum glFormat;
   CALL(1, "%u, %p, %d, %d, %d, %d, %d, %d", texture,
         data, size, x, y, width, height, format);

   glhTextureBind(GL_TEXTURE_2D, texture);

   glFormat = glhTextureFormatForGlhckFormat(format);
   if (_glhckIsCompressedFormat(format)) {
      GL_CALL(glCompressedTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, glFormat, size, data));
   } else{
      GL_CALL(glTexSubImage2D(GL_TEXTURE_2D, 0, x, y,
               width, height, glFormat, GL_UNSIGNED_BYTE, data));
   }

   glhTextureBindRestore();
}

/* \brief link FBO with texture attachmetn */
int glhFramebufferLinkWithTexture(GLuint object, GLuint texture, _glhckRttAttachmentType type)
{
   GLenum glAttachment;
   CALL(0, "%d, %d, %d", object, texture, type);

   glAttachment = glhAttachmentTypeForGlhckType(type);
   GL_CALL(glBindFramebuffer(GL_FRAMEBUFFER, object));
   GL_CALL(glFramebufferTexture2D(GL_FRAMEBUFFER, glAttachment, GL_TEXTURE_2D, texture, 0));

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
#define indicesToOpenGL(iprec, tunion) \
   GL_CALL(glDrawElements(glGeometryType, geometry->indexCount, iprec, &geometry->indices.tunion[0]))

/* \brief draw interleaved geometry */
inline void glhGeometryRender(const glhckGeometry *geometry, glhckGeometryType type)
{
   GLenum glGeometryType;
   // printf("%s (%d)\n", glhckIndexTypeString(geometry->indexType), geometry->indexCount);

   glGeometryType = glhGeometryTypeForGlhckType(type);
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
      GL_CALL(glDrawArrays(glGeometryType, 0, geometry->vertexCount));
   }
}

/* the helper is no longer needed */
#undef indicesToOpenGL

/* vim: set ts=8 sw=3 tw=0 :*/
