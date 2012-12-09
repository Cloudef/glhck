#include "internal.h"
#include <limits.h> /* for type limits */

#define GLHCK_CHANNEL GLHCK_CHANNEL_VDATA

/* This file contains all the methods that access
 * object's internal vertexdata, since the GLHCK object's
 * internal vertexdata structure is quite complex */

/* conversion constants for vertexdata conversion */
#define GLHCK_BYTE_CMAGIC  CHAR_MAX
#define GLHCK_BYTE_VMAGIC  UCHAR_MAX - CHAR_MAX
#define GLHCK_BYTE_VBIAS   CHAR_MAX / (UCHAR_MAX - 1.0f)
#define GLHCK_BYTE_VSCALE  UCHAR_MAX - 1.0f

#define GLHCK_SHORT_CMAGIC  SHRT_MAX
#define GLHCK_SHORT_VMAGIC  USHRT_MAX - SHRT_MAX
#define GLHCK_SHORT_VBIAS   SHRT_MAX / (USHRT_MAX - 1.0f)
#define GLHCK_SHORT_VSCALE  USHRT_MAX - 1.0f

/* 1. 2D vector conversion function name
 * 2. 3D vector conversion function name
 * 3. 2D internal vector cast
 * 4. 3D internal vector cast
 * 5. Base cast of vector's units
 * 6. Vector magic constant */
#define glhckMagicDefine(namev2, namev3, castv2, castv3, bcast, magic)                    \
inline static void namev2(                                                                \
   castv2 *vb, const glhckVector3f *vh,                                                   \
   glhckVector3f *max, glhckVector3f *min)                                                \
{                                                                                         \
   vb->x = (bcast)floorf(((vh->x - min->x) / (max->x - min->x)) * magic + 0.5f);          \
   vb->y = (bcast)floorf(((vh->y - min->y) / (max->y - min->y)) * magic + 0.5f);          \
}                                                                                         \
inline static void namev3(                                                                \
   castv3 *vb, const glhckVector3f *vh,                                                   \
   glhckVector3f *max, glhckVector3f *min)                                                \
{                                                                                         \
   namev2((castv2*)vb, vh, max, min);                                                     \
   vb->z = (bcast)floorf(((vh->z - min->z) / (max->z - min->z)) * magic + 0.5f);          \
}

/* convert vector to internal byte format */
glhckMagicDefine(glhckMagic2b, glhckMagic3b,
      glhckVector2b, glhckVector3b, char, GLHCK_BYTE_VMAGIC);

/* convert vector to internal short format */
glhckMagicDefine(glhckMagic2s, glhckMagic3s,
      glhckVector2s, glhckVector3s, short, GLHCK_SHORT_VMAGIC);

/* \brief get min/max from import data */
static void _glhckMaxMinImportData(const glhckImportVertexData *import, size_t memb,
      glhckVector3f *vmin, glhckVector3f *vmax, glhckVector3f *nmin, glhckVector3f *nmax)
{
   size_t i;
   glhckSetV3(vmax, &import[0].vertex);
   glhckSetV3(vmin, &import[0].vertex);
   glhckSetV3(nmax, &import[0].normal);
   glhckSetV3(nmin, &import[0].normal);

   /* find max && min first */
   for (i = 1; i != memb; ++i) {
      glhckMaxV3(vmax, &import[i].vertex);
      glhckMinV3(vmin, &import[i].vertex);
      glhckMaxV3(nmax, &import[i].normal);
      glhckMinV3(nmin, &import[i].normal);
   }
}

/* \brief get /min/max from index data */
static void _glhckMaxMinIndexData(const glhckImportIndexData *import, size_t memb,
      unsigned int *mini, unsigned int *maxi)
{
   size_t i;

   *mini = *maxi = import[0];
   for (i = 1; i != memb; ++i) {
      if (*mini > import[i]) *mini = import[i];
      if (*maxi < import[i]) *maxi = import[i];
   }
}

