#include "../internal.h"
#include <limits.h> /* for limits */
#include <stdio.h>  /* for snprintf */

#define GLHCK_CHANNEL GLHCK_CHANNEL_RENDER
#include "helper_stub.h"

/*
 * generate/delete mappings
 */

void stubGenerate(int n, unsigned int *names) {
   int i;
   if (!names) return;
   for (i = 0; i != n; ++i) names[i] = 1;
}
void stubDelete(int n, const unsigned int *names) {
   (void)n; (void)names;
}

/*
 * binding mappings
 */

void stubTextureBind(glhckTextureTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
void stubTextureActive(unsigned int index) {
   CALL(2, "%u", index);
}
void stubRenderbufferBind(unsigned int object) {
   CALL(2, "%u", object);
}
void stubFramebufferBind(glhckFramebufferTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
void stubProgramBind(unsigned int obj) {
   CALL(2, "%u", obj);
}
void stubProgramDelete(unsigned int obj) {
   CALL(2, "%u", obj);
}
unsigned int stubShaderCompile(glhckShaderType type, const char *effectKey, const char *memoryContents) {
   CALL(2, "%d, %s, %p", type, effectKey, memoryContents);
   RET(2, "%u", 1);
   return 1;
}
void stubShaderDelete(unsigned int obj) {
   CALL(2, "%u", obj);
}
void stubHwBufferBind(glhckHwBufferTarget target, unsigned int object) {
   CALL(2, "%d, %u", target, object);
}
void stubHwBufferBindBase(glhckHwBufferTarget target, unsigned int index, unsigned int object) {
   CALL(2, "%d, %u, %u", target, index, object);
}
void stubHwBufferBindRange(glhckHwBufferTarget target, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size) {
   CALL(2, "%d, %u, %u, %td, %td", target, index, object, offset, size);
}
void stubHwBufferCreate(glhckHwBufferTarget target, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage) {
   CALL(2, "%d, %td, %p, %d", target, size, data, usage);
}
void stubHwBufferFill(glhckHwBufferTarget target, ptrdiff_t offset, ptrdiff_t size, const void *data) {
   CALL(2, "%d, %td, %td, %p", target, offset, size, data);
}
void* stubHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access) {
   CALL(2, "%d, %d", target, access);
   RET(2, "%p", NULL);
   return NULL;
}
void stubHwBufferUnmap(glhckHwBufferTarget target) {
   CALL(2, "%d", target);
}

/*
 * shared stub renderer functions
 */

/* \brief set time to renderer */
void stubTime(float time)
{
   CALL(2, "%f", time);
}

/* \brief set projection matrix */
void stubSetProjection(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief set view matrix */
void stubSetView(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief set orthographic projection for 2D drawing */
void stubSetOrthographic(const kmMat4 *m)
{
   CALL(2, "%p", m);
}

/* \brief resize viewport */
void stubViewport(int x, int y, int width, int height)
{
   CALL(2, "%d, %d, %d, %d", x, y, width, height);
}

/* \brief bind light to renderer */
void stubLightBind(glhckLight *light)
{
   CALL(2, "%p", light);
}

/* \brief render frustum */
void stubFrustumRender(glhckFrustum *frustum)
{
   CALL(2, "%p", frustum);
}

/* \brief render single 3d object */
void stubObjectRender(const glhckObject *object)
{
   CALL(2, "%p", object);
}

/* \brief render text */
void stubTextRender(const glhckText *text)
{
   CALL(2, "%p", text);
}

/* \brief clear OpenGL buffers */
void stubClear(unsigned int bufferBits)
{
   CALL(2, "%u", bufferBits);
}

/* \brief set OpenGL clear color */
void stubClearColor(const glhckColorb *color)
{
   CALL(2, "%p", color);
}

/* \brief set cull side */
void stubCullFace(glhckCullFaceType face)
{
   CALL(2, "%d", face);
}

/* \brief get pixels from OpenGL */
void stubBufferGetPixels(int x, int y, int width, int height,
      glhckTextureFormat format, void *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %p", x, y, width, height, format, data);
}

/* \brief blend func wrapper */
void stubBlendFunc(unsigned int blenda, unsigned int blendb)
{
   CALL(2, "%u, %u", blenda, blendb);
}

/* \brief set texture parameters */
void stubTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params)
{
   CALL(0, "%d, %p", target, params);
   assert(params);
}

/* \brief generate mipmaps for texture */
void stubTextureMipmap(glhckTextureTarget target)
{
   CALL(0, "%d", target);
}

/* \brief create texture from data and upload it to OpenGL */
void stubTextureImage(glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, int datatype, int size, const void *data)
{
   CALL(0, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, width, height, depth, border, format, datatype, size, data);
}

/* \brief fill texture with data */
void stubTextureFill(glhckTextureTarget target, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, int datatype, int size, const void *data)
{
   CALL(1, "%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %p", target, level, x, y, z, width, height, depth, format, datatype, size, data);
}

/* \brief glRenderbufferStorage wrapper */
void stubRenderbufferStorage(int width, int height, glhckTextureFormat format)
{
   CALL(0, "%d, %d, %d", width, height, format);
}

/* \brief glFramebufferTexture wrapper with error checking */
int stubFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, unsigned int texture, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %d, %u, %d", framebufferTarget, textureTarget, texture, attachment);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief glFramebufferRenderbuffer wrapper with error checking */
int stubFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, unsigned int buffer, glhckFramebufferAttachmentType attachment)
{
   CALL(0, "%d, %u, %d", framebufferTarget, buffer, attachment);
   RET(0, "%d", RETURN_OK);
   return RETURN_OK;
}

unsigned int stubProgramLink(unsigned int vsobj, unsigned int fsobj)
{
   CALL(2, "%u, %u", vsobj, fsobj);
   RET(2, "%u", 1);
   return 1;
}

int stubShadersPath(const char *pathPrefix, const char *pathSuffix)
{
   CALL(2, "%s, %s", pathPrefix, pathSuffix);
   return 1;
}

int stubShadersDirectiveToken(const char *token, const char *directive)
{
   CALL(2, "%s, %s", token, directive);
   return 1;
}

/* \brief attach uniform buffer object to shader */
unsigned int stubProgramAttachUniformBuffer(unsigned int program, const char *uboName, unsigned int location)
{
   CALL(0, "%u, %s, %u", program, uboName, location);
   RET(0, "%u", 0);
   return 0;
}

/* \brief create uniform buffer from shader */
_glhckHwBufferShaderUniform* stubProgramUniformBufferList(unsigned int program, const char *uboName, int *uboSize)
{
   CALL(0, "%u, %s, %p", program, uboName, uboSize);
   if (uboSize) *uboSize = 0;
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get attribute list from program */
_glhckShaderAttribute* stubProgramAttributeList(unsigned int obj)
{
   CALL(0, "%u", obj);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief get uniform list from program */
_glhckShaderUniform* stubProgramUniformList(unsigned int obj)
{
   CALL(0, "%u", obj);
   RET(0, "%p", NULL);
   return NULL;
}

/* \brief set shader uniform */
void stubProgramUniform(unsigned int obj, _glhckShaderUniform *uniform, int count, const void *value)
{
   CALL(2, "%u, %p, %d, %p", obj, uniform, count, value);
}

/* vim: set ts=8 sw=3 tw=0 :*/
