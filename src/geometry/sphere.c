#include "../internal.h"

/* tracing channel for this file */
#define GLHCK_CHANNEL GLHCK_CHANNEL_GEOMETRY

/* \brief create new sphere object */
GLHCKAPI glhckObject* glhckSphereNew(kmScalar size)
{
   return glhckEllipsoidNew(size, size, size);
}

/* \brief create new sphere object (specify precision) */
GLHCKAPI glhckObject* glhckSphereNewEx(kmScalar size, unsigned char itype, unsigned char vtype)
{
   return glhckEllipsoidNewEx(size, size, size, itype, vtype);
}

/* \brief create new ellipsoid object */
GLHCKAPI glhckObject* glhckEllipsoidNew(kmScalar rx, kmScalar ry, kmScalar rz)
{
   unsigned char itype, vtype;
   glhckGetGlobalPrecision(&itype, &vtype);

   if (vtype == GLHCK_VTX_AUTO)
      vtype = GLHCK_VTX_V3F;

   if (itype == GLHCK_IDX_AUTO)
      itype = GLHCK_IDX_UINT;

   return glhckEllipsoidNewEx(rx, ry, rz, itype, vtype);
}

/* \brief create new ellipsoid object (specify precision) */
GLHCKAPI glhckObject* glhckEllipsoidNewEx(kmScalar rx, kmScalar ry, kmScalar rz, unsigned char itype, unsigned char vtype)
{
   const int bandPower = 4;
   const int bandPoints = bandPower * bandPower;
   const int bandMask = bandPoints - 2;
   const unsigned int sectionsInBand = (bandPoints/2) - 1;
   const unsigned int totalPoints = sectionsInBand * bandPoints;
   const kmScalar sectionArc = 6.28/sectionsInBand;
   glhckObject *object = NULL;
   CALL(0, "%f, %f, %f, %u, %u", rx, ry, rz, itype, vtype);

   /* generate ellipsoid tristrip */
   glhckImportVertexData vertices[totalPoints];
   memset(vertices, 0, sizeof(vertices));
   for (unsigned int i = 0; i < totalPoints; ++i) {
      kmScalar x = (kmScalar)(i&1)+(i>>bandPower);
      kmScalar y = (kmScalar)((i&bandMask)>>1)+((i>>bandPower)*(sectionsInBand));
      x *= (kmScalar)sectionArc/2.0f;
      y *= (kmScalar)sectionArc*-1;

      vertices[i].vertex.x = -rx * sin(x) * sin(y);
      vertices[i].vertex.y = -ry * cos(x);
      vertices[i].vertex.z = -rz * sin(x) * cos(y);
      vertices[i].normal.x = sin(x) * sin(y);
      vertices[i].normal.y = cos(x);
      vertices[i].normal.z = sin(x) * cos(y);
      vertices[i].coord.x = sin(x) * sin(y);
      vertices[i].coord.y = cos(x);
   }

   /* create new object */
   if (!(object = glhckObjectNew()))
      goto fail;

   /* insert vertices to object's geometry */
   if (glhckObjectInsertVertices(object, vtype, &vertices[0], totalPoints) != RETURN_OK)
      goto fail;

   RET(0, "%p", object);
   return object;

fail:
   IFDO(glhckObjectFree, object);
   RET(0, "%p", NULL);
   return NULL;
}

/* vim: set ts=8 sw=3 tw=0 :*/
