#include "../internal.h"
#include <limits.h> /* for limits */
#include <stdio.h>  /* for snprintf */

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#include "helper_opengl.h"

#if GLHCK_USE_GLES1 || GLHCK_USE_GLES2
#  include <EGL/egl.h>
#endif

#ifndef GLEW_OES_framebuffer_object
#  define GLEW_OES_framebuffer_object 0
#endif

#ifndef NDEBUG
void GL_ERROR(unsigned int line, const char *func, const char *glfunc)
{
   GLenum error;
   if ((error = glGetError()) != GL_NO_ERROR)
      DEBUG(GLHCK_DBG_ERROR, "GL @%d:%-20s %-20s >> %s",
            line, func, glfunc,
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
            error==GL_INVALID_OPERATION?
            "GL_INVALID_OPERATION":
            "GL_UNKNOWN_ERROR");
}
GLenum GL_CHECK_ERROR(const char *func, const char *glfunc, GLenum error)
{
   if (error != GL_NO_ERROR &&
       error != GL_FRAMEBUFFER_COMPLETE)
      DEBUG(GLHCK_DBG_ERROR, "GL @%-20s %-20s >> %s",
            func, glfunc,
            error==GL_FRAMEBUFFER_UNDEFINED?
            "GL_FRAMEBUFFER_UNDEFINED":
            error==GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT?
            "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT":
            error==GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER?
            "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER":
            error==GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER?
            "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER":
            error==GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT?
            "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT":
            error==GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE?
            "GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE":
            error==GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS?
            "GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS":
            error==GL_FRAMEBUFFER_UNSUPPORTED?
            "GL_FRAMEBUFFER_UNSUPPORTED":
            "GL_UNKNOWN_ERROR");
   return error;
}
#endif

GLenum glhckCullFaceTypeToGL[] = {
   GL_BACK, /* GLHCK_BACK */
   GL_FRONT, /* GLHCK_FRONT */
};

GLenum glhckFaceOrientationToGL[] = {
   GL_CW, /* GLHCK_CW */
   GL_CCW, /* GLHCK_CCW */
};

GLenum glhckGeometryTypeToGL[] = {
   GL_POINTS, /* GLHCK_POINTS */
   GL_LINES, /* GLHCK_LINES */
   GL_LINE_LOOP, /* GLHCK_LINE_LOOP */
   GL_LINE_STRIP, /* GLHCK_LINE_STRIP */
   GL_TRIANGLES, /* GLHCK_TRIANGLES */
   GL_TRIANGLE_STRIP, /* GLHCK_TRIANGLE_STRIP */
};

GLenum glhckFramebufferTargetToGL[] = {
   GL_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER */
   GL_READ_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER_READ */
   GL_DRAW_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER_DRAW */
};

GLenum glhckFramebufferAttachmentTypeToGL[] = {
   GL_COLOR_ATTACHMENT0, /* GLHCK_COLOR_ATTACHMENT0 */
   GL_COLOR_ATTACHMENT1, /* GLHCK_COLOR_ATTACHMENT1 */
   GL_COLOR_ATTACHMENT2, /* GLHCK_COLOR_ATTACHMENT2 */
   GL_COLOR_ATTACHMENT3, /* GLHCK_COLOR_ATTACHMENT3 */
   GL_COLOR_ATTACHMENT4, /* GLHCK_COLOR_ATTACHMENT4 */
   GL_COLOR_ATTACHMENT5, /* GLHCK_COLOR_ATTACHMENT5 */
   GL_COLOR_ATTACHMENT6, /* GLHCK_COLOR_ATTACHMENT6 */
   GL_COLOR_ATTACHMENT7, /* GLHCK_COLOR_ATTACHMENT7 */
   GL_COLOR_ATTACHMENT8, /* GLHCK_COLOR_ATTACHMENT8 */
   GL_COLOR_ATTACHMENT9, /* GLHCK_COLOR_ATTACHMENT9 */
   GL_COLOR_ATTACHMENT10, /* GLHCK_COLOR_ATTACHMENT10 */
   GL_COLOR_ATTACHMENT11, /* GLHCK_COLOR_ATTACHMENT11 */
   GL_COLOR_ATTACHMENT12, /* GLHCK_COLOR_ATTACHMENT12 */
   GL_COLOR_ATTACHMENT13, /* GLHCK_COLOR_ATTACHMENT13 */
   GL_COLOR_ATTACHMENT14, /* GLHCK_COLOR_ATTACHMENT14 */
   GL_COLOR_ATTACHMENT15, /* GLHCK_COLOR_ATTACHMENT15 */
   GL_DEPTH_ATTACHMENT, /* GLHCK_DEPTH_ATTACHMENT */
   GL_STENCIL_ATTACHMENT, /* GLHCK_STENCIL_ATTACHMENT */
};

GLenum glhckHwBufferTargetToGL[] = {
   GL_ARRAY_BUFFER, /* GLHCK_ARRAY_BUFFER */
   GL_COPY_READ_BUFFER, /* GLHCK_COPY_READ_BUFFER */
   GL_COPY_WRITE_BUFFER, /* GLHCK_COPY_WRITE_BUFFER */
   GL_ELEMENT_ARRAY_BUFFER, /* GLHCK_ELEMENT_ARRAY_BUFFER */
   GL_PIXEL_PACK_BUFFER, /* GLHCK_PIXEL_PACK_BUFFER */
   GL_PIXEL_UNPACK_BUFFER, /* GLHCK_PIXEL_UNPACK_BUFFER */
   GL_TEXTURE_BUFFER, /* GLHCK_TEXTURE_BUFFER */
   GL_TRANSFORM_FEEDBACK_BUFFER, /* GLHCK_TRANSFORM_FEEDBACK_BUFFER */
   GL_UNIFORM_BUFFER, /* GLHCK_UNIFORM_BUFFER */
#if !EMSCRIPTEN
   GL_SHADER_STORAGE_BUFFER, /* GLHCK_SHADER_STORAGE_BUFFER */
   GL_ATOMIC_COUNTER_BUFFER, /* GLHCK_ATOMIC_COUNTER_BUFFER */
#endif
};

GLenum glhckHwBufferStoreTypeToGL[] = {
   GL_STREAM_DRAW, /* GLHCK_BUFFER_STREAM_DRAW */
   GL_STREAM_READ, /* GLHCK_BUFFER_STREAM_READ */
   GL_STREAM_COPY, /* GLHCK_BUFFER_STREAM_COPY */
   GL_STATIC_DRAW, /* GLHCK_BUFFER_STATIC_DRAW */
   GL_STATIC_READ, /* GLHCK_BUFFER_STATIC_READ */
   GL_STATIC_COPY, /* GLHCK_BUFFER_STATIC_COPY */
   GL_DYNAMIC_DRAW, /* GLHCK_BUFFER_DYNAMIC_DRAW */
   GL_DYNAMIC_READ, /* GLHCK_BUFFER_DYNAMIC_READ */
   GL_DYNAMIC_COPY, /* GLHCK_BUFFER_DYNAMIC_COPY */
};

GLenum glhckHwBufferAccessTypeToGL[] = {
   GL_READ_ONLY, /* GLHCK_BUFFER_READ_ONLY */
   GL_WRITE_ONLY, /* GLHCK_BUFFER_WRITE_ONLY */
   GL_READ_WRITE, /* GLHCK_BUFFER_READ_WRITE */
};

GLenum glhckShaderTypeToGL[] = {
   GL_VERTEX_SHADER, /* GLHCK_VERTEX_SHADER */
   GL_FRAGMENT_SHADER, /* GLHCK_FRAGMENT_SHADER */
};

GLenum glhckTextureWrapToGL[] = {
   GL_REPEAT, /* GLHCK_REPEAT */
   GL_MIRRORED_REPEAT, /* GLHCK_MIRRORED_REPEAT */
   GL_CLAMP_TO_EDGE, /* GLHCK_CLAMP_TO_EDGE */
   GL_CLAMP_TO_BORDER, /* GLHCK_CLAMP_TO_BORDER */
};

GLenum glhckTextureFilterToGL[] = {
   GL_NEAREST, /* GLHCK_NEAREST */
   GL_LINEAR, /* GLHCK_LINEAR */
   GL_NEAREST_MIPMAP_NEAREST, /* GLHCK_NEAREST_MIPMAP_NEAREST */
   GL_LINEAR_MIPMAP_NEAREST, /* GLHCK_LINEAR_MIPMAP_NEAREST */
   GL_NEAREST_MIPMAP_LINEAR, /* GLHCK_NEAREST_MIPMAP_LINEAR */
   GL_LINEAR_MIPMAP_LINEAR, /* GLHCK_LINEAR_MIPMAP_LINEAR */
};

GLenum glhckTextureCompareModeToGL[] = {
   0, /* GLHCK_COMPARE_NONE */
   GL_COMPARE_REF_TO_TEXTURE, /* GLHCK_COMPARE_REF_TO_TEXTURE */
};

