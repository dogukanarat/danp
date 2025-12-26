/**
 * @file test_zerocopy.c
 * @brief Zero-copy and SFP tests for DANP library
 *
 * This file contains unit tests for DANP zero-copy and fragmentation including:
 * - Zero-copy buffer allocation and deallocation
 * - Packet chaining functionality
 * - SFP (Small Fragmentation Protocol) header packing/unpacking
 * - Zero-copy send/receive operations
 * - Automatic fragmentation and reassembly
 */

#include "danp/danp.h"
#include "unity.h"
#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define TEST_NODE_ID 42
#define SERVER_PORT 60
#define CLIENT_PORT 61

/* ============================================================================
 * Test Loopback Interface
 * ============================================================================
 */

static danp_packet_t *loopback_last_packet = NULL;

static int32_t loopback_tx(void *iface_common, danp_packet_t *packet)
{
    (void)iface_common;

    /* Allocate new packet and copy data (simulate driver behavior) */
    loopback_last_packet = danp_buffer_allocate();
    if (loopback_last_packet)
    {
        memcpy(loopback_last_packet, packet, sizeof(danp_packet_t));
        loopback_last_packet->next = NULL;
    }

    return 0;
}

static danp_interface_t loopback_iface = {
    .name = "TEST_LOOPBACK_ZEROCOPY",
    .address = TEST_NODE_ID,
    .mtu = 128,
    .tx_func = loopback_tx,
};
static bool loopback_registered = false;

/* ============================================================================
 * Test Setup and Teardown
 * ============================================================================
 */

void setUp(void)
{
    danp_config_t cfg = {.local_node = TEST_NODE_ID};
    danp_init(&cfg);

    if (!loopback_registered)
    {
        memset(&loopback_iface, 0, sizeof(loopback_iface));
        loopback_iface.name = "TEST_LOOPBACK_ZEROCOPY";
        loopback_iface.address = TEST_NODE_ID;
        loopback_iface.mtu = 128;
        loopback_iface.tx_func = loopback_tx;
        danp_register_interface(&loopback_iface);
        loopback_registered = true;

        /* Install route for loopback testing */
        char route[64];
        sprintf(route, "%d:TEST_LOOPBACK_ZEROCOPY", TEST_NODE_ID);
        danp_route_table_load(route);
    }

    loopback_last_packet = NULL;
}

void tearDown(void)
{
    if (loopback_last_packet)
    {
        danp_buffer_free(loopback_last_packet);
        loopback_last_packet = NULL;
    }
}

/* ============================================================================
 * Buffer Management Tests
 * ============================================================================
 */

/**
 * @brief Test danp_buffer_get() allocates packet with NULL next pointer
 */
void test_buffer_get_initializes_next_to_null(void)
{
    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);
    TEST_ASSERT_NULL(pkt->next);
    danp_buffer_free(pkt);
}

/**
 * @brief Test danp_buffer_get() is equivalent to danp_buffer_allocate()
 */
void test_buffer_get_allocates_from_pool(void)
{
    size_t initial_free = danp_buffer_get_free_count();

    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);

    size_t after_alloc = danp_buffer_get_free_count();
    TEST_ASSERT_EQUAL(initial_free - 1, after_alloc);

    danp_buffer_free(pkt);

    size_t after_free = danp_buffer_get_free_count();
    TEST_ASSERT_EQUAL(initial_free, after_free);
}

/**
 * @brief Test danp_buffer_free_chain() frees all packets in chain
 */
void test_buffer_free_chain_frees_all_packets(void)
{
    size_t initial_free = danp_buffer_get_free_count();

    /* Create chain of 3 packets */
    danp_packet_t *pkt1 = danp_buffer_get();
    danp_packet_t *pkt2 = danp_buffer_get();
    danp_packet_t *pkt3 = danp_buffer_get();

    TEST_ASSERT_NOT_NULL(pkt1);
    TEST_ASSERT_NOT_NULL(pkt2);
    TEST_ASSERT_NOT_NULL(pkt3);

    pkt1->next = pkt2;
    pkt2->next = pkt3;
    pkt3->next = NULL;

    size_t after_alloc = danp_buffer_get_free_count();
    TEST_ASSERT_EQUAL(initial_free - 3, after_alloc);

    /* Free entire chain */
    danp_buffer_free_chain(pkt1);

    size_t after_free = danp_buffer_get_free_count();
    TEST_ASSERT_EQUAL(initial_free, after_free);
}

