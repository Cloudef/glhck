#ifndef __glhck_opengl_helper_h__
#define __glhck_opengl_helper_h__

#include <glhck/glhck.h>
#include <limits.h> /* for limits */
#include <stdio.h>  /* for snprintf */
#include <stdlib.h>

#include "types/shader.h"
#include "types/hwbuffer.h"
#include "pool/pool.h"

#if !defined(OPENGL3) && !defined(OPENGL2)
#  error "Must have one of the above defined before inclusion."
#endif

#ifndef __STRING
#  define __STRING(x) #x
#endif

#ifndef GLchar
#  define GLchar char
#endif

/* check gl errors on debug build */
#ifdef NDEBUG
#  define GL_CALL(x) x
#else
#  define GL_CALL(x) x; GL_ERROR(__LINE__, __func__, __STRING(x));
#endif

/* check return value of gl function on debug build */
#ifdef NDEBUG
#  define GL_CHECK(x) x
#else
#  define GL_CHECK(x) GL_CHECK_ERROR(__func__, __STRING(x), x)
#endif

#if defined(__MINGW32__) || defined(__CYGWIN__)
#  define GLCB __stdcall
#elif (_MSC_VER >= 800) || defined(_STDCALL_SUPPORTED) || defined(__BORLANDC__)
#  define GLCB __stdcall
#else
#  define GLCB
#endif

#ifdef OPENGL3
#  define GL_LUMINANCE GL_INVALID_ENUM
#  define GL_LUMINANCE_ALPHA GL_INVALID_ENUM
#endif

#ifdef OPENGL2
#  define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS GL_INVALID_ENUM
#  define GL_COMPARE_REF_TO_TEXTURE GL_INVALID_ENUM
#  define GL_UNSIGNED_INT_VEC2 GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_VEC3 GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_VEC4 GL_INVALID_VALUE
#  define GL_SAMPLER_1D_ARRAY GL_INVALID_VALUE
#  define GL_SAMPLER_2D_ARRAY GL_INVALID_VALUE
#  define GL_SAMPLER_1D_ARRAY_SHADOW GL_INVALID_VALUE
#  define GL_SAMPLER_2D_ARRAY_SHADOW GL_INVALID_VALUE
#  define GL_SAMPLER_CUBE_SHADOW GL_INVALID_VALUE
#  define GL_SAMPLER_BUFFER GL_INVALID_VALUE
#  define GL_SAMPLER_2D_RECT GL_INVALID_VALUE
#  define GL_SAMPLER_2D_RECT_SHADOW GL_INVALID_VALUE
#  define GL_INT_SAMPLER_1D GL_INVALID_VALUE
#  define GL_INT_SAMPLER_2D GL_INVALID_VALUE
#  define GL_INT_SAMPLER_3D GL_INVALID_VALUE
#  define GL_INT_SAMPLER_CUBE GL_INVALID_VALUE
#  define GL_INT_SAMPLER_1D_ARRAY GL_INVALID_VALUE
#  define GL_INT_SAMPLER_2D_ARRAY GL_INVALID_VALUE
#  define GL_INT_SAMPLER_BUFFER GL_INVALID_VALUE
#  define GL_INT_SAMPLER_2D_RECT GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_1D GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_2D GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_3D GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_CUBE GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_1D_ARRAY GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_2D_ARRAY GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_BUFFER GL_INVALID_VALUE
#  define GL_UNSIGNED_INT_SAMPLER_2D_RECT GL_INVALID_VALUE
#endif

#ifndef NDEBUG
__attribute__((used))
static void GL_ERROR(unsigned int line, const char *func, const char *glfunc)
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

__attribute__((used))
static GLenum GL_CHECK_ERROR(const char *func, const char *glfunc, GLenum error)
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

__attribute__((used))
static GLenum glhckCullFaceTypeToGL[] = {
   GL_BACK, /* GLHCK_BACK */
   GL_FRONT, /* GLHCK_FRONT */
};

__attribute__((used))
static GLenum glhckFaceOrientationToGL[] = {
   GL_CW, /* GLHCK_CW */
   GL_CCW, /* GLHCK_CCW */
};

__attribute__((used))
static GLenum glhckGeometryTypeToGL[] = {
   GL_POINTS, /* GLHCK_POINTS */
   GL_LINES, /* GLHCK_LINES */
   GL_LINE_LOOP, /* GLHCK_LINE_LOOP */
   GL_LINE_STRIP, /* GLHCK_LINE_STRIP */
   GL_TRIANGLES, /* GLHCK_TRIANGLES */
   GL_TRIANGLE_STRIP, /* GLHCK_TRIANGLE_STRIP */
};

__attribute__((used))
static GLenum glhckFramebufferTargetToGL[] = {
   GL_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER */
   GL_READ_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER_READ */
   GL_DRAW_FRAMEBUFFER, /* GLHCK_FRAMEBUFFER_DRAW */
};