GLenum glhckCompareFuncToGL[] = {
   GL_LEQUAL, /* GLHCK_LEQUAL */
   GL_GEQUAL, /* GLHCK_GEQUAL */
   GL_LESS, /* GLHCK_LESS */
   GL_GREATER, /* GLHCK_GREATER */
   GL_EQUAL, /* GLHCK_EQUAL */
   GL_NOTEQUAL, /* GLHCK_NOTEQUAL */
   GL_ALWAYS, /* GLHCK_ALWAYS */
   GL_NEVER, /* GLHCK_NEVER */
};

GLenum glhckTextureTargetToGL[] = {
   GL_TEXTURE_1D, /* GLHCK_TEXTURE_1D */
   GL_TEXTURE_2D, /* GLHCK_TEXTURE_2D */
   GL_TEXTURE_3D, /* GLHCK_TEXTURE_3D */
   GL_TEXTURE_CUBE_MAP, /* GLHCK_TEXTURE_CUBE_MAP */
};

GLenum glhckTextureFormatToGL[] = {
   GL_RED, /* GLHCK_RED */
   GL_RG, /* GLHCK_RG */
   GL_ALPHA, /* GLHCK_ALPHA */
   GL_LUMINANCE, /* GLHCK_LUMINANCE */
   GL_LUMINANCE_ALPHA, /* GLHCK_LUMINANCE_ALPHA */
   GL_RGB, /* GLHCK_RGB */
   GL_BGR, /* GLHCK_BGR */
   GL_RGBA, /* GLHCK_RGBA */
   GL_BGRA, /* GLHCK_BGRA */
   GL_DEPTH_COMPONENT, /* GLHCK_DEPTH_COMPONENT */
   GL_DEPTH_COMPONENT16, /* GLHCK_DEPTH_COMPONENT16 */
   GL_DEPTH_COMPONENT24, /* GLHCK_DEPTH_COMPONENT24 */
   GL_DEPTH_COMPONENT32, /* GLHCK_DEPTH_COMPONENT32 */
   GL_DEPTH_STENCIL, /* GLHCK_DEPTH_STENCIL */
   GL_COMPRESSED_RGBA_S3TC_DXT1_EXT, /* GLHCK_COMPRESSED_RGB_DXT1 */
   GL_COMPRESSED_RGBA_S3TC_DXT5_EXT, /* GLHCK_COMPRESSED_RGBA_DXT5 */
};

GLenum glhckDataTypeToGL[] = {
   GL_UNSIGNED_BYTE, /* GLHCK_COMPRESSED */
   GL_UNSIGNED_BYTE, /* GLHCK_UNSIGNED_BYTE */
   GL_BYTE, /* GLHCK_BYTE */
   GL_UNSIGNED_SHORT, /* GLHCK_UNSIGNED_SHORT */
   GL_SHORT, /* GLHCK_SHORT */
   GL_UNSIGNED_INT, /* GLHCK_UNSIGNED_INT */
   GL_INT, /* GLHCK_INT */
   GL_FLOAT, /* GLHCK_FLOAT */
   GL_UNSIGNED_BYTE_3_3_2, /* GLHCK_UNSIGNED_BYTE_3_3_2 */
   GL_UNSIGNED_BYTE_2_3_3_REV, /* GLHCK_UNSIGNED_BYTE_2_3_3_REV */
   GL_UNSIGNED_SHORT_5_6_5, /* GLHCK_UNSIGNED_SHORT_5_6_5 */
   GL_UNSIGNED_SHORT_5_6_5_REV, /* GLHCK_UNSIGNED_SHORT_5_6_5_REV */
   GL_UNSIGNED_SHORT_4_4_4_4, /* GLHCK_UNSIGNED_SHORT_4_4_4_4 */
   GL_UNSIGNED_SHORT_4_4_4_4_REV, /* GLHCK_UNSIGNED_SHORT_4_4_4_4_REV */
   GL_UNSIGNED_SHORT_5_5_5_1, /* GLHCK_UNSIGNED_SHORT_5_5_5_1 */
   GL_UNSIGNED_SHORT_1_5_5_5_REV, /* GLHCK_UNSIGNED_SHORT_1_5_5_5_REV */
   GL_UNSIGNED_INT_8_8_8_8, /* GLHCK_UNSIGNED_INT_8_8_8_8 */
   GL_UNSIGNED_INT_8_8_8_8_REV, /* GLHCK_UNSIGNED_INT_8_8_8_8_REV */
   GL_UNSIGNED_INT_10_10_10_2, /* GLHCK_UNSIGNED_INT_10_10_10_2 */
   GL_UNSIGNED_INT_2_10_10_10_REV, /* GLHCK_UNSIGNED_INT_2_10_10_10_REV */
};

GLenum glhckBlendingModeToGL[] = {
   GL_ZERO, /* GLHCK_ZERO */
   GL_ONE, /* GLHCK_ONE */
   GL_SRC_COLOR, /* GLHCK_SRC_COLOR */
   GL_ONE_MINUS_SRC_COLOR, /* GLHCK_ONE_MINUS_SRC_COLOR */
   GL_SRC_ALPHA, /* GLHCK_SRC_ALPHA */
   GL_ONE_MINUS_SRC_ALPHA, /* GLHCK_ONE_MINUS_SRC_ALPHA */
   GL_DST_ALPHA, /* GLHCK_DST_ALPHA */
   GL_ONE_MINUS_DST_ALPHA, /* GLHCK_ONE_MINUS_DST_ALPHA */
   GL_DST_COLOR, /* GLHCK_DST_COLOR */
   GL_ONE_MINUS_DST_COLOR, /* GLHCK_ONE_MINUS_DST_COLOR */
   GL_SRC_ALPHA_SATURATE, /* GLHCK_SRC_ALPHA_SATURATE */
   GL_CONSTANT_COLOR, /* GLHCK_CONSTANT_COLOR */
   GL_CONSTANT_ALPHA, /* GLHCK_CONSTANT_ALPHA */
   GL_ONE_MINUS_CONSTANT_ALPHA, /* GLHCK_ONE_MINUS_CONSTANT_ALPHA */
};

