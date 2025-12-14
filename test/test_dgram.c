/**
 * @file testDgram.c
 * @brief DGRAM (datagram) socket tests for DANP library
 *
 * This file contains unit tests for DANP datagram sockets including:
 * - Socket creation and binding
 * - Unreliable message transmission (UDP-like)
 * - Multiple message handling
 * - Loopback communication
 */

#include "danp/danp.h"
#include "unity.h"
#include <stdio.h>
#include <string.h>

/* ============================================================================
 * Test Configuration
 * ============================================================================
 */

/* Test node and port identifiers */
#define TEST_NODE_ID 10  /* Local node ID for all tests */
#define PORT_A 20        /* First test port */
#define PORT_B 21        /* Second test port */

static danp_interface_t loopback_iface;
static bool loopback_registered = false;

static int32_t loopback_tx(void *iface_common, danp_packet_t *packet)
{
    danp_interface_t *iface = (danp_interface_t *)iface_common;
    uint8_t buffer[DANP_HEADER_SIZE + DANP_MAX_PACKET_SIZE];

    memcpy(buffer, &packet->header_raw, DANP_HEADER_SIZE);
    if (packet->length > 0)
    {
        memcpy(buffer + DANP_HEADER_SIZE, packet->payload, packet->length);
    }

    danp_input(iface, buffer, DANP_HEADER_SIZE + packet->length);
    return 0;
}

static void setup_loopback_interface(void)
{
    if (!loopback_registered)
    {
        memset(&loopback_iface, 0, sizeof(loopback_iface));
        loopback_iface.name = "TEST_LOOPBACK_DGRAM";
        loopback_iface.address = TEST_NODE_ID;
        loopback_iface.mtu = 128;
        loopback_iface.tx_func = loopback_tx;
        loopback_iface.next = NULL;
        danp_register_interface(&loopback_iface);
        loopback_registered = true;
    }

    char route_entry[32];
    int written = snprintf(route_entry, sizeof(route_entry), "%u:%s", TEST_NODE_ID, loopback_iface.name);
    TEST_ASSERT_TRUE(written > 0 && written < (int)sizeof(route_entry));
    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load(route_entry));
}
/* ============================================================================
 * Test Setup and Teardown
 * ============================================================================
 */

/**
 * @brief Setup function called before each test
 *
 * Initializes the DANP core and registers a loopback interface for testing.
 * The loopback interface simulates a network by feeding packets back
 * to the local node.
 */
void setUp(void)
{
    /* Initialize DANP core with test node ID */
    danp_config_t config = {.local_node = TEST_NODE_ID};
    danp_init(&config);

    setup_loopback_interface();
}

/**
 * @brief Teardown function called after each test
 */
void tearDown(void)
{
    /* No cleanup needed for current tests */
}

/* ============================================================================
 * DGRAM Socket Tests
 * ============================================================================
 */

/**
 * @brief Test basic datagram send/receive on same node via loopback
 *
 * Scenario:
 * 1. Create two sockets (A and B) bound to different ports
 * 2. Socket A sends a message to socket B
 * 3. Socket B receives the message
 * 4. Verify message content and sender information
 */
void test_dgram_send_recv_same_node(void)
{
    /* Create and bind socket A */
    danp_socket_t *socket_a = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket_a, PORT_A);

    /* Create and bind socket B */
    danp_socket_t *socket_b = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket_b, PORT_B);

    /* Send message from socket A to socket B */
    const char *message = "HelloUnity";
    int32_t bytes_sent = danp_send_to(socket_a, (void *)message, 10, TEST_NODE_ID, PORT_B);
    TEST_ASSERT_EQUAL(10, bytes_sent);

    /* Receive message at socket B */
    char buffer[32];
    uint16_t source_node, source_port;

    /* Note: Mock driver is synchronous, so message is immediately available */
    int32_t bytes_received = danp_recv_from(
        socket_b, buffer, 32, &source_node, &source_port, DANP_WAIT_FOREVER);

    /* Verify received data matches sent data */
    TEST_ASSERT_EQUAL(10, bytes_received);
    buffer[bytes_received] = '\0';
    TEST_ASSERT_EQUAL_STRING("HelloUnity", buffer);

    /* Verify sender information is correct */
    TEST_ASSERT_EQUAL(TEST_NODE_ID, source_node);
    TEST_ASSERT_EQUAL(PORT_A, source_port);

    /* Cleanup sockets */
    danp_close(socket_a);
    danp_close(socket_b);
}