__attribute__((used))
static GLenum glhckFramebufferAttachmentTypeToGL[] = {
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

__attribute__((used))
static GLenum glhckHwBufferTargetToGL[] = {
   GL_ARRAY_BUFFER, /* GLHCK_ARRAY_BUFFER */
   GL_COPY_READ_BUFFER, /* GLHCK_COPY_READ_BUFFER */
   GL_COPY_WRITE_BUFFER, /* GLHCK_COPY_WRITE_BUFFER */
   GL_ELEMENT_ARRAY_BUFFER, /* GLHCK_ELEMENT_ARRAY_BUFFER */
   GL_PIXEL_PACK_BUFFER, /* GLHCK_PIXEL_PACK_BUFFER */
   GL_PIXEL_UNPACK_BUFFER, /* GLHCK_PIXEL_UNPACK_BUFFER */
   GL_TEXTURE_BUFFER, /* GLHCK_TEXTURE_BUFFER */
   GL_TRANSFORM_FEEDBACK_BUFFER, /* GLHCK_TRANSFORM_FEEDBACK_BUFFER */
   GL_UNIFORM_BUFFER, /* GLHCK_UNIFORM_BUFFER */
   GL_SHADER_STORAGE_BUFFER, /* GLHCK_SHADER_STORAGE_BUFFER */
   GL_ATOMIC_COUNTER_BUFFER, /* GLHCK_ATOMIC_COUNTER_BUFFER */
};

__attribute__((used))
static GLenum glhckHwBufferStoreTypeToGL[] = {
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

__attribute__((used))
static GLenum glhckHwBufferAccessTypeToGL[] = {
   GL_READ_ONLY, /* GLHCK_BUFFER_READ_ONLY */
   GL_WRITE_ONLY, /* GLHCK_BUFFER_WRITE_ONLY */
   GL_READ_WRITE, /* GLHCK_BUFFER_READ_WRITE */
};

__attribute__((used))
static GLenum glhckShaderTypeToGL[] = {
   GL_VERTEX_SHADER, /* GLHCK_VERTEX_SHADER */
   GL_FRAGMENT_SHADER, /* GLHCK_FRAGMENT_SHADER */
};

__attribute__((used))
static GLenum glhckTextureWrapToGL[] = {
   GL_REPEAT, /* GLHCK_REPEAT */
   GL_MIRRORED_REPEAT, /* GLHCK_MIRRORED_REPEAT */
   GL_CLAMP_TO_EDGE, /* GLHCK_CLAMP_TO_EDGE */
   GL_CLAMP_TO_BORDER, /* GLHCK_CLAMP_TO_BORDER */
};

__attribute__((used))
static GLenum glhckTextureFilterToGL[] = {
   GL_NEAREST, /* GLHCK_NEAREST */
   GL_LINEAR, /* GLHCK_LINEAR */
   GL_NEAREST_MIPMAP_NEAREST, /* GLHCK_NEAREST_MIPMAP_NEAREST */
   GL_LINEAR_MIPMAP_NEAREST, /* GLHCK_LINEAR_MIPMAP_NEAREST */
   GL_NEAREST_MIPMAP_LINEAR, /* GLHCK_NEAREST_MIPMAP_LINEAR */
   GL_LINEAR_MIPMAP_LINEAR, /* GLHCK_LINEAR_MIPMAP_LINEAR */
};

__attribute__((used))
static GLenum glhckTextureCompareModeToGL[] = {
   0, /* GLHCK_COMPARE_NONE */
   GL_COMPARE_REF_TO_TEXTURE, /* GLHCK_COMPARE_REF_TO_TEXTURE */
};

__attribute__((used))
static GLenum glhckCompareFuncToGL[] = {
   GL_LEQUAL, /* GLHCK_LEQUAL */
   GL_GEQUAL, /* GLHCK_GEQUAL */
   GL_LESS, /* GLHCK_LESS */
   GL_GREATER, /* GLHCK_GREATER */
   GL_EQUAL, /* GLHCK_EQUAL */
   GL_NOTEQUAL, /* GLHCK_NOTEQUAL */
   GL_ALWAYS, /* GLHCK_ALWAYS */
   GL_NEVER, /* GLHCK_NEVER */
};

__attribute__((used))
static GLenum glhckTextureTargetToGL[] = {
   GL_TEXTURE_1D, /* GLHCK_TEXTURE_1D */
   GL_TEXTURE_2D, /* GLHCK_TEXTURE_2D */
   GL_TEXTURE_3D, /* GLHCK_TEXTURE_3D */
   GL_TEXTURE_CUBE_MAP, /* GLHCK_TEXTURE_CUBE_MAP */
};

__attribute__((used))
static GLenum glhckTextureFormatToGL[] = {
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

__attribute__((used))
static GLenum glhckDataTypeToGL[] = {
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

__attribute__((used))
static GLenum glhckBlendingModeToGL[] = {
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

__attribute__((used))
static GLenum glhckShaderVariableTypeToGL[] = {
   GL_FLOAT, /* GLHCK_SHADER_FLOAT */
   GL_FLOAT_VEC2, /* GLHCK_SHADER_FLOAT_VEC2 */
   GL_FLOAT_VEC3, /* GLHCK_SHADER_FLOAT_VEC3 */
   GL_FLOAT_VEC4, /* GLHCK_SHADER_FLOAT_VEC4 */
   GL_DOUBLE, /* GLHCK_SHADER_DOUBLE */
   GL_DOUBLE_VEC2, /* GLHCK_SHADER_DOUBLE_VEC2 */
   GL_DOUBLE_VEC3, /* GLHCK_SHADER_DOUBLE_VEC3 */
   GL_DOUBLE_VEC4, /* GLHCK_SHADER_DOUBLE_VEC4 */
   GL_INT, /* GLHCK_SHADER_INT */
   GL_INT_VEC2, /* GLHCK_SHADER_INT_VEC2 */
   GL_INT_VEC3, /* GLHCK_SHADER_INT_VEC3 */
   GL_INT_VEC4, /* GLHCK_SHADER_INT_VEC4 */
   GL_UNSIGNED_INT, /* GLHCK_SHADER_UNSIGNED_INT */
   GL_UNSIGNED_INT_VEC2, /* GLHCK_SHADER_UNSIGNED_INT_VEC2 */
   GL_UNSIGNED_INT_VEC3, /* GLHCK_SHADER_UNSIGNED_INT_VEC3 */
   GL_UNSIGNED_INT_VEC4, /* GLHCK_SHADER_UNSIGNED_INT_VEC4 */
   GL_BOOL, /* GLHCK_SHADER_BOOL */
   GL_BOOL_VEC2, /* GLHCK_SHADER_BOOL_VEC2 */
   GL_BOOL_VEC3, /* GLHCK_SHADER_BOOL_VEC3 */
   GL_BOOL_VEC4, /* GLHCK_SHADER_BOOL_VEC4 */
   GL_FLOAT_MAT2, /* GLHCK_SHADER_FLOAT_MAT2 */
   GL_FLOAT_MAT3, /* GLHCK_SHADER_FLOAT_MAT3 */
   GL_FLOAT_MAT4, /* GLHCK_SHADER_FLOAT_MAT4 */
   GL_FLOAT_MAT2x3, /* GLHCK_SHADER_FLOAT_MAT2x3 */
   GL_FLOAT_MAT2x4, /* GLHCK_SHADER_FLOAT_MAT2x4 */
   GL_FLOAT_MAT3x2, /* GLHCK_SHADER_FLOAT_MAT3x2 */
   GL_FLOAT_MAT3x4, /* GLHCK_SHADER_FLOAT_MAT3x4 */
   GL_FLOAT_MAT4x2, /* GLHCK_SHADER_FLOAT_MAT4x2 */
   GL_FLOAT_MAT4x3, /* GLHCK_SHADER_FLOAT_MAT4x3 */
   GL_DOUBLE_MAT2, /* GLHCK_SHADER_DOUBLE_MAT2 */
   GL_DOUBLE_MAT3, /* GLHCK_SHADER_DOUBLE_MAT3 */
   GL_DOUBLE_MAT4, /* GLHCK_SHADER_DOUBLE_MAT4 */
   GL_DOUBLE_MAT2x3, /* GLHCK_SHADER_DOUBLE_MAT2x3 */
   GL_DOUBLE_MAT2x4, /* GLHCK_SHADER_DOUBLE_MAT2x4 */
   GL_DOUBLE_MAT3x2, /* GLHCK_SHADER_DOUBLE_MAT3x2 */
   GL_DOUBLE_MAT3x4, /* GLHCK_SHADER_DOUBLE_MAT3x4 */
   GL_DOUBLE_MAT4x2, /* GLHCK_SHADER_DOUBLE_MAT4x2 */
   GL_DOUBLE_MAT4x3, /* GLHCK_SHADER_DOUBLE_MAT4x3 */
   GL_SAMPLER_1D, /* GLHCK_SHADER_SAMPLER_1D */
   GL_SAMPLER_2D, /* GLHCK_SHADER_SAMPLER_2D */
   GL_SAMPLER_3D, /* GLHCK_SHADER_SAMPLER_3D */
   GL_SAMPLER_CUBE, /* GLHCK_SHADER_SAMPLER_CUBE */
   GL_SAMPLER_1D_SHADOW, /* GLHCK_SHADER_SAMPLER_1D_SHADOW */
   GL_SAMPLER_2D_SHADOW, /* GLHCK_SHADER_SAMPLER_2D_SHADOW */
   GL_SAMPLER_1D_ARRAY, /* GLHCK_SHADER_SAMPLER_1D_ARRAY */
   GL_SAMPLER_2D_ARRAY, /* GLHCK_SHADER_SAMPLER_2D_ARRAY */
   GL_SAMPLER_1D_ARRAY_SHADOW, /* GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW */
   GL_SAMPLER_2D_ARRAY_SHADOW, /* GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW */
   GL_SAMPLER_2D_MULTISAMPLE, /* GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE */
   GL_SAMPLER_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY */
   GL_SAMPLER_CUBE_SHADOW, /* GLHCK_SHADER_SAMPLER_CUBE_SHADOW */
   GL_SAMPLER_BUFFER, /* GLHCK_SHADER_SAMPLER_BUFFER */
   GL_SAMPLER_2D_RECT, /* GLHCK_SHADER_SAMPLER_2D_RECT */
   GL_SAMPLER_2D_RECT_SHADOW, /* GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW */
   GL_INT_SAMPLER_1D, /* GLHCK_SHADER_INT_SAMPLER_1D */
   GL_INT_SAMPLER_2D, /* GLHCK_SHADER_INT_SAMPLER_2D */
   GL_INT_SAMPLER_3D, /* GLHCK_SHADER_INT_SAMPLER_3D */
   GL_INT_SAMPLER_CUBE, /* GLHCK_SHADER_INT_SAMPLER_CUBE */
   GL_INT_SAMPLER_1D_ARRAY, /* GLHCK_SHADER_INT_SAMPLER_1D_ARRAY */
   GL_INT_SAMPLER_2D_ARRAY, /* GLHCK_SHADER_INT_SAMPLER_2D_ARRAY */
   GL_INT_SAMPLER_2D_MULTISAMPLE, /* GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE */
   GL_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
   GL_INT_SAMPLER_BUFFER, /* GLHCK_SHADER_INT_SAMPLER_BUFFER */
   GL_INT_SAMPLER_2D_RECT, /* GLHCK_SHADER_INT_SAMPLER_2D_RECT */
   GL_UNSIGNED_INT_SAMPLER_1D, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D */
   GL_UNSIGNED_INT_SAMPLER_2D, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D */
   GL_UNSIGNED_INT_SAMPLER_3D, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D */
   GL_UNSIGNED_INT_SAMPLER_CUBE, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE */
   GL_UNSIGNED_INT_SAMPLER_1D_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY */
   GL_UNSIGNED_INT_SAMPLER_2D_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY */
   GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE */
   GL_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
   GL_UNSIGNED_INT_SAMPLER_BUFFER, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER */
   GL_UNSIGNED_INT_SAMPLER_2D_RECT, /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT */
   GL_IMAGE_1D, /* GLHCK_SHADER_IMAGE_1D */
   GL_IMAGE_2D, /* GLHCK_SHADER_IMAGE_2D */
   GL_IMAGE_3D, /* GLHCK_SHADER_IMAGE_3D */
   GL_IMAGE_2D_RECT, /* GLHCK_SHADER_IMAGE_2D_RECT */
   GL_IMAGE_CUBE, /* GLHCK_SHADER_IMAGE_CUBE */
   GL_IMAGE_BUFFER, /* GLHCK_SHADER_IMAGE_BUFFER */
   GL_IMAGE_1D_ARRAY, /* GLHCK_SHADER_IMAGE_1D_ARRAY */
   GL_IMAGE_2D_ARRAY, /* GLHCK_SHADER_IMAGE_2D_ARRAY */
   GL_IMAGE_2D_MULTISAMPLE, /* GLHCK_SHADER_IMAGE_2D_MULTISAMPLE */
   GL_IMAGE_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY */
   GL_INT_IMAGE_1D, /* GLHCK_SHADER_INT_IMAGE_1D */
   GL_INT_IMAGE_2D, /* GLHCK_SHADER_INT_IMAGE_2D */
   GL_INT_IMAGE_3D, /* GLHCK_SHADER_INT_IMAGE_3D */
   GL_INT_IMAGE_2D_RECT, /* GLHCK_SHADER_INT_IMAGE_2D_RECT */
   GL_INT_IMAGE_CUBE, /* GLHCK_SHADER_INT_IMAGE_CUBE */
   GL_INT_IMAGE_BUFFER, /* GLHCK_SHADER_INT_IMAGE_BUFFER */
   GL_INT_IMAGE_1D_ARRAY, /* GLHCK_SHADER_INT_IMAGE_1D_ARRAY */
   GL_INT_IMAGE_2D_ARRAY, /* GLHCK_SHADER_INT_IMAGE_2D_ARRAY */
   GL_INT_IMAGE_2D_MULTISAMPLE, /* GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE */
   GL_INT_IMAGE_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY */
   GL_UNSIGNED_INT_IMAGE_1D, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D */
   GL_UNSIGNED_INT_IMAGE_2D, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D */
   GL_UNSIGNED_INT_IMAGE_3D, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D */
   GL_UNSIGNED_INT_IMAGE_2D_RECT, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT */
   GL_UNSIGNED_INT_IMAGE_CUBE, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE */
   GL_UNSIGNED_INT_IMAGE_BUFFER, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER */
   GL_UNSIGNED_INT_IMAGE_1D_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY */
   GL_UNSIGNED_INT_IMAGE_2D_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY */
   GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE */
   GL_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY, /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY */
   GL_UNSIGNED_INT_ATOMIC_COUNTER, /* GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER */
   GL_INVALID_VALUE, /* GLHCK_SHADER_VARIABLE_TYPE_UNKNOWN */
};

__attribute__((used))
static const char* glhckShaderVariableTypeToString[] = {
   "float", /* GLHCK_SHADER_FLOAT */
   "vec2", /* GLHCK_SHADER_FLOAT_VEC2 */
   "vec3", /* GLHCK_SHADER_FLOAT_VEC3 */
   "vec4", /* GLHCK_SHADER_FLOAT_VEC4 */
   "double", /* GLHCK_SHADER_DOUBLE */
   "dvec2", /* GLHCK_SHADER_DOUBLE_VEC2 */
   "dvec3", /* GLHCK_SHADER_DOUBLE_VEC3 */
   "dvec4", /* GLHCK_SHADER_DOUBLE_VEC4 */
   "int", /* GLHCK_SHADER_INT */
   "ivec2", /* GLHCK_SHADER_INT_VEC2 */
   "ivec3", /* GLHCK_SHADER_INT_VEC3 */
   "ivec4", /* GLHCK_SHADER_INT_VEC4 */
   "uint", /* GLHCK_SHADER_UNSIGNED_INT */
   "uvec2", /* GLHCK_SHADER_UNSIGNED_INT_VEC2 */
   "uvec3", /* GLHCK_SHADER_UNSIGNED_INT_VEC3 */
   "uvec4", /* GLHCK_SHADER_UNSIGNED_INT_VEC4 */
   "bool", /* GLHCK_SHADER_BOOL */
   "bvec2", /* GLHCK_SHADER_BOOL_VEC2 */
   "bvec3", /* GLHCK_SHADER_BOOL_VEC3 */
   "bvec4", /* GLHCK_SHADER_BOOL_VEC4 */
   "mat2", /* GLHCK_SHADER_FLOAT_MAT2 */
   "mat3", /* GLHCK_SHADER_FLOAT_MAT3 */
   "mat4", /* GLHCK_SHADER_FLOAT_MAT4 */
   "mat2x3", /* GLHCK_SHADER_FLOAT_MAT2x3 */
   "mat2x4", /* GLHCK_SHADER_FLOAT_MAT2x4 */
   "mat3x2", /* GLHCK_SHADER_FLOAT_MAT3x2 */
   "mat3x4", /* GLHCK_SHADER_FLOAT_MAT3x4 */
   "mat4x2", /* GLHCK_SHADER_FLOAT_MAT4x2 */
   "mat4x3", /* GLHCK_SHADER_FLOAT_MAT4x3 */
   "dmat2", /* GLHCK_SHADER_DOUBLE_MAT2 */
   "dmat3", /* GLHCK_SHADER_DOUBLE_MAT3 */
   "dmat4", /* GLHCK_SHADER_DOUBLE_MAT4 */
   "dmat2x3", /* GLHCK_SHADER_DOUBLE_MAT2x3 */
   "dmat2x4", /* GLHCK_SHADER_DOUBLE_MAT2x4 */
   "dmat3x2", /* GLHCK_SHADER_DOUBLE_MAT3x2 */
   "dmat3x4", /* GLHCK_SHADER_DOUBLE_MAT3x4 */
   "dmat4x2", /* GLHCK_SHADER_DOUBLE_MAT4x2 */
   "dmat4x3", /* GLHCK_SHADER_DOUBLE_MAT4x3 */
   "sampler1D", /* GLHCK_SHADER_SAMPLER_1D */
   "sampler2D", /* GLHCK_SHADER_SAMPLER_2D */
   "sampler3D", /* GLHCK_SHADER_SAMPLER_3D */
   "samplerCube", /* GLHCK_SHADER_SAMPLER_CUBE */
   "sampler1DShadow", /* GLHCK_SHADER_SAMPLER_1D_SHADOW */
   "sampler2DShadow", /* GLHCK_SHADER_SAMPLER_2D_SHADOW */
   "sampler1DArray", /* GLHCK_SHADER_SAMPLER_1D_ARRAY */
   "sampler2DArray", /* GLHCK_SHADER_SAMPLER_2D_ARRAY */
   "sampler1DArrayShadow", /* GLHCK_SHADER_SAMPLER_1D_ARRAY_SHADOW */
   "sampler2DArrayShadow", /* GLHCK_SHADER_SAMPLER_2D_ARRAY_SHADOW */
   "sampler2DMS", /* GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE */
   "sampler2DMSArray", /* GLHCK_SHADER_SAMPLER_2D_MULTISAMPLE_ARRAY */
   "samplerCubeShadow", /* GLHCK_SHADER_SAMPLER_CUBE_SHADOW */
   "samplerBuffer", /* GLHCK_SHADER_SAMPLER_BUFFER */
   "sampler2DRect", /* GLHCK_SHADER_SAMPLER_2D_RECT */
   "sampler2DRectShadow", /* GLHCK_SHADER_SAMPLER_2D_RECT_SHADOW */
   "isampler1D", /* GLHCK_SHADER_INT_SAMPLER_1D */
   "isampler2D", /* GLHCK_SHADER_INT_SAMPLER_2D */
   "isampler3D", /* GLHCK_SHADER_INT_SAMPLER_3D */
   "isamplerCube", /* GLHCK_SHADER_INT_SAMPLER_CUBE */
   "isampler1DArray", /* GLHCK_SHADER_INT_SAMPLER_1D_ARRAY */
   "isampler2DArray", /* GLHCK_SHADER_INT_SAMPLER_2D_ARRAY */
   "isampler2DMS", /* GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE */
   "isampler2DMSArray", /* GLHCK_SHADER_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
   "isamplerBuffer", /* GLHCK_SHADER_INT_SAMPLER_BUFFER */
   "isampler2DRect", /* GLHCK_SHADER_INT_SAMPLER_2D_RECT */
   "usampler1D", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D */
   "usampler2D", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D */
   "usampler3D", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_3D */
   "usamplerCube", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_CUBE */
   "usampler2DArray", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_1D_ARRAY */
   "usampler2DArray", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_ARRAY */
   "usampler2DMS", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE */
   "usampler2DMSArray", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_MULTISAMPLE_ARRAY */
   "usamplerBuffer", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_BUFFER */
   "usampler2DRect", /* GLHCK_SHADER_UNSIGNED_INT_SAMPLER_2D_RECT */
   "image1D", /* GLHCK_SHADER_IMAGE_1D */
   "image2D", /* GLHCK_SHADER_IMAGE_2D */
   "image3D", /* GLHCK_SHADER_IMAGE_3D */
   "image2DRect", /* GLHCK_SHADER_IMAGE_2D_RECT */
   "imageCube", /* GLHCK_SHADER_IMAGE_CUBE */
   "imageBuffer", /* GLHCK_SHADER_IMAGE_BUFFER */
   "image1DArray", /* GLHCK_SHADER_IMAGE_1D_ARRAY */
   "image2DArray", /* GLHCK_SHADER_IMAGE_2D_ARRAY */
   "image2DMS", /* GLHCK_SHADER_IMAGE_2D_MULTISAMPLE */
   "image2DMSArray", /* GLHCK_SHADER_IMAGE_2D_MULTISAMPLE_ARRAY */
   "iimage1D", /* GLHCK_SHADER_INT_IMAGE_1D */
   "iimage2D", /* GLHCK_SHADER_INT_IMAGE_2D */
   "iimage3D", /* GLHCK_SHADER_INT_IMAGE_3D */
   "iimage2DRect", /* GLHCK_SHADER_INT_IMAGE_2D_RECT */
   "iimageCube", /* GLHCK_SHADER_INT_IMAGE_CUBE */
   "iimageBuffer", /* GLHCK_SHADER_INT_IMAGE_BUFFER */
   "iimage1DArray", /* GLHCK_SHADER_INT_IMAGE_1D_ARRAY */
   "iimage2DArray", /* GLHCK_SHADER_INT_IMAGE_2D_ARRAY */
   "iimage2DMS", /* GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE */
   "iimage2DMSArray", /* GLHCK_SHADER_INT_IMAGE_2D_MULTISAMPLE_ARRAY */
   "uimage1D", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D */
   "uimage2D", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D */
   "uimage3D", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_3D */
   "uimage2DRect", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_RECT */
   "uimageCube", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_CUBE */
   "uimageBuffer", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_BUFFER */
   "uimage1DArray", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_1D_ARRAY */
   "uimage2DArray", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_ARRAY */
   "uimage2DMS", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE */
   "uimage2DMSArray", /* GLHCK_SHADER_UNSIGNED_INT_IMAGE_2D_MULTISAMPLE_ARRAY */
   "atomic_uint", /* GLHCK_SHADER_UNSIGNED_INT_ATOMIC_COUNTER */
   "unknown", /* GLHCK_SHADER_VARIABLE_TYPE_UNKNOWN */
};

__attribute__((used))
static enum glhckShaderVariableType glhShaderVariableTypeToGlhckType(GLenum type)
{
   for (enum glhckShaderVariableType i = 0; i < GLHCK_SHADER_VARIABLE_TYPE_LAST; ++i)
      if (glhckShaderVariableTypeToGL[i] == type)
         return i;
   return GLHCK_SHADER_VARIABLE_TYPE_UNKNOWN;
}

/*
 * binding mappings
 */

__attribute__((used))
static void glhTextureGenerate(GLsizei n, GLuint *textures) {
   GL_CALL(glGenTextures(n, textures));
}
__attribute__((used))
static void glhTextureDelete(GLsizei n, const GLuint *textures) {
   GL_CALL(glDeleteTextures(n, textures));
}
__attribute__((used))
static void glhTextureBind(glhckTextureTarget target, GLuint object) {
   GL_CALL(glBindTexture(glhckTextureTargetToGL[target], object));
}
__attribute__((used))
static void glhTextureActive(GLuint index) {
   GL_CALL(glActiveTexture(GL_TEXTURE0+index));
}
__attribute__((used))
static void glhRenderbufferGenerate(GLsizei n, GLuint *renderbuffers) {
   GL_CALL(glGenRenderbuffers(n, renderbuffers));
}
__attribute__((used))
static void glhRenderbufferDelete(GLsizei n, const GLuint *renderbuffers) {
   GL_CALL(glDeleteRenderbuffers(n, renderbuffers));
}
__attribute__((used))
static void glhRenderbufferBind(GLuint object) {
   GL_CALL(glBindRenderbuffer(GL_RENDERBUFFER, object));
}
__attribute__((used))
static void glhFramebufferGenerate(GLsizei n, GLuint *framebuffers) {
   GL_CALL(glGenFramebuffers(n, framebuffers));
}
__attribute__((used))
static void glhFramebufferDelete(GLsizei n, const GLuint *framebuffers) {
   GL_CALL(glDeleteFramebuffers(n, framebuffers));
}
__attribute__((used))
static void glhFramebufferBind(glhckFramebufferTarget target, GLuint object) {
   GL_CALL(glBindFramebuffer(glhckFramebufferTargetToGL[target], object));
}
__attribute__((used))
static void glhHwBufferGenerate(GLsizei n, GLuint *buffers) {
   GL_CALL(glGenBuffers(n, buffers));
}
__attribute__((used))
static void glhHwBufferDelete(GLsizei n, const GLuint *buffers) {
   GL_CALL(glDeleteBuffers(n, buffers));
}
__attribute__((used))
static void glhHwBufferBind(glhckHwBufferTarget target, GLuint object) {
   GL_CALL(glBindBuffer(glhckHwBufferTargetToGL[target], object));
}
__attribute__((used))
static void glhHwBufferBindBase(glhckHwBufferTarget target, GLuint index, GLuint object) {
#ifdef OPENGL2
   /* stub */
   (void)target, (void)index, (void)object;
#else
   GL_CALL(glBindBufferBase(glhckHwBufferTargetToGL[target], index, object));
#endif
}
__attribute__((used))
static void glhHwBufferBindRange(glhckHwBufferTarget target, GLuint index, GLuint object, GLintptr offset, GLsizeiptr size) {
#ifdef OPENGL2
   /* stub */
   (void)target, (void)index, (void)object, (void)offset, (void)size;
#else
   GL_CALL(glBindBufferRange(glhckHwBufferTargetToGL[target], index, object, offset, size));
#endif
}
__attribute__((used))
static void glhHwBufferCreate(glhckHwBufferTarget target, GLsizeiptr size, const GLvoid *data, glhckHwBufferStoreType usage) {
   GL_CALL(glBufferData(glhckHwBufferTargetToGL[target], size, data, glhckHwBufferStoreTypeToGL[usage]));
}
__attribute__((used))
static void glhHwBufferFill(glhckHwBufferTarget target, GLintptr offset, GLsizeiptr size, const GLvoid *data) {
   GL_CALL(glBufferSubData(glhckHwBufferTargetToGL[target], offset, size, data));
}
__attribute__((used))
static void* glhHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access) {
   return glMapBuffer(glhckHwBufferTargetToGL[target], glhckHwBufferAccessTypeToGL[access]);
}
__attribute__((used))
static void glhHwBufferUnmap(glhckHwBufferTarget target) {
   GL_CALL(glUnmapBuffer(glhckHwBufferTargetToGL[target]));
}
__attribute__((used))
static void glhProgramBind(GLuint program) {
   GL_CALL(glUseProgram(program));
}
__attribute__((used))
static void glhProgramDelete(GLuint program) {
   GL_CALL(glDeleteProgram(program));
}
__attribute__((used))
static void glhShaderDelete(GLuint shader) {
   GL_CALL(glDeleteShader(shader));
}

/*
 * shared OpenGL renderer functions
 */

/* \brief clear OpenGL buffers */
__attribute__((used))
static void glhClear(GLuint bufferBits)
{
   CALL(2, "%u", bufferBits);

   GLuint glBufferBits = 0;
   if (bufferBits & GLHCK_COLOR_BUFFER_BIT)
      glBufferBits |= GL_COLOR_BUFFER_BIT;
   if (bufferBits & GLHCK_DEPTH_BUFFER_BIT)
      glBufferBits |= GL_DEPTH_BUFFER_BIT;

   GL_CALL(glClear(glBufferBits));
}

/* \brief set OpenGL clear color */
__attribute__((used))
static void glhClearColor(const glhckColor color)
{
   CALL(2, "%u", color);
   float fr = (float)((color & 0xFF000000) >> 24) / 255;
   float fg = (float)((color & 0x00FF0000) >> 16) / 255;
   float fb = (float)((color & 0x0000FF00) >> 8) / 255;
   float fa = (float)((color & 0x000000FF)) / 255;
   GL_CALL(glClearColor(fr, fg, fb, fa));
}

/* \brief set front face */
__attribute__((used))
static void glhFrontFace(glhckFaceOrientation orientation)
{
   GL_CALL(glFrontFace(glhckFaceOrientationToGL[orientation]));
}

/* \brief set cull side */
__attribute__((used))
static void glhCullFace(glhckCullFaceType face)
{
   GL_CALL(glCullFace(glhckCullFaceTypeToGL[face]));
}

/* \brief get pixels from OpenGL */
__attribute__((used))
static void glhBufferGetPixels(GLint x, GLint y, GLsizei width, GLsizei height, glhckTextureFormat format, glhckDataType type, GLvoid *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %p", x, y, width, height, format, type, data);
   GL_CALL(glReadPixels(x, y, width, height, glhckTextureFormatToGL[format], glhckDataTypeToGL[type], data));
}

/* \brief blend func wrapper */
__attribute__((used))
static void glhBlendFunc(glhckBlendingMode blenda, glhckBlendingMode blendb)
{
   GL_CALL(glBlendFunc(glhckBlendingModeToGL[blenda], glhckBlendingModeToGL[blendb]));
}

/* \brief set texture parameters */
__attribute__((used))
static void glhTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params)
{
   CALL(0, "%d, %p", target, params);
   assert(params);

   GLenum glTarget    = glhckTextureTargetToGL[target];
   GLenum minFilter   = glhckTextureFilterToGL[params->minFilter];
   GLenum magFilter   = glhckTextureFilterToGL[params->magFilter];
   GLenum wrapS       = glhckTextureWrapToGL[params->wrapS];
   GLenum wrapT       = glhckTextureWrapToGL[params->wrapT];
   GLenum wrapR       = glhckTextureWrapToGL[params->wrapR];
   GLenum compareMode = glhckTextureCompareModeToGL[params->compareMode];
   GLenum compareFunc = glhckCompareFuncToGL[params->compareFunc];

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

   if (GLAD_GL_VERSION_1_4) {
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_WRAP_R, wrapR));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_MODE, compareMode));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_COMPARE_FUNC, compareFunc));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_BASE_LEVEL, params->baseLevel));
      GL_CALL(glTexParameteri(glTarget, GL_TEXTURE_MAX_LEVEL, params->maxLevel));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_LOD_BIAS, params->biasLod));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MIN_LOD, params->minLod));
      GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MAX_LOD, params->maxLod));
   }

   if (GL_EXT_texture_filter_anisotropic) {
      if (params->mipmap && params->maxAnisotropy) {
         float max;
         GL_CALL(glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &max));
         if (max > params->maxAnisotropy) max = params->maxAnisotropy;
         if (max > 0.0f) GL_CALL(glTexParameterf(glTarget, GL_TEXTURE_MAX_ANISOTROPY_EXT, max));
      }
   }
}

