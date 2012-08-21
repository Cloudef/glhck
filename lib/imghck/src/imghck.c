#include <assert.h>

#ifdef __APPLE__
#   include <malloc/malloc.h>
#else
#   include <malloc.h>
#endif

static void imghckCompressDXT1RGB(
      int channels,
      const unsigned char * uncompressed,
      unsigned char compressed[8]);
static void imghckCompressDXT5RGBA(
      const unsigned char * uncompressed,
      unsigned char compressed[8]);

/* size for DXT1 compression with the dimensions */
size_t imghckSizeForDXT1(unsigned int width, unsigned int height)
{
   return ((width+3) >> 2) * ((height+3) >> 2) * 8;
}

/* size for DXT5 compression with the dimensions */
size_t imghckSizeForDXT5(unsigned int width, unsigned int height)
{
   return ((width+3) >> 2) * ((height+3) >> 2) * 16;
}

/* compress uncompressed buffer with DXT1 compression */
int imghckConvertToDXT1(unsigned char * out,
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels)
{
   unsigned char ublock[16*3];
   unsigned char cblock[8];
   unsigned int i, j, x, y;
   unsigned int index = 0, chan_step = 1;
   unsigned int block_count = 0;
   unsigned int idx, mx, my;

   assert(out != NULL);
   assert(uncompressed != NULL);
   assert(width > 0 && height > 0);
   assert(channels >= 1 || channels <= 4);

   /* for channels == 1 or 2, I do not step forward for R,G,B values */
   if (channels < 3) chan_step = 0;

   /* go through each block */
   for (j = 0; j < height; j += 4) {
      for (i = 0; i < width; i += 4) {
         idx = 0, mx = 4, my = 4;
         /* copy this block into a new one */
         if (j+4 >= height)
            my = height - j;

         if (i+4 >= width)
            mx = width - i;

         for (y = 0; y < my; ++y) {
            for (x = 0; x < mx; ++x) {
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels];
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step];
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step+chan_step];
            }

            for (x = mx; x < 4; ++x) {
               ublock[idx++] = ublock[0];
               ublock[idx++] = ublock[1];
               ublock[idx++] = ublock[2];
            }
         }

         for (y = my; y < 4; ++y) {
            for (x = 0; x < 4; ++x) {
               ublock[idx++] = ublock[0];
               ublock[idx++] = ublock[1];
               ublock[idx++] = ublock[2];
            }
         }

         /* compress the block */
         ++block_count;
         imghckCompressDXT1RGB(3, ublock, cblock);

         /* copy the data from the block into the main block */
         for (x = 0; x < 8; ++x) out[index++] = cblock[x];
      }
   }
   return 1;
}

/* compress uncompressed buffer with DXT5 compression */
int imghckConvertToDXT5(unsigned char * out,
      const unsigned char * uncompressed,
      unsigned int width, unsigned int height, int channels)
{
   unsigned char ublock[16*4];
   unsigned char cblock[8];
   unsigned int i, j, x, y;
   unsigned int index = 0, chan_step = 1;
   unsigned int block_count = 0, has_alpha;
   unsigned int idx, mx, my;

   assert(out != NULL);
   assert(uncompressed != NULL);
   assert(width > 0 || height > 0);
   assert(channels >= 1 && channels <= 4);

   /* for channels == 1 or 2, I do not step forward for R,G,B vales */
   if (channels < 3) chan_step = 0;

   /* channels = 1 or 3 have no alpha, 2 & 4 do have alpha */
   has_alpha = 1 - (channels & 1);

   /* go through each block */
   for (j = 0; j < height; j += 4) {
      for (i = 0; i < width; i += 4) {
         idx = 0, mx = 4, my = 4;

         if (j+4 >= height)
            my = height - j;
         if (i+4 >= width)
            mx = width - i;

         for (y = 0; y < my; ++y) {
            for (x = 0; x < mx; ++x) {
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels];
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step];
               ublock[idx++] = uncompressed[(j+y)*width*channels+(i+x)*channels+chan_step+chan_step];
               ublock[idx++] =
                  has_alpha * uncompressed[(j+y)*width*channels+(i+x)*channels+channels-1]
                  + (1-has_alpha)*255;
            }

            for (x = mx; x < 4; ++x) {
               ublock[idx++] = ublock[0];
               ublock[idx++] = ublock[1];
               ublock[idx++] = ublock[2];
               ublock[idx++] = ublock[3];
            }
         }

         for (y = my; y < 4; ++y) {
            for (x = 0; x < 4; ++x) {
               ublock[idx++] = ublock[0];
               ublock[idx++] = ublock[1];
               ublock[idx++] = ublock[2];
               ublock[idx++] = ublock[3];
            }
         }

         imghckCompressDXT5RGBA(ublock, cblock);
         for (x = 0; x < 8; ++x) out[index++] = cblock[x];

         ++block_count;
         imghckCompressDXT1RGB(4, ublock, cblock);
         for (x = 0; x < 8; ++x) out[index++] = cblock[x];
      }
   }
   return 1;
}

