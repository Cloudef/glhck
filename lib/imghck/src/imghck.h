#ifndef _IMGHCK_H_
#define _IMGHCK_H_

size_t imghckSizeForDXT1(unsigned int width, unsigned int height);
size_t imghckSizeForDXT5(unsigned int width, unsigned int height);

int imghckConvertToDXT1(unsigned char * out,
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels);

int imghckConvertToDXT5(unsigned char * out,
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels);

#endif /* _IMGHCK_H_ */

/* vim: set ts=8 sw=3 tw=0 :*/