/* \brief generate mipmaps for texture */
__attribute__((used))
static void glhTextureMipmap(glhckTextureTarget target)
{
   CALL(0, "%d", target);

   if (!glGenerateMipmap)
      return;

   GL_CALL(glGenerateMipmap(glhckTextureTargetToGL[target]));
}

/* \brief create texture from data and upload it to OpenGL */
__attribute__((used))
static void glhTextureImage(glhckTextureTarget target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint border, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data)
{
   CALL(0, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, width, height, depth, border, format, datatype, size, data);

   GLenum glTarget = glhckTextureTargetToGL[target];
   GLenum glFormat = glhckTextureFormatToGL[format];
   GLenum glDataType = glhckDataTypeToGL[datatype];

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
__attribute__((used))
static void glhTextureFill(glhckTextureTarget target, GLint level, GLint x, GLint y, GLint z, GLsizei width, GLsizei height, GLsizei depth, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, x, y, z, width, height, depth, format, datatype, size, data);

   GLenum glTarget = glhckTextureTargetToGL[target];
   GLenum glFormat = glhckTextureFormatToGL[format];
   GLenum glDataType = glhckDataTypeToGL[datatype];

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
__attribute__((used))
static void glhRenderbufferStorage(GLsizei width, GLsizei height, glhckTextureFormat format)
{
   CALL(0, "%d, %d, %d", width, height, format);
   glRenderbufferStorage(GL_RENDERBUFFER, glhckTextureFormatToGL[format], width, height);
}

/* \brief glFramebufferTexture wrapper with error checking */
__attribute__((used))
static GLint glhFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, GLuint texture, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %d, %u, %d", framebufferTarget, textureTarget, texture, attachment);

   GLenum glTarget = glhckFramebufferTargetToGL[framebufferTarget];
   GLenum glTexTarget = glhckTextureTargetToGL[textureTarget];
   GLenum glAttachment = glhckFramebufferAttachmentTypeToGL[attachment];

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
__attribute__((used))
static GLint glhFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, GLuint buffer, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %u, %d", framebufferTarget, buffer, attachment);

   GLenum glTarget = glhckFramebufferTargetToGL[framebufferTarget];
   GLenum glAttachment = glhckFramebufferAttachmentTypeToGL[attachment];
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
__attribute__((used))
static GLuint glhProgramAttachUniformBuffer(GLuint program, const GLchar *uboName, GLuint location)
{
   CALL(0, "%u, %s, %u", program, uboName, location);

   GLuint ubo;
   if ((ubo = glGetUniformBlockIndex(program, uboName))) {
      GL_CALL(glUniformBlockBinding(program, ubo, location));
   }

   RET(0, "%u", ubo);
   return ubo;
}

/* \brief create uniform buffer from shader */
__attribute__((used))
static chckIterPool* glhProgramUniformBufferPool(GLuint program, const GLchar *uboName, GLsizei *uboSize)
{
   GLint *indices = NULL;
   GLchar *uname = NULL;
   chckIterPool *pool = NULL;
   CALL(0, "%u, %s, %p", program, uboName, uboSize);

   if (uboSize)
      *uboSize = 0;

   /* get uniform block index */
   GLuint index;
   if ((index = glGetUniformBlockIndex(program, uboName)) == GL_INVALID_INDEX)
      goto fail;

   /* get uniform block size and initialize the hw buffer */
   GLsizei size;
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_DATA_SIZE, &size));

   if (uboSize)
      *uboSize = size;

   /* get uniform count for UBO */
   GLint count;
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORMS, &count));

   if (!count)
      goto no_uniforms;

   /* allocate space for uniform iteration */
   if (!(indices = malloc(sizeof(GLuint) * count)))
      goto fail;

   /* get indices for UBO's member uniforms */
   GL_CALL(glGetActiveUniformBlockiv(program, index, GL_UNIFORM_BLOCK_ACTIVE_UNIFORM_INDICES, indices));

   if (!(pool = chckIterPoolNew(32, 32, sizeof(struct glhckHwBufferShaderUniform))))
      goto fail;

   for (GLint i = 0; i < count; ++i) {
      index = (GLuint)indices[i];

      /* get uniform information */
      GLenum type;
      GLsizei length;
      GLchar name[255];
      GL_CALL(glGetActiveUniform(program, index, sizeof(name) - 1, &length, &size, &type, name));

      if (!length)
         continue;

      /* cut out [0] for arrays */
      if (!strcmp(name + strlen(name) - 3, "[0]"))
         length -= 3;

      /* allocate name */
      if (!(uname = calloc(1, length + 4)))
         continue;

      /* allocate new uniform slot */
      struct glhckHwBufferShaderUniform *u;
      if (!(u = chckIterPoolAdd(pool, NULL, NULL)))
         continue;

      /* assign information */
      memcpy(uname, name, length);
      u->name = uname;
      glGetActiveUniformsiv(program, 1, &index, GL_UNIFORM_OFFSET, &u->offset);

      u->type = glhShaderVariableTypeToGlhckType(type);
      u->typeName = glhckShaderVariableTypeToString[u->type];
      u->size = size;
   }

   free(indices);
   RET(0, "%p", pool);
   return pool;