/* \brief map glhck shader constants to OpenGL shader constants */
GLenum glhShaderVariableTypeForGlhckType(_glhckShaderVariableType type)
{
   switch (type) {
      case GLHCK_SHADER_FLOAT:return GL_FLOAT;
      case GLHCK_SHADER_FLOAT_VEC2:return GL_FLOAT_VEC2;
      case GLHCK_SHADER_FLOAT_VEC3:return GL_FLOAT_VEC3;
      case GLHCK_SHADER_FLOAT_VEC4:return GL_FLOAT_VEC4;
      case GLHCK_SHADER_DOUBLE:return GL_DOUBLE;
      case GLHCK_SHADER_DOUBLE_VEC2:return GL_DOUBLE_VEC2;
      case GLHCK_SHADER_DOUBLE_VEC3:return GL_DOUBLE_VEC3;
      case GLHCK_SHADER_DOUBLE_VEC4:return GL_DOUBLE_VEC4;
      case GLHCK_SHADER_INT:return GL_INT;
      case GLHCK_SHADER_INT_VEC2:return GL_INT_VEC2;
      case GLHCK_SHADER_INT_VEC3:return GL_INT_VEC3;
      case GLHCK_SHADER_INT_VEC4:return GL_INT_VEC4;
      case GLHCK_SHADER_UNSIGNED_INT:return GL_UNSIGNED_INT;
      case GLHCK_SHADER_UNSIGNED_INT_VEC2:return GL_UNSIGNED_INT_VEC2;
      case GLHCK_SHADER_UNSIGNED_INT_VEC3:return GL_UNSIGNED_INT_VEC3;
      case GLHCK_SHADER_UNSIGNED_INT_VEC4:return GL_UNSIGNED_INT_VEC4;
      case GLHCK_SHADER_BOOL:return GL_BOOL;
      case GLHCK_SHADER_BOOL_VEC2:return GL_BOOL_VEC2;
      case GLHCK_SHADER_BOOL_VEC3:return GL_BOOL_VEC3;
      case GLHCK_SHADER_BOOL_VEC4:return GL_BOOL_VEC4;
      case GLHCK_SHADER_FLOAT_MAT2:return GL_FLOAT_MAT2;
      case GLHCK_SHADER_FLOAT_MAT3:return GL_FLOAT_MAT3;
      case GLHCK_SHADER_FLOAT_MAT4:return GL_FLOAT_MAT4;
      case GLHCK_SHADER_FLOAT_MAT2x3:return GL_FLOAT_MAT2x3;
      case GLHCK_SHADER_FLOAT_MAT2x4:return GL_FLOAT_MAT2x4;
      case GLHCK_SHADER_FLOAT_MAT3x2:return GL_FLOAT_MAT3x2;
      case GLHCK_SHADER_FLOAT_MAT3x4:return GL_FLOAT_MAT3x4;
      case GLHCK_SHADER_FLOAT_MAT4x2:return GL_FLOAT_MAT4x2;
      case GLHCK_SHADER_FLOAT_MAT4x3:return GL_FLOAT_MAT4x3;
      case GLHCK_SHADER_DOUBLE_MAT2:return GL_DOUBLE_MAT2;
      case GLHCK_SHADER_DOUBLE_MAT3:return GL_DOUBLE_MAT3;
      case GLHCK_SHADER_DOUBLE_MAT4:return GL_DOUBLE_MAT4;
      case GLHCK_SHADER_DOUBLE_MAT2x3:return GL_DOUBLE_MAT2x3;
      case GLHCK_SHADER_DOUBLE_MAT2x4:return GL_DOUBLE_MAT2x4;
      case GLHCK_SHADER_DOUBLE_MAT3x2:return GL_DOUBLE_MAT3x2;
      case GLHCK_SHADER_DOUBLE_MAT3x4:return GL_DOUBLE_MAT3x4;
      case GLHCK_SHADER_DOUBLE_MAT4x2:return GL_DOUBLE_MAT4x2;
      case GLHCK_SHADER_DOUBLE_MAT4x3:return GL_DOUBLE_MAT4x3;
      case GLHCK_SHADER_SAMPLER_1D:return GL_SAMPLER_1D;
      case GLHCK_SHADER_SAMPLER_2D:return GL_SAMPLER_2D;
      case GLHCK_SHADER_SAMPLER_3D:return GL_SAMPLER_3D;
      case GLHCK_SHADER_SAMPLER_CUBE:return GL_SAMPLER_CUBE;
      case GLHCK_SHADER_SAMPLER_1D_SHADOW:return GL_SAMPLER_1D_SHADOW;
      case GLHCK_SHADER_SAMPLER_2D_SHADOW:return GL_SAMPLER_2D_SHADOW;
      case GLHCK_SHADER_SAMPLER_1D_ARRAY:return GL_SAMPLER_1D_ARRAY;
      case GLHCK_SHADER_SAMPLER_2D_ARRAY:return GL_SAMPLER_2D_ARRAY;
      case GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW:return GL_SAMPLER_1D_ARRAY_SHADOW;
      case GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW:return GL_SAMPLER_2D_ARRAY_SHADOW;
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE:return GL_SAMPLER_2D_MULTISAMPLE;
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY:return GL_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_SAMPLER_CUBE_SHADOW:return GL_SAMPLER_CUBE_SHADOW;
      case GLHCK_SHADER_SAMPLER_BUFFER:return GL_SAMPLER_BUFFER;
      case GLHCK_SHADER_SAMPLER_2D_RECT:return GL_SAMPLER_2D_RECT;
      case GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW:return GL_SAMPLER_2D_RECT_SHADOW;
      case GLHCK_SHADER_INT_SAMPLER_1D:return GL_INT_SAMPLER_1D;
      case GLHCK_SHADER_INT_SAMPLER_2D:return GL_INT_SAMPLER_2D;
      case GLHCK_SHADER_INT_SAMPLER_3D:return GL_INT_SAMPLER_3D;
      case GLHCK_SHADER_INT_SAMPLER_CUBE:return GL_INT_SAMPLER_CUBE;
      case GLHCK_SHADER_INT_SAMPLER_1D_ARRAY:return GL_INT_SAMPLER_1D_ARRAY;
      case GLHCK_SHADER_INT_SAMPLER_2D_ARRAY:return GL_INT_SAMPLER_2D_ARRAY;
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE:return GL_INT_SAMPLER_2D_MULTISAMPLE;
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:return GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_INT_SAMPLER_BUFFER:return GL_INT_SAMPLER_BUFFER;
      case GLHCK_SHADER_INT_SAMPLER_2D_RECT:return GL_INT_SAMPLER_2D_RECT;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D:return GL_UNSIGNED_INT_SAMPLER_1D;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D:return GL_UNSIGNED_INT_SAMPLER_2D;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D:return GL_UNSIGNED_INT_SAMPLER_3D;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE:return GL_UNSIGNED_INT_SAMPLER_CUBE;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY:return GL_UNSIGNED_INT_SAMPLER_1D_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY:return GL_UNSIGNED_INT_SAMPLER_2D_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:return GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:return GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER:return GL_UNSIGNED_INT_SAMPLER_BUFFER;
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT:return GL_UNSIGNED_INT_SAMPLER_2D_RECT;
#if !EMSCRIPTEN
      case GLHCK_SHADER_IMAGE_1D:return GL_IMAGE_1D;
      case GLHCK_SHADER_IMAGE_2D:return GL_IMAGE_2D;
      case GLHCK_SHADER_IMAGE_3D:return GL_IMAGE_3D;
      case GLHCK_SHADER_IMAGE_2D_RECT:return GL_IMAGE_2D_RECT;
      case GLHCK_SHADER_IMAGE_CUBE:return GL_IMAGE_CUBE;
      case GLHCK_SHADER_IMAGE_BUFFER:return GL_IMAGE_BUFFER;
      case GLHCK_SHADER_IMAGE_1D_ARRAY:return GL_IMAGE_1D_ARRAY;
      case GLHCK_SHADER_IMAGE_2D_ARRAY:return GL_IMAGE_2D_ARRAY;
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE:return GL_IMAGE_2D_MULTISAMPLE;
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY:return GL_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_INT_IMAGE_1D:return GL_INT_IMAGE_1D;
      case GLHCK_SHADER_INT_IMAGE_2D:return GL_INT_IMAGE_2D;
      case GLHCK_SHADER_INT_IMAGE_3D:return GL_INT_IMAGE_3D;
      case GLHCK_SHADER_INT_IMAGE_2D_RECT:return GL_INT_IMAGE_2D_RECT;
      case GLHCK_SHADER_INT_IMAGE_CUBE:return GL_INT_IMAGE_CUBE;
      case GLHCK_SHADER_INT_IMAGE_BUFFER:return GL_INT_IMAGE_BUFFER;
      case GLHCK_SHADER_INT_IMAGE_1D_ARRAY:return GL_INT_IMAGE_1D_ARRAY;
      case GLHCK_SHADER_INT_IMAGE_2D_ARRAY:return GL_INT_IMAGE_2D_ARRAY;
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE:return GL_INT_IMAGE_2D_MULTISAMPLE;
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY:return GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D:return GL_UNSIGNED_INT_IMAGE_1D;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D:return GL_UNSIGNED_INT_IMAGE_2D;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D:return GL_UNSIGNED_INT_IMAGE_3D;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT:return GL_UNSIGNED_INT_IMAGE_2D_RECT;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE:return GL_UNSIGNED_INT_IMAGE_CUBE;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER:return GL_UNSIGNED_INT_IMAGE_BUFFER;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY:return GL_UNSIGNED_INT_IMAGE_1D_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY:return GL_UNSIGNED_INT_IMAGE_2D_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:return GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE;
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:return GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER:return GL_UNSIGNED_INT_ATOMIC_COUNTER;
#endif
      default:break;
   }
   assert(0 && "BAD ENUM OR NOT IMPLEMENTED");
   return 0;
}