/**
 * @brief Test danp_buffer_free_chain() handles NULL gracefully
 */
void test_buffer_free_chain_handles_null(void)
{
    /* Should not crash */
    danp_buffer_free_chain(NULL);
    TEST_PASS();
}

/**
 * @brief Test danp_buffer_free_chain() handles single packet
 */
void test_buffer_free_chain_handles_single_packet(void)
{
    size_t initial_free = danp_buffer_get_free_count();

    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);
    pkt->next = NULL;

    danp_buffer_free_chain(pkt);

    size_t after_free = danp_buffer_get_free_count();
    TEST_ASSERT_EQUAL(initial_free, after_free);
}

/* ============================================================================
 * Zero-Copy Socket Tests
 * ============================================================================
 */

/**
 * @brief Test danp_send_packet() sends packet without copying
 */
void test_send_packet_zero_copy(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, CLIENT_PORT);

    /* Set remote endpoint */
    sock->remote_node = TEST_NODE_ID;
    sock->remote_port = SERVER_PORT;
    sock->state = DANP_SOCK_ESTABLISHED;

    /* Allocate and fill packet */
    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);

    const char *test_data = "Zero-copy test";
    memcpy(pkt->payload, test_data, strlen(test_data));
    pkt->length = strlen(test_data);

    /* Send packet (ownership transfers) */
    int32_t ret = danp_send_packet(sock, pkt);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify packet was transmitted */
    TEST_ASSERT_NOT_NULL(loopback_last_packet);
    TEST_ASSERT_EQUAL(pkt->length, loopback_last_packet->length);
    TEST_ASSERT_EQUAL_MEMORY(test_data, loopback_last_packet->payload, strlen(test_data));

    danp_close(sock);
}

/**
 * @brief Test danp_recv_packet() receives packet without copying
 */
void test_recv_packet_zero_copy(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    /* Simulate received packet */
    danp_packet_t *rx_pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(rx_pkt);

    const char *test_data = "Received data";
    memcpy(rx_pkt->payload, test_data, strlen(test_data));
    rx_pkt->length = strlen(test_data);

    /* Put packet in RX queue */
    osal_message_queue_send(sock->rx_queue, &rx_pkt, 0);

    /* Receive packet (zero-copy) */
    danp_packet_t *received = danp_recv_packet(sock, 1000);
    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL(strlen(test_data), received->length);
    TEST_ASSERT_EQUAL_MEMORY(test_data, received->payload, strlen(test_data));

    /* User must free received packet */
    danp_buffer_free(received);

    danp_close(sock);
}

/**
 * @brief Test danp_recv_packet() returns NULL on timeout
 */
void test_recv_packet_timeout(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    /* Try to receive with short timeout */
    danp_packet_t *received = danp_recv_packet(sock, 100);
    TEST_ASSERT_NULL(received);

    danp_close(sock);
}

/**
 * @brief Test danp_send_packet_to() sends to specific destination
 */
void test_send_packet_to(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, CLIENT_PORT);

    /* Allocate and fill packet */
    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);

    const char *test_data = "Send to test";
    memcpy(pkt->payload, test_data, strlen(test_data));
    pkt->length = strlen(test_data);

    /* Send packet to specific destination (use TEST_NODE_ID for routing) */
    uint16_t dst_node = TEST_NODE_ID;
    uint16_t dst_port = 55;
    int32_t ret = danp_send_packet_to(sock, pkt, dst_node, dst_port);
    TEST_ASSERT_EQUAL(0, ret);

    /* Verify packet was transmitted with correct addressing */
    TEST_ASSERT_NOT_NULL(loopback_last_packet);

    uint16_t dst, src;
    uint8_t d_port, s_port, flags;
    danp_unpack_header(loopback_last_packet->header_raw, &dst, &src, &d_port, &s_port, &flags);

    TEST_ASSERT_EQUAL(dst_node, dst);
    TEST_ASSERT_EQUAL(dst_port, d_port);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, src);
    TEST_ASSERT_EQUAL(CLIENT_PORT, s_port);

    danp_close(sock);
}

/**
 * @brief Test danp_recv_packet_from() receives with source information
 */
