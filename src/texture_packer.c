#include "internal.h"
#include <stdlib.h> /* for malloc */
#include <assert.h> /* for assert */

#ifndef NULLDO
#  define NULLDO(f,x) { f(x); x = NULL; }
#endif

typedef struct tpRect {
   int x1, x2, y1, y2;
} tpRect;

typedef struct tpTexture {
   int width, height, x, y;
   int longest_edge, area;
   int flipped, placed;
} tpTexture;

typedef struct tpNode {
   struct tpNode *next;
   int x, y, width, height;
} tpNode;

static void rect_set(tpRect *rect, int x1, int y1, int x2, int y2)
{
   assert(rect);
   rect->x1 = x1; rect->y1 = y1;
   rect->x2 = x2; rect->y2 = y2;
}

static void texture_set(tpTexture *t, int width, int height)
{
   assert(t);
   t->width   = width;  t->height  = height;
   t->x       = 0;      t->y       = 0;
   t->flipped = 0;      t->placed  = 0;
   t->area    = width * height;
   t->longest_edge = width >= height ? width:height;
}

static void texture_place(tpTexture *t, int x, int y, int flipped)
{
   assert(t);
   t->x        = x;        t->y        = y;
   t->flipped  = flipped;  t->placed   = 1;
}

static void node_set(tpNode *n, int x, int y, int width, int height)
{
   assert(n);
   n->x       = x;      n->y       = y;
   n->width   = width;  n->height  = height;
   n->next    = NULL;
}

static int node_fits(tpNode *n, int width, int height, int *edge_count)
{
   int ec = 0;
   assert(n);

   if (width == n->width  || height == n->height ||
       width == n->height || height == n->width) {
      if (width == n->width)        ec += (height==n->height) ? 2:1;
      else if (width == n->height)  ec += (height==n->width)  ? 2:1;
      else if (height == n->width)  ec++;
      else if (height == n->height) ec++;
   }

   *edge_count = ec;
   return ((width  <= n->width && height <= n->height) ||
           (height <= n->width && width  <= n->height));
}

static void node_get_rect(tpNode *n, tpRect *r)
{
   assert(n);
   rect_set(r, n->x, n->y, n->x + n->width - 1, n->y + n->height - 1);
}

#ifndef NDEBUG
static int rect_intersects(tpRect *r1, tpRect *r2)
{
   assert(r1 && r2);
   return !(r1->x2 < r2->x1 ||
            r1->x1 > r2->x2 ||
            r1->y2 < r2->y1 ||
            r1->y1 > r2->y2);
}

static void node_validate(tpNode *n1, tpNode *n2)
{
   tpRect r1, r2;
   assert(n1 && n2);
   node_get_rect(n1, &r1);
   node_get_rect(n2, &r2);
   assert(!rect_intersects(&r1, &r2));
}
#endif

static int node_merge(tpNode *n1, tpNode *n2)
{
   int ret;
   tpRect r1, r2;

   node_get_rect(n1, &r1);
   node_get_rect(n2, &r2);

   r1.x2++; r1.y2++;
   r2.x2++; r2.y2++;

   /* if we share the top edge then.. */
   if (r1.x1 == r2.x1 && r1.x2 == r2.x2 && r1.y1 == r2.y2) {
      n1->y        = n2->y;
      n1->height  += n2->height;
      ret          = 1;
   }
   /* if we share the bottom edge  */
   else if (r1.x1 == r2.x1 && r1.x2 == r2.x2 && r1.y2 == r2.y1) {
      n1->height  += n2->height;
      ret          = 1;
   }
   /* if we share the left edge */
   else if (r1.y1 == r2.y1 && r1.y2 == r2.y1 && r1.x1 == r2.x2) {
      n1->x        = n2->x;
      n1->width   += n2->width;
      ret          = 1;
   }
   /* if we share the left edge */
   else if (r1.y1 == r2.y1 && r1.y2 == r2.y1 && r1.x2 == r2.x1) {
      n1->width   += n2->width;
      ret          = 1;
   } else
      ret = 0;

   return ret;
}

static void glhckTexturePackerReset(_glhckTexturePacker *tp)
{
   tpNode *next, *kill;
   tp->texture_count = 0;
   tp->longest_edge  = 0;
   tp->total_area    = 0;
   tp->texture_index = 0;

   if(tp->textures)
      NULLDO(free, tp->textures);

   if (tp->free_list) {
      next = tp->free_list;
      while (next) {
         kill = next;
         next = next->next;
         NULLDO(free, kill);
      }
   }
   tp->free_list = NULL;
}

static void glhckTexturePackerNodeNew(_glhckTexturePacker *tp, int x, int y, int width, int height)
{
   tpNode *node;
   node = malloc(sizeof(tpNode));
   node_set(node, x, y, width, height);
   node->next = tp->free_list;
   tp->free_list = node;
}

