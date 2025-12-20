/* danpZmq.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "osal/osal.h"
#include "danp/danp.h"
#include "../danp_debug.h"
#include "danp/drivers/danp_zmq.h"
#include <assert.h>
#include <pthread.h>
#include <stdio.h>
#include <zmq.h>

/* LCOV_EXCL_START */ /* Requires live ZMQ endpoints; excluded from unit coverage */

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

static void *zmq_context = NULL;

/* Functions */

static int32_t danp_zmq_tx(void *iface_common, danp_packet_t *packet)
{
    danp_zmq_interface_t *iface = (danp_zmq_interface_t *)iface_common;
    uint8_t buffer[DANP_MAX_PACKET_SIZE + 4];
    memcpy(buffer, &packet->header_raw, 4);
    if (packet->length > 0)
    {
        memcpy(buffer + 4, packet->payload, packet->length);
    }

    // Manually unpack for logging (Shift values changed!)
    uint16_t dst = (packet->header_raw >> 22) & 0xFF;
    uint8_t d_port = (packet->header_raw >> 8) & 0x3F;
    uint8_t flags = packet->header_raw & 0x03;

    danp_log_message(
        DANP_LOG_VERBOSE,
        "ZMQ TX: dst=%u port=%u flags=0x%02X len=%u",
        dst,
        d_port,
        flags,
        packet->length);

    // ZMQ Topic    // 1. Send Topic (2 bytes, NodeID)
    uint16_t topic = packet->header_raw >> 22 & 0xFF; // Extract DST Node
    zmq_send(iface->pub_sock, &topic, 2, ZMQ_SNDMORE);
    zmq_send(iface->pub_sock, buffer, 4 + packet->length, 0);
    return 0;
}

static void danp_zmq_rx_routine(void *arg)
{
    danp_zmq_interface_t *iface = (danp_zmq_interface_t *)arg;
    while (1)
    {
        zmq_recv(iface->sub_sock, NULL, 0, 0);
        uint8_t buffer[256];
        int32_t len = zmq_recv(iface->sub_sock, buffer, sizeof(buffer), 0);
        if (len >= 4)
        {
            uint32_t header_raw;
            memcpy(&header_raw, buffer, 4);

            uint16_t dst = (header_raw >> 22) & 0xFF;
            uint8_t d_port = (header_raw >> 8) & 0x3F;
            uint8_t flags = header_raw & 0x03;

            danp_log_message(
                DANP_LOG_VERBOSE,
                "ZMQ RX: [dst]=%u, [port]=%u [flags]=0x%02X [len]=%d",
                dst,
                d_port,
                flags,
                len - 4);
            danp_input((danp_interface_t *)iface, buffer, len);
        }
        else
        {
            danp_log_message(DANP_LOG_WARN, "ZMQ RX: received packet too short");
        }
    }
}

void danp_zmq_init(
    danp_zmq_interface_t *iface,
    const char *pub_bind_endpoint,
    const char **sub_connect_endpoints,
    int32_t sub_count,
    uint16_t node_id)
{
    if (zmq_context == NULL)
    {
        zmq_context = zmq_ctx_new();
        if (zmq_context == NULL)
        {
            danp_log_message(DANP_LOG_ERROR, "Failed to create ZMQ context");
            return;
        }
    }

    // PUB
    iface->pub_sock = zmq_socket(zmq_context, ZMQ_PUB);
    zmq_bind(iface->pub_sock, pub_bind_endpoint);

    // SUB
    iface->sub_sock = zmq_socket(zmq_context, ZMQ_SUB);
    for (int i = 0; i < sub_count; i++)
    {
        zmq_connect(iface->sub_sock, sub_connect_endpoints[i]);
    }

    // Filter: Subscribe to My Node ID
    uint16_t topic = node_id;
    zmq_setsockopt(iface->sub_sock, ZMQ_SUBSCRIBE, &topic, 2);

    // Also subscribe to Broadcast (0xFF) if needed, or Promiscuous mode
    // For now, just specific node.

    iface->common.name = "ZMQ";
    iface->common.address = node_id;
    iface->common.mtu = DANP_MAX_PACKET_SIZE;
    iface->common.tx_func = danp_zmq_tx;

    pthread_create((pthread_t*)&iface->rx_thread_id, NULL, danp_zmq_rx_routine, iface);
}

/* LCOV_EXCL_STOP */