/* \brief convert vertex data to internal format */
static void _glhckConvertVertexData(
      glhckGeometryVertexType type, glhckVertexData internal,
      const glhckImportVertexData *import, size_t memb,
      glhckVector3f *bias, glhckVector3f *scale)
{
   size_t i;
   char no_convert = 0;
   float biasMagic = 0.0f, scaleMagic = 0.0f;
   glhckVector3f vmin, vmax,
                 nmin, nmax;
   CALL(0, "%d, %p, %zu, %p, %p", type, import, memb, bias, scale);

   /* default bias scale */
   bias->x  = bias->y  = bias->z  = 0.0f;
   scale->x = scale->y = scale->z = 1.0f;

   /* get min and max from import data */
   _glhckMaxMinImportData(import, memb,
         &vmin, &vmax, &nmin, &nmax);

   /* do we need conversion? */
   if ((vmax.x + vmin.x == 1 &&
        vmax.y + vmin.y == 1 &&
        vmax.z + vmin.z == 1) ||
       (vmax.x + vmin.x == 0 &&
        vmax.y + vmin.y == 0 &&
        vmax.z + vmin.z == 0))
      no_convert = 1;

   /* lie about bounds by 1 point so,
    * we don't get artifacts */
   vmax.x++; vmax.y++; vmax.z++;
   vmin.x--; vmin.y--; vmin.z--;

   if (type == GLHCK_VERTEX_V3F) {
      memcpy(internal.v3f, import, memb * sizeof(glhckVertexData3f));

      /* center geometry */
      for (i = 0; !no_convert && i != memb; ++i) {
         internal.v3f[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
         internal.v3f[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
         internal.v3f[i].vertex.z -= 0.5f * (vmax.z-vmin.z)+vmin.z;
      }
   } else {
      /* handle vertex data conversion */
      for (i = 0; i != memb; ++i) {
         switch (type) {
            case GLHCK_VERTEX_V3B:
               memcpy(&internal.v3b[i].color,  &import[i].color,  sizeof(glhckColorb));

               if (no_convert) {
                  glhckSetV3(&internal.v3b[i].vertex, &import[i].vertex);
                  glhckSetV3(&internal.v3b[i].normal, &import[i].normal);
               } else {
                  glhckMagic3b(&internal.v3b[i].vertex, &import[i].vertex, &vmax, &vmin);
                  glhckMagic3b(&internal.v3b[i].normal, &import[i].normal, &vmax, &vmin);
               }

               internal.v3b[i].coord.x = import[i].coord.x * GLHCK_BYTE_CMAGIC;
               internal.v3b[i].coord.y = import[i].coord.y * GLHCK_BYTE_CMAGIC;

               biasMagic  = GLHCK_BYTE_VBIAS;
               scaleMagic = GLHCK_BYTE_VSCALE;
               break;

            case GLHCK_VERTEX_V2B:
               memcpy(&internal.v2b[i].color,  &import[i].color,  sizeof(glhckColorb));

               if (no_convert) {
                  glhckSetV2(&internal.v2b[i].vertex, &import[i].vertex);
               } else {
                  glhckMagic2b(&internal.v2b[i].vertex, &import[i].vertex, &vmax, &vmin);
               }

               internal.v2b[i].coord.x = import[i].coord.x * GLHCK_BYTE_CMAGIC;
               internal.v2b[i].coord.y = import[i].coord.y * GLHCK_BYTE_CMAGIC;

               biasMagic  = GLHCK_BYTE_VBIAS;
               scaleMagic = GLHCK_BYTE_VSCALE;
               break;

            case GLHCK_VERTEX_V3S:
               memcpy(&internal.v3s[i].color,  &import[i].color,  sizeof(glhckColorb));

               if (no_convert) {
                  glhckSetV3(&internal.v3s[i].vertex, &import[i].vertex);
                  glhckSetV3(&internal.v3s[i].normal, &import[i].normal);
               } else {
                  glhckMagic3s(&internal.v3s[i].vertex, &import[i].vertex, &vmax, &vmin);
                  glhckMagic3s(&internal.v3s[i].normal, &import[i].normal, &vmax, &vmin);
               }

               internal.v3s[i].coord.x = import[i].coord.x * GLHCK_SHORT_CMAGIC;
               internal.v3s[i].coord.y = import[i].coord.y * GLHCK_SHORT_CMAGIC;

               biasMagic  = GLHCK_SHORT_VBIAS;
               scaleMagic = GLHCK_SHORT_VSCALE;
               break;

            case GLHCK_VERTEX_V2S:
               memcpy(&internal.v2s[i].color,  &import[i].color,  sizeof(glhckColorb));

               if (no_convert) {
                  glhckSetV2(&internal.v2s[i].vertex, &import[i].vertex);
               } else {
                  glhckMagic2s(&internal.v2s[i].vertex, &import[i].vertex, &vmax, &vmin);
               }

               internal.v2s[i].coord.x = import[i].coord.x * GLHCK_SHORT_CMAGIC;
               internal.v2s[i].coord.y = import[i].coord.y * GLHCK_SHORT_CMAGIC;

               biasMagic  = GLHCK_SHORT_VBIAS;
               scaleMagic = GLHCK_SHORT_VSCALE;
               break;

            case GLHCK_VERTEX_V2F:
               memcpy(&internal.v2f[i].color,  &import[i].color,  sizeof(glhckColorb));
               memcpy(&internal.v2f[i].vertex, &import[i].vertex, sizeof(glhckVector2f));
               memcpy(&internal.v2f[i].coord,  &import[i].coord,  sizeof(glhckVector2f));

               if (no_convert) {
                  internal.v2f[i].vertex.x -= 0.5f * (vmax.x-vmin.x)+vmin.x;
                  internal.v2f[i].vertex.y -= 0.5f * (vmax.y-vmin.y)+vmin.y;
               }
               break;

            default:
               DEBUG(GLHCK_DBG_ERROR, "Something terrible happenned.");
               return;
         }
      }
   }

   DEBUG(GLHCK_DBG_CRAP, "CONV: "VEC3S" : "VEC3S" (%d:%f:%f)",
         VEC3(&vmax),  VEC3(&vmin), no_convert, biasMagic, scaleMagic);

   /* no need to process further */
   if (no_convert) return;

   if (type == GLHCK_VERTEX_V3F || type == GLHCK_VERTEX_V2F) {
      /* fix bias after centering */
      bias->x = 0.5f * (vmax.x - vmin.x) + vmin.x;
      bias->y = 0.5f * (vmax.y - vmin.y) + vmin.y;
      bias->z = 0.5f * (vmax.z - vmin.z) + vmin.z;
   } else {
      /* fix geometry bias && scale after conversion */
      bias->x  = biasMagic*(vmax.x-vmin.x)+vmin.x;
      bias->y  = biasMagic*(vmax.y-vmin.y)+vmin.y;
      bias->z  = biasMagic*(vmax.z-vmin.z)+vmin.z;
      scale->x = (vmax.x-vmin.x)/scaleMagic;
      scale->y = (vmax.y-vmin.y)/scaleMagic;
      scale->z = (vmax.z-vmin.z)/scaleMagic;
   }

   DEBUG(GLHCK_DBG_CRAP, "Converted bias "VEC3S, VEC3(bias));
   DEBUG(GLHCK_DBG_CRAP, "Converted scale "VEC3S, VEC3(scale));
}

/* \brief convert index data to internal format */
static void _glhckConvertIndexData(
      glhckGeometryIndexType type, glhckIndexData internal,
      const glhckImportIndexData *import, size_t memb)
{
   size_t i;
   CALL(0, "%d, %p, %zu", type, import, memb);
   if (type == GLHCK_INDEX_INTEGER) {
      memcpy(internal.ivi, import, memb *sizeof(glhckImportIndexData));
   } else {
      for (i = 0; i != memb; ++i) {
         switch (type) {
            case GLHCK_INDEX_BYTE:
               internal.ivb[i] = import[i];
               break;

            case GLHCK_INDEX_SHORT:
               internal.ivs[i] = import[i];
               break;

            default:
               DEBUG(GLHCK_DBG_ERROR, "Something terrible happenned.");
               break;
         }
      }
   }
}

/* \brief free geometry's vertex data */
static void _glhckGeometryFreeVertices(glhckGeometry *geometry)
{
   /* free vertices depending on type */
   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         IFDO(_glhckFree, geometry->vertices.v3b);
         break;

      case GLHCK_VERTEX_V2B:
         IFDO(_glhckFree, geometry->vertices.v2b);
         break;

      case GLHCK_VERTEX_V3S:
         IFDO(_glhckFree, geometry->vertices.v3s);
         break;

      case GLHCK_VERTEX_V2S:
         IFDO(_glhckFree, geometry->vertices.v2s);
         break;

      case GLHCK_VERTEX_V3F:
         IFDO(_glhckFree, geometry->vertices.v3f);
         break;

      case GLHCK_VERTEX_V2F:
         IFDO(_glhckFree, geometry->vertices.v2f);
         break;

      default:
         break;
   }

   /* set vertex type to none */
   geometry->vertexType   = GLHCK_VERTEX_NONE;
   geometry->vertexCount  = 0;
   geometry->textureRange = 1;
}

/* \brief assign vertices to object internally */
static void _glhckGeometrySetVertices(glhckGeometry *geometry,
      glhckGeometryVertexType type, void *data, size_t memb)
{
   unsigned short textureRange = 1;

   /* free old vertices */
   _glhckGeometryFreeVertices(geometry);

   /* assign vertices depending on type */
   switch (type) {
      case GLHCK_VERTEX_V3B:
         geometry->vertices.v3b = (glhckVertexData3b*)data;
         textureRange = GLHCK_BYTE_CMAGIC;
         break;

      case GLHCK_VERTEX_V2B:
         geometry->vertices.v2b = (glhckVertexData2b*)data;
         textureRange = GLHCK_BYTE_CMAGIC;
         break;

      case GLHCK_VERTEX_V3S:
         geometry->vertices.v3s = (glhckVertexData3s*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V2S:
         geometry->vertices.v2s = (glhckVertexData2s*)data;
         textureRange = GLHCK_SHORT_CMAGIC;
         break;

      case GLHCK_VERTEX_V3F:
         geometry->vertices.v3f = (glhckVertexData3f*)data;
         break;

      case GLHCK_VERTEX_V2F:
         geometry->vertices.v2f = (glhckVertexData2f*)data;
         break;

      default:
         return;
         break;
   }

   /* set vertex type */
   geometry->vertexType   = type;
   geometry->vertexCount  = memb;
   geometry->textureRange = textureRange;
}

/* \brief free indices from object */
static void _glhckGeometryFreeIndices(glhckGeometry *geometry)
{
   /* free indices depending on type */
   switch (geometry->indexType) {
      case GLHCK_INDEX_BYTE:
         IFDO(_glhckFree, geometry->indices.ivb);
         break;

      case GLHCK_INDEX_SHORT:
         IFDO(_glhckFree, geometry->indices.ivs);
         break;

      case GLHCK_INDEX_INTEGER:
         IFDO(_glhckFree, geometry->indices.ivi);
         break;

      default:
         break;
   }

   /* set index type to none */
   geometry->indexType  = GLHCK_INDEX_NONE;
   geometry->indexCount = 0;
}

/* \brief assign indices to object internally */
static void _glhckGeometrySetIndices(glhckGeometry *geometry,
      glhckGeometryIndexType type, void *data, size_t memb)
{
   /* free old indices */
   _glhckGeometryFreeIndices(geometry);

   /* assign indices depending on type */
   switch (type) {
      case GLHCK_INDEX_BYTE:
         geometry->indices.ivb = (glhckIndexb*)data;
         break;

      case GLHCK_INDEX_SHORT:
         geometry->indices.ivs = (glhckIndexs*)data;
         break;

      case GLHCK_INDEX_INTEGER:
         geometry->indices.ivi = (glhckIndexi*)data;
         break;

      default:
         return;
         break;
   }

   /* set index type */
   geometry->indexType  = type;
   geometry->indexCount = memb;
}

/* \brief insert vertices into object */
int _glhckGeometryInsertVertices(
      glhckGeometry *geometry, size_t memb,
      glhckGeometryVertexType type,
      const glhckImportVertexData *vertices)
{
   void *data = NULL;
   glhckVertexData vd;
   CALL(0, "%p, %zu, %d, %p", geometry, memb, type, vertices);
   assert(geometry);

   /* default to V3F on NONE type */
   if (type == GLHCK_VERTEX_NONE) {
      type = GLHCK_VERTEX_V3F;
   }

   /* check vertex precision conflicts */
   if (memb > glhckIndexTypeMaxPrecision(geometry->indexType))
      goto bad_precision;

   /* allocate depending on type */
   switch (type) {
      case GLHCK_VERTEX_V3B:
         data = vd.v3b = _glhckMalloc(memb * sizeof(glhckVertexData3b));
         break;

      case GLHCK_VERTEX_V2B:
         data = vd.v2b = _glhckMalloc(memb * sizeof(glhckVertexData2b));
         break;

      case GLHCK_VERTEX_V3S:
         data = vd.v3s = _glhckMalloc(memb * sizeof(glhckVertexData3s));
         break;

      case GLHCK_VERTEX_V2S:
         data = vd.v2s = _glhckMalloc(memb * sizeof(glhckVertexData2s));
         break;

      case GLHCK_VERTEX_V3F:
         data = vd.v3f = _glhckMalloc(memb * sizeof(glhckVertexData3f));
         break;

      case GLHCK_VERTEX_V2F:
         data = vd.v2f = _glhckMalloc(memb * sizeof(glhckVertexData2f));
         break;

      case GLHCK_VERTEX_NONE:
         break;
   }
   if (!data) goto fail;

   /* convert and assign */
   _glhckConvertVertexData(type, vd, vertices, memb, &geometry->bias, &geometry->scale);
   _glhckGeometrySetVertices(geometry, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %s, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          glhckIndexTypeString(geometry->indexType),
                          geometry);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief insert index data into object */
int _glhckGeometryInsertIndices(
      glhckGeometry *geometry, size_t memb,
      glhckGeometryIndexType type,
      const glhckImportIndexData *indices)
{
   void *data = NULL;
   glhckIndexData id;
   unsigned int mini, maxi;
   CALL(0, "%p, %zu, %d, %p", geometry, memb, type, indices);
   assert(geometry);

   /* autodetect */
   if (type == GLHCK_INDEX_NONE) {
      _glhckMaxMinIndexData(indices, memb, &mini, &maxi);
      type = glhckIndexTypeForValue(maxi);
   }

   /* check vertex precision conflicts */
   if (geometry->vertexCount > glhckIndexTypeMaxPrecision(geometry->indexType))
      goto bad_precision;

   /* allocate depending on type */
   switch (type) {
      case GLHCK_INDEX_BYTE:
         data = id.ivb = _glhckMalloc(memb * sizeof(glhckIndexb));
         break;

      case GLHCK_INDEX_SHORT:
         data = id.ivs = _glhckMalloc(memb * sizeof(glhckIndexs));
         break;

      case GLHCK_INDEX_INTEGER:
         data = id.ivi = _glhckMalloc(memb * sizeof(glhckIndexi));
         break;

      case GLHCK_INDEX_NONE:
         break;
   }
   if (!data) goto fail;

   /* convert and assign */
   _glhckConvertIndexData(type, id, indices, memb);
   _glhckGeometrySetIndices(geometry, type, data, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

bad_precision:
   DEBUG(GLHCK_DBG_ERROR, "Internal indices precision is %s, however there are more vertices\n"
                          "in geometry(%p) than index can hold.",
                          glhckIndexTypeString(geometry->indexType),
                          geometry);
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief create new geometry object */
glhckGeometry* _glhckGeometryNew(void)
{
   glhckGeometry *geometry;

   if (!(geometry = _glhckCalloc(1, sizeof(glhckGeometry))))
      return NULL;

   /* default geometry type */
   geometry->type = GLHCK_TRIANGLE_STRIP;

   /* default scale */
   geometry->scale.x = geometry->scale.y = geometry->scale.z = 1.0f;

   return geometry;
}

glhckGeometry* _glhckGeometryCopy(glhckGeometry *src)
{
   glhckGeometry *geometry;
   if (!src) return NULL;

   if (!(geometry = _glhckCopy(src, sizeof(glhckGeometry))))
      return NULL;

   /* FIXME: copy indices and vertices */

   return geometry;
}

/* \brief release geometry object */
void _glhckGeometryFree(glhckGeometry *geometry)
{
   assert(geometry);
   IFDO(_glhckFree, geometry->transformedCoordinates);
   _glhckGeometryFreeVertices(geometry);
   _glhckGeometryFreeIndices(geometry);
   _glhckFree(geometry);
}

/***
 * public api
 ***/

/* \brief get element size for index type */
GLHCKAPI size_t glhckIndexTypeElementSize(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return sizeof(glhckIndexb);

      case GLHCK_INDEX_SHORT:
         return sizeof(glhckIndexs);

      case GLHCK_INDEX_INTEGER:
         return sizeof(glhckIndexi);

      default:
         break;
   }

   /* default */
   return 0;
}

/* \brief return the maxium index precision allowed for object */
GLHCKAPI glhckIndexi glhckIndexTypeMaxPrecision(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return UCHAR_MAX;

      case GLHCK_INDEX_SHORT:
         return USHRT_MAX;

      case GLHCK_INDEX_INTEGER:
         return UINT_MAX;

      default:
         break;
   }

   /* default */
   return UINT_MAX;
}

/* \brief get string for index precision type */
GLHCKAPI const char* glhckIndexTypeString(glhckGeometryIndexType type)
{
   switch (type) {
      case GLHCK_INDEX_BYTE:
         return "GLHCK_INDEX_BYTE";

      case GLHCK_INDEX_SHORT:
         return "GLHCK_INDEX_SHORT";

      case GLHCK_INDEX_INTEGER:
         return "GLHCK_INDEX_INTEGER";

      default:
         break;
   }

   /* default */
   return "GLHCK_INDEX_NONE";
}

/* \brief is value withing precision range? */
GLHCKAPI int glhckIndexTypeWithinRange(unsigned int value, glhckGeometryIndexType type)
{
   return (value <  glhckIndexTypeMaxPrecision(type));
}

/* \brief you should feed this function the value with highest precision */
GLHCKAPI glhckGeometryIndexType glhckIndexTypeForValue(unsigned int value)
{
   glhckGeometryIndexType itype = GLHCK_INDEX_INTEGER;
   if (glhckIndexTypeWithinRange(value, GLHCK_INDEX_BYTE))
      itype = GLHCK_INDEX_BYTE;
   else if (glhckIndexTypeWithinRange(value, GLHCK_INDEX_SHORT))
      itype = GLHCK_INDEX_SHORT;

   return itype;
}

/* \brief get element size for vertex type */
GLHCKAPI size_t glhckVertexTypeElementSize(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
         return sizeof(glhckVertexData3b);

      case GLHCK_VERTEX_V2B:
         return sizeof(glhckVertexData2b);

      case GLHCK_VERTEX_V3S:
         return sizeof(glhckVertexData3s);

      case GLHCK_VERTEX_V2S:
         return sizeof(glhckVertexData2s);

      case GLHCK_VERTEX_V3F:
         return sizeof(glhckVertexData3f);

      case GLHCK_VERTEX_V2F:
         return sizeof(glhckVertexData2f);

      default:
         break;
   }

   /* default */
   return 0;
}

/* \brief get maxium precision for vertex */
GLHCKAPI float glhckVertexTypeMaxPrecision(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
      case GLHCK_VERTEX_V2B:
         return CHAR_MAX;

      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V2S:
         return SHRT_MAX;

      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
         break;

      default:
         break;
   }

   /* default */
   return 0;
}

/* \brief get string for vertex precision type */
GLHCKAPI const char* glhckVertexTypeString(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
         return "GLHCK_VERTEX_V3B";

      case GLHCK_VERTEX_V2B:
         return "GLHCK_VERTEX_V2B";

      case GLHCK_VERTEX_V3S:
         return "GLHCK_VERTEX_V3S";

      case GLHCK_VERTEX_V2S:
         return "GLHCK_VERTEX_V2S";

      case GLHCK_VERTEX_V3F:
         return "GLHCK_VERTEX_V3F";

      case GLHCK_VERTEX_V2F:
         return "GLHCK_VERTEX_V2F";

      default:
         break;
   }

   /* default */
   return "GLHCK_VERTEX_NONE";
}

