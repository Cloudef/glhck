#ifndef _IMGHCK_H_
#define _IMGHCK_H_

unsigned char* imghckConvertToDXT1(
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels,
      size_t *out_size);

unsigned char* imghckConvertToDXT5(
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels,
      size_t *out_size);

#endif /* _IMGHCK_H_ */

/* vim: set ts=8 sw=3 tw=0 :*/
