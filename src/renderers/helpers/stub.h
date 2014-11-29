#ifndef __glhck_stub_helper_h__
#define __glhck_stub_helper_h__

#include <limits.h> /* for limits */
#include <stdio.h>  /* for snprintf */

#include "pool/pool.h"

struct glhckShaderUniform;

/*
 * generate/delete mappings
 */

__attribute__((used))
static void stubGenerate(int n, unsigned int *names) {
   if (!names) return;
   for (int i = 0; i != n; ++i) names[i] = 1;
}
__attribute__((used))
static void stubDelete(int n, const unsigned int *names) {
   (void)n; (void)names;
}

/*
 * binding mappings
 */

__attribute__((used))
static void stubTextureBind(glhckTextureTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
__attribute__((used))
static void stubTextureActive(unsigned int index) {
   CALL(2, "%u", index);
}
__attribute__((used))
static void stubRenderbufferBind(unsigned int object) {
   CALL(2, "%u", object);
}
__attribute__((used))
static void stubFramebufferBind(glhckFramebufferTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
__attribute__((used))
static void stubProgramBind(unsigned int obj) {
   CALL(2, "%u", obj);
}
__attribute__((used))
static void stubProgramDelete(unsigned int obj) {
   CALL(2, "%u", obj);
}
__attribute__((used))
static unsigned int stubShaderCompile(glhckShaderType type, const char *effectKey, const char *memoryContents) {
   CALL(2, "%d, %s, %p", type, effectKey, memoryContents);
   RET(2, "%u", 1);
   return 1;
}
__attribute__((used))
static void stubShaderDelete(unsigned int obj) {
   CALL(2, "%u", obj);
}
__attribute__((used))
static void stubHwBufferBind(glhckHwBufferTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
__attribute__((used))
static void stubHwBufferBindBase(glhckHwBufferTarget target, unsigned int index, unsigned int object) {
   CALL(2, "%d, %u, %u", target, index, object);
}
__attribute__((used))
static void stubHwBufferBindRange(glhckHwBufferTarget target, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size) {
   CALL(2, "%d, %u, %u, %td, %td", target, index, object, offset, size);
}
__attribute__((used))
static void stubHwBufferCreate(glhckHwBufferTarget target, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage) {
   CALL(2, "%d, %td, %p, %d", target, size, data, usage);
}
__attribute__((used))
static void stubHwBufferFill(glhckHwBufferTarget target, ptrdiff_t offset, ptrdiff_t size, const void *data) {
   CALL(2, "%d, %td, %td, %p", target, offset, size, data);
}
__attribute__((used))
static void* stubHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access) {
   CALL(2, "%d, %d", target, access);
   RET(2, "%p", NULL);
   return NULL;
}
__attribute__((used))
static void stubHwBufferUnmap(glhckHwBufferTarget target) {
   CALL(2, "%d", target);
}

/*
 * shared stub renderer functions
 */

/* \brief set time to renderer */
__attribute__((used))
static void stubTime(float time)
{
   CALL(2, "%f", time);
}

/* \brief set projection matrix */
__attribute__((used))
static void stubProjectionMatrix(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief set view matrix */
__attribute__((used))
static void stubModelMatrix(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief set texture matrix */
__attribute__((used))
static void stubTextureMatrix(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief resize viewport */
__attribute__((used))
static void stubViewport(int x, int y, int width, int height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
}

/* \brief set OpenGL clear color */
__attribute__((used))
static void stubClearColor(const glhckColor color)
{
   CALL(2, "%u", color);
}

/* \brief clear OpenGL buffers */
__attribute__((used))
static void stubClear(unsigned int bufferBits)
{
   CALL(2, "%u", bufferBits);
}

/* \brief set fragment color */
__attribute__((used))
static void stubColor(const glhckColor color)
{
   CALL(2, "%u", color);
}

__attribute__((used))
static void stubVertexPointer (int size, glhckDataType type, int stride, const void *pointer)
{
   CALL(2, "%d, %u, %d, %p", size, type, stride, pointer);
}

__attribute__((used))
static void stubCoordPointer(int size, glhckDataType type, int stride, const void *pointer)
{
   CALL(2, "%d, %u, %d, %p", size, type, stride, pointer);
}

__attribute__((used))
static void stubNormalPointer(glhckDataType type, int stride, const void *pointer)
{
   CALL(2, "%u, %d, %p", type, stride, pointer);
}

__attribute__((used))
static void stubDrawArrays(glhckGeometryType type, int first, int count)
{
   CALL(2, "%u, %d, %d", type, first, count);
}

__attribute__((used))
static void stubDrawElements(glhckGeometryType type, int count, glhckDataType dataType, const void *indices)
{
   CALL(2, "%u, %d, %u, %p", type, count, dataType, indices);
}

/* \brief set front face */
__attribute__((used))
static void stubFrontFace(glhckFaceOrientation orientation)
{
   CALL(2, "%d", orientation);
}

/* \brief set cull side */
__attribute__((used))
static void stubCullFace(glhckCullFaceType face)
{
   CALL(2, "%d", face);
}

/* \brief get pixels from OpenGL */
__attribute__((used))
static void stubBufferGetPixels(int x, int y, int width, int height, glhckTextureFormat format, glhckDataType type, void *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %p", x, y, width, height, format, type, data);
}

/* \brief blend func wrapper */
__attribute__((used))
static void stubBlendFunc(unsigned int blenda, unsigned int blendb)
{
   CALL(2, "%u, %u", blenda, blendb);
}

/* \brief set texture parameters */
__attribute__((used))
static void stubTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params)
{
   CALL(0, "%d, %p", target, params);
   assert(params);
}

/* \brief generate mipmaps for texture */
__attribute__((used))
static void stubTextureGenerateMipmap(glhckTextureTarget target)
{
   CALL(0, "%d", target);
}

/* \brief create texture from data and upload it to OpenGL */
__attribute__((used))
static void stubTextureImage(glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, int datatype, int size, const void *data)
{
   CALL(0, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, width, height, depth, border, format, datatype, size, data);
}

/* \brief fill texture with data */
__attribute__((used))
static void stubTextureFill(glhckTextureTarget target, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, int datatype, int size, const void *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, x, y, z, width, height, depth, format, datatype, size, data);
}

/* \brief glRenderbufferStorage wrapper */
__attribute__((used))
static void stubRenderbufferStorage(int width, int height, glhckTextureFormat format)
{
   CALL(0, "%d, %d, %d", width, height, format);
}

/* \brief glFramebufferTexture wrapper with error checking */
__attribute__((used))
static int stubFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, unsigned int texture, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %d, %u, %d", framebufferTarget, textureTarget, texture, attachment);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief glFramebufferRenderbuffer wrapper with error checking */
__attribute__((used))
static int stubFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, unsigned int buffer, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %u, %d", framebufferTarget, buffer, attachment);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

__attribute__((used))
static unsigned int stubProgramLink(unsigned int vsobj, unsigned int fsobj)
{
   CALL(2, "%u, %u", vsobj, fsobj);
   RET(2, "%u", 1);
   return 1;
}

__attribute__((used))
static int stubShadersPath(const char *pathPrefix, const char *pathSuffix)
{
   CALL(2, "%s, %s", pathPrefix, pathSuffix);
   return 1;
}

__attribute__((used))
static int stubShadersDirectiveToken(const char *token, const char *directive)
{
   CALL(2, "%s, %s", token, directive);
   return 1;
}

/* \brief attach uniform buffer object to shader */
__attribute__((used))
static unsigned int stubProgramAttachUniformBuffer(unsigned int program, const char *uboName, unsigned int location)
{
   CALL(0, "%u, %s, %u", program, uboName, location);
   RET(0, "%u", 0);
   return 0;
}

/* \brief create uniform buffer from shader */
__attribute__((used))
static chckIterPool* stubProgramUniformBufferPool(unsigned int program, const char *uboName, int *uboSize)
{
   CALL(0, "%u, %s, %p", program, uboName, uboSize);
   if (uboSize) *uboSize = 0;
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get attribute list from program */
__attribute__((used))
static chckIterPool* stubProgramAttributePool(unsigned int obj)
{
   CALL(0, "%u", obj);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get uniform list from program */
__attribute__((used))
static chckIterPool* stubProgramUniformPool(unsigned int obj)
{
   CALL(0, "%u", obj);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief set shader uniform */
__attribute__((used))
static void stubProgramUniform(unsigned int obj, const struct glhckShaderUniform *uniform, int count, const void *value)
{
   CALL(2, "%u, %p, %d, %p", obj, uniform, count, value);
}

#endif /* __glhck_stub_helper_h__ */
