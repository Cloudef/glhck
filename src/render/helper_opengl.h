#ifndef __glhck_opengl_helper_h__
#define __glhck_opengl_helper_h__

#include "../internal.h"

#if EMSCRIPTEN
/* we don't define GLEW_USE_LIB_ES11 since we need all the constants.
 * bit of hack, but yeah.. */
#  include <GL/glew.h>
#  undef GLEW_VERSION_1_4
#  undef GLEW_VERSION_2_0
#  undef GLEW_VERSION_3_0
#  undef GLEW_ES_VERSION_1_0
#  define GLEW_VERSION_1_4 0
#  define GLEW_VERSION_2_0 0
#  define GLEW_VERSION_3_0 0
#  define GLEW_ES_VERSION_1_0 1
#else
#  if GLHCK_USE_GLES1
#     define GLEW_USE_LIB_ES11
#  elif GLHCK_USE_GLES2
#     define GLEW_USE_LIB_ES20
#  endif
#  include "GL/glew.h"
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
void GL_ERROR(unsigned int line, const char *func, const char *glfunc);
#endif

/* check return value of gl function on debug build */
#ifdef NDEBUG
#  define GL_CHECK(x) x
#else
#  define GL_CHECK(x) GL_CHECK_ERROR(__func__, __STRING(x), x)
GLenum GL_CHECK_ERROR(const char *func, const char *glfunc, GLenum error);
#endif

/*** mapping tables ***/
extern GLenum glhckCullFaceTypeToGL[];
extern GLenum glhckFaceOrientationToGL[];
extern GLenum glhckGeometryTypeToGL[];
extern GLenum glhckFramebufferTargetToGL[];
extern GLenum glhckFramebufferAttachmentTypeToGL[];
extern GLenum glhckHwBufferTargetToGL[];
extern GLenum glhckHwBufferStoreTypeToGL[];
extern GLenum glhckHwBufferAccessTypeToGL[];
extern GLenum glhckShaderTypeToGL[];
extern GLenum glhckTextureWrapToGL[];
extern GLenum glhckTextureFilterToGL[];
extern GLenum glhckTextureCompareModeToGL[];
extern GLenum glhckCompareFuncToGL[];
extern GLenum glhckTextureTargetToGL[];
extern GLenum glhckTextureFormatToGL[];
extern GLenum glhckDataTypeToGL[];
extern GLenum glhckBlendingModeToGL[];

/*** check if driver setup is supported ***/
int glhCheckSupport(const char *renderName);

/*** binding mappings ***/
void glhTextureBind(glhckTextureTarget target, GLuint object);
void glhTextureActive(GLuint index);
void glhRenderbufferBind(GLuint object);
void glhFramebufferBind(glhckFramebufferTarget target, GLuint object);
void glhHwBufferBind(glhckHwBufferTarget target, GLuint object);
void glhHwBufferBindBase(glhckHwBufferTarget target, GLuint index, GLuint object);
void glhHwBufferBindRange(glhckHwBufferTarget target, GLuint index, GLuint object, GLintptr offset, GLsizeiptr size);
void glhHwBufferCreate(glhckHwBufferTarget target, GLsizeiptr size, const GLvoid *data, glhckHwBufferStoreType usage);
void glhHwBufferFill(glhckHwBufferTarget target, GLintptr offset, GLsizeiptr size, const GLvoid *data);
void* glhHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access);
void glhHwBufferUnmap(glhckHwBufferTarget target);

/*** shader mapping functions ***/
GLenum glhShaderVariableTypeForGlhckType(_glhckShaderVariableType type);
_glhckShaderVariableType glhGlhckShaderVariableTypeForOpenGLType(GLenum type);
const GLchar* glhShaderVariableNameForOpenGLConstant(GLenum type);
const GLchar* glhShaderVariableNameForGlhckConstant(GLenum type);

/*** shared opengl functions ***/
void glhClear(GLuint bufferBits);
void glhClearColor(const glhckColorb *color);
void glhFrontFace(glhckFaceOrientation orientation);
void glhCullFace(glhckCullFaceType face);
void glhBufferGetPixels(GLint x, GLint y, GLsizei width, GLsizei height, glhckTextureFormat format, glhckDataType type, GLvoid *data);
void glhBlendFunc(GLenum blenda, GLenum blendb);
void glhTextureFill(glhckTextureTarget target, GLint level, GLint x, GLint y, GLint z, GLsizei width, GLsizei height, GLsizei depth, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data);
void glhTextureImage(glhckTextureTarget target, GLint level, GLsizei width, GLsizei height, GLsizei depth, GLint border, glhckTextureFormat format, GLint datatype, GLsizei size, const GLvoid *data);
void glhTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params);
void glhTextureMipmap(glhckTextureTarget target);
void glhRenderbufferStorage(GLsizei width, GLsizei height, glhckTextureFormat format);
GLint glhFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, GLuint texture, glhckFramebufferAttachmentType attachment);
GLint glhFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, GLuint buffer, glhckFramebufferAttachmentType attachment);
GLuint glhProgramAttachUniformBuffer(GLuint program, const GLchar *uboName, GLuint location);
_glhckHwBufferShaderUniform* glhProgramUniformBufferList(GLuint program, const GLchar *uboName, GLsizei *size);
_glhckShaderAttribute* glhProgramAttributeList(GLuint obj);
_glhckShaderUniform* glhProgramUniformList(GLuint obj);
void glhProgramUniform(GLuint obj, _glhckShaderUniform *uniform, GLsizei count, const GLvoid *value);
void glhGeometryRender(const glhckGeometry *geometry, glhckGeometryType type);

/*** misc ***/
void glhSetupDebugOutput(void);

#endif /* __glhck_opengl_helper_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
