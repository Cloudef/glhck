#ifndef __vertexdata_h__
#define __vertexdata_h__

/* conversion defines for vertexdata conversion macro */
#if GLHCK_PRECISION_VERTEX == GLHCK_BYTE
#  define GLHCK_VERTEX_MAGIC  255.0f - 127.0f
#  define GLHCK_BIAS_OFFSET   127.0f / 255.0f
#  define GLHCK_SCALE_OFFSET  127.0f - 1.0f
#elif GLHCK_PRECISION_VERTEX == GLHCK_SHORT
#  define GLHCK_VERTEX_MAGIC  65536.0f - 32768.0f
#  define GLHCK_BIAS_OFFSET   32768.0f / 65536.0f
#  define GLHCK_SCALE_OFFSET  65536.0f - 1.0f
#endif

#if GLHCK_PRECISION_COORD == GLHCK_BYTE
#  define GLHCK_COORD_MAGIC 127.0f - 255.0f
#elif GLHCK_PRECISION_COORD == GLHCK_SHORT
#  define GLHCK_COORD_MAGIC 65536.0f - 32768.0f
#endif

/* helper macro for vertexdata conversion */
#define convert2d(dst, src, max, min, magic, cast)                         \
dst.x = (cast)floorf(((src.x - min.x) / (max.x - min.x)) * magic + 0.5f);  \
dst.y = (cast)floorf(((src.y - min.y) / (max.y - min.y)) * magic + 0.5f);
#define convert3d(dst, src, max, min, magic, cast)                         \
convert2d(dst, src, max, min, magic, cast)                                 \
dst.z = (cast)floorf(((src.z - min.z) / (max.z - min.z)) * magic + 0.5f);

/* find max && min ranges */
#define max2d(dst, src) \
if (src.x > dst.x)      \
   dst.x = src.x;       \
if (src.y > dst.y)      \
   dst.y = src.y;
#define max3d(dst, src) \
max2d(dst, src)         \
if (src.z > dst.z)      \
   dst.z = src.z;
#define min2d(dst, src) \
if (src.x < dst.x)      \
   dst.x = src.x;       \
if (src.y < dst.y)      \
   dst.y = src.y;
#define min3d(dst, src) \
min2d(dst, src)         \
if (src.z < dst.z)      \
   dst.z = src.z;

/* assign 3d */
#define set2d(dst, src) \
dst.x = src.x;          \
dst.y = src.y;
#define set3d(dst, src) \
set2d(dst, src)         \
dst.z = src.z;

#endif /* __vertexdata_h__ */

/* vim: set ts=8 sw=3 tw=0 :*/