/* \brief map OpenGL shader constants to glhck shader constants */
_glhckShaderVariableType glhGlhckShaderVariableTypeForOpenGLType(GLenum type)
{
   switch (type) {
      case GL_FLOAT:return GLHCK_SHADER_FLOAT;
      case GL_FLOAT_VEC2:return GLHCK_SHADER_FLOAT_VEC2;
      case GL_FLOAT_VEC3:return GLHCK_SHADER_FLOAT_VEC3;
      case GL_FLOAT_VEC4:return GLHCK_SHADER_FLOAT_VEC4;
      case GL_DOUBLE:return GLHCK_SHADER_DOUBLE;
      case GL_DOUBLE_VEC2:return GLHCK_SHADER_DOUBLE_VEC2;
      case GL_DOUBLE_VEC3:return GLHCK_SHADER_DOUBLE_VEC3;
      case GL_DOUBLE_VEC4:return GLHCK_SHADER_DOUBLE_VEC4;
      case GL_INT:return GLHCK_SHADER_INT;
      case GL_INT_VEC2:return GLHCK_SHADER_INT_VEC2;
      case GL_INT_VEC3:return GLHCK_SHADER_INT_VEC3;
      case GL_INT_VEC4:return GLHCK_SHADER_INT_VEC4;
      case GL_UNSIGNED_INT:return GLHCK_SHADER_UNSIGNED_INT;
      case GL_UNSIGNED_INT_VEC2:return GLHCK_SHADER_UNSIGNED_INT_VEC2;
      case GL_UNSIGNED_INT_VEC3:return GLHCK_SHADER_UNSIGNED_INT_VEC3;
      case GL_UNSIGNED_INT_VEC4:return GLHCK_SHADER_UNSIGNED_INT_VEC4;
      case GL_BOOL:return GLHCK_SHADER_BOOL;
      case GL_BOOL_VEC2:return GLHCK_SHADER_BOOL_VEC2;
      case GL_BOOL_VEC3:return GLHCK_SHADER_BOOL_VEC3;
      case GL_BOOL_VEC4:return GLHCK_SHADER_BOOL_VEC4;
      case GL_FLOAT_MAT2:return GLHCK_SHADER_FLOAT_MAT2;
      case GL_FLOAT_MAT3:return GLHCK_SHADER_FLOAT_MAT3;
      case GL_FLOAT_MAT4:return GLHCK_SHADER_FLOAT_MAT4;
      case GL_FLOAT_MAT2x3:return GLHCK_SHADER_FLOAT_MAT2x3;
      case GL_FLOAT_MAT2x4:return GLHCK_SHADER_FLOAT_MAT2x4;
      case GL_FLOAT_MAT3x2:return GLHCK_SHADER_FLOAT_MAT3x2;
      case GL_FLOAT_MAT3x4:return GLHCK_SHADER_FLOAT_MAT3x4;
      case GL_FLOAT_MAT4x2:return GLHCK_SHADER_FLOAT_MAT4x2;
      case GL_FLOAT_MAT4x3:return GLHCK_SHADER_FLOAT_MAT4x3;
      case GL_DOUBLE_MAT2:return GLHCK_SHADER_DOUBLE_MAT2;
      case GL_DOUBLE_MAT3:return GLHCK_SHADER_DOUBLE_MAT3;
      case GL_DOUBLE_MAT4:return GLHCK_SHADER_DOUBLE_MAT4;
      case GL_DOUBLE_MAT2x3:return GLHCK_SHADER_DOUBLE_MAT2x3;
      case GL_DOUBLE_MAT2x4:return GLHCK_SHADER_DOUBLE_MAT2x4;
      case GL_DOUBLE_MAT3x2:return GLHCK_SHADER_DOUBLE_MAT3x2;
      case GL_DOUBLE_MAT3x4:return GLHCK_SHADER_DOUBLE_MAT3x4;
      case GL_DOUBLE_MAT4x2:return GLHCK_SHADER_DOUBLE_MAT4x2;
      case GL_DOUBLE_MAT4x3:return GLHCK_SHADER_DOUBLE_MAT4x3;
      case GL_SAMPLER_1D:return GLHCK_SHADER_SAMPLER_1D;
      case GL_SAMPLER_2D:return GLHCK_SHADER_SAMPLER_2D;
      case GL_SAMPLER_3D:return GLHCK_SHADER_SAMPLER_3D;
      case GL_SAMPLER_CUBE:return GLHCK_SHADER_SAMPLER_CUBE;
      case GL_SAMPLER_1D_SHADOW:return GLHCK_SHADER_SAMPLER_1D_SHADOW;
      case GL_SAMPLER_2D_SHADOW:return GLHCK_SHADER_SAMPLER_2D_SHADOW;
      case GL_SAMPLER_1D_ARRAY:return GLHCK_SHADER_SAMPLER_1D_ARRAY;
      case GL_SAMPLER_2D_ARRAY:return GLHCK_SHADER_SAMPLER_2D_ARRAY;
      case GL_SAMPLER_1D_ARRAY_SHADOW:return GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW;
      case GL_SAMPLER_2D_ARRAY_SHADOW:return GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW;
      case GL_SAMPLER_2D_MULTISAMPLE:return GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE;
      case GL_SAMPLER_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GL_SAMPLER_CUBE_SHADOW:return GLHCK_SHADER_SAMPLER_CUBE_SHADOW;
      case GL_SAMPLER_BUFFER:return GLHCK_SHADER_SAMPLER_BUFFER;
      case GL_SAMPLER_2D_RECT:return GLHCK_SHADER_SAMPLER_2D_RECT;
      case GL_SAMPLER_2D_RECT_SHADOW:return GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW;
      case GL_INT_SAMPLER_1D:return GLHCK_SHADER_INT_SAMPLER_1D;
      case GL_INT_SAMPLER_2D:return GLHCK_SHADER_INT_SAMPLER_2D;
      case GL_INT_SAMPLER_3D:return GLHCK_SHADER_INT_SAMPLER_3D;
      case GL_INT_SAMPLER_CUBE:return GLHCK_SHADER_INT_SAMPLER_CUBE;
      case GL_INT_SAMPLER_1D_ARRAY:return GLHCK_SHADER_INT_SAMPLER_1D_ARRAY;
      case GL_INT_SAMPLER_2D_ARRAY:return GLHCK_SHADER_INT_SAMPLER_2D_ARRAY;
      case GL_INT_SAMPLER_2D_MULTISAMPLE:return GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE;
      case GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GL_INT_SAMPLER_BUFFER:return GLHCK_SHADER_INT_SAMPLER_BUFFER;
      case GL_INT_SAMPLER_2D_RECT:return GLHCK_SHADER_INT_SAMPLER_2D_RECT;
      case GL_UNSIGNED_INT_SAMPLER_1D:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D;
      case GL_UNSIGNED_INT_SAMPLER_2D:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D;
      case GL_UNSIGNED_INT_SAMPLER_3D:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D;
      case GL_UNSIGNED_INT_SAMPLER_CUBE:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE;
      case GL_UNSIGNED_INT_SAMPLER_1D_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY;
      case GL_UNSIGNED_INT_SAMPLER_2D_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY;
      case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE;
      case GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY;
      case GL_UNSIGNED_INT_SAMPLER_BUFFER:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER;
      case GL_UNSIGNED_INT_SAMPLER_2D_RECT:return GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT;
#if !EMSCRIPTEN
      case GL_IMAGE_1D:return GLHCK_SHADER_IMAGE_1D;
      case GL_IMAGE_2D:return GLHCK_SHADER_IMAGE_2D;
      case GL_IMAGE_3D:return GLHCK_SHADER_IMAGE_3D;
      case GL_IMAGE_2D_RECT:return GLHCK_SHADER_IMAGE_2D_RECT;
      case GL_IMAGE_CUBE:return GLHCK_SHADER_IMAGE_CUBE;
      case GL_IMAGE_BUFFER:return GLHCK_SHADER_IMAGE_BUFFER;
      case GL_IMAGE_1D_ARRAY:return GLHCK_SHADER_IMAGE_1D_ARRAY;
      case GL_IMAGE_2D_ARRAY:return GLHCK_SHADER_IMAGE_2D_ARRAY;
      case GL_IMAGE_2D_MULTISAMPLE:return GLHCK_SHADER_IMAGE_2D_MULTISAMPLE;
      case GL_IMAGE_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GL_INT_IMAGE_1D:return GLHCK_SHADER_INT_IMAGE_1D;
      case GL_INT_IMAGE_2D:return GLHCK_SHADER_INT_IMAGE_2D;
      case GL_INT_IMAGE_3D:return GLHCK_SHADER_INT_IMAGE_3D;
      case GL_INT_IMAGE_2D_RECT:return GLHCK_SHADER_INT_IMAGE_2D_RECT;
      case GL_INT_IMAGE_CUBE:return GLHCK_SHADER_INT_IMAGE_CUBE;
      case GL_INT_IMAGE_BUFFER:return GLHCK_SHADER_INT_IMAGE_BUFFER;
      case GL_INT_IMAGE_1D_ARRAY:return GLHCK_SHADER_INT_IMAGE_1D_ARRAY;
      case GL_INT_IMAGE_2D_ARRAY:return GLHCK_SHADER_INT_IMAGE_2D_ARRAY;
      case GL_INT_IMAGE_2D_MULTISAMPLE:return GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE;
      case GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GL_UNSIGNED_INT_IMAGE_1D:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D;
      case GL_UNSIGNED_INT_IMAGE_2D:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D;
      case GL_UNSIGNED_INT_IMAGE_3D:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D;
      case GL_UNSIGNED_INT_IMAGE_2D_RECT:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT;
      case GL_UNSIGNED_INT_IMAGE_CUBE:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE;
      case GL_UNSIGNED_INT_IMAGE_BUFFER:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER;
      case GL_UNSIGNED_INT_IMAGE_1D_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY;
      case GL_UNSIGNED_INT_IMAGE_2D_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY;
      case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE;
      case GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY:return GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY;
      case GL_UNSIGNED_INT_ATOMIC_COUNTER:return GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER;
#endif
      default:break;
   }
   assert(0 && "BAD ENUM OR NOT IMPLEMENTED");
   return 0;
}