/* helper functions */
static inline int convert_bit_range(int c, int from_bits, int to_bits)
{
   int b = (1 << (from_bits - 1)) + c * ((1 << to_bits) - 1);
   return (b + (b >> from_bits)) >> from_bits;
}

static inline int rgb_to_565(int r, int g, int b)
{
   return
      (convert_bit_range(r, 8, 5) << 11) |
      (convert_bit_range(g, 8, 6) << 05) |
      (convert_bit_range(b, 8, 5) << 00);
}

static inline void rgb_888_from_565(unsigned int c, int *r, int *g, int *b)
{
   *r = convert_bit_range((c >> 11) & 31, 5, 8);
   *g = convert_bit_range((c >> 05) & 63, 6, 8);
   *b = convert_bit_range((c >> 00) & 31, 5, 8);
}

static void compute_color_line_STDEV(
      const unsigned char * uncompressed,
      int channels,
      float point[3], float direction[3])
{
   unsigned int i;
   const float inv_16 = 1.0f / 16.0f;
   float sum_r = 0.0f, sum_g = 0.0f, sum_b = 0.0f;
   float sum_rr = 0.0f, sum_gg = 0.0f, sum_bb = 0.0f;
   float sum_rg = 0.0f, sum_rb = 0.0f, sum_gb = 0.0f;

   /* calculate all data needed for the covariance matrix */
   for (i = 0; i < 16*channels; i += channels) {
      sum_r += uncompressed[i+0];
      sum_rr += uncompressed[i+0] * uncompressed[i+0];
      sum_g += uncompressed[i+1];
      sum_gg += uncompressed[i+1] * uncompressed[i+1];
      sum_b += uncompressed[i+2];
      sum_bb += uncompressed[i+2] * uncompressed[i+2];
      sum_rg += uncompressed[i+0] * uncompressed[i+1];
      sum_rb += uncompressed[i+0] * uncompressed[i+2];
      sum_gb += uncompressed[i+1] * uncompressed[i+2];
   }

   /* convert the sums to averages */
   sum_r *= inv_16;
   sum_g *= inv_16;
   sum_b *= inv_16;

   /* and convert the squares to the squares of the value - avg_value */
   sum_rr -= 16.0f * sum_r * sum_r;
   sum_gg -= 16.0f * sum_g * sum_g;
   sum_bb -= 16.0f * sum_b * sum_b;
   sum_rg -= 16.0f * sum_r * sum_g;
   sum_rb -= 16.0f * sum_r * sum_b;
   sum_gb -= 16.0f * sum_g * sum_b;

   /* the point on the color line is the average      */
   point[0] = sum_r;
   point[1] = sum_g;
   point[2] = sum_b;

   /*
    * The following idea was from ryg.
    * (https://mollyrocket.com/forums/viewtopic.php?t=392)
    * The method worked great (less RMSE than mine) most of
    * the time, but had some issues handling some simple
    * boundary cases, like full green next to full red,
    * which would generate a covariance matrix like this:
    *
    * | 1  -1  0 |
    * | -1  1  0 |
    * | 0   0  0 |
    *
    * For a given starting vector, the power method can
    * generate all zeros!  So no starting with {1,1,1}
    * as I was doing!  This kind of error is still a
    * slight posibillity, but will be very rare.
    */

   /* 1st iteration, don't use all 1.0 values! */
   sum_r = 1.0f;
   sum_g = 2.718281828f;
   sum_b = 3.141592654f;
   direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
   direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
   direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;

   /* 2nd iteration, use results from the 1st guy */
   sum_r = direction[0];
   sum_g = direction[1];
   sum_b = direction[2];
   direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
   direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
   direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;

   /* 3rd iteration, use results from the 2nd guy */
   sum_r = direction[0];
   sum_g = direction[1];
   sum_b = direction[2];
   direction[0] = sum_r*sum_rr + sum_g*sum_rg + sum_b*sum_rb;
   direction[1] = sum_r*sum_rg + sum_g*sum_gg + sum_b*sum_gb;
   direction[2] = sum_r*sum_rb + sum_g*sum_gb + sum_b*sum_bb;
}

