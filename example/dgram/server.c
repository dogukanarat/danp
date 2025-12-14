/* server.c - DGRAM Server Example */

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

void taskEchoServer(void *arg)
{
    printf("[Server] Echo server starting...\n");

    // Create Socket (DGRAM)
    danp_socket_t *sock_dgram = danp_socket(DANP_TYPE_DGRAM);
    int32_t bindResult = danp_bind(sock_dgram, DGRAM_PORT);
    assert(bindResult == 0);

    printf("[Server] Listening dgram on port %d\n", DGRAM_PORT);

    while (1)
    {
        // Handle DGRAM messages
        char dgramBuf[64];
        uint16_t dgramSrcNode, dgramSrcPort;
        // printf("[Server] Waiting for message...\n");
        int32_t dgramLen = danp_recv_from(
            sock_dgram,
            dgramBuf,
            sizeof(dgramBuf) - 1,
            &dgramSrcNode,
            &dgramSrcPort,
            5000); // 5s timeout
        if (dgramLen > 0)
        {
            dgramBuf[dgramLen] = 0;
            printf("[Server] Recv DGRAM: %s\n", dgramBuf);

            // Echo back to sender
            danp_send_to(sock_dgram, dgramBuf, dgramLen, dgramSrcNode, dgramSrcPort);
        }
        else
        {
            // Sleep briefly to yield CPU time
            osalDelayMs(1);
        }
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
    osalThreadAttr_t threadAttr = {
        .name = "EchoServer",
        .stackSize = 2048,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
    };
    osalThreadCreate(taskEchoServer, NULL, &threadAttr);

    while (1)
    {
        osalDelayMs(1000);
    }

    return 0;
}
