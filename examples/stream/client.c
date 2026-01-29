/* client.c - STREAM Client Example */

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
    int32_t ret = 0;
    osal_delay_ms(1000); // Wait for server to boot
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

        osal_delay_ms(1000);
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
    configure_route(NODE_SERVER, zmq_iface.common.name);
    danp_config_t config = {
        .local_node = NODE_CLIENT,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);
    osal_thread_attr_t threadAttr = {
        .name = "ClientThread",
        .stack_size = 2048,
        .stack_mem = NULL,
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .cb_mem = NULL,
        .cb_size = 0,
    };
    osal_thread_create(taskClient, NULL, &threadAttr);

    while (1)
    {
        osal_delay_ms(1000);
    }

    return 0;
}
