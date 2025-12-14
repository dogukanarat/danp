/* client.c - DGRAM Client Example */

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

void taskClient(void *arg)
{
    osalDelayMs(1000); // Wait for server to boot
    printf("[Client] Starting DGRAM Client...\n");

    // Create Socket (DGRAM)
    danp_socket_t *sock_dgram = danp_socket(DANP_TYPE_DGRAM);

    // Connect to Server (DGRAM) - sets default destination
    int32_t connectResult = danp_connect(sock_dgram, NODE_SERVER, DGRAM_PORT);
    assert(connectResult == 0);

    for (int i = 0; i < 5; i++)
    {
        char msg[32];
        sprintf(msg, "Ping %d", i);
        printf("[Client] Sending DGRAM: %s\n", msg);

        if (danp_send(sock_dgram, msg, strlen(msg)) < 0)
        {
            printf("[Client] DGRAM Send Failed!\n");
        }

        // Wait for reply
        char reply[64];
        int32_t reply_len = danp_recv(sock_dgram, reply, sizeof(reply) - 1, 1000);

        if (reply_len > 0)
        {
            reply[reply_len] = '\0';
            printf("[Client] Got DGRAM Reply: %s\n", reply);
        }
        else
        {
            printf("[Client] DGRAM Receive Failed/Timeout (ret=%d)\n", reply_len);
        }

        osalDelayMs(1000);
    }

    danp_close(sock_dgram); // Close the DGRAM socket
    printf("[Client] DGRAM socket closed.\n");
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
        .name = "Client",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskClient, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
