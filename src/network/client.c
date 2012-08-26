#include "../internal.h"
#include <assert.h>    /* for assert */

#define GLHCK_CHANNEL GLHCK_CHANNEL_NETWORK

/* define for this feature */
#if GLHCK_USE_NETWORK

#include "packets.h"
#include <enet/enet.h> /* for enet   */

/* \brief local client struct */
typedef struct __GLHCKclient {
   ENetHost *enet;
   ENetPeer *peer;
} __GLHCKclient;
static __GLHCKclient _GLHCKclient;
static char _glhckClientInitialized = 0;

/* \brief initialize enet internally */
static int _glhckEnetInit(const char *host, int port)
{
   ENetEvent event;
   ENetAddress address;
   CALL(0, "%s, %d", host, port);

   /* initialize enet */
   if (enet_initialize() != 0)
      goto enet_init_fail;

   /* set host parameters */
   address.host = ENET_HOST_ANY;
   address.port = port;

   /* set host address, if specified */
   if (host) enet_address_set_host(&address, host);
   else      enet_address_set_host(&address, "127.0.0.1");

   /* create client */
   _GLHCKclient.enet = enet_host_create(NULL,
         1     /* outgoing connections */,
         1     /* max channels */,
         0     /* download bandwidth */,
         0     /* upload bandwidth */);

   if (!_GLHCKclient.enet)
      goto enet_client_fail;

   /* initialize connection */
   _GLHCKclient.peer = enet_host_connect(_GLHCKclient.enet,
         &address, 1, 0);

   if (!_GLHCKclient.peer)
      goto enet_client_init_fail;

   if (enet_host_service(_GLHCKclient.enet, &event, 5000) <= 0 ||
         event.type != ENET_EVENT_TYPE_CONNECT)
      goto fail;

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

enet_init_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to initialize ENet");
   goto fail;
enet_client_fail:
   DEBUG(GLHCK_DBG_ERROR, "Failed to create ENet client");
   goto fail;
enet_client_init_fail:
   DEBUG(GLHCK_DBG_ERROR, "No available peers for iniating an ENet connection");
fail:
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief disconnect enet internally */
static void _glhckEnetDisconnect(void)
{
   ENetEvent event;
   TRACE(0);

   if (!_GLHCKclient.peer)
      return;

   if (!_glhckClientInitialized)
      goto force_disconnect;

   /* soft disconnect */
   enet_peer_disconnect(_GLHCKclient.peer, 0);
   while (enet_host_service(_GLHCKclient.enet, &event, 3000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_RECEIVE:
            enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            DEBUG(GLHCK_DBG_CRAP, "Disconnected from server");
            return;

         default:
            break;
      }
   }

force_disconnect:
   /* force disconnect */
   enet_peer_reset(_GLHCKclient.peer);
   DEBUG(GLHCK_DBG_CRAP, "Disconnected from server with force");
}

/* \brief destroy enet internally */
static void _glhckEnetDestroy(void)
{
   TRACE(0);

   if (!_GLHCKclient.enet)
      return;

   /* disconnect && kill */
   _glhckEnetDisconnect();
   enet_host_destroy(_GLHCKclient.enet);
   _GLHCKclient.enet = NULL;
   _GLHCKclient.peer = NULL;
}

/* \brief update enet state internally */
static int _glhckEnetUpdate(void)
{
   ENetEvent event;
   TRACE(2);

   /* wait up to 1000 milliseconds for an event */
   while (enet_host_service(_GLHCKclient.enet, &event, 1000) > 0) {
      switch (event.type) {
         case ENET_EVENT_TYPE_RECEIVE:
            printf("A packet of length %zu containing %s was received from %s on channel %u.\n",
                  event.packet->dataLength,
                  (char*)event.packet->data,
                  (char*)event.peer->data,
                  event.channelID);

            /* Clean up the packet now that we're done using it. */
            enet_packet_destroy(event.packet);
            break;

         case ENET_EVENT_TYPE_DISCONNECT:
            printf("%s disconected.\n", (char*)event.peer->data);

            /* Reset the peer's client information. */
            event.peer->data = NULL;
            break;

         default:
            break;
      }
   }

   RET(2, "%d", RETURN_OK);
   return RETURN_OK;
}

/* \brief send packet */
static void _glhckEnetSend(unsigned char *data, size_t size)
{
   ENetPacket *packet;
   packet = enet_packet_create(data, size, ENET_PACKET_FLAG_RELIABLE);
   enet_peer_send(_GLHCKclient.peer, 0, packet);
   enet_host_flush(_GLHCKclient.enet);
}

/* public api */

/* \brief initialize glhck server */
GLHCKAPI int glhckClientInit(const char *host, int port)
{
   CALL(0, "%s, %d", host, port);

   /* reinit, if initialized */
   if (_glhckClientInitialized)
      glhckClientKill();

   /* null struct */
   memset(&_GLHCKclient, 0, sizeof(__GLHCKclient));

   /* initialize enet */
   if (_glhckEnetInit(host, port) != RETURN_OK)
      goto fail_connect;

   _glhckClientInitialized = 1;
   DEBUG(GLHCK_DBG_CRAP, "Connected to server [%s:%d]",
         host?host:"127.0.0.1", port);

   RET(0, "%d", RETURN_OK);
   return RETURN_OK;

fail_connect:
   DEBUG(GLHCK_DBG_ERROR, "Failed to connect to server [%s:%d]",
         host?host:"127.0.0.1", port);
fail:
   _glhckEnetDestroy();
   RET(0, "%d", RETURN_FAIL);
   return RETURN_FAIL;
}

/* \brief kill glhck server */
GLHCKAPI void glhckClientKill(void)
{
   TRACE(0);

   /* is initialized? */
   if (!_glhckClientInitialized)
      return;

   /* kill enet */
   _glhckEnetDestroy();

   _glhckClientInitialized = 0;
   DEBUG(GLHCK_DBG_CRAP, "Killed ENet client");
}

/* \brief update server state */
GLHCKAPI void glhckClientUpdate(void)
{
   TRACE(2);

   /* is initialized? */
   if (!_glhckClientInitialized)
      return;

   /* updat enet */
   _glhckEnetUpdate();
}

/* \brief 'render' object to network */
GLHCKAPI void glhckClientObjectRender(const glhckObject *object)
{
   _glhckNetObjectPacket packet;
   TRACE(2);

   if (!_glhckClientInitialized)
      return;

   /* zero out packet */
   memset(&packet, 0, sizeof(_glhckNetObjectPacket));

   /* header */
   packet.type = GLHCK_NET_PACKET_OBJECT;

   /* geometry */
   packet.geometry.indicesCount = htonl(object->geometry.indicesCount);
   packet.geometry.vertexCount  = htonl(object->geometry.vertexCount);
   snprintf(packet.geometry.bias,  sizeof(packet.geometry.bias),  VEC3NS, VEC3(&object->geometry.bias));
   snprintf(packet.geometry.scale, sizeof(packet.geometry.scale), VEC3NS, VEC3(&object->geometry.scale));
   packet.geometry.type  = htonl(object->geometry.type);
   packet.geometry.flags = htonl(object->geometry.flags);

   /* view */
   snprintf(packet.view.translation,  sizeof(packet.view.translation), VEC3NS, VEC3(&object->view.translation));
   snprintf(packet.view.target,       sizeof(packet.view.target),      VEC3NS, VEC3(&object->view.target));
   snprintf(packet.view.rotation,     sizeof(packet.view.rotation),    VEC3NS, VEC3(&object->view.rotation));
   snprintf(packet.view.scaling,      sizeof(packet.view.scaling),     VEC3NS, VEC3(&object->view.scaling));

   /* material */
   memcpy(&packet.material.color, &object->material.color, sizeof(_glhckNetColor));

   /* send packet */
   _glhckEnetSend((unsigned char*)&packet, sizeof(_glhckNetObjectPacket));
}

#else /* ^ network support defined */

static void _glhckClientStub()
{
   DEBUG(GLHCK_DBG_ERROR, "GLhck was not build with network transparency");
}

GLHCKAPI int glhckClientInit(const char *host, int port)
{
   _glhckClientStub();
   return RETURN_FAIL;
}
GLHCKAPI void glhckClientKill(void)
{
   _glhckClientStub();
}
GLHCKAPI void glhckClientUpdate(void)
{
   _glhckClientStub();
}

GLHCKAPI void glhckClientObjectRender(const glhckObject *object)
{
   _glhckClientStub();
}

#endif /* GLHCK_USE_NETWORK */

/* vim: set ts=8 sw=3 tw=0 :*/
