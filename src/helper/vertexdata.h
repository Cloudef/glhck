#ifndef __vertexdata_h__
#define __vertexdata_h__

/* conversion constants for vertexdata conversion */
#define GLHCK_BYTE_CMAGIC  CHAR_MAX
#define GLHCK_BYTE_VMAGIC  UCHAR_MAX - CHAR_MAX
#define GLHCK_BYTE_VBIAS   CHAR_MAX / (UCHAR_MAX - 1.0f)
#define GLHCK_BYTE_VSCALE  UCHAR_MAX - 1.0f

#define GLHCK_SHORT_CMAGIC  SHRT_MAX
#define GLHCK_SHORT_VMAGIC  USHRT_MAX - SHRT_MAX
#define GLHCK_SHORT_VBIAS   SHRT_MAX / (USHRT_MAX - 1.0f)
#define GLHCK_SHORT_VSCALE  USHRT_MAX - 1.0f

/* assign 2D v2 vector to v1 vector regardless
 * of datatype */
#define glhckSetV2(v1, v2) \
   (v1)->x = (v2)->x;      \
   (v1)->y = (v2)->y;

/* assign 3D v2 vector to v1 vector regardless
 * of datatype */
#define glhckSetV3(v1, v2) \
   glhckSetV2(v1, v2);     \
   (v1)->z = (v2)->z;

/* assign the max units from vectors to v1 */
#define glhckMaxV2(v1, v2) \
   if ((v1)->x < (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y < (v2)->y) (v1)->y = (v2)->y;
#define glhckMaxV3(v1, v2) \
   glhckMaxV2(v1, v2);     \
   if ((v1)->z < (v2)->z) (v1)->z = (v2)->z;

/* assign the min units from vectors to v1 */
#define glhckMinV2(v1, v2) \
   if ((v1)->x > (v2)->x) (v1)->x = (v2)->x; \
   if ((v1)->y > (v2)->y) (v1)->y = (v2)->y;
#define glhckMinV3(v1, v2) \
   glhckMinV2(v1, v2);     \
   if ((v1)->z > (v2)->z) (v1)->z = (v2)->z;

#endif /* __vertexdata_h__ */