/* get master colors for RGBDXT1 block compression */
static void LSE_master_colors_max_min(
      int *cmax, int *cmin, int channels,
      const unsigned char * uncompressed)
{
   unsigned int i, j;
   int c0[3], c1[3];
   float sum_x[] = { 0.0f, 0.0f, 0.0f };
   float sum_x2[] = { 0.0f, 0.0f, 0.0f };
   float dot_max = 1.0f, dot_min = -1.0f;
   float vec_len2 = 0.0f;
   float dot;

   assert(channels >= 3 && channels <= 4);

   compute_color_line_STDEV(uncompressed, channels, sum_x, sum_x2);
   vec_len2 = 1.0f / (0.00001f +
         sum_x2[0]*sum_x2[0] + sum_x2[1]*sum_x2[1] + sum_x2[2]*sum_x2[2]);

   dot_max = (
         sum_x2[0] * uncompressed[0] +
         sum_x2[1] * uncompressed[1] +
         sum_x2[2] * uncompressed[2]
         );

   dot_min = dot_max;
   for (i = 1; i < 16; ++i) {
      dot = (
            sum_x2[0] * uncompressed[i*channels+0] +
            sum_x2[1] * uncompressed[i*channels+1] +
            sum_x2[2] * uncompressed[i*channels+2]
            );
      if (dot < dot_min)      dot_min = dot;
      else if (dot > dot_max) dot_max = dot;
   }

   /* and the offset (from the average location) */
   dot = sum_x2[0]*sum_x[0] + sum_x2[1]*sum_x[1] + sum_x2[2]*sum_x[2];
   dot_min -= dot;
   dot_max -= dot;

   /* post multiply by the scaling factor */
   dot_min *= vec_len2;
   dot_max *= vec_len2;

   for (i = 0; i < 3; ++i) {
      /* color 0 */
      c0[i] = (int)(0.5f + sum_x[i] + dot_max * sum_x2[i]);
      if (c0[i] < 0)        c0[i] = 0;
      else if (c0[i] > 255) c0[i] = 255;

      /* color 1 */
      c1[i] = (int)(0.5f + sum_x[i] + dot_min * sum_x2[i]);
      if (c1[i] < 0)        c1[i] = 0;
      else if (c1[i] > 255) c1[i] = 255;
   }

   i = rgb_to_565(c0[0], c0[1], c0[2]);
   j = rgb_to_565(c1[0], c1[1], c1[2]);
   if (i > j) {
      *cmax = i;
      *cmin = j;
   } else {
      *cmax = j;
      *cmin = i;
   }
}