/* return shader variable name for OpenGL shader variable constant */
const GLchar* glhShaderVariableNameForGlhckConstant(GLenum type)
{
   switch (type) {
      case GLHCK_SHADER_FLOAT: return "float";
      case GLHCK_SHADER_FLOAT_VEC2: return "vec2";
      case GLHCK_SHADER_FLOAT_VEC3: return "vec3";
      case GLHCK_SHADER_FLOAT_VEC4: return "vec4";
      case GLHCK_SHADER_DOUBLE: return "double";
      case GLHCK_SHADER_DOUBLE_VEC2: return "dvec2";
      case GLHCK_SHADER_DOUBLE_VEC3: return "dvec3";
      case GLHCK_SHADER_DOUBLE_VEC4: return "dvec4";
      case GLHCK_SHADER_INT: return "int";
      case GLHCK_SHADER_INT_VEC2: return "ivec2";
      case GLHCK_SHADER_INT_VEC3: return "ivec3";
      case GLHCK_SHADER_INT_VEC4: return "ivec4";
      case GLHCK_SHADER_UNSIGNED_INT: return "uint";
      case GLHCK_SHADER_UNSIGNED_INT_VEC2: return "uvec2";
      case GLHCK_SHADER_UNSIGNED_INT_VEC3: return "uvec3";
      case GLHCK_SHADER_UNSIGNED_INT_VEC4: return "uvec4";
      case GLHCK_SHADER_BOOL: return "bool";
      case GLHCK_SHADER_BOOL_VEC2: return "bvec2";
      case GLHCK_SHADER_BOOL_VEC3: return "bvec3";
      case GLHCK_SHADER_BOOL_VEC4: return "bvec4";
      case GLHCK_SHADER_FLOAT_MAT2: return "mat2";
      case GLHCK_SHADER_FLOAT_MAT3: return "mat3";
      case GLHCK_SHADER_FLOAT_MAT4: return "mat4";
      case GLHCK_SHADER_FLOAT_MAT2x3: return "mat2x3";
      case GLHCK_SHADER_FLOAT_MAT2x4: return "mat2x4";
      case GLHCK_SHADER_FLOAT_MAT3x2: return "mat3x2";
      case GLHCK_SHADER_FLOAT_MAT3x4: return "mat3x4";
      case GLHCK_SHADER_FLOAT_MAT4x2: return "mat4x2";
      case GLHCK_SHADER_FLOAT_MAT4x3: return "mat4x3";
      case GLHCK_SHADER_DOUBLE_MAT2: return "dmat2";
      case GLHCK_SHADER_DOUBLE_MAT3: return "dmat3";
      case GLHCK_SHADER_DOUBLE_MAT4: return "dmat4";
      case GLHCK_SHADER_DOUBLE_MAT2x3: return "dmat2x3";
      case GLHCK_SHADER_DOUBLE_MAT2x4: return "dmat2x4";
      case GLHCK_SHADER_DOUBLE_MAT3x2: return "dmat3x2";
      case GLHCK_SHADER_DOUBLE_MAT3x4: return "dmat3x4";
      case GLHCK_SHADER_DOUBLE_MAT4x2: return "dmat4x2";
      case GLHCK_SHADER_DOUBLE_MAT4x3: return "dmat4x3";
      case GLHCK_SHADER_SAMPLER_1D: return "sampler1D";
      case GLHCK_SHADER_SAMPLER_2D: return "sampler2D";
      case GLHCK_SHADER_SAMPLER_3D: return "sampler3D";
      case GLHCK_SHADER_SAMPLER_CUBE: return "samplerCube";
      case GLHCK_SHADER_SAMPLER_1D_SHADOW: return "sampler1DShadow";
      case GLHCK_SHADER_SAMPLER_2D_SHADOW: return "sampler2DShadow";
      case GLHCK_SHADER_SAMPLER_1D_ARRAY: return "sampler1DArray";
      case GLHCK_SHADER_SAMPLER_2D_ARRAY: return "sampler2DArray";
      case GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW: return "sampler1DArrayShadow";
      case GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW: return "sampler2DArrayShadow";
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE: return "sampler2DMS";
      case GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY: return "sampler2DMSArray";
      case GLHCK_SHADER_SAMPLER_CUBE_SHADOW: return "samplerCubeShadow";
      case GLHCK_SHADER_SAMPLER_BUFFER: return "samplerBuffer";
      case GLHCK_SHADER_SAMPLER_2D_RECT: return "sampler2DRect";
      case GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW: return "sampler2DRectShadow";
      case GLHCK_SHADER_INT_SAMPLER_1D: return "isampler1D";
      case GLHCK_SHADER_INT_SAMPLER_2D: return "isampler2D";
      case GLHCK_SHADER_INT_SAMPLER_3D: return "isampler3D";
      case GLHCK_SHADER_INT_SAMPLER_CUBE: return "isamplerCube";
      case GLHCK_SHADER_INT_SAMPLER_1D_ARRAY: return "isampler1DArray";
      case GLHCK_SHADER_INT_SAMPLER_2D_ARRAY: return "isampler2DArray";
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE: return "isampler2DMS";
      case GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "isampler2DMSArray";
      case GLHCK_SHADER_INT_SAMPLER_BUFFER: return "isamplerBuffer";
      case GLHCK_SHADER_INT_SAMPLER_2D_RECT: return "isampler2DRect";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D: return "usampler1D";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D: return "usampler2D";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D: return "usampler3D";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE: return "usamplerCube";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY: return "usampler2DArray";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY: return "usampler2DArray";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE: return "usampler2DMS";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY: return "usampler2DMSArray";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER: return "usamplerBuffer";
      case GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT: return "usampler2DRect";
      case GLHCK_SHADER_IMAGE_1D: return "image1D";
      case GLHCK_SHADER_IMAGE_2D: return "image2D";
      case GLHCK_SHADER_IMAGE_3D: return "image3D";
      case GLHCK_SHADER_IMAGE_2D_RECT: return "image2DRect";
      case GLHCK_SHADER_IMAGE_CUBE: return "imageCube";
      case GLHCK_SHADER_IMAGE_BUFFER: return "imageBuffer";
      case GLHCK_SHADER_IMAGE_1D_ARRAY: return "image1DArray";
      case GLHCK_SHADER_IMAGE_2D_ARRAY: return "image2DArray";
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE: return "image2DMS";
      case GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY: return "image2DMSArray";
      case GLHCK_SHADER_INT_IMAGE_1D: return "iimage1D";
      case GLHCK_SHADER_INT_IMAGE_2D: return "iimage2D";
      case GLHCK_SHADER_INT_IMAGE_3D: return "iimage3D";
      case GLHCK_SHADER_INT_IMAGE_2D_RECT: return "iimage2DRect";
      case GLHCK_SHADER_INT_IMAGE_CUBE: return "iimageCube";
      case GLHCK_SHADER_INT_IMAGE_BUFFER: return "iimageBuffer";
      case GLHCK_SHADER_INT_IMAGE_1D_ARRAY: return "iimage1DArray";
      case GLHCK_SHADER_INT_IMAGE_2D_ARRAY: return "iimage2DArray";
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE: return "iimage2DMS";
      case GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY: return "iimage2DMSArray";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D: return "uimage1D";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D: return "uimage2D";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D: return "uimage3D";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT: return "uimage2DRect";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE: return "uimageCube";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER: return "uimageBuffer";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY: return "uimage1DArray";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY: return "uimage2DArray";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE: return "uimage2DMS";
      case GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY: return "uimage2DMSArray";
      case GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER: return "atomic_uint";
      default:break;
   }
   return "unknown";
}

/* return shader variable name for glhck shader variable constant */
const GLchar* glhShaderVariableNameForOpenGLConstant(GLenum type) {
   return glhShaderVariableNameForGlhckConstant(glhGlhckShaderVariableTypeForOpenGLType(type));
}

/*
 * binding mappings
 */

void glhTextureBind(glhckTextureTarget target, GLuint object) {
   GL_CALL(glBindTexture(glhckTextureTargetToGL[target], object));
}
void glhTextureActive(GLuint index) {
   GL_CALL(glActiveTexture(GL_TEXTURE0+index));
}
void glhRenderbufferBind(GLuint object) {
   GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, object));
}
void glhFramebufferBind(glhckFramebufferTarget target, GLuint object) {
   GL_CALL(glBindFramebuffer(glhckFramebufferTargetToGL[target], object));
}
void glhHwBufferBind(glhckHwBufferTarget target, GLuint object) {
   GL_CALL(glBindBuffer(glhckHwBufferTargetToGL[target], object));
}
void glhHwBufferBindBase(glhckHwBufferTarget target, GLuint index, GLuint object) {
   GL_CALL(glBindBufferBase(glhckHwBufferTargetToGL[target], index, object));
}
void glhHwBufferBindRange(glhckHwBufferTarget target, GLuint index, GLuint object, GLintptr offset, GLsizeiptr size) {
   GL_CALL(glBindBufferRange(glhckHwBufferTargetToGL[target], index, object, offset, size));
}
void glhHwBufferCreate(glhckHwBufferTarget target, GLsizeiptr size, const GLvoid *data, glhckHwBufferStoreType usage) {
   GL_CALL(glBufferData(glhckHwBufferTargetToGL[target], size, data, glhckHwBufferStoreTypeToGL[usage]));
}
void glhHwBufferFill(glhckHwBufferTarget target, GLintptr offset, GLsizeiptr size, const GLvoid *data) {
   GL_CALL(glBufferSubData(glhckHwBufferTargetToGL[target], offset, size, data));
}
void* glhHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access) {
   return glMapBuffer(glhckHwBufferTargetToGL[target], glhckHwBufferAccessTypeToGL[access]);
}
void glhHwBufferUnmap(glhckHwBufferTarget target) {
   GL_CALL(glUnmapBuffer(glhckHwBufferTargetToGL[target]));
}

