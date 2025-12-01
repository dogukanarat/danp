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

#include "danp/danp.h"
#include "unity.h"

/* ============================================================================
 * External Function Declarations
 * ============================================================================
 */

/**
 * @brief Initialize the mock driver for testing
 * @param node_id The local node ID for loopback operation
 */
void mockDriverInit(uint16_t node_id);

/* ============================================================================
 * Test Configuration
 * ============================================================================
 */

/* Test node and port identifiers */
#define TEST_NODE_ID 50   /* Local node ID for all tests */
#define SERVER_PORT 10    /* Default server port */
#define CLIENT_PORT 11    /* Default client port */

/* ============================================================================
 * Test Setup and Teardown
 * ============================================================================
 */

/**
 * @brief Setup function called before each test
 *
 * Initializes the DANP core and mock driver for loopback testing.
 * The mock driver simulates a network by looping back packets sent
 * to the local node, enabling synchronous handshake testing.
 */
void setUp(void)
{
    /* Initialize DANP core with test node ID */
    danpConfig_t config = {.localNode = TEST_NODE_ID};
    danpInit(&config);

    /* Initialize mock driver in loopback mode */
    mockDriverInit(TEST_NODE_ID);
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
    danpSocket_t *server_socket = danpSocket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(server_socket);
    TEST_ASSERT_EQUAL(0, danpBind(server_socket, SERVER_PORT));
    danpListen(server_socket, 5);

    /* Step 2: Create and setup client socket */
    danpSocket_t *client_socket = danpSocket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(client_socket);
    danpBind(client_socket, CLIENT_PORT);

    /* Step 3: Client initiates connection (blocking) */
    /* The mock loopback driver handles SYN -> SYN-ACK -> ACK synchronously */
    int32_t connect_result = danpConnect(client_socket, TEST_NODE_ID, SERVER_PORT);
    TEST_ASSERT_EQUAL(0, connect_result);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, client_socket->state);

    /* Step 4: Server accepts the incoming connection */
    danpSocket_t *accepted_socket = danpAccept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, accepted_socket->state);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, accepted_socket->remoteNode);
    TEST_ASSERT_EQUAL(CLIENT_PORT, accepted_socket->remotePort);

    /* Step 5: Client sends data reliably */
    const char *payload = "SecureData";
    int32_t bytes_sent = danpSend(client_socket, (void *)payload, 10);
    TEST_ASSERT_EQUAL(10, bytes_sent);

    /* Step 6: Server receives the data */
    char buffer[32];
    int32_t bytes_received = danpRecv(accepted_socket, buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, bytes_received);
    buffer[bytes_received] = 0;
    TEST_ASSERT_EQUAL_STRING("SecureData", buffer);

    /* Step 7: Verify sequence numbers (RDP internal state) */
    /* Client sent 1 packet, so transmit sequence should be 1 */
    TEST_ASSERT_EQUAL(1, client_socket->txSeq);
    /* Server received 1 packet, so expected receive sequence should be 1 */
    TEST_ASSERT_EQUAL(1, accepted_socket->rxExpectedSeq);
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
    danpSocket_t *server_socket = danpSocket(DANP_TYPE_STREAM);
    danpBind(server_socket, 12);
    danpListen(server_socket, 5);

    danpSocket_t *client_socket = danpSocket(DANP_TYPE_STREAM);
    danpBind(client_socket, 13);
    int32_t connect_result = danpConnect(client_socket, TEST_NODE_ID, 12);
    TEST_ASSERT_EQUAL(0, connect_result);

    danpSocket_t *accepted_socket = danpAccept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, accepted_socket->state);

    /* Step 2: Close client socket (triggers RST) */
    danpClose(client_socket);

    /* Step 3: Verify server socket receives RST and transitions to CLOSED */
    /* The RST packet is looped back immediately through the mock driver,
     * so the accepted socket should be CLOSED synchronously */
    TEST_ASSERT_EQUAL(DANP_SOCK_CLOSED, accepted_socket->state);

    /* Cleanup */
    danpClose(server_socket);
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
    danpSocket_t *socket = danpSocket(DANP_TYPE_STREAM);

    /* Verify socket creation succeeded with correct type and state */
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL(DANP_TYPE_STREAM, socket->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, socket->state);

    /* Bind socket to a port */
    int32_t bind_result = danpBind(socket, 99);
    TEST_ASSERT_EQUAL(0, bind_result);
    TEST_ASSERT_EQUAL_UINT16(99, socket->localPort);

    /* Put socket in listening state */
    int32_t listen_result = danpListen(socket, 5);
    TEST_ASSERT_EQUAL(0, listen_result);
    TEST_ASSERT_EQUAL(DANP_SOCK_LISTENING, socket->state);

    /* Cleanup */
    danpClose(socket);
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
    danpSocket_t *server_socket = danpSocket(DANP_TYPE_STREAM);
    danpBind(server_socket, 14);
    danpListen(server_socket, 5);

    danpSocket_t *client_socket = danpSocket(DANP_TYPE_STREAM);
    danpBind(client_socket, 15);
    danpConnect(client_socket, TEST_NODE_ID, 14);

    danpSocket_t *accepted_socket = danpAccept(server_socket, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(accepted_socket);

    /* Step 2: Client sends data to server */
    const char *client_message = "ClientData";
    int32_t client_bytes_sent = danpSend(client_socket, (void *)client_message, 10);
    TEST_ASSERT_EQUAL(10, client_bytes_sent);

    /* Step 3: Server receives data from client */
    char server_buffer[32];
    int32_t server_bytes_received = danpRecv(
        accepted_socket, server_buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, server_bytes_received);
    server_buffer[server_bytes_received] = '\0';
    TEST_ASSERT_EQUAL_STRING("ClientData", server_buffer);

    /* Step 4: Server sends data to client */
    const char *server_message = "ServerData";
    int32_t server_bytes_sent = danpSend(accepted_socket, (void *)server_message, 10);
    TEST_ASSERT_EQUAL(10, server_bytes_sent);

    /* Step 5: Client receives data from server */
    char client_buffer[32];
    int32_t client_bytes_received = danpRecv(
        client_socket, client_buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, client_bytes_received);
    client_buffer[client_bytes_received] = '\0';
    TEST_ASSERT_EQUAL_STRING("ServerData", client_buffer);

    /* Cleanup */
    danpClose(client_socket);
    danpClose(server_socket);
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
    RUN_TEST(test_stream_handshake_and_data_transfer);
    RUN_TEST(test_stream_close_triggers_rst);
    RUN_TEST(test_stream_socket_creation_and_states);
    RUN_TEST(test_stream_bidirectional_communication);

    return UNITY_END();
}
