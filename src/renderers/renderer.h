#ifndef __glhck_renderer_h__
#define __glhck_renderer_h__

#include <glhck/glhck.h>
#include <kazmath/kazmath.h>
#include "pool/pool.h"

#define INVALID_RENDERER (chckPoolIndex)-1

struct glhckShaderUniform;

struct glhckRendererAPI {

   /* view */
   void (*viewport) (int x, int y, int width, int height);
   void (*projectionMatrix) (const kmMat4 *m);
   void (*modelMatrix) (const kmMat4 *m);
   void (*textureMatrix) (const kmMat4 *m);

   /* render */
   void (*time) (float time);
   void (*clearColor) (const glhckColor color);
   void (*clear) (unsigned int bufferBits);
   void (*color) (const glhckColor color);
   void (*blendFunc) (glhckBlendingMode blenda, glhckBlendingMode blendb);
   void (*depthTest) (int);

   /* geometry */
   void (*vertexPointer) (int size, glhckDataType type, int stride, const void *pointer);
   void (*coordPointer) (int size, glhckDataType type, int stride, const void *pointer);
   void (*normalPointer) (glhckDataType type, int stride, const void *pointer);
   void (*drawArrays) (glhckGeometryType type, int first, int count);
   void (*drawElements) (glhckGeometryType type, int count, glhckDataType dataType, const void *indices);

   /* screen control */
   void (*bufferGetPixels) (int x, int y, int width, int height, glhckTextureFormat format, glhckDataType type, void *data);

   /* textures */
   void (*textureGenerate) (int count, unsigned int *objects);
   void (*textureDelete) (int count, const unsigned int *objects);
   void (*textureBind) (glhckTextureTarget target, unsigned int object);
   void (*textureActive) (unsigned int index);
   void (*textureFill) (glhckTextureTarget target, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, int datatype, int size, const void *data);
   void (*textureImage) (glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, int datatype, int size, const void *data);
   void (*textureParameter) (glhckTextureTarget target, const glhckTextureParameters *params);
   void (*textureGenerateMipmap) (glhckTextureTarget target);

   /* renderbuffers */
   void (*renderbufferGenerate) (int count, unsigned int *objects);
   void (*renderbufferDelete) (int count, const unsigned int *objects);
   void (*renderbufferBind) (unsigned int object);
   void (*renderbufferStorage) (int width, int height, glhckTextureFormat format);

   /* framebuffers */
   void (*framebufferGenerate) (int count, unsigned int *objects);
   void (*framebufferDelete) (int count, const unsigned int *objects);
   void (*framebufferBind) (glhckFramebufferTarget target, unsigned int object);
   int (*framebufferTexture) (glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, unsigned int texture, glhckFramebufferAttachmentType type);
   int (*framebufferRenderbuffer) (glhckFramebufferTarget framebufferTarget, unsigned int buffer, glhckFramebufferAttachmentType type);

   /* hardware buffer objects */
   void (*hwBufferGenerate) (int count, unsigned int *objects);
   void (*hwBufferDelete) (int count, const unsigned int *objects);
   void (*hwBufferBind) (glhckHwBufferTarget target, unsigned int object);
   void (*hwBufferCreate) (glhckHwBufferTarget target, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage);
   void (*hwBufferFill) (glhckHwBufferTarget target, ptrdiff_t offset, ptrdiff_t size, const void *data);
   void (*hwBufferBindBase) (glhckHwBufferTarget target, unsigned int index, unsigned int object);
   void (*hwBufferBindRange) (glhckHwBufferTarget target, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size);
   void* (*hwBufferMap) (glhckHwBufferTarget target, glhckHwBufferAccessType access);
   void (*hwBufferUnmap) (glhckHwBufferTarget target);

   /* shaders */
   void (*programBind) (unsigned int program);
   unsigned int (*programLink) (unsigned int vertexShader, unsigned int fragmentShader);
   void (*programDelete) (unsigned int program);
   unsigned int (*shaderCompile) (glhckShaderType type, const char *effectKey, const char *contentsFromMemory);
   void (*shaderDelete) (unsigned int shader);
   chckIterPool* (*programUniformBufferPool) (unsigned int program, const char *uboName, int *size);
   chckIterPool* (*programAttributePool) (unsigned int program);
   chckIterPool* (*programUniformPool) (unsigned int program);
   void (*programUniform) (unsigned int program, const struct glhckShaderUniform *uniform, int count, const void *value);
   unsigned int (*programAttachUniformBuffer) (unsigned int program, const char *name, unsigned int location);
   int (*shadersPath) (const char *pathPrefix, const char *pathSuffix);
   int (*shadersDirectiveToken) (const char* token, const char *directive);
};

struct glhckRendererInfo {
   int (*constructor)(struct glhckRendererInfo*);
   void (*destructor)(void);
   glhckRendererContextInfo context;
   glhckRendererFeatures features;
   struct glhckRendererAPI api;
};

int _glhckRendererInit(void);
void _glhckRendererTerminate(void);
int _glhckRendererRegister(const char *file);
void _glhckRendererDeactivate(void);
int _glhckRendererActivate(size_t renderer);
size_t _glhckRendererGetActive(void);

#endif /* __glhck_renderer_h__ */