/*
 * shared OpenGL renderer functions
 */

/* \brief clear OpenGL buffers */
void glhClear(GLuint bufferBits)
{
   GLuint glBufferBits = 0;
   CALL(2, "%u", bufferBits);

   if (bufferBits & GLHCK_COLOR_BUFFER_BIT)
      glBufferBits |= GL_COLOR_BUFFER_BIT;
   if (bufferBits & GLHCK_DEPTH_BUFFER_BIT)
      glBufferBits |= GL_DEPTH_BUFFER_BIT;

   GL_CALL(glClear(glBufferBits));
}

/* \brief set OpenGL clear color */
void glhClearColor(const glhckColorb *color)
{
   CALL(2, "%p", color);
   float fr = (float)color->r/255;
   float fg = (float)color->g/255;
   float fb = (float)color->b/255;
   float fa = (float)color->a/255;
   GL_CALL(glClearColor(fr, fg, fb, fa));
}

/* \brief set front face */
void glhFrontFace(glhckFaceOrientation orientation)
{
   GL_CALL(glFrontFace(glhckFaceOrientationToGL[orientation]));
}

/* \brief set cull side */
void glhCullFace(glhckCullFaceType face)
{
   GL_CALL(glCullFace(glhckCullFaceTypeToGL[face]));
}

/* \brief get pixels from OpenGL */
void glhBufferGetPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      glhckTextureFormat format, glhckDataType type, GLvoid *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %p", x, y, width, height, format, type, data);
   GL_CALL(glReadPixels(x, y, width, height, glhckTextureFormatToGL[format], glhckDataTypeToGL[type], data));
}

/* \brief blend func wrapper */
void glhBlendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   GL_CALL(glBlendFunc(glhckBlendingModeToGL[blenda], glhckBlendingModeToGL[blendb]));
}

/* \brief set texture parameters */
void glhTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params)
{
   GLenum glTarget;
   GLenum minFilter, magFilter;
   GLenum wrapS, wrapT, wrapR;
   GLenum compareMode, compareFunc;
   CALL(0, "%d, %p", target, params);
   assert(params);

   glTarget    = glhckTextureTargetToGL[target];
   minFilter   = glhckTextureFilterToGL[params->minFilter];
   magFilter   = glhckTextureFilterToGL[params->magFilter];
   wrapS       = glhckTextureWrapToGL[params->wrapS];
   wrapT       = glhckTextureWrapToGL[params->wrapT];
   wrapR       = glhckTextureWrapToGL[params->wrapR];
   compareMode = glhckTextureCompareModeToGL[params->compareMode];
   compareFunc = glhckCompareFuncToGL[params->compareFunc];

   /* remap filters, if no mipmap possible */
   if (!glGenerateMipmap || !params->mipmap) {
      switch (minFilter) {
         case GL_NEAREST_MIPMAP_NEAREST:
         case GL_NEAREST_MIPMAP_LINEAR:
            minFilter = GL_NEAREST;
         case GL_LINEAR_MIPMAP_NEAREST:
         case GL_LINEAR_MIPMAP_LINEAR:
            magFilter = GL_LINEAR;
         default:break;
      }
   }

   GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_MIN_FILTER, minFilter));
   GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_MAG_FILTER, magFilter));
   GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_WRAP_S, wrapS));
   GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_WRAP_T, wrapT));

   if (GLEW_VERSION_1_4) {
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, wrapR));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_MODE, compareMode));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_FUNC, compareFunc));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, params->baseLevel));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, params->maxLevel));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_LOD_BIAS, params->biasLod));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MIN_LOD, params->minLod));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MAX_LOD, params->maxLod));
   }

   if (GLEW_EXT_texture_filter_anisotropic) {
      if (params->mipmap && params->maxAnisotropy) {
         float max;
         GL_CALL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max));
         if (max > params->maxAnisotropy) max = params->maxAnisotropy;
         if (max > 0.0f) GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max));
      }
   }
}

/* \brief generate mipmaps for texture */
void glhTextureMipmap(glhckTextureTarget target)
{
   CALL(0, "%d", target);
   if (!glGenerateMipmap) return;
   GL_CALL(glGenerateMipmap(glhckTextureTargetToGL[target]));
}

/* \brief create texture from data and upload it to OpenGL */
void glhTextureImage(glhckTextureTarget target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint border, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data)
{
   GLenum glTarget, glFormat, glDataType;
   CALL(0, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, width, height, depth, border, format, datatype, size, data);

   glTarget = glhckTextureTargetToGL[target];
   glFormat = glhckTextureFormatToGL[format];
   glDataType = glhckDataTypeToGL[datatype];

   if (_glhckIsCompressedFormat(format)) {
      switch (target) {
         case GLHCK_TEXTURE_1D:
            GL_CALL(glCompressedTexImage1D(glTarget, level, glFormat, width, border, size, data));
            break;
         case GLHCK_TEXTURE_2D:
            GL_CALL(glCompressedTexImage2D(glTarget, level, glFormat, width, height, border, size, data));
            break;
         case GLHCK_TEXTURE_3D:
            GL_CALL(glCompressedTexImage3D(glTarget, level, glFormat, width, height, depth, border, size, data));
            break;
         default:assert(0 && "texture type not supported");break;
      }
   } else {
      switch (target) {
         case GLHCK_TEXTURE_1D:
            GL_CALL(glTexImage1D(glTarget, level, glFormat, width, border, glFormat, glDataType, data));
            break;
         case GLHCK_TEXTURE_2D:
         case GLHCK_TEXTURE_CUBE_MAP:
            GL_CALL(glTexImage2D(glTarget, level, glFormat, width, height, border, glFormat, glDataType, data));
            break;
         case GLHCK_TEXTURE_3D:
            GL_CALL(glTexImage3D(glTarget, level, glFormat, width, height, depth, border, glFormat, glDataType, data));
            break;
         default:assert(0 && "texture type not supported");break;
      }
   }
}

/* \brief fill texture with data */
void glhTextureFill(glhckTextureTarget target, GLint level, GLint x, GLint y, GLint z, GLsizei width, GLsizei height, GLsizei depth, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data)
{
   GLenum glTarget, glFormat, glDataType;
   CALL(1, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, x, y, z, width, height, depth, format, datatype, size, data);

   glTarget = glhckTextureTargetToGL[target];
   glFormat = glhckTextureFormatToGL[format];
   glDataType = glhckDataTypeToGL[datatype];

   if (_glhckIsCompressedFormat(format)) {
      switch (target) {
         case GLHCK_TEXTURE_1D:
            GL_CALL(glCompressedTexSubImage1D(glTarget, level, x, width, glFormat, size, data));
            break;
         case GLHCK_TEXTURE_2D:
            GL_CALL(glCompressedTexSubImage2D(glTarget, level, x, y, width, height, glFormat, size, data));
            break;
         case GLHCK_TEXTURE_3D:
            GL_CALL(glCompressedTexSubImage3D(glTarget, level, x, y, z, width, height, depth, glFormat, size, data));
            break;
         default:assert(0 && "texture type not supported");break;
      }
   } else {
      switch (target) {
         case GLHCK_TEXTURE_1D:
            GL_CALL(glTexSubImage1D(glTarget, level, x, width, glFormat, glDataType, data));
            break;
         case GLHCK_TEXTURE_2D:
            GL_CALL(glTexSubImage2D(glTarget, level, x, y, width, height, glFormat, glDataType, data));
            break;
         case GLHCK_TEXTURE_3D:
            GL_CALL(glTexSubImage3D(glTarget, level, x, y, z, width, height, depth, glFormat, glDataType, data));
            break;
         default:assert(0 && "texture type not supported");break;
      }
   }
}

/* \brief glRenderbufferStorage wrapper */
void glhRenderbufferStorage(GLsizei width, GLsizei height, glhckTextureFormat format)
{
   CALL(0, "%d, %d, %d", width, height, format);
   glRenderbufferStorage(GL_RENDERBUFFER, glhckTextureFormatToGL[format], width, height);
}

