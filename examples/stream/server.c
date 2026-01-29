/* server.c - STREAM Server Example */

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal/osal_thread.h"
#include "osal/osal_time.h"
#include "danp/danp.h"
#include "danp/drivers/danp_zmq.h"

#include "../example_definitions.h"

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

void taskStreamServer(void *arg)
{
    printf("[Server] Stream server starting...\n");

    // Create Listening Socket (STREAM)
    danp_socket_t *sock_stream = danp_socket(DANP_TYPE_STREAM);
    int32_t bindResult = danp_bind(sock_stream, STREAM_PORT);
    assert(bindResult == 0);
    danp_listen(sock_stream, 5);

    printf("[Server] Listening stream on port %d\n", STREAM_PORT);

    while (1)
    {
        printf("[Server] Waiting for client...\n");
        danp_socket_t *client = danp_accept(sock_stream, 5000); // 5s timeout
        if (client)
        {
            printf("[Server] Client Accepted! Node: %d\n", client->remote_node);

            char buf[64];
            while (1)
            {
                // printf("[Server] Waiting for data from client...\n");
                // Use a short timeout to prevent excessive blocking, allowing internal DANP processing to run.
                int32_t len = danp_recv(client, buf, sizeof(buf) - 1, 5000); // 1000ms timeout
                if (len > 0)
                {
                    buf[len] = 0;
                    printf("[Server] Recv: %s\n", buf);

                    // Echo back
                    // Check danpSend result for errors/disconnects
                    if (danp_send(client, "ACK from Server", 15) < 0)
                    {
                        printf("[Server] danpSend failed. Closing client socket.\n");
                        break;
                    }
                }
                else if (len < 0)
                { // Error or Disconnect
                    printf("[Server] danpRecv failed (ret=%d). Closing client socket.\n", len);
                    break;
                }
                else
                { // len == 0 (Timeout or Graceful Close)
                    // Sleep briefly to yield CPU time to DANP internal processing/packet management tasks
                    osal_delay_ms(1);
                }
            }

            danp_close(client); // Close the accepted socket
            printf("[Server] Client socket closed.\n");
        }

        osal_delay_ms(100);
    }
}

int main(int argc, char **argv)
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
    osal_thread_attr_t threadAttr = {
        .name = "StreamServer",
        .stack_size = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osal_thread_create(taskStreamServer, NULL, &threadAttr);

    while (1)
    {
        osal_delay_ms(1000);
    }

    return 0;
}
