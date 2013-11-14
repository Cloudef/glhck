#include "internal.h"
#include <stdint.h> /* for standard integers */

#define GLHCK_CHANNEL GLHCK_CHANNEL_MEMBUFFER

/* \brief is current computer big endian? */
static int _glhckIsBigEndian(void)
{
   union {
      uint32_t i;
      char c[4];
   } bint = {0x01020304};
   return bint.c[0] == 1;
}

/* \brief flip endianess of variable */
static void _glhckFlipEndian(void *v, size_t size)
{
   size_t s;
   unsigned char b[size];
   assert(v);
   memcpy(b, v, size);
   for (s = 0; s < size; ++s) memset(v+s, b[size - s - 1], 1);
}

/* \brief free glhck memory buffer */
void _glhckBufferFree(_glhckBuffer *buf)
{
   assert(buf);
   if (buf->buffer && buf->freeBuffer) _glhckFree(buf->buffer);
   _glhckFree(buf);
}

/* \brief create new glhck buffer with pointer and size
 * Buffer won't be copied nor freed! */
_glhckBuffer* _glhckBufferNewFromPointer(void *ptr, size_t size, _glhckBufferEndianType endianess)
{
   _glhckBuffer *buf;

   if (!(buf = _glhckCalloc(1, sizeof(_glhckBuffer))))
      return NULL;

   if (endianess == GLHCK_BUFFER_ENDIAN_NATIVE) {
      buf->endianess = _glhckIsBigEndian();
   } else {
      buf ->endianess = endianess;
   }

   buf->size = size;
   buf->buffer = buf->curpos = ptr;
   return buf;
}

/* \brief create new glhck memory buffer
 * Buffer with the size will be allocated */
_glhckBuffer* _glhckBufferNew(size_t size, _glhckBufferEndianType endianess)
{
   void *data;
   _glhckBuffer *buf;
   assert(size > 0);

   if (!(data = _glhckMalloc(size)))
      goto fail;

   if (!(buf = _glhckBufferNewFromPointer(data, size, endianess)))
      goto fail;

   buf->freeBuffer = 1;
   return buf;

fail:
   IFDO(_glhckFree, data);
   return NULL;
}

/* \brief is current buffer endianess same as our machine? */
int _glhckBufferIsNativeEndian(_glhckBuffer *buf) { return _glhckIsBigEndian() == buf->endianess; }

/* \brief read bytes from buffer */
size_t _glhckBufferRead(void *dst, size_t size, size_t memb, _glhckBuffer *buf)
{
   assert(dst && buf);

   if (size * memb > buf->size - (buf->curpos - buf->buffer))
      return 0;

   memcpy(dst, buf->curpos, size * memb);
   buf->curpos += size * memb;
   return memb;
}

/* \brief read 8 bit unsigned integer from buffer */
int _glhckBufferReadUInt8(_glhckBuffer *buf, unsigned char *i)
{
   uint8_t r;
   assert(i);

   if (_glhckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   *i = r;
   return RETURN_OK;
}

/* \brief read 8 bit signed integer from buffer */
int _glhckBufferReadInt8(_glhckBuffer *buf, char *i) { return _glhckBufferReadUInt8(buf, (unsigned char*)i); }

/* \brief read 16 bit unsigned integer from buffer */
int _glhckBufferReadUInt16(_glhckBuffer *buf, unsigned short *i)
{
   uint16_t r;
   assert(i);

   if (_glhckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   if (!_glhckBufferIsNativeEndian(buf)) _glhckFlipEndian(&r, sizeof(r));
   *i = r;
   return RETURN_OK;
}

/* \brief read 16 bit signed integer from buffer */
int _glhckBufferReadInt16(_glhckBuffer *buf, short *i) { return _glhckBufferReadUInt16(buf, (unsigned short*)i); }

/* \brief read 32 bit unsigned integer from buffer */
int _glhckBufferReadUInt32(_glhckBuffer *buf, unsigned int *i)
{
   uint32_t r;
   assert(i);

   if (_glhckBufferRead(&r, sizeof(r), 1, buf) != 1)
      return RETURN_FAIL;

   if (!_glhckBufferIsNativeEndian(buf)) _glhckFlipEndian(&r, sizeof(r));
   *i = r;
   return RETURN_OK;
}

/* \brief read 32 bit signed integer from buffer */
int _glhckBufferReadInt32(_glhckBuffer *buf, int *i) { return _glhckBufferReadUInt32(buf, (unsigned int*)i); }

/* \brief read string from buffer */
int _glhckBufferReadString(_glhckBuffer *buf, size_t bytes, char **str)
{
   union {
      uint8_t l8;
      uint16_t l16;
      uint32_t l32;
   } u;
   size_t len;
   assert(buf && str && bytes > 0 && bytes <= sizeof(uint32_t));
   *str = NULL;

   if (bytes == sizeof(uint32_t)) {
      if (_glhckBufferReadUInt32(buf, &u.l32) != RETURN_OK)
         return RETURN_FAIL;
      len = u.l32;
   } else if (bytes == sizeof(uint16_t)) {
      if (_glhckBufferReadUInt16(buf, &u.l16) != RETURN_OK)
         return RETURN_FAIL;
      len = u.l16;
   } else {
      if (_glhckBufferRead(&u.l8, bytes, 1, buf) != 1)
         return RETURN_FAIL;
      len = u.l8;
   }

   if (len) {
      if (!(*str = _glhckCalloc(1, len+1)))
         return RETURN_FAIL;
      _glhckBufferRead(*str, 1, len, buf);
   }
   return RETURN_OK;
}

/* vim: set ts=8 sw=3 tw=0 :*/