/* \brief get v2 counterpart of the vertexdata */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeGetV2Counterpart(glhckGeometryVertexType type)
{
   switch (type) {
      case GLHCK_VERTEX_V3B:
      case GLHCK_VERTEX_V2B:
         return GLHCK_VERTEX_V2B;

      case GLHCK_VERTEX_V3S:
      case GLHCK_VERTEX_V2S:
         return GLHCK_VERTEX_V2S;

      case GLHCK_VERTEX_V3F:
      case GLHCK_VERTEX_V2F:
         return GLHCK_VERTEX_V2F;

      default:
         break;
   }

   /* default */
   return type;
}

/* \brief is value withing precision range? */
GLHCKAPI int glhckVertexTypeWithinRange(float value, glhckGeometryVertexType type)
{
   return (value <  glhckVertexTypeMaxPrecision(type) &&
           value > -glhckVertexTypeMaxPrecision(type));
}

/* \brief get optimal vertex type for size */
GLHCKAPI glhckGeometryVertexType glhckVertexTypeForSize(size_t width, size_t height)
{
   glhckGeometryVertexType vtype = GLHCK_VERTEX_V3F;
   if (glhckVertexTypeWithinRange(width, GLHCK_VERTEX_V3B) &&
         glhckVertexTypeWithinRange(height, GLHCK_VERTEX_V3B))
      vtype = GLHCK_VERTEX_V3B;
   else if (glhckVertexTypeWithinRange(width, GLHCK_VERTEX_V3S) &&
         glhckVertexTypeWithinRange(height, GLHCK_VERTEX_V3S))
      vtype = GLHCK_VERTEX_V3S;
   return vtype;
}

