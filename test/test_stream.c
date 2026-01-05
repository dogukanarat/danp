/**
 * @file testStream.c
 * @brief STREAM (reliable) socket tests for DANP library
 *
 * This file contains unit tests for DANP stream sockets including:
 * - Socket creation and state transitions
 * - Connection establishment (three-way handshake)
 * - Reliable data transfer
 * - Connection termination (RST)
 * - Bidirectional communication
 */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "danp/danp.h"

/* ============================================================================
 * Test Configuration
 * ============================================================================
 */

/* Test node and port identifiers */
#define TEST_NODE_ID 50   /* Local node ID for all tests */
#define SERVER_PORT 10    /* Default server port */
#define CLIENT_PORT 11    /* Default client port */

void danp_log_message_with_func_name(
    danp_log_level_t level,
    const char *func_name,
    const char *message,
    va_list args);

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
        loopback_iface.name = "TEST_LOOPBACK_STREAM";
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
 * Test Enable Flags (set to 1 to run, 0 to skip)
 * ============================================================================
 */
#define ENABLE_TEST_STREAM_HANDSHAKE 1
#define ENABLE_TEST_STREAM_CLOSE_RST 1
#define ENABLE_TEST_STREAM_SOCKET_STATES 1
#define ENABLE_TEST_STREAM_BIDIRECTIONAL 1

/* ============================================================================
 * Test Setup and Teardown
 * ============================================================================
 */

/**
 * @brief Setup function called before each test
 *
 * Initializes the DANP core and registers a loopback interface for testing.
 * The loopback interface feeds packets back to the local node, enabling
 * synchronous handshake testing.
 */
void setUp(void)
{
    /* Initialize DANP core with test node ID */
    danp_config_t config = {.local_node = TEST_NODE_ID, .log_function = danp_log_message_with_func_name};
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
 * STREAM Socket Tests
 * ============================================================================
 */

/**
 * @brief Test complete stream connection handshake and data transfer
 *
 * This test verifies the entire reliable stream protocol:
 * 1. Server socket listens on a port
 * 2. Client socket connects (SYN -> SYN-ACK -> ACK handshake)
 * 3. Server accepts the connection
 * 4. Client sends data reliably
 * 5. Server receives data
 * 6. Sequence numbers are correctly maintained
 *
 * Note: Mock driver handles the handshake synchronously via loopback
 */
void test_stream_handshake_and_data_transfer(void)
{
    /* Step 1: Create and setup server socket */
    danp_socket_t *server_socket = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(server_socket);
    TEST_ASSERT_EQUAL(0, danp_bind(server_socket, SERVER_PORT));
    danp_listen(server_socket, 5);

    /* Step 2: Create and setup client socket */
    danp_socket_t *client_socket = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(client_socket);
    danp_bind(client_socket, CLIENT_PORT);

    /* Step 3: Client initiates connection (blocking) */
    /* The mock loopback driver handles SYN -> SYN-ACK -> ACK synchronously */
    int32_t connect_result = danp_connect(client_socket, TEST_NODE_ID, SERVER_PORT);
    TEST_ASSERT_EQUAL(0, connect_result);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, client_socket->state);

    /* Step 4: Server accepts the incoming connection */
    danp_socket_t *accepted_socket = danp_accept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, accepted_socket->state);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, accepted_socket->remote_node);
    TEST_ASSERT_EQUAL(CLIENT_PORT, accepted_socket->remote_port);

    /* Step 5: Client sends data reliably */
    const char *payload = "SecureData";
    int32_t bytes_sent = danp_send(client_socket, (void *)payload, 10);
    TEST_ASSERT_EQUAL(10, bytes_sent);

    /* Step 6: Server receives the data */
    char buffer[32];
    int32_t bytes_received = danp_recv(accepted_socket, buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, bytes_received);
    buffer[bytes_received] = 0;
    TEST_ASSERT_EQUAL_STRING("SecureData", buffer);

    /* Step 7: Verify sequence numbers (RDP internal state) */
    /* Client sent 1 packet, so transmit sequence should be 1 */
    TEST_ASSERT_EQUAL(1, client_socket->tx_seq);
    /* Server received 1 packet, so expected receive sequence should be 1 */
    TEST_ASSERT_EQUAL(1, accepted_socket->rx_expected_seq);
}

/**
 * @brief Test that closing a socket triggers RST and closes peer socket
 *
 * This test verifies the connection reset (RST) mechanism:
 * 1. Establish a connection between client and server
 * 2. Close the client socket (sends RST)
 * 3. Verify the server socket is automatically closed
 *
 * Note: The RST packet is looped back immediately and processed
 * synchronously, so the peer socket state changes instantly.
 */
void test_stream_close_triggers_rst(void)
{
    /* Step 1: Create and connect sockets */
    danp_socket_t *server_socket = danp_socket(DANP_TYPE_STREAM);
    danp_bind(server_socket, 12);
    danp_listen(server_socket, 5);

    danp_socket_t *client_socket = danp_socket(DANP_TYPE_STREAM);
    danp_bind(client_socket, 13);
    int32_t connect_result = danp_connect(client_socket, TEST_NODE_ID, 12);
    TEST_ASSERT_EQUAL(0, connect_result);

    danp_socket_t *accepted_socket = danp_accept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, accepted_socket->state);

    /* Step 2: Close client socket (triggers RST) */
    danp_close(client_socket);

    /* Step 3: Verify server socket receives RST and transitions to CLOSED */
    /* The RST packet is looped back immediately through the mock driver,
     * so the accepted socket should be CLOSED synchronously */
    TEST_ASSERT_EQUAL(DANP_SOCK_CLOSED, accepted_socket->state);

    /* Cleanup */
    danp_close(server_socket);
}

