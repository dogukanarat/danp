/* server.c - Zero-Copy Server Example */

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
        printf("[Server] Failed to format route entry for destination %u\n", destination);
        return;
    }

    if (danp_route_table_load(table_entry) != 0)
    {
        printf("[Server] Failed to install route '%s'\n", table_entry);
    }
    else
    {
        printf("[Server] Installed static route: %s\n", table_entry);
    }
}

void taskServer(void *arg)
{
    printf("[Server] Starting Zero-Copy Server...\n");

    // Create and bind server socket
    danp_socket_t *server_sock = danp_socket(DANP_TYPE_STREAM);
    assert(server_sock != NULL);

    int32_t bind_ret = danp_bind(server_sock, ZEROCOPY_PORT);
    assert(bind_ret == 0);

    danp_listen(server_sock, 5);
    printf("[Server] Listening on port %u (zero-copy mode)...\n", ZEROCOPY_PORT);

    // Accept connection
    danp_socket_t *client_sock = danp_accept(server_sock, DANP_WAIT_FOREVER);
    assert(client_sock != NULL);
    printf("[Server] Client connected!\n");

    // Process messages using zero-copy API
    for (int i = 0; i < 10; i++)
    {
        // Receive packet without copying
        danp_packet_t *pkt = danp_recv_packet(client_sock, 10000);
        if (!pkt)
        {
            printf("[Server] Receive timeout or error\n");
            break;
        }

        // Process data directly from packet payload (zero-copy!)
        printf("[Server] Received (zero-copy): '%.*s' (%u bytes)\n", pkt->length, pkt->payload, pkt->length);

        // Allocate new packet for response
        danp_packet_t *reply = danp_buffer_get();
        if (reply)
        {
            // Prepare response
            const char *response = "ACK";
            memcpy(reply->payload, response, strlen(response));
            reply->length = strlen(response);

            // Send response without copying
            if (danp_send_packet(client_sock, reply) < 0)
            {
                printf("[Server] Failed to send reply\n");
                danp_buffer_free(reply);
            }
            else
            {
                printf("[Server] Sent ACK (zero-copy)\n");
            }
        }

        // Free received packet
        danp_buffer_free(pkt);
    }

    printf("[Server] Closing connection...\n");
    danp_close(client_sock);
    danp_close(server_sock);
}

int main()
{
    const char *subEndpoints[] = {"tcp://localhost:5556"};
    danp_zmq_interface_t zmq_iface = {0};
    danp_zmq_init(&zmq_iface, "tcp://*:5555", subEndpoints, 1, NODE_SERVER);
    danp_register_interface((danp_interface_t *)&zmq_iface);
    configure_route(NODE_CLIENT, zmq_iface.common.name);

    danp_config_t config = {
        .local_node = NODE_SERVER,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);

    osalThreadAttr_t threadAttr = {
        .name = "ServerTask",
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .stackSize = 4096,
    };
    osalThreadHandle_t server_thread = osalThreadCreate(taskServer, NULL, &threadAttr);
    assert(server_thread != NULL);

    osalThreadJoin(server_thread, NULL);

    return 0;
}
