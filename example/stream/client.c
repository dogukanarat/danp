/* client.c - STREAM Client Example */

#include "../example_definitions.h"
#include "danp/drivers/danp_zmq.h"
#include "osal/osal.h"
#include "danp/danp.h"
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

void taskClient(void *arg)
{
    int32_t ret = 0;
    osalDelayMs(1000); // Wait for server to boot
    printf("[Client] Starting Stream Client...\n");

    // 1. Create Socket (STREAM)
    danp_socket_t *sock_stream = danp_socket(DANP_TYPE_STREAM);

    // 2. Connect to Server (STREAM)
    int32_t connectResult = danp_connect(sock_stream, NODE_SERVER, STREAM_PORT);
    assert(connectResult == 0);

    for (int i = 0; i < 5; i++)
    {
        char msg[32];
        sprintf(msg, "Hello %d", i);
        printf("[Client] Sending: %s\n", msg);

        ret = danp_send(sock_stream, msg, strlen(msg));
        if (ret < 0)
        {
            printf("[Client] Send Failed! Breaking stream loop.\n");
            break; // Break the loop on send failure
        }

        // Wait for reply
        char reply[64];
        ret = danp_recv(sock_stream, reply, sizeof(reply) - 1, 5000);
        if (ret < 0)
        {
            printf("[Client] Receive Failed! Breaking stream loop.\n");
            break; // Break the loop on receive failure
        }
        reply[ret] = '\0';
        printf("[Client] Got Reply: %s\n", reply);

        osalDelayMs(1000);
    }

    danp_close(sock_stream); // Close the stream socket
    printf("[Client] Stream socket closed.\n");
}

int main()
{
    const char *subEndpoints[] = {"tcp://localhost:5555"};
    danp_zmq_interface_t zmq_iface = {0};
    danp_zmq_init(&zmq_iface, "tcp://*:5556", subEndpoints, 1, NODE_CLIENT);
    danp_register_interface((danp_interface_t *)&zmq_iface);
    danp_config_t config = {
        .local_node = NODE_CLIENT,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);
    osalThreadAttr_t threadAttr = {
        .name = "ClientThread",
        .stackSize = 2048,
        .stackMem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cbMem = NULL,
        .cbSize = 0,
    };
    osalThreadCreate(taskClient, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