/**
 * @brief Test socket creation and state transitions
 *
 * This test verifies proper socket lifecycle management:
 * 1. Socket creation with correct type
 * 2. Initial state is OPEN
 * 3. Port binding succeeds
 * 4. Listen transitions socket to LISTENING state
 */
void test_stream_socket_creation_and_states(void)
{
    /* Create a stream socket */
    danp_socket_t *socket = danp_socket(DANP_TYPE_STREAM);

    /* Verify socket creation succeeded with correct type and state */
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL(DANP_TYPE_STREAM, socket->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, socket->state);

    /* Bind socket to a valid port within DANP_MAX_PORTS */
    const uint16_t test_port = 30;
    int32_t bind_result = danp_bind(socket, test_port);
    TEST_ASSERT_EQUAL(0, bind_result);
    TEST_ASSERT_EQUAL_UINT16(test_port, socket->local_port);

    /* Put socket in listening state */
    int32_t listen_result = danp_listen(socket, 5);
    TEST_ASSERT_EQUAL(0, listen_result);
    TEST_ASSERT_EQUAL(DANP_SOCK_LISTENING, socket->state);

    /* Cleanup */
    danp_close(socket);
}

/**
 * @brief Test bidirectional communication over stream sockets
 *
 * This test verifies that both peers can send and receive data:
 * 1. Establish connection
 * 2. Client sends data to server
 * 3. Server receives client's data
 * 4. Server sends data to client
 * 5. Client receives server's data
 *
 * This ensures full-duplex communication works correctly.
 */
void test_stream_bidirectional_communication(void)
{
    /* Step 1: Setup and establish connection */
    danp_socket_t *server_socket = danp_socket(DANP_TYPE_STREAM);
    danp_bind(server_socket, 14);
    danp_listen(server_socket, 5);

    danp_socket_t *client_socket = danp_socket(DANP_TYPE_STREAM);
    danp_bind(client_socket, 15);
    danp_connect(client_socket, TEST_NODE_ID, 14);

    danp_socket_t *accepted_socket = danp_accept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);

    /* Step 2: Client sends data to server */
    const char *client_message = "ClientData";
    int32_t client_bytes_sent = danp_send(client_socket, (void *)client_message, 10);
    TEST_ASSERT_EQUAL(10, client_bytes_sent);

    /* Step 3: Server receives data from client */
    char server_buffer[32];
    int32_t server_bytes_received = danp_recv(
        accepted_socket, server_buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, server_bytes_received);
    server_buffer[server_bytes_received] = '\0';
    TEST_ASSERT_EQUAL_STRING("ClientData", server_buffer);

    /* Step 4: Server sends data to client */
    const char *server_message = "ServerData";
    int32_t server_bytes_sent = danp_send(accepted_socket, (void *)server_message, 10);
    TEST_ASSERT_EQUAL(10, server_bytes_sent);

    /* Step 5: Client receives data from server */
    char client_buffer[32];
    int32_t client_bytes_received = danp_recv(
        client_socket, client_buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, client_bytes_received);
    client_buffer[client_bytes_received] = '\0';
    TEST_ASSERT_EQUAL_STRING("ServerData", client_buffer);

    /* Cleanup */
    danp_close(client_socket);
    danp_close(server_socket);
}

void test_stream_accept_timeout_returns_null(void)
{
    danp_socket_t *server_socket = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(server_socket);
    TEST_ASSERT_EQUAL_INT32(0, danp_bind(server_socket, SERVER_PORT));
    danp_listen(server_socket, 1);

    danp_socket_t *client_socket = danp_accept(server_socket, 0);
    TEST_ASSERT_NULL(client_socket);

    danp_close(server_socket);
}

/* ============================================================================
 * Test Runner
 * ============================================================================
 */

/**
 * @brief Main test runner
 *
 * Executes all STREAM socket tests in sequence
 */
int main(void)
{
    UNITY_BEGIN();

    /* Run all STREAM socket tests */
#if ENABLE_TEST_STREAM_HANDSHAKE
    RUN_TEST(test_stream_handshake_and_data_transfer);
#endif
#if ENABLE_TEST_STREAM_CLOSE_RST
    RUN_TEST(test_stream_close_triggers_rst);
#endif
#if ENABLE_TEST_STREAM_SOCKET_STATES
    RUN_TEST(test_stream_socket_creation_and_states);
#endif
#if ENABLE_TEST_STREAM_BIDIRECTIONAL
    RUN_TEST(test_stream_bidirectional_communication);
#endif
#if ENABLE_TEST_STREAM_BIDIRECTIONAL
    RUN_TEST(test_stream_accept_timeout_returns_null);
#endif

    return UNITY_END();
}