fail:
no_uniforms:
   IFDO(chckIterPoolFree, pool);
   IFDO(free, indices);
   IFDO(free, uname);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get attribute pool from program */
__attribute__((used))
static chckIterPool* glhProgramAttributePool(GLuint obj)
{
   GLchar *aname = NULL;
   chckIterPool *pool = NULL;
   CALL(0, "%u", obj);

   /* get attribute count */
   GLint count;
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_ATTRIBUTES, &count));

   if (!count)
      goto no_attributes;

   if (!(pool = chckIterPoolNew(32, 32, sizeof(struct glhckShaderAttribute))))
      goto fail;

   for (GLint i = 0; i != count; ++i) {
      /* get attribute information */
      GLenum type;
      GLchar name[255];
      GLsizei size, length;
      GL_CALL(glGetActiveAttrib(obj, i, sizeof(name) - 1, &length, &size, &type, name));

      if (!length)
         continue;

      if (!(aname = strdup(name)))
         continue;

      /* allocate new uniform slot */
      struct glhckShaderAttribute *a;
      if (!(a = chckIterPoolAdd(pool, NULL, NULL)))
         continue;

      /* assign information */
      a->name = aname;
      a->type = glhShaderVariableTypeToGlhckType(type);
      a->typeName = glhckShaderVariableTypeToString[a->type];
      a->location = glGetAttribLocation(obj, a->name);
      a->size = size;
   }

   RET(0, "%p", pool);
   return pool;

