/* server.c - STREAM Server Example */

#include "../example_definitions.h"
#include "danp/drivers/danp_zmq.h"
#include "danp/danp.h"
#include "osal/osal.h"
#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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
                    osalDelayMs(1);
                }
            }

            danp_close(client); // Close the accepted socket
            printf("[Server] Client socket closed.\n");
        }

        osalDelayMs(100);
    }
}

int main(int argc, char **argv)
{
    const char *subEndpoints[] = {"tcp://localhost:5556"};
    danp_zmq_interface_t zmq_iface = {0};
    danp_zmq_init(&zmq_iface, "tcp://*:5555", subEndpoints, 1, NODE_SERVER);
    danp_register_interface((danp_interface_t *)&zmq_iface);
    danp_config_t config = {
        .local_node = NODE_SERVER,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);
    osalThreadAttr_t threadAttr = {
        .name = "StreamServer",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskStreamServer, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
