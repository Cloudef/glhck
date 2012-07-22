#ifndef __imagedata_h__
#define __imagedata_h__

/* imagedata helpers */

#define IMAGE_DIMENSIONS_OK(w, h) \
   (((w) > 0) && ((h) > 0) &&     \
    ((unsigned long long)(w) * (unsigned long long)(h) <= (1ULL << 29) - 1))

#endif /* __imagedata_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