static int next_pow2(int v)
{
   int p;
   for (p = 1; p < v; p = p * 2);
   return p;
}

#ifndef NDEBUG
static void validate(_glhckTexturePacker *tp)
{
   tpNode *c, *f = tp->free_list;

   while (f) {
      c = tp->free_list;
      while (c) {
         if (f != c)
            node_validate(f, c);
         c = c->next;
      }
      f = f->next;
   }
}
#else
#  define validate(ignore) ;
#endif

static int merge_nodes(_glhckTexturePacker *tp)
{
   tpNode *prev, *c, *f = tp->free_list;

   while (f) {
      prev  = NULL;
      c     = tp->free_list;
      while (c) {
         if (f != c) {
            if (node_merge(f, c)) {
               assert(prev);
               prev->next = c->next;
               free(c);
               return 1;
            }
         }
         prev = c;
         c = c->next;
      }
      f = f->next;
   }
   return 0;
}

_glhckTexturePacker* _glhckTexturePackerNew(void)
{
   _glhckTexturePacker *tp;

   tp = calloc(1, sizeof(_glhckTexturePacker));
   if(!tp) return NULL;

   /* null */
   tp->free_list = NULL;
   tp->textures  = NULL;

   return tp;
}

void _glhckTexturePackerFree(_glhckTexturePacker *tp)
{
   glhckTexturePackerReset(tp);
   free(tp);
}

void _glhckTexturePackerCount(_glhckTexturePacker *tp, short texture_count)
{
   glhckTexturePackerReset(tp);
   tp->texture_count = texture_count;
   tp->textures      = calloc(texture_count, sizeof(tpTexture));
}

short _glhckTexturePackerAdd(_glhckTexturePacker *tp, int width, int height)
{
   assert(tp->texture_index <= tp->texture_count);
   if (tp->texture_index < tp->texture_count) {
      texture_set(&tp->textures[tp->texture_index], width, height);
      tp->texture_index++;
      if (width  > tp->longest_edge) tp->longest_edge = width;
      if (height > tp->longest_edge) tp->longest_edge = height;
      tp->total_area += width * height;
   }
   return tp->texture_index-1;
}

int _glhckTexturePackerGetLocation(const _glhckTexturePacker *tp, int index, int *in_x, int *in_y, int *in_width, int *in_height)
{
   int ret = 0, x = 0, y = 0, width = 0, height = 0;
   tpTexture *t;
   assert(index < tp->texture_count);

   if (index < tp->texture_count) {
      t = &tp->textures[index];
      x = t->x;
      y = t->y;

      if (t->flipped) {
         width  = t->height;
         height = t->width;
      } else {
         width  = t->width;
         height = t->height;
      }
      ret = t->flipped;
   }

   *in_x = x;
   *in_y = y;
   *in_width  = width;
   *in_height = height;
   return ret;
}

void checkDimensions(_glhckTexturePacker *tp, int *width, int *height)
{
   tpTexture *t;
   unsigned short i;
   for (i = 0; i != tp->texture_count; ++i) {
      t = &tp->textures[i];
      if (t->width  > *width)  *width  = t->width;
      if (t->height > *height) *height = t->height;
   }
}