/* \brief glFramebufferTexture wrapper with error checking */
GLint glhFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, GLuint texture, glhckFramebufferAttachmentType attachment)
{
   GLenum glTarget, glTexTarget, glAttachment;
   CALL(0, "%d, %d, %u, %d", framebufferTarget, textureTarget, texture, attachment);

   glTarget     = glhckFramebufferTargetToGL[framebufferTarget];
   glTexTarget  = glhckTextureTargetToGL[textureTarget];
   glAttachment = glhckFramebufferAttachmentTypeToGL[attachment];

   switch (textureTarget) {
      case GLHCK_TEXTURE_1D:
         GL_CALL(glFramebufferTexture1D(glTarget, glAttachment, glTexTarget, texture, 0));
         break;
      case GLHCK_TEXTURE_2D:
      case GLHCK_TEXTURE_CUBE_MAP:
         GL_CALL(glFramebufferTexture2D(glTarget, glAttachment, glTexTarget, texture, 0));
         break;
      case GLHCK_TEXTURE_3D:
         GL_CALL(glFramebufferTexture3D(glTarget, glAttachment, glTexTarget, texture, 0, 0));
         break;
      default:assert(0 && "texture type not supported");break;
   }

   if (GL_CHECK(glCheckFramebufferStatus(glTarget)) != GL_FRAMEBUFFER_COMPLETE)
      goto fbo_fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fbo_fail:
   DEBUG(GLHCK_DBG_ERROR, "Framebuffer is not complete");
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief glFramebufferRenderbuffer wrapper with error checking */
GLint glhFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, GLuint buffer, glhckFramebufferAttachmentType attachment)
{
   GLenum glTarget, glAttachment;
   CALL(0, "%d, %u, %d", framebufferTarget, buffer, attachment);

   glTarget     = glhckFramebufferTargetToGL[framebufferTarget];
   glAttachment = glhckFramebufferAttachmentTypeToGL[attachment];
   glFramebufferRenderbuffer(glTarget, glAttachment, GL_RENDERBUFFER, buffer);

   if (GL_CHECK(glCheckFramebufferStatus(glTarget)) != GL_FRAMEBUFFER_COMPLETE)
      goto fbo_fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fbo_fail:
   DEBUG(GLHCK_DBG_ERROR, "Framebuffer is not complete");
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief attach uniform buffer object to shader */
GLuint glhProgramAttachUniformBuffer(GLuint program, const GLchar *uboName, GLuint location)
{
   GLuint ubo;
   CALL(0, "%u, %s, %u", program, uboName, location);

   if ((ubo = glGetUniformBlockIndex(program, uboName))) {
      GL_CALL(glUniformBlockBinding(program, ubo, location));
   }

   RET(0, "%u", ubo);
   return ubo;
}

/* \brief create uniform buffer from shader */
_glhckHwBufferShaderUniform* glhProgramUniformBufferList(GLuint program, const GLchar *uboName, GLsizei *uboSize)
{
   GLchar name[255], *uname = NULL;
   GLuint index;
   GLint count, i, *indices = NULL;
   GLsizei length, size;
   GLenum type;
   _glhckHwBufferShaderUniform *uniforms = NULL, *u, *un;
   CALL(0, "%u, %s, %p", program, uboName, uboSize);
   if (uboSize) *uboSize = 0;

   /* get uniform block index */
   index = glGetUniformBlockIndex(program, uboName);
   if (index == GL_INVALID_INDEX) goto fail;

   /* get uniform block size and initialize the hw buffer */
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_DATA_SIZE, &size));
   if (uboSize) *uboSize = size;

   /* get uniform count for UBO */
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &count));
   if (!count) goto no_uniforms;

   /* allocate space for uniform iteration */
   if (!(indices = _glhckMalloc(sizeof(GLuint) * count)))
      goto fail;

   /* get indices for UBO's member uniforms */
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices));

   for (i = 0; i < count; ++i) {
      index = (GLuint)indices[i];

      /* get uniform information */
      GL_CALL(glGetActiveUniform(program, index, sizeof(name)-1, &length, &size, &type, name));

      /* cut out [0] for arrays */
      if (!strcmp(name+strlen(name)-3, "[0]"))
         length -= 3;

      /* allocate name */
      if (!(uname = _glhckCalloc(1, length+4)))
         goto fail;

      /* allocate new uniform slot */
      memcpy(uname, name, length);
      for (u = uniforms; u && u->next; u = u->next);
      if (u) u = u->next  = _glhckCalloc(1, sizeof(_glhckHwBufferShaderUniform));
      else   u = uniforms = _glhckCalloc(1, sizeof(_glhckHwBufferShaderUniform));
      if (!u) goto fail;

      /* assign information */
      u->name = uname;
      glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &u->offset);
      u->typeName = glhShaderVariableNameForOpenGLConstant(type);
      u->type = glhGlhckShaderVariableTypeForOpenGLType(type);
      u->size = size;
   }

   _glhckFree(indices);
   RET(0, "%p", uniforms);
   return uniforms;

fail:
no_uniforms:
   if (uniforms) {
      for (u = uniforms; u; u = un) {
         un = u->next;
         _glhckFree(u);
      }
   }
   IFDO(_glhckFree, indices);
   IFDO(_glhckFree, uname);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get attribute list from program */
_glhckShaderAttribute* glhProgramAttributeList(GLuint obj)
{
   GLenum type;
   GLchar name[255];
   GLint count, i;
   GLsizei size, length;
   _glhckShaderAttribute *attributes = NULL, *a, *an;
   CALL(0, "%u", obj);

   /* get attribute count */
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_ATTRIBUTES, &count));
   if (!count) goto no_attributes;

   for (i = 0; i != count; ++i) {
      /* get attribute information */
      GL_CALL(glGetActiveAttrib(obj, i, sizeof(name)-1, &length, &size, &type, name));

      /* allocate new attribute slot */
      for (a = attributes; a && a->next; a = a->next);
      if (a) a = a->next    = _glhckCalloc(1, sizeof(_glhckShaderAttribute));
      else   a = attributes = _glhckCalloc(1, sizeof(_glhckShaderAttribute));
      if (!a || !(a->name = _glhckMalloc(length+1)))
         goto fail;

      /* assign information */
      memcpy(a->name, name, length+1);
      a->typeName = glhShaderVariableNameForOpenGLConstant(type);
      a->location = glGetAttribLocation(obj, a->name);
      a->type = glhGlhckShaderVariableTypeForOpenGLType(type);
      a->size = size;
   }

   RET(0, "%p", attributes);
   return attributes;

fail:
no_attributes:
   if (attributes) {
      for (a = attributes; a; a = an) {
         an = a->next;
         _glhckFree(a->name);
         _glhckFree(a);
      }
   }
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get uniform list from program */
_glhckShaderUniform* glhProgramUniformList(GLuint obj)
{
   GLenum type;
   GLchar name[255], *uname = NULL, *tmp;
   GLint count, i, i2;
   GLuint location;
   GLsizei length, size;
   _glhckShaderUniform *uniforms = NULL, *u, *un;
   CALL(0, "%u", obj);

   /* get uniform count */
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_UNIFORMS, &count));
   if (!count) goto no_uniforms;

   for (i = 0; i != count; ++i) {
      /* get uniform information */
      GL_CALL(glGetActiveUniform(obj, i, sizeof(name)-1, &length, &size, &type, name));

      /* cut out [0] for arrays */
      if (!strcmp(name+strlen(name)-3, "[0]"))
         length -= 3;

      /* allocate name */
      if (!(uname = _glhckCalloc(1, length+4)))
         goto fail;

      /* get uniform location */
      memcpy(uname, name, length);
      location = glGetUniformLocation(obj, uname);
      if (location == UINT_MAX) {
         _glhckFree(uname);
         continue;
      }

      /* allocate new uniform slot */
      for (u = uniforms; u && u->next; u = u->next);
      if (u) u = u->next  = _glhckCalloc(1, sizeof(_glhckShaderUniform));
      else   u = uniforms = _glhckCalloc(1, sizeof(_glhckShaderUniform));
      if (!u) goto fail;

      /* assign information */
      u->name = uname;
      u->typeName = glhShaderVariableNameForOpenGLConstant(type);
      u->location = location;
      u->type = glhGlhckShaderVariableTypeForOpenGLType(type);
      u->size = size;

      /* store for iterating the array slots */
      tmp = uname;

      /* generate uniform bindings for array slots */
      for (i2 = 1; i2 < size; ++i2) {
         if (!(uname = _glhckCalloc(1, length+4)))
            goto fail;

         /* get uniform location */
         snprintf(uname, length+4, "%s[%d]", tmp, i2);
         location = glGetUniformLocation(obj, uname);
         if (location == UINT_MAX) {
            _glhckFree(uname);
            continue;
         }

         for (u = uniforms; u && u->next; u = u->next);
         if (u) u = u->next  = _glhckCalloc(1, sizeof(_glhckShaderUniform));
         else   u = uniforms = _glhckCalloc(1, sizeof(_glhckShaderUniform));
         if (!u) goto fail;

         u->name = uname;
         u->typeName = glhShaderVariableNameForOpenGLConstant(type);
         u->location = location;
         u->type = glhGlhckShaderVariableTypeForOpenGLType(type);
         u->size = size;
      }
   }

   RET(0, "%p", uniforms);
   return uniforms;

fail:
no_uniforms:
   if (uniforms) {
      for (u = uniforms; u; u = un) {
         un = u->next;
         _glhckFree(u);
      }
   }
   IFDO(_glhckFree, uname);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief set shader uniform */