/* \brief does vertex type have normal? */
GLHCKAPI int glhckVertexTypeHasNormal(glhckGeometryVertexType type)
{
   return (type == GLHCK_VERTEX_V3B || type == GLHCK_VERTEX_V3S || type == GLHCK_VERTEX_V3F);
}

/* \brief does vertex type have color? */
GLHCKAPI int glhckVertexTypeHasColor(glhckGeometryVertexType type)
{
   return 1;
}

/* \brief retieve internal vertex data for index */
GLHCKAPI void glhckGeometryVertexDataForIndex(
      glhckGeometry *geometry, glhckIndexi ix,
      glhckVector3f *vertex, glhckVector3f *normal,
      glhckVector2f *coord, glhckColorb *color,
      unsigned int *vmagic)
{
   assert(ix < geometry->vertexCount);
   if (vertex) memset(vertex, 0, sizeof(glhckVector3f));
   if (normal) memset(normal, 0, sizeof(glhckVector3f));
   if (coord)  memset(coord,  0, sizeof(glhckVector2f));
   if (color)  memset(color,  0, sizeof(glhckColorb));
   if (vmagic) *vmagic = 1;

   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         if (vertex) { glhckSetV3(vertex, &geometry->vertices.v3b[ix].vertex); }
         if (normal) { glhckSetV3(normal, &geometry->vertices.v3b[ix].normal); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v3b[ix].coord); }
         if (vmagic) *vmagic = GLHCK_BYTE_VMAGIC;
         break;

      case GLHCK_VERTEX_V2B:
         if (vertex) { glhckSetV2(vertex, &geometry->vertices.v2b[ix].vertex); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v2b[ix].coord); }
         if (vmagic) *vmagic = GLHCK_BYTE_VMAGIC;
         break;

      case GLHCK_VERTEX_V3S:
         if (vertex) { glhckSetV3(vertex, &geometry->vertices.v3s[ix].vertex); }
         if (normal) { glhckSetV3(normal, &geometry->vertices.v3s[ix].normal); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v3s[ix].coord); }
         if (vmagic) *vmagic = GLHCK_SHORT_VMAGIC;
         break;

      case GLHCK_VERTEX_V2S:
         if (vertex) { glhckSetV2(vertex, &geometry->vertices.v2s[ix].vertex); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v2s[ix].coord); }
         if (vmagic) *vmagic = GLHCK_SHORT_VMAGIC;
         break;

      case GLHCK_VERTEX_V3F:
         if (vertex) { glhckSetV3(vertex, &geometry->vertices.v3f[ix].vertex); }
         if (normal) { glhckSetV3(normal, &geometry->vertices.v3f[ix].normal); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v3f[ix].coord); }
         break;

      case GLHCK_VERTEX_V2F:
         if (vertex) { glhckSetV2(vertex, &geometry->vertices.v2f[ix].vertex); }
         if (coord)  { glhckSetV2(coord, &geometry->vertices.v2f[ix].coord); }
         break;

      default:
         break;
   }

   if (color) { memcpy(color, &geometry->vertices.v2b[ix].color, sizeof(glhckColorb)); }
}