void test_recv_packet_from(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    /* Simulate received packet from specific source */
    danp_packet_t *rx_pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(rx_pkt);

    const char *test_data = "Packet with source";
    memcpy(rx_pkt->payload, test_data, strlen(test_data));
    rx_pkt->length = strlen(test_data);

    /* Set header with specific source (use valid 6-bit port: 0-63) */
    uint16_t expected_src_node = 77;
    uint16_t expected_src_port = 33;  /* Valid port (< 64) */
    rx_pkt->header_raw = danp_pack_header(
        DANP_PRIORITY_NORMAL,
        TEST_NODE_ID,  /* dst */
        expected_src_node,  /* src */
        SERVER_PORT,  /* dst port */
        expected_src_port,  /* src port */
        DANP_FLAG_NONE);

    /* Put packet in RX queue */
    osal_message_queue_send(sock->rx_queue, &rx_pkt, 0);

    /* Receive packet with source info */
    uint16_t src_node = 0;
    uint16_t src_port = 0;
    danp_packet_t *received = danp_recv_packet_from(sock, &src_node, &src_port, 1000);

    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL(strlen(test_data), received->length);
    TEST_ASSERT_EQUAL_MEMORY(test_data, received->payload, strlen(test_data));
    TEST_ASSERT_EQUAL(expected_src_node, src_node);
    TEST_ASSERT_EQUAL(expected_src_port, src_port);

    /* User must free received packet */
    danp_buffer_free(received);

    danp_close(sock);
}

/**
 * @brief Test danp_recv_packet_from() with NULL pointers for optional args
 */
void test_recv_packet_from_null_args(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    /* Simulate received packet */
    danp_packet_t *rx_pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(rx_pkt);

    const char *test_data = "Test data";
    memcpy(rx_pkt->payload, test_data, strlen(test_data));
    rx_pkt->length = strlen(test_data);
    rx_pkt->header_raw = danp_pack_header(0, TEST_NODE_ID, 88, SERVER_PORT, 77, 0);

    /* Put packet in RX queue */
    osal_message_queue_send(sock->rx_queue, &rx_pkt, 0);

    /* Receive packet with NULL source pointers (should not crash) */
    danp_packet_t *received = danp_recv_packet_from(sock, NULL, NULL, 1000);

    TEST_ASSERT_NOT_NULL(received);
    TEST_ASSERT_EQUAL(strlen(test_data), received->length);

    danp_buffer_free(received);
    danp_close(sock);
}

/* ============================================================================
 * SFP (Small Fragmentation Protocol) Tests
 * ============================================================================
 */

/**
 * @brief Test SFP fragmentation for small message (no fragmentation needed)
 */
void test_sfp_send_small_message(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, CLIENT_PORT);
    sock->remote_node = TEST_NODE_ID;
    sock->remote_port = SERVER_PORT;
    sock->state = DANP_SOCK_ESTABLISHED;

    /* Send small message (< 127 bytes) */
    const char *msg = "Small message";
    int32_t sent = danp_send_sfp(sock, (void *)msg, strlen(msg));

    TEST_ASSERT_EQUAL(strlen(msg), sent);

    danp_close(sock);
}

/**
 * @brief Test SFP fragmentation for large message (requires fragmentation)
 */
void test_sfp_send_large_message(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, CLIENT_PORT);
    sock->remote_node = TEST_NODE_ID;
    sock->remote_port = SERVER_PORT;
    sock->state = DANP_SOCK_ESTABLISHED;

    /* Create large message (> 127 bytes) */
    char large_msg[300];
    memset(large_msg, 'A', sizeof(large_msg));
    large_msg[sizeof(large_msg) - 1] = '\0';

    int32_t sent = danp_send_sfp(sock, large_msg, strlen(large_msg));

    /* Should succeed */
    TEST_ASSERT_EQUAL(strlen(large_msg), sent);

    danp_close(sock);
}

/**
 * @brief Test SFP send with NULL socket fails
 */
void test_sfp_send_null_socket(void)
{
    const char *msg = "Test";
    int32_t ret = danp_send_sfp(NULL, (void *)msg, strlen(msg));
    TEST_ASSERT_LESS_THAN(0, ret);
}

/**
 * @brief Test SFP send with NULL data fails
 */
void test_sfp_send_null_data(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    int32_t ret = danp_send_sfp(sock, NULL, 10);
    TEST_ASSERT_LESS_THAN(0, ret);

    danp_close(sock);
}