/* compress RGB block with DXT1 compression */
static void imghckCompressDXT1RGB(
      int channels,
      const unsigned char * uncompressed,
      unsigned char compressed[8])
{
   int next_bit;
   int enc_c0, enc_c1;
   int c0[4], c1[4];
   unsigned int i;
   float color_line[] = { 0.0f, 0.0f, 0.0f, 0.0f };
   float vec_len2 = 0.0f, dot_offset = 0.0f, dot_product;
   int swizzle4[] = { 0, 2, 3, 1 };
   int next_value;

   /* get the master colors */
   LSE_master_colors_max_min(&enc_c0, &enc_c1, channels, uncompressed);

   /* store the 565 color 0 and color 1 */
   compressed[0] = (enc_c0 >> 0) & 255;
   compressed[1] = (enc_c0 >> 8) & 255;
   compressed[2] = (enc_c1 >> 0) & 255;
   compressed[3] = (enc_c1 >> 8) & 255;

   /* zero out the compressed data  */
   compressed[4] = 0;
   compressed[5] = 0;
   compressed[6] = 0;
   compressed[7] = 0;

   /* reconstitute the master color vectors */
   rgb_888_from_565(enc_c0, &c0[0], &c0[1], &c0[2]);
   rgb_888_from_565(enc_c1, &c1[0], &c1[1], &c1[2]);

   /* the new vector */
   vec_len2 = 0.0f;
   for (i = 0; i < 3; ++i) {
      color_line[i] = (float)(c1[i] - c0[i]);
      vec_len2 += color_line[i] * color_line[i];
   }

   if (vec_len2 > 0.0f)
      vec_len2 = 1.0f / vec_len2;

   /* pre-proform the scaling */
   color_line[0] *= vec_len2;
   color_line[1] *= vec_len2;
   color_line[2] *= vec_len2;

   /* compute the offset (constant) portion of the dot product */
   dot_offset = color_line[0]*c0[0] + color_line[1]*c0[1] + color_line[2]*c0[2];

   /* store the rest of the bits */
   next_bit = 8*4;
   for (i = 0; i < 16; ++i) {
      /* find the dot product of this color, to place it on the line
       * (should be [-1,1]) */
      next_value = 0;
      dot_product =
         color_line[0] * uncompressed[i*channels+0] +
         color_line[1] * uncompressed[i*channels+1] +
         color_line[2] * uncompressed[i*channels+2] -
         dot_offset;

      next_value = (int)(dot_product * 3.0f + 0.5);
      if (next_value > 3)      next_value = 3;
      else if (next_value < 0) next_value = 0;

      compressed[next_bit >> 3] |= swizzle4[ next_value ] << (next_bit & 7);
      next_bit += 2;
   }
}

/* compress RGBA block with DXT5 compression */
static void imghckCompressDXT5RGBA(
      const unsigned char *uncompressed,
      unsigned char compressed[8])
{
   int next_bit;
   int a0, a1;
   int svalue, value, factor;
   unsigned int i;
   float scale_me;
   int swizzle8[] = { 1, 7, 6, 5, 4, 3, 2, 0 };

   /* get the alpha limits (a0 > a1) */
   a0 = a1 = uncompressed[3];
   for (i = 4+3; i < 16*4; i += 4) {
      if (uncompressed[i] > a0)
         a0 = uncompressed[i];
      else if (uncompressed[i] < a1)
         a1 = uncompressed[i];
   }

   /* store those limits, and zero the rest of the compressed dataset */
   compressed[0] = a0;
   compressed[1] = a1;

   /* zero out the compressed data */
   compressed[2] = 0;
   compressed[3] = 0;
   compressed[4] = 0;
   compressed[5] = 0;
   compressed[6] = 0;
   compressed[7] = 0;

   /* store the all of the alpha values */
   next_bit = 8*2;

   if ((factor = a0 - a1) <= 0) factor = 1;
   scale_me = 7.9999f / factor;

   for (i = 3; i < 16*4; i += 4) {
      /* convert this alpha value to a 3 bit number */
      value = (int)((uncompressed[i] - a1) * scale_me);
      svalue = swizzle8[ value&7 ];

      /* OK, store this value, start with the 1st byte */
      compressed[next_bit >> 3] |= svalue << (next_bit & 7);
      if ((next_bit & 7) > 5) {
         /* spans 2 bytes, fill in the start of the 2nd byte */
         compressed[1 + (next_bit >> 3)] |= svalue >> (8 - (next_bit & 7) );
      }
      next_bit += 3;
   }
}

/* vim: set ts=8 sw=3 tw=0 :*/