fail:
no_attributes:
   IFDO(chckIterPoolFree, pool);
   IFDO(free, aname);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get uniform pool from program */
__attribute__((used))
static chckIterPool* glhProgramUniformPool(GLuint obj)
{
   GLchar *uname = NULL;
   chckIterPool *pool = NULL;
   CALL(0, "%u", obj);

   /* get uniform count */
   GLint count;
   GL_CALL(glGetProgramiv(obj, GL_ACTIVE_UNIFORMS, &count));

   if (!count)
      goto no_uniforms;

   if (!(pool = chckIterPoolNew(32, 32, sizeof(struct glhckShaderUniform))))
      goto fail;

   for (GLint i = 0; i != count; ++i) {
      /* get uniform information */
      GLenum type;
      GLchar name[255];
      GLsizei length, size;
      GL_CALL(glGetActiveUniform(obj, i, sizeof(name) - 1, &length, &size, &type, name));

      if (!length)
         continue;

      /* cut out [0] for arrays */
      if (!strcmp(name + strlen(name) - 3, "[0]"))
         length -= 3;

      /* allocate name */
      if (!(uname = calloc(1, length + 4)))
         continue;

      /* get uniform location */
      GLuint location;
      memcpy(uname, name, length);
      if ((location = glGetUniformLocation(obj, uname)) == UINT_MAX) {
         free(uname);
         continue;
      }

      /* allocate new uniform slot */
      struct glhckShaderUniform *u;
      if (!(u = chckIterPoolAdd(pool, NULL, NULL)))
         continue;

      /* assign information */
      u->name = uname;
      u->type = glhShaderVariableTypeToGlhckType(type);
      u->typeName = glhckShaderVariableTypeToString[u->type];
      u->location = location;
      u->size = size;

      /* store for iterating the array slots */
      GLchar *tmp = uname;

      /* generate uniform bindings for array slots */
      for (GLint i2 = 1; i2 < size; ++i2) {
         if (!(uname = calloc(1, length + 4)))
            continue;

         /* get uniform location */
         snprintf(uname, length + 4, "%s[%d]", tmp, i2);
         if ((location = glGetUniformLocation(obj, uname)) == UINT_MAX) {
            free(uname);
            continue;
         }

         /* allocate new uniform slot */
         if (!(u = chckIterPoolAdd(pool, NULL, NULL)))
            continue;

         /* assign information */
         u->name = uname;
         u->type = glhShaderVariableTypeToGlhckType(type);
         u->typeName = glhckShaderVariableTypeToString[u->type];
         u->location = location;
         u->size = size;
      }
   }

   RET(0, "%p", pool);
   return pool;

fail:
no_uniforms:
   IFDO(chckIterPoolFree, pool);
   IFDO(free, uname);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief set shader uniform */
__attribute__((used))
static void glhProgramUniform(GLuint obj, const struct glhckShaderUniform *uniform, GLsizei count, const GLvoid *value)
{
   CALL(2, "%u, %p, %d, %p", obj, uniform, count, value);

   /* automatically figure out the data type */

   /* FIXME: resolve function at link time and store in pointer */
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
#ifdef OPENGL3
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
#endif
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

/*
 * misc
 */

/* \brief debug output callback function */
__attribute__((used))
static GLCB void glhDebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const GLvoid *user)
{
   (void)source; (void)type; (void)id; (void)severity; (void)length; (void)user;
   printf(" [GPU::DBG] %s\n", message);
}

/* \brief setup OpenGL debug output extension */
__attribute__((used))
static void glhSetupDebugOutput(void)
{
   if (!GL_KHR_debug)
      return;

   GL_CALL(glDebugMessageCallbackARB(glhDebugCallback, NULL));
   GL_CALL(glEnable(GL_DEBUG_OUTPUT));
}

/*
 * opengl init
 */

__attribute__((used))
static int glhRendererFeatures(glhckRendererFeatures *features)
{
   assert(features);

   GLchar *vendor = (GLchar*)GL_CALL(glGetString(GL_VENDOR));
   GLchar *renderer = (GLchar*)GL_CALL(glGetString(GL_RENDERER));

   GLint shaderMajor = 0, shaderMinor = 0;
   if (GLAD_GL_VERSION_1_4) {
      GLchar *shaderVersion = (GLchar*)GL_CALL(glGetString(GL_SHADING_LANGUAGE_VERSION));
      for (; shaderVersion && !sscanf(shaderVersion, "%d.%d", &shaderMajor, &shaderMinor); ++shaderVersion);
   }

   /* try to identify driver */
   if (vendor && strstr(vendor, "MESA"))
      features->driver = GLHCK_DRIVER_MESA;
   else if (vendor && strstr(vendor, "NVIDIA"))
      features->driver = GLHCK_DRIVER_NVIDIA;
   else if (vendor && strstr(vendor, "ATI"))
      features->driver = GLHCK_DRIVER_ATI;
   else if (vendor && strstr(vendor, "Imagination Technologies"))
      features->driver = GLHCK_DRIVER_IMGTEC;
   else
      features->driver = GLHCK_DRIVER_OTHER;

   DEBUG(3, "OpenGL [%d.%d] [%s]", GLVersion.major, GLVersion.minor, vendor);
   DEBUG(3, "%s [GLSL %d.%d]", (renderer ? renderer : "Generic"), shaderMajor, shaderMinor);

#ifdef OPENGL3
   GLint num;
   glGetIntegerv(GL_NUM_EXTENSIONS, &num);
   for (GLint i = 0; i < num; ++i)
      DEBUG(3, "%s", glGetStringi(GL_EXTENSIONS, i));
#else
   GLchar *extensions = (GLchar*)GL_CALL(glGetString(GL_EXTENSIONS));

   /* FIXME: remove strtok with strcspn */
   if (extensions) {
      char *extcpy = strdup(extensions);
      char *s = strtok(extcpy, " ");
      while (s) {
         DEBUG(3, "%s", s);
         s = strtok(NULL, " ");
      }
      free(extcpy);
   }
#endif

   features->version.major = GLVersion.major;
   features->version.minor = GLVersion.minor;
   features->version.shaderMajor = shaderMajor;
   features->version.shaderMinor = shaderMinor;
   GL_CALL(glGetIntegerv(GL_MAX_TEXTURE_SIZE, &features->texture.maxTextureSize));
   GL_CALL(glGetIntegerv(GL_MAX_RENDERBUFFER_SIZE, &features->texture.maxRenderbufferSize));
   features->texture.hasNativeNpotSupport = 1;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

#endif /* __glhck_opengl_helper_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
