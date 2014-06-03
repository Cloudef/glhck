#ifndef __glhck_texture_h__
#define __glhck_texture_h__

#include <glhck/glhck.h>

/* internal texture flags */
enum glhckTextureFlags {
   GLHCK_TEXTURE_IMPORT_NONE  = 0,
   GLHCK_TEXTURE_IMPORT_ALPHA = 1<<0,
   GLHCK_TEXTURE_IMPORT_TEXT  = 1<<1,
};

int _glhckHasAlpha(glhckTextureFormat format);
unsigned int _glhckNumChannels(unsigned int format);
int _glhckUnitSizeForTexture(glhckTextureFormat format, glhckDataType type);
int _glhckSizeForTexture(glhckTextureTarget target, int width, int height, int depth, glhckTextureFormat format, glhckDataType type);
int _glhckIsCompressedFormat(unsigned int format);
void _glhckNextPow2(int width, int height, int depth, int *outWidth, int *outHeight, int *outDepth, int limitToSize);
const kmVec3* _glhckTextureGetInternalScale(const glhckHandle handle);
unsigned int _glhckTextureGetObject(const glhckHandle handle);
const kmVec3* _glhckTextureGetInternalScale(const glhckHandle handle);

#endif /* __glhck_texture_h__ */