/* \brief set vertexdata for index */
GLHCKAPI void glhckGeometrySetVertexDataForIndex(
      glhckGeometry *geometry, glhckIndexi ix,
      glhckVector3f *vertex, glhckVector3f *normal,
      glhckVector2f *coord, glhckColorb *color)
{
   assert(ix < geometry->vertexCount);
   switch (geometry->vertexType) {
      case GLHCK_VERTEX_V3B:
         if (vertex) { glhckSetV3(&geometry->vertices.v3b[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&geometry->vertices.v3b[ix].normal, normal); }
         if (coord)  { glhckSetV2(&geometry->vertices.v3b[ix].coord, coord); }
         break;

      case GLHCK_VERTEX_V2B:
         if (vertex) { glhckSetV2(&geometry->vertices.v2b[ix].vertex, vertex); }
         if (coord)  { glhckSetV2(&geometry->vertices.v2b[ix].coord, coord); }
         break;

      case GLHCK_VERTEX_V3S:
         if (vertex) { glhckSetV3(&geometry->vertices.v3s[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&geometry->vertices.v3s[ix].normal, normal); }
         if (coord)  { glhckSetV2(&geometry->vertices.v3s[ix].coord, coord); }
         break;

      case GLHCK_VERTEX_V2S:
         if (vertex) { glhckSetV2(&geometry->vertices.v2s[ix].vertex, vertex); }
         if (coord)  { glhckSetV2(&geometry->vertices.v2s[ix].coord, coord); }
         break;

      case GLHCK_VERTEX_V3F:
         if (vertex) { glhckSetV3(&geometry->vertices.v3f[ix].vertex, vertex); }
         if (normal) { glhckSetV3(&geometry->vertices.v3f[ix].normal, normal); }
         if (coord)  { glhckSetV2(&geometry->vertices.v3f[ix].coord, coord); }
         break;

      case GLHCK_VERTEX_V2F:
         if (vertex) { glhckSetV2(&geometry->vertices.v2f[ix].vertex, vertex); }
         if (coord)  { glhckSetV2(&geometry->vertices.v2f[ix].coord, coord); }
         break;

      default:
         break;
   }

   if (color) { memcpy(&geometry->vertices.v2b[ix].color, color, sizeof(glhckColorb)); }
}

/* \brief transform coordinates with vec4 (off x, off y, width, height) and rotation */
GLHCKAPI void glhckGeometryTransformCoordinates(
      glhckGeometry *geometry, const glhckRect *transformed, short degrees)
{
   size_t i;
   kmVec2 out, center = { 0.5f, 0.5f };
   glhckVector2f coord;
   glhckCoordTransform *newCoords;
   CALL(2, "%p, %p, %d", geometry, transformed, degrees);
   assert(geometry);

   if (transformed->w == 0.f || transformed->h == 0.f)
      return;

   if (!(newCoords = _glhckMalloc(sizeof(glhckCoordTransform))))
      return;

   /* transform coordinates */
   for (i = 0; i != geometry->vertexCount; ++i) {
      glhckGeometryVertexDataForIndex(geometry, i,
            NULL, NULL, &coord, NULL, NULL);
      out.x = (float)coord.x/geometry->textureRange;
      out.y = (float)coord.y/geometry->textureRange;

      if (geometry->transformedCoordinates) {
         if (geometry->transformedCoordinates->degrees != 0)
            kmVec2RotateBy(&out, &out, -geometry->transformedCoordinates->degrees, &center);

         out.x -= geometry->transformedCoordinates->transform.x;
         out.x /= geometry->transformedCoordinates->transform.w;
         out.y -= geometry->transformedCoordinates->transform.y;
         out.y /= geometry->transformedCoordinates->transform.h;
      }

      if (degrees != 0) kmVec2RotateBy(&out, &out, degrees, &center);
      out.x *= transformed->w;
      out.x += transformed->x;
      out.y *= transformed->h;
      out.y += transformed->y;

      coord.x = out.x*geometry->textureRange;
      coord.y = out.y*geometry->textureRange;
      glhckGeometrySetVertexDataForIndex(geometry, i,
            NULL, NULL, &coord, NULL);
   }

   /* assign to geometry */
   newCoords->degrees   = degrees;
   newCoords->transform = *transformed;
   IFDO(_glhckFree, geometry->transformedCoordinates);
   geometry->transformedCoordinates = newCoords;
}

/* \brief calculate object's bounding box */
GLHCKAPI void glhckGeometryCalculateBB(glhckGeometry *geometry, kmAABB *bb)
{
   size_t i;
   glhckVector3f vertex;
   glhckVector3f min, max;
   CALL(2, "%p, %p", geometry, bb);
   assert(geometry && bb);

   glhckGeometryVertexDataForIndex(geometry, 0,
         &vertex, NULL, NULL, NULL, NULL);
   glhckSetV3(&max, &vertex);
   glhckSetV3(&min, &vertex);

   /* find min and max vertices */
   for(i = 1; i != geometry->vertexCount; ++i) {
      glhckGeometryVertexDataForIndex(geometry, i,
            &vertex, NULL, NULL, NULL, NULL);
      glhckMaxV3(&max, &vertex);
      glhckMinV3(&min, &vertex);
   }

   glhckSetV3(&bb->min, &min);
   glhckSetV3(&bb->max, &max);
}

/* \brief assign indices to object */
GLHCKAPI int glhckGeometrySetIndices(glhckGeometry *geometry,
      glhckGeometryIndexType type, void *data, size_t memb)
{
   void *idata;
   size_t size;
   CALL(0, "%p, %d, %p, %zu", geometry, type, data, memb);
   assert(geometry);

   size = memb * glhckIndexTypeElementSize(type);
   if (!(idata = _glhckMalloc(size)))
      goto fail;

   memcpy(idata, data, size);
   _glhckGeometrySetIndices(geometry, type, idata, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief assign vertices to object */
GLHCKAPI int glhckGeometrySetVertices(glhckGeometry *geometry,
      glhckGeometryVertexType type, void *data, size_t memb)
{
   void *vdata;
   size_t size;
   CALL(0, "%p, %d, %p, %zu", geometry, type, data, memb);
   assert(geometry);

   size = memb * glhckVertexTypeElementSize(type);
   if (!(vdata = _glhckMalloc(size)))
      goto fail;

   memcpy(vdata, data, size);
   _glhckGeometrySetVertices(geometry, type, vdata, memb);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
