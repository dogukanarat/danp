/* danpZmq.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danpDebug.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <zmq.h>

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

static void *ZmqContext = NULL;

/* Functions */

int32_t danpZmqTx(void *ifaceCommon, danpPacket_t *packet)
{
    danpZmqInterface_t *iface = (danpZmqInterface_t *)ifaceCommon;
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];
    memcpy(buffer, &packet->headerRaw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    // Manually unpack for logging (Shift values changed!)
    uint16_t dst = (packet->headerRaw >> 22) & 0xFF;
    uint8_t dPort = (packet->headerRaw >> 8) & 0x3F;
    uint8_t flags = packet->headerRaw & 0x03;

    danpLogMessage(
        DANP_LOG_VERBOSE,
        "ZMQ TX: dst=%u port=%u flags=0x%02X len=%u",
        dst,
        dPort,
        flags,
        packet->length);

    // ZMQ Topic    // 1. Send Topic (2 bytes, NodeID)
    uint16_t topic = packet->headerRaw >> 22 & 0xFF; // Extract DST Node
    zmq_send(iface->pubSock, &topic, 2, ZMQ_SNDMORE);
    zmq_send(iface->pubSock, buffer, 4 + packet->length, 0);
    return 0;
}

void *zmqRxRoutine(void *arg)
{
    danpZmqInterface_t *iface = (danpZmqInterface_t *)arg;
    while (1)
    {
        zmq_recv(iface->subSock, NULL, 0, 0);
        uint8_t buffer[256];
        int32_t len = zmq_recv(iface->subSock, buffer, sizeof(buffer), 0);
        if (len >= 4)
        {
            uint32_t headerRaw;
            memcpy(&headerRaw, buffer, 4);

            uint16_t dst = (headerRaw >> 22) & 0xFF;
            uint8_t dPort = (headerRaw >> 8) & 0x3F;
            uint8_t flags = headerRaw & 0x03;

            danpLogMessage(
                DANP_LOG_VERBOSE,
                "ZMQ RX: [dst]=%u, [port]=%u [flags]=0x%02X [len]=%d",
                dst,
                dPort,
                flags,
                len - 4);
            danpInput(iface, buffer, len);
        }
        else
        {
            danpLogMessage(DANP_LOG_WARN, "ZMQ RX: received packet too short");
        }
    }
    return NULL;
}

void danpZmqInit(
    danpZmqInterface_t *iface,
    const char *pubBindEndpoint,
    const char **subConnectEndpoints,
    int32_t subCount,
    uint16_t nodeId)
{
    if (ZmqContext == NULL)
    {
        ZmqContext = zmq_ctx_new();
        if (ZmqContext == NULL)
        {
            danpLogMessage(DANP_LOG_ERROR, "Failed to create ZMQ context");
            return;
        }
    }

    // PUB
    iface->pubSock = zmq_socket(ZmqContext, ZMQ_PUB);
    zmq_bind(iface->pubSock, pubBindEndpoint);

    // SUB
    iface->subSock = zmq_socket(ZmqContext, ZMQ_SUB);
    for (int i = 0; i < subCount; i++)
    {
        zmq_connect(iface->subSock, subConnectEndpoints[i]);
    }

    // Filter: Subscribe to My Node ID
    uint16_t topic = nodeId;
    zmq_setsockopt(iface->subSock, ZMQ_SUBSCRIBE, &topic, 2);

    // Also subscribe to Broadcast (0xFF) if needed, or Promiscuous mode
    // For now, just specific node.

    iface->common.name = "ZMQ";
    iface->common.address = nodeId;
    iface->common.mtu = DANP_MAX_PACKET_SIZE;
    iface->common.txFunc = danpZmqTx;

    pthread_create(&iface->rxThreadId, NULL, zmqRxRoutine, iface);
}
