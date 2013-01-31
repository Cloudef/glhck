#ifndef __glhck_opengl_helper_h__
#define __glhck_opengl_helper_h__

#if GLHCK_USE_GLES1
#  include <GLES/gl.h> /* for opengl ES 1.x */
#  include <GLES/glext.h>
#  define GL_FRAMEBUFFER_COMPLETE                        GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_UNDEFINED                       GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT           GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT   GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE          GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS        GL_FRAMEBUFFER_COMPLETE_OES
#  define GL_FRAMEBUFFER_UNSUPPORTED                     GL_FRAMEBUFFER_UNSUPPORTED_OES
#  define GL_FRAMEBUFFER                                 GL_FRAMEBUFFER_OES
#elif GLHCK_USE_GLES2
#  include <GLES2/gl2.h> /* for opengl ES 2.x */
#  include <GLES2/gl2ext.h>
#  warning "GLES 2.x not working yet"
#else
/* FIXME: include glew instead of gl.h and glext.h here in future */
#  define GL_GLEXT_PROTOTYPES
#  include <GL/gl.h>
#  include <GL/glext.h>
#endif

#if GLHCK_USE_GLES1
#  ifdef GL_OES_element_index_uint
#     ifndef GL_UNSIGNED_INT
#        define GL_UNSIGNED_INT 0x1405
#     endif
#  else
#     error "GLHCK needs GL_OES_element_index_uint for GLESv1 support!"
#  endif
#endif /* GLESv1 SUPPORT */

/* check gl errors on debug build */
#ifdef NDEBUG
#  define GL_CALL(x) x
#else
#  define GL_CALL(x) x; GL_ERROR(__LINE__, __func__, __STRING(x));
static inline void GL_ERROR(unsigned int line, const char *func, const char *glfunc)
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
#endif

/* check return value of gl function on debug build */
#ifdef NDEBUG
#  define GL_CHECK(x) x
#else
#  define GL_CHECK(x) GL_CHECK_ERROR(__func__, __STRING(x), x)
static inline GLenum GL_CHECK_ERROR(const char *func, const char *glfunc, GLenum error)
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

/* declare gl generation function */
#define DECLARE_GL_GEN_FUNC(x,y)                                     \
static void x(int count, unsigned int *objects)                      \
{                                                                    \
   CALL(0, "%d, %p", count, objects);                                \
   GL_CALL(y(count, objects));                                       \
}

/* declare gl bind function */
#define DECLARE_GL_BIND_FUNC(x,y)                                    \
static void x(unsigned int object)                                   \
{                                                                    \
   CALL(2, "%d", object);                                            \
   GL_CALL(y);                                                       \
}

/*** glhck mapping functions ***/
GLenum glhRenderPropertyForGlhckProperty(glhckRenderProperty property);
GLenum glhGeometryTypeForGlhckType(glhckGeometryType type);
GLenum glhTextureFormatForGlhckFormat(glhckTextureFormat format);
GLenum glhBlendingModeForGlhckMode(glhckBlendingMode mode);
GLenum glhAttachmentTypeForGlhckType(_glhckRttAttachmentType type);
GLenum glhShaderTypeForGlhckType(glhckShaderType type);
GLenum glhShaderVariableTypeForGlhckType(_glhckShaderVariableType type);
_glhckShaderVariableType glhGlhckShaderVariableTypeForGLType(GLenum type);
const GLchar* glhShaderVariableNameForOpenGLConstant(GLenum type);
const GLchar* glhShaderVariableNameForGlhckConstant(GLenum type);

/*** shared opengl functions ***/
void glhGetIntegerv(GLenum pname, GLint *params);
void glhClear(void);
void glhClearColor(GLchar r, GLchar g, GLchar b, GLchar a);
void glhBufferGetPixels(GLint x, GLint y, GLsizei width, GLsizei height,
      glhckTextureFormat format, GLvoid *data);
void glhBlendFunc(GLenum blenda, GLenum blendb);
void glhTextureBind(GLenum type, GLuint texture);
void glhTextureBindRestore(void);
GLuint glhTextureCreate(const GLvoid *buffer, GLsizei size,
      GLsizei width, GLsizei height, glhckTextureFormat format,
      GLuint reuseTextureObject, unsigned int flags);
void glhTextureFill(GLuint texture, const GLvoid *data, GLsizei size,
      GLint x, GLint y, GLsizei width, GLsizei height, glhckTextureFormat format);
int glhFramebufferLinkWithTexture(GLuint object, GLuint texture, _glhckRttAttachmentType type);
void glhGeometryRender(const glhckGeometry *geometry, glhckGeometryType type);

#endif /* __glhck_opengl_helper_h__ */
