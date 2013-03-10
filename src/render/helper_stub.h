#ifndef __glhck_stub_helper_h__
#define __glhck_stub_helper_h__

/*** generate/delete mappings ***/
void stubGenerate(int n, unsigned int *names);
void stubDelete(int n, const unsigned int *names);

/*** binding mappings ***/
void stubTextureBind(glhckTextureTarget target, unsigned int object);
void stubTextureActive(unsigned int index);
void stubRenderbufferBind(unsigned int object);
void stubFramebufferBind(glhckFramebufferTarget target, unsigned int object);
void stubProgramBind(unsigned int obj);
void stubProgramDelete(unsigned int obj);
unsigned int stubShaderCompile(glhckShaderType type, const char *effectKey, const char *memoryContents);
void stubShaderDelete(unsigned int obj);
void stubHwBufferBind(glhckHwBufferTarget target, unsigned int object);
void stubHwBufferBindBase(glhckHwBufferTarget target, unsigned int index, unsigned int object);
void stubHwBufferBindRange(glhckHwBufferTarget target, unsigned int index, unsigned int object, ptrdiff_t offset, ptrdiff_t size);
void stubHwBufferCreate(glhckHwBufferTarget target, ptrdiff_t size, const void *data, glhckHwBufferStoreType usage);
void stubHwBufferFill(glhckHwBufferTarget target, ptrdiff_t offset, ptrdiff_t size, const void *data);
void* stubHwBufferMap(glhckHwBufferTarget target, glhckHwBufferAccessType access);
void stubHwBufferUnmap(glhckHwBufferTarget target);

/*** shared stub functions ***/
void stubTime(float time);
void stubSetProjection(const kmMat4 *m);
void stubSetView(const kmMat4 *m);
void stubSetOrthographic(const kmMat4 *m);
void stubViewport(int x, int y, int width, int height);
void stubLightBind(glhckLight *light);
void stubFrustumRender(glhckFrustum *frustum);
void stubObjectRender(const _glhckObject *object);
void stubTextRender(const _glhckText *text);
void stubClear(unsigned int bufferBits);
void stubClearColor(const glhckColorb *color);
void stubCullFace(glhckCullFaceType face);
void stubBufferGetPixels(int x, int y, int width, int height, glhckTextureFormat format, void *data);
void stubBlendFunc(unsigned int blenda, unsigned int blendb);
void stubTextureFill(glhckTextureTarget target, int level, int x, int y, int z, int width, int height, int depth, glhckTextureFormat format, int datatype, int size, const void *data);
void stubTextureImage(glhckTextureTarget target, int level, int width, int height, int depth, int border, glhckTextureFormat format, int datatype, int size, const void *data);
void stubTextureParameter(glhckTextureTarget target, const glhckTextureParameters *params);
void stubTextureMipmap(glhckTextureTarget target);
void stubRenderbufferStorage(int width, int height, glhckTextureFormat format);
int stubFramebufferTexture(glhckFramebufferTarget framebufferTarget, glhckTextureTarget textureTarget, unsigned int texture, glhckFramebufferAttachmentType attachment);
int stubFramebufferRenderbuffer(glhckFramebufferTarget framebufferTarget, unsigned int buffer, glhckFramebufferAttachmentType attachment);
unsigned int stubProgramLink(unsigned int vsobj, unsigned int fsobj);
int stubShadersPath(const char *pathPrefix, const char *pathSuffix);
int stubShadersDirectiveToken(const char *token, const char *directive);
unsigned int stubProgramAttachUniformBuffer(unsigned int program, const char *uboName, unsigned int location);
_glhckHwBufferShaderUniform* stubProgramUniformBufferList(unsigned int program, const char *uboName, int *size);
_glhckShaderAttribute* stubProgramAttributeList(unsigned int obj);
_glhckShaderUniform* stubProgramUniformList(unsigned int obj);
void stubProgramSetUniform(unsigned int obj, _glhckShaderUniform *uniform, int count, const void *value);

#endif /* __glhck_stub_helper_h__ */