void glhProgramUniform(GLuint obj, _glhckShaderUniform *uniform, GLsizei count, const GLvoid *value)
{
   CALL(2, "%u, %p, %d, %p", obj, uniform, count, value);

   /* automatically figure out the data type */
   switch (uniform->type) {
      case GLHCK_SHADER_FLOAT:
         GL_CALL(glUniform1fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC2:
         GL_CALL(glUniform2fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC3:
         GL_CALL(glUniform3fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_VEC4:
         GL_CALL(glUniform4fv(uniform->location, count, (GLfloat*)value));
         break;
      case GLHCK_SHADER_DOUBLE:
         GL_CALL(glUniform1dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC2:
         GL_CALL(glUniform2dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC3:
         GL_CALL(glUniform3dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_VEC4:
         GL_CALL(glUniform4dv(uniform->location, count, (GLdouble*)value));
         break;
      case GLHCK_SHADER_INT:
      case GLHCK_SHADER_BOOL:
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
         GL_CALL(glUniform1iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC2:
      case GLHCK_SHADER_BOOL_VEC2:
         GL_CALL(glUniform2iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC3:
      case GLHCK_SHADER_BOOL_VEC3:
         GL_CALL(glUniform3iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_INT_VEC4:
      case GLHCK_SHADER_BOOL_VEC4:
         GL_CALL(glUniform4iv(uniform->location, count, (GLint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT:
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
         GL_CALL(glUniform1uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC2:
         GL_CALL(glUniform2uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC3:
         GL_CALL(glUniform3uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_UNSIGNED_INT_VEC4:
         GL_CALL(glUniform4uiv(uniform->location, count, (GLuint*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2:
         GL_CALL(glUniformMatrix2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3:
         GL_CALL(glUniformMatrix3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4:
         GL_CALL(glUniformMatrix4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2x3:
         GL_CALL(glUniformMatrix2x3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT2x4:
         GL_CALL(glUniformMatrix2x4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3x2:
         GL_CALL(glUniformMatrix3x2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT3x4:
         GL_CALL(glUniformMatrix3x4fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4x2:
         GL_CALL(glUniformMatrix4x2fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_FLOAT_MAT4x3:
         GL_CALL(glUniformMatrix4x3fv(uniform->location, count, 0, (GLfloat*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2:
         GL_CALL(glUniformMatrix2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3:
         GL_CALL(glUniformMatrix3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4:
         GL_CALL(glUniformMatrix4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2x3:
         GL_CALL(glUniformMatrix2x3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT2x4:
         GL_CALL(glUniformMatrix2x4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3x2:
         GL_CALL(glUniformMatrix3x2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT3x4:
         GL_CALL(glUniformMatrix3x4dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4x2:
         GL_CALL(glUniformMatrix4x2dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      case GLHCK_SHADER_DOUBLE_MAT4x3:
         GL_CALL(glUniformMatrix4x3dv(uniform->location, count, 0, (GLdouble*)value));
         break;
      default:
         DEBUG(GLHCK_DBG_ERROR, "uniform type not implemented.");
         break;
   }
}

/* \brief draw interleaved geometry (arrays) */
static void glhDrawArrays(const glhckGeometry *geometry, glhckGeometryType gtype)
{
   GL_CALL(glDrawArrays(glhckGeometryTypeToGL[gtype], 0, geometry->vertexCount));
}

/* \brief draw interleaved geometry (elements) */
static void glhDrawElements(const glhckGeometry *geometry, glhckGeometryType gtype)
{
   __GLHCKindexType *type = GLHCKIT(geometry->indexType);
   GL_CALL(glDrawElements(glhckGeometryTypeToGL[gtype], geometry->indexCount, glhckDataTypeToGL[type->dataType], geometry->indices));
}

/* \brief draw geometry */
void glhGeometryRender(const glhckGeometry *geometry, glhckGeometryType type)
{
   if (geometry->indices) glhDrawElements(geometry, type);
   else glhDrawArrays(geometry, type);
}

/*
 * misc
 */

/* \brief debug output callback function */
void glhDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, void *user)
{
   (void)source; (void)type; (void)id; (void)severity; (void)length; (void)user;
   printf(" [GPU::DBG] %s\n", message);
}

/* \brief setup OpenGL debug output extension */
void glhSetupDebugOutput(void)
{
   if (!GLEW_KHR_debug) return;
   GL_CALL(glDebugMessageCallbackARB(glhDebugCallback, NULL));
   GL_CALL(glEnable(GL_DEBUG_OUTPUT));
}

/*
 * opengl init
 */

static int glhRenderFeatures(const char *renderName)
{
   GLchar *version, *vendor, *extensions, *extcpy, *s;
   GLchar *shaderVersion, *renderer;
   GLint major = 0, minor = 0, patch = 0;
   GLint shaderMajor = 0, shaderMinor = 0, shaderPatch = 0;
   glhckRenderFeatures *features;
   CALL(0, "%s", renderName);
   assert(renderName);

   if (GLEW_VERSION_3_0) {
      GL_CALL(glGetIntegerv(GL_MAJOR_VERSION, &major));
      GL_CALL(glGetIntegerv(GL_MINOR_VERSION, &minor));
   } else {
      version = (GLchar*)GL_CALL(glGetString(GL_VERSION));
      for (; version &&
          !sscanf(version, "%d.%d.%d", &major, &minor, &patch);
          ++version);
   }

   vendor = (GLchar*)GL_CALL(glGetString(GL_VENDOR));
   renderer = (GLchar*)GL_CALL(glGetString(GL_RENDERER));

   if (GLEW_VERSION_1_4 || GLEW_ES_VERSION_2_0) {
      shaderVersion = (GLchar*)GL_CALL(glGetString(GL_SHADING_LANGUAGE_VERSION));
      for (; shaderVersion &&
            !sscanf(shaderVersion, "%d.%d.%d", &shaderMajor, &shaderMinor, &shaderPatch);
            ++shaderVersion);
   }

   /* try to identify driver */
   if (vendor && _glhckStrupstr(vendor, "MESA"))
      GLHCKR()->driver = GLHCK_DRIVER_MESA;
   else if (vendor && strstr(vendor, "NVIDIA"))
      GLHCKR()->driver = GLHCK_DRIVER_NVIDIA;
   else if (vendor && strstr(vendor, "ATI"))
      GLHCKR()->driver = GLHCK_DRIVER_ATI;
   else if (vendor && strstr(vendor, "Imagination Technologies"))
      GLHCKR()->driver = GLHCK_DRIVER_IMGTEC;
   else
      GLHCKR()->driver = GLHCK_DRIVER_OTHER;

   DEBUG(3, "GLEW [%s]", glewGetString(GLEW_VERSION));
   DEBUG(3, "%s [%d.%d.%d] [%s]", renderName, major, minor, patch, vendor);
   DEBUG(3, "%s [%d.%d.%d]", (renderer?renderer:"Generic"), shaderMajor, shaderMinor, shaderPatch);
   extensions = (char*)GL_CALL(glGetString(GL_EXTENSIONS));

   if (extensions) {
      extcpy = _glhckStrdup(extensions);
      s = strtok(extcpy, " ");
      while (s) {
         DEBUG(3, "%s", s);
         s = strtok(NULL, " ");
      }
      _glhckFree(extcpy);
   }

   features = &GLHCKR()->features;
   features->version.major = major;
   features->version.minor = minor;
   features->version.patch = patch;
   features->version.shaderMajor = shaderMajor;
   features->version.shaderMinor = shaderMinor;
   features->version.shaderPatch = shaderPatch;
   GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &features->texture.maxTextureSize));
   GL_CALL(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &features->texture.maxRenderbufferSize));
   features->texture.hasNativeNpotSupport = (GLEW_VERSION_2_0 || GLEW_VERSION_3_0);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

int glhCheckSupport(const char *renderName)
{
   if (glewInit() != GLEW_OK)
      return RETURN_FAIL;

#if GLHCK_USE_GLES1
#if 0
   if (!GLEW_OES_element_index_uint) {
      DEBUG(GLHCK_DBG_ERROR, "GLES1.1 needs GL_OES_element_index_uint extension!");
      return RETURN_FAIL;
   }
#endif

   if (GLEW_OES_framebuffer_object) {
      DEBUG(GLHCK_DBG_ERROR, "GLES1.1 needs GL_OES_framebuffer_object extension!");
      return RETURN_FAIL;
   }

#if !EMSCRIPTEN
   glGenerateMipmap = glGenerateMipmapOES;
   glBindFramebuffer = glBindFramebufferOES;
   glGenFramebuffers = glGenFramebuffersOES;
   glDeleteFramebuffers = glDeleteFramebuffersOES;
   glFramebufferTexture2D = glFramebufferTexture2DOES;
   glCheckFramebufferStatus = glCheckFramebufferStatusOES;
   glFramebufferRenderbuffer = glFramebufferRenderbufferOES;
   glBindRenderbuffer = glBindRenderbufferOES;
   glGenRenderbuffers = glGenRenderbuffersOES;
   glDeleteRenderbuffers = glDeleteRenderbuffersOES;
   glRenderbufferStorage = glRenderbufferStorageOES;
#endif
#endif

   /* fill the renderer's features struct */
   if (glhRenderFeatures(renderName) != RETURN_OK)
      return RETURN_FAIL;

   return RETURN_OK;
}

/* vim: set ts=8 sw=3 tw=0 :*/
