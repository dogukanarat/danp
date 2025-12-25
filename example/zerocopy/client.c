/* client.c - Zero-Copy Client Example */

#include "../example_definitions.h"
#include "danp/danp.h"
#include "danp/drivers/danp_zmq.h"
#include "osal/osal.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define ZEROCOPY_PORT 50

extern void danpZmqInit(
    const char *pubBindEndpoint,
    const char **subConnectEndpoints,
    int subCount,
    uint16_t nodeId);
extern void danpLogMessageCallback(
    danp_log_level_t level,
    const char *funcName,
    const char *message,
    va_list args);

static void configure_route(uint16_t destination, const char *iface_name)
{
    char table_entry[32];
    int written = snprintf(table_entry, sizeof(table_entry), "%u:%s", destination, iface_name);
    if (written <= 0 || written >= (int)sizeof(table_entry))
    {
        printf("[Client] Failed to format route entry for destination %u\n", destination);
        return;
    }

    if (danp_route_table_load(table_entry) != 0)
    {
        printf("[Client] Failed to install route '%s'\n", table_entry);
    }
    else
    {
        printf("[Client] Installed static route: %s\n", table_entry);
    }
}

void taskClient(void *arg)
{
    osalDelayMs(1000); // Wait for server to boot
    printf("[Client] Starting Zero-Copy Client...\n");

    // Create and connect socket
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    assert(sock != NULL);

    int32_t connect_ret = danp_connect(sock, NODE_SERVER, ZEROCOPY_PORT);
    assert(connect_ret == 0);
    printf("[Client] Connected to server (zero-copy mode)!\n");

    // Send messages using zero-copy API
    for (int i = 0; i < 10; i++)
    {
        // Allocate packet from pool
        danp_packet_t *pkt = danp_buffer_get();
        if (!pkt)
        {
            printf("[Client] Failed to allocate packet\n");
            break;
        }

        // Fill payload directly (no intermediate copy!)
        char msg[64];
        sprintf(msg, "Message %d (zero-copy)", i);
        memcpy(pkt->payload, msg, strlen(msg));
        pkt->length = strlen(msg);

        printf("[Client] Sending (zero-copy): '%s'\n", msg);

        // Send packet without copying (ownership transfers to stack)
        if (danp_send_packet(sock, pkt) < 0)
        {
            printf("[Client] Send failed\n");
            danp_buffer_free(pkt);
            break;
        }

        // Receive reply without copying
        danp_packet_t *reply = danp_recv_packet(sock, 5000);
        if (!reply)
        {
            printf("[Client] Receive timeout or error\n");
            break;
        }

        // Process reply directly from packet payload (zero-copy!)
        printf("[Client] Received (zero-copy): '%.*s' (%u bytes)\n", reply->length, reply->payload, reply->length);

        // Free reply packet
        danp_buffer_free(reply);

        osalDelayMs(500);
    }

    printf("[Client] Closing connection...\n");
    danp_close(sock);
}

int main()
{
    const char *subEndpoints[] = {"tcp://localhost:5555"};
    danp_zmq_interface_t zmq_iface = {0};
    danp_zmq_init(&zmq_iface, "tcp://*:5556", subEndpoints, 1, NODE_CLIENT);
    danp_register_interface((danp_interface_t *)&zmq_iface);
    configure_route(NODE_SERVER, zmq_iface.common.name);

    danp_config_t config = {
        .local_node = NODE_CLIENT,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);

    osalThreadAttr_t threadAttr = {
        .name = "ClientTask",
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .stackSize = 4096,
    };
    osalThreadHandle_t client_thread = osalThreadCreate(taskClient, NULL, &threadAttr);
    assert(client_thread != NULL);

    osalThreadJoin(client_thread, NULL);

    return 0;
}