/**
 * @brief Test sending and receiving multiple messages in sequence
 *
 * This test verifies that the socket can handle multiple messages
 * queued up for reception without losing data or corrupting messages.
 */
void test_dgram_multiple_messages(void)
{
    /* Create and bind sockets */
    danp_socket_t *socket_a = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket_a, PORT_A);

    danp_socket_t *socket_b = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket_b, PORT_B);

    /* Send three different messages in sequence */
    const char *message_1 = "First";
    const char *message_2 = "Second";
    const char *message_3 = "Third";

    danp_send_to(socket_a, (void *)message_1, 5, TEST_NODE_ID, PORT_B);
    danp_send_to(socket_a, (void *)message_2, 6, TEST_NODE_ID, PORT_B);
    danp_send_to(socket_a, (void *)message_3, 5, TEST_NODE_ID, PORT_B);

    /* Receive and verify all three messages */
    char buffer[32];
    uint16_t source_node, source_port;

    /* Receive first message */
    int32_t received_1 = danp_recv_from(
        socket_b, buffer, 32, &source_node, &source_port, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(5, received_1);
    buffer[received_1] = '\0';
    TEST_ASSERT_EQUAL_STRING("First", buffer);

    /* Receive second message */
    int32_t received_2 = danp_recv_from(
        socket_b, buffer, 32, &source_node, &source_port, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(6, received_2);
    buffer[received_2] = '\0';
    TEST_ASSERT_EQUAL_STRING("Second", buffer);

    /* Receive third message */
    int32_t received_3 = danp_recv_from(
        socket_b, buffer, 32, &source_node, &source_port, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(5, received_3);
    buffer[received_3] = '\0';
    TEST_ASSERT_EQUAL_STRING("Third", buffer);

    /* Cleanup sockets */
    danp_close(socket_a);
    danp_close(socket_b);
}

/**
 * @brief Test socket creation and port binding
 *
 * This test verifies:
 * 1. Socket can be created with DGRAM type
 * 2. Socket starts in OPEN state
 * 3. Socket can be bound to a specific port
 * 4. Port binding updates socket's local port field
 */
void test_dgram_socket_creation_and_binding(void)
{
    /* Create a datagram socket */
    danp_socket_t *socket = danp_socket(DANP_TYPE_DGRAM);

    /* Verify socket was created successfully */
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL(DANP_TYPE_DGRAM, socket->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, socket->state);

    /* Bind socket to a specific port */
    /* Use a port within DANP_MAX_PORTS (64) so binding is valid */
    const uint16_t test_port = 40;
    int32_t bind_result = danp_bind(socket, test_port);

    /* Verify binding succeeded and port was set */
    TEST_ASSERT_EQUAL(0, bind_result);
    TEST_ASSERT_EQUAL_UINT16(test_port, socket->local_port);

    /* Cleanup */
    danp_close(socket);
}

void test_dgram_send_to_rejects_large_payload(void)
{
    danp_socket_t *socket = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket, PORT_A);

    uint8_t payload[DANP_MAX_PACKET_SIZE];
    setup_loopback_interface();
    int32_t rc = danp_send_to(socket, payload, DANP_MAX_PACKET_SIZE, TEST_NODE_ID, PORT_B);
    TEST_ASSERT_EQUAL_INT32(-1, rc);

    danp_close(socket);
}

void test_dgram_recv_timeout_returns_error(void)
{
    danp_socket_t *socket = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(socket, PORT_A);

    char buffer[8];
    uint16_t src_node = 0;
    uint16_t src_port = 0;
    int32_t rc = danp_recv_from(socket, buffer, sizeof(buffer), &src_node, &src_port, 0);
    TEST_ASSERT_EQUAL_INT32(-1, rc);

    danp_close(socket);
}

/* ============================================================================
 * Test Runner
 * ============================================================================
 */

/**
 * @brief Main test runner
 *
 * Executes all DGRAM socket tests in sequence
 */
int main(void)
{
    UNITY_BEGIN();

    /* Run all DGRAM socket tests */
    RUN_TEST(test_dgram_send_recv_same_node);
    RUN_TEST(test_dgram_multiple_messages);
    RUN_TEST(test_dgram_socket_creation_and_binding);
    RUN_TEST(test_dgram_send_to_rejects_large_payload);
    RUN_TEST(test_dgram_recv_timeout_returns_error);

    return UNITY_END();
}