int _glhckTexturePackerPack(_glhckTexturePacker *tp, int *in_width, int *in_height, int force_power_of_two, int one_pixel_border)
{
   tpTexture *t;
   int width, height;
   unsigned short count, i, i2;
   int least_y, least_x;
   int edge_count, ec;
   int index, longest_edge, most_area;
   int flipped, wid, hit, y;
   tpNode *previous_best_fit, *best_fit, *previous, *search;
   assert(tp);

   if (one_pixel_border) {
      for (i = 0; i != tp->texture_count; ++i) {
         t = &tp->textures[i];
         t->width  += 2;
         t->height += 2;
      }
      tp->longest_edge += 2;
   }

   if (force_power_of_two)
      tp->longest_edge = next_pow2(tp->longest_edge);

   width = tp->longest_edge;
   count = round((float)tp->total_area/(tp->longest_edge * tp->longest_edge));
   height = (count + 2) * tp->longest_edge;

   if (force_power_of_two)
      height = next_pow2(height);

   _glhckPrintf("\4Total area: \3%d", tp->total_area);
   _glhckPrintf("\4Count: \3%d", count);
   _glhckPrintf("\1Initial size: \3%d\5x\3%d", width, height);

   /* more sane packing area */
   if (width > height) {
      if (height != width/2) {
         height  = width/2;
         width  /= 2;
         checkDimensions(tp, &width, &height);
      }
   } else if (height > width) {
      if (width != height/2) {
         width   = height/2;
         height /= 2;
         checkDimensions(tp, &width, &height);
      }
   }

   _glhckPrintf("\2Good size: \3%d\5x\3%d", width, height);

   tp->debug_count = 0;
   glhckTexturePackerNodeNew(tp, 0, 0, width, height);

   for (i = 0; i != tp->texture_count; ++i) {
      index        = 0;
      longest_edge = 0;
      most_area    = 0;

      for(i2 = 0; i2 != tp->texture_count; ++i2) {
         t = &tp->textures[i2];
         if (!t->placed) {
            if (t->longest_edge > longest_edge) {
               most_area    = t->area;
               longest_edge = t->longest_edge;
               index        = i2;
            } else if (t->longest_edge == longest_edge) {
               if (t->area > most_area) {
                  most_area = t->area;
                  index     = i2;
               }
            }
         }
      }

      least_y = 0x7FFFFFFF;
      least_x = 0x7FFFFFFF;
      t = &tp->textures[index];
      previous_best_fit = NULL;
      best_fit          = NULL;
      previous          = NULL;
      search            = tp->free_list;
      edge_count        = 0;

      while (search) {
         ec = 0;
         if (node_fits(search, t->width, t->height, &ec)) {
            if (ec == 2) {
               previous_best_fit = previous;
               best_fit          = search;
               edge_count        = ec;
               break;
            }
            else if (search->y < least_y) {
               least_y           = search->y;
               least_x           = search->x;
               previous_best_fit = previous;
               best_fit          = search;
               edge_count        = ec;
            }
            else if (search->y == least_y && search->x < least_x) {
               least_x           = search->x;
               previous_best_fit = previous;
               best_fit          = search;
               edge_count        = ec;
            }
         }
         previous = search;
         search   = search->next;
      }
      assert(best_fit); /* should always find a fit */

      validate(tp);
      switch(edge_count) {
         case 0:
            if (t->longest_edge <= best_fit->width) {
               flipped  = 0;
               wid      = t->width;
               hit      = t->height;

               if (hit > wid) {
                  wid      = t->height;
                  hit      = t->width;
                  flipped  = 1;
               }

               texture_place(t, best_fit->x, best_fit->y, flipped);
               glhckTexturePackerNodeNew(tp, best_fit->x, best_fit->y + hit, best_fit->width, best_fit->height - hit);

               best_fit->x        += wid;
               best_fit->width    -= wid;
               best_fit->height    = hit;
               validate(tp);
            } else {
               assert(t->longest_edge <= best_fit->height);
               flipped  = 0;
               wid      = t->width;
               hit      = t->height;

               if (hit < wid) {
                  wid      = t->height;
                  hit      = t->width;
                  flipped  = 1;
               }

               texture_place(t, best_fit->x, best_fit->y, flipped);
               glhckTexturePackerNodeNew(tp, best_fit->x, best_fit->y + hit, best_fit->width, best_fit->height - hit);

               best_fit->x     += wid;
               best_fit->width -= wid;
               best_fit->height = hit;
               validate(tp);
            }
            break;
         case 1:
            if (t->width == best_fit->width) {
               texture_place(t, best_fit->x, best_fit->y, 0);
               best_fit->y      += t->height;
               best_fit->height -= t->height;
               validate(tp);
            }
            else if (t->height == best_fit->height) {
               texture_place(t, best_fit->x, best_fit->y, 0);
               best_fit->x     += t->width;
               best_fit->width -= t->width;
               validate(tp);
            }
            else if (t->width == best_fit->height) {
               texture_place(t, best_fit->x, best_fit->y, 1);
               best_fit->x     += t->height;
               best_fit->width -= t->height;
               validate(tp);
            }
            else if (t->height == best_fit->width) {
               texture_place(t, best_fit->x, best_fit->y, 1);
               best_fit->y      += t->width;
               best_fit->height -= t->width;
               validate(tp);
            }
            break;
         case 2:
            flipped = t->width != best_fit->width || t->height != best_fit->height;
            texture_place(t, best_fit->x, best_fit->y, flipped);
            if (previous_best_fit)  previous_best_fit->next = best_fit->next;
            else                    tp->free_list = best_fit->next;
            NULLDO(free, best_fit);
            validate(tp);
            break;
      }
      while (merge_nodes(tp)); /* merge as much as we can */
   }

   height = 0;
   for(i = 0; i < tp->texture_count; ++i) {
      t = &tp->textures[i];
      if (one_pixel_border) {
         t->width    -= 2;
         t->height   -= 2;
         t->x++;
         t->y++;
      }

      y = t->flipped?t->y+t->width:t->y+t->height;
      height = y>height?y:height;
   }

   if (force_power_of_two)
      height = next_pow2(height);

   *in_width   = width;
   *in_height  = height;

   return (width * height) - tp->total_area;
}

/* vim: set ts=8 sw=3 tw=0 :*/
