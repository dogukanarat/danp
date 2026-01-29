/* sfp_example.c - SFP (Small Fragmentation Protocol) Example */

#include <assert.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "osal/osal_thread.h"
#include "osal/osal_time.h"
#include "danp/danp.h"
#include "danp/danp_buffer.h"
#include "danp/drivers/danp_zmq.h"

#include "../example_definitions.h"

#define SFP_PORT 51

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
        printf("[SFP] Failed to format route entry for destination %u\n", destination);
        return;
    }

    if (danp_route_table_load(table_entry) != 0)
    {
        printf("[SFP] Failed to install route '%s'\n", table_entry);
    }
    else
    {
        printf("[SFP] Installed static route: %s\n", table_entry);
    }
}

void taskServer(void *arg)
{
    printf("[SFP-Server] Starting SFP Server...\n");

    danp_socket_t *server_sock = danp_socket(DANP_TYPE_STREAM);
    assert(server_sock != NULL);

    danp_bind(server_sock, SFP_PORT);
    danp_listen(server_sock, 5);
    printf("[SFP-Server] Listening on port %u...\n", SFP_PORT);

    danp_socket_t *client_sock = danp_accept(server_sock, DANP_WAIT_FOREVER);
    assert(client_sock != NULL);
    printf("[SFP-Server] Client connected!\n");

    // Receive large fragmented message
    printf("[SFP-Server] Waiting for fragmented message...\n");
    danp_packet_t *pkt_chain = danp_recv_sfp(client_sock, 30000);
    if (!pkt_chain)
    {
        printf("[SFP-Server] Failed to receive fragmented message\n");
    }
    else
    {
        // Reassemble and print message
        printf("[SFP-Server] Received fragmented message:\n");
        printf("========================================\n");

        uint16_t total_bytes = 0;
        uint8_t fragment_count = 0;
        danp_packet_t *current = pkt_chain;

        while (current)
        {
            printf("  Fragment %u: %u bytes\n", fragment_count, current->length);
            printf("    Data: '%.*s'\n", current->length, current->payload);
            total_bytes += current->length;
            fragment_count++;
            current = current->next;
        }

        printf("========================================\n");
        printf("[SFP-Server] Total: %u bytes in %u fragments\n", total_bytes, fragment_count);

        // Free the packet chain
        danp_buffer_free_chain(pkt_chain);
    }

    printf("[SFP-Server] Closing connection...\n");
    danp_close(client_sock);
    danp_close(server_sock);
}

void taskClient(void *arg)
{
    osal_delay_ms(1000); // Wait for server
    printf("[SFP-Client] Starting SFP Client...\n");

    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    assert(sock != NULL);

    int32_t connect_ret = danp_connect(sock, NODE_SERVER, SFP_PORT);
    assert(connect_ret == 0);
    printf("[SFP-Client] Connected to server!\n");

    // Create a large message that will be fragmented (> 127 bytes)
    char large_message[512];
    memset(large_message, 'A', sizeof(large_message));

    // Add some structure to the message
    sprintf(large_message,
            "This is a large message that will be fragmented using SFP. "
            "It contains %zu bytes, which exceeds the MTU of %d bytes. "
            "The SFP protocol will automatically fragment this into multiple packets. ",
            sizeof(large_message),
            DANP_MAX_PACKET_SIZE);

    // Fill rest with pattern
    size_t msg_len = strlen(large_message);
    for (size_t i = msg_len; i < sizeof(large_message) - 1; i++)
    {
        large_message[i] = 'A' + (i % 26);
    }
    large_message[sizeof(large_message) - 1] = '\0';

    printf("[SFP-Client] Sending large message (%zu bytes)...\n", strlen(large_message));

    // Send using SFP (automatic fragmentation)
    int32_t sent = danp_send_sfp(sock, large_message, strlen(large_message));
    if (sent < 0)
    {
        printf("[SFP-Client] Failed to send fragmented message\n");
    }
    else
    {
        printf("[SFP-Client] Successfully sent %d bytes (auto-fragmented)\n", sent);
    }

    osal_delay_ms(2000);

    printf("[SFP-Client] Closing connection...\n");
    danp_close(sock);
}

int main(int argc, char **argv)
{
    uint16_t node_id = NODE_CLIENT;
    const char *mode = "client";

    if (argc > 1)
    {
        mode = argv[1];
    }

    if (strcmp(mode, "server") == 0)
    {
        node_id = NODE_SERVER;
    }

    const char *pub_endpoint = (node_id == NODE_SERVER) ? "tcp://*:5555" : "tcp://*:5556";
    const char *sub_endpoint = (node_id == NODE_SERVER) ? "tcp://localhost:5556" : "tcp://localhost:5555";
    const char *subEndpoints[] = {sub_endpoint};
    uint16_t peer_node = (node_id == NODE_SERVER) ? NODE_CLIENT : NODE_SERVER;

    danp_zmq_interface_t zmq_iface = {0};
    danp_zmq_init(&zmq_iface, pub_endpoint, subEndpoints, 1, node_id);
    danp_register_interface((danp_interface_t *)&zmq_iface);
    configure_route(peer_node, zmq_iface.common.name);

    danp_config_t config = {
        .local_node = node_id,
        .log_function = danpLogMessageCallback,
    };
    danp_init(&config);

    osal_thread_attr_t threadAttr = {
        .name = (node_id == NODE_SERVER) ? "SFP-Server" : "SFP-Client",
        .priority = OSAL_THREAD_PRIORITY_NORMAL,
        .stack_size = 8192,
    };

    osal_thread_handle_t thread;
    if (node_id == NODE_SERVER)
    {
        thread = osal_thread_create(taskServer, NULL, &threadAttr);
    }
    else
    {
        thread = osal_thread_create(taskClient, NULL, &threadAttr);
    }

    assert(thread != NULL);
    osal_thread_join(thread);

    return 0;
}
