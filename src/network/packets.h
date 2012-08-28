#ifndef __packets_h__
#define __packets_h__

typedef enum _glhckNetPacketType {
   GLHCK_NET_PACKET_OBJECT,
} _glhckNetPacketType;

typedef struct _glhckNetVector3d {
   unsigned int x, y, z;
} _glhckNetVector3d;

typedef struct _glhckNetVector2d {
   unsigned int x, y;
} _glhckNetVector2d;

typedef struct _glhckNetColor {
   unsigned char r, g, b, a;
} _glhckNetColor;

typedef struct _glhckNetView {
   _glhckNetVector3d translation, target, rotation, scaling;
} _glhckNetView;

typedef struct _glhckNetMaterial {
   _glhckNetColor color;
   unsigned int flags;
} _glhckNetMaterial;

typedef struct _glhckNetGeometry {
   size_t indicesCount, vertexCount;
   _glhckNetVector3d bias, scale;
   unsigned int type, flags;
} _glhckNetGeometry;

typedef struct _glhckNetVertexData {
   _glhckNetVector3d vertex, normal;
   _glhckNetVector2d coord;
   _glhckNetColor color;
} _glhckNetVertexData;

typedef struct _glhckNetObjectPacket {
   unsigned char type;
   struct _glhckNetGeometry geometry;
   struct _glhckNetView view;
   struct _glhckNetMaterial material;
} _glhckNetObjectPacket;

typedef struct _glhckNetPacket {
   unsigned char type;
} _glhckNetPacket;

#endif /* __packets_h__ */