/**
 * @brief Test SFP send with zero length fails
 */
void test_sfp_send_zero_length(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    const char *msg = "Test";
    int32_t ret = danp_send_sfp(sock, (void *)msg, 0);
    TEST_ASSERT_LESS_THAN(0, ret);

    danp_close(sock);
}

/**
 * @brief Test SFP recv with NULL socket fails
 */
void test_sfp_recv_null_socket(void)
{
    danp_packet_t *pkt = danp_recv_sfp(NULL, 1000);
    TEST_ASSERT_NULL(pkt);
}

/**
 * @brief Test SFP recv timeout
 */
void test_sfp_recv_timeout(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);
    danp_listen(sock, 5);

    /* Try to receive with short timeout */
    danp_packet_t *pkt = danp_recv_sfp(sock, 100);
    TEST_ASSERT_NULL(pkt);

    danp_close(sock);
}

/**
 * @brief Test SFP send rejects DGRAM sockets
 */
void test_sfp_send_rejects_dgram(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    char data[200];
    memset(data, 'A', sizeof(data));

    /* SFP should reject DGRAM sockets */
    int32_t result = danp_send_sfp(sock, data, sizeof(data));
    TEST_ASSERT_EQUAL(-EINVAL, result);

    danp_close(sock);
}

/**
 * @brief Test SFP recv rejects DGRAM sockets
 */
void test_sfp_recv_rejects_dgram(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);

    danp_bind(sock, SERVER_PORT);

    /* SFP should reject DGRAM sockets */
    danp_packet_t *pkt = danp_recv_sfp(sock, 100);
    TEST_ASSERT_NULL(pkt);

    danp_close(sock);
}

/* ============================================================================
 * Packet Chaining Tests
 * ============================================================================
 */

/**
 * @brief Test packet chain iteration
 */
void test_packet_chain_iteration(void)
{
    /* Create chain of 5 packets */
    danp_packet_t *head = NULL;
    danp_packet_t *tail = NULL;

    for (int i = 0; i < 5; i++)
    {
        danp_packet_t *pkt = danp_buffer_get();
        TEST_ASSERT_NOT_NULL(pkt);

        pkt->payload[0] = 'A' + i;
        pkt->length = 1;
        pkt->next = NULL;

        if (!head)
        {
            head = pkt;
            tail = pkt;
        }
        else
        {
            tail->next = pkt;
            tail = pkt;
        }
    }

    /* Iterate and verify */
    int count = 0;
    danp_packet_t *current = head;
    while (current)
    {
        TEST_ASSERT_EQUAL('A' + count, current->payload[0]);
        count++;
        current = current->next;
    }

    TEST_ASSERT_EQUAL(5, count);

    /* Free chain */
    danp_buffer_free_chain(head);
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================
 */

int main(void)
{
    UNITY_BEGIN();

    /* Buffer management tests */
    RUN_TEST(test_buffer_get_initializes_next_to_null);
    RUN_TEST(test_buffer_get_allocates_from_pool);
    RUN_TEST(test_buffer_free_chain_frees_all_packets);
    RUN_TEST(test_buffer_free_chain_handles_null);
    RUN_TEST(test_buffer_free_chain_handles_single_packet);

    /* Zero-copy socket tests */
    RUN_TEST(test_send_packet_zero_copy);
    RUN_TEST(test_recv_packet_zero_copy);
    RUN_TEST(test_recv_packet_timeout);
    RUN_TEST(test_send_packet_to);
    RUN_TEST(test_recv_packet_from);
    RUN_TEST(test_recv_packet_from_null_args);

    /* SFP tests */
    RUN_TEST(test_sfp_send_small_message);
    RUN_TEST(test_sfp_send_large_message);
    RUN_TEST(test_sfp_send_null_socket);
    RUN_TEST(test_sfp_send_null_data);
    RUN_TEST(test_sfp_send_zero_length);
    RUN_TEST(test_sfp_recv_null_socket);
    RUN_TEST(test_sfp_recv_timeout);
    RUN_TEST(test_sfp_send_rejects_dgram);
    RUN_TEST(test_sfp_recv_rejects_dgram);

    /* Packet chaining tests */
    RUN_TEST(test_packet_chain_iteration);

    return UNITY_END();
}
