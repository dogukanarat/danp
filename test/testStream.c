#include "danp/danp.h"
#include "unity.h"


void mockDriverInit(uint16_t nodeId);

#define TEST_NODE_ID 50
#define SERVER_PORT 10
#define CLIENT_PORT 11

void setUp(void)
{
    danpConfig_t cfg = {.localNode = TEST_NODE_ID};
    danpInit(&cfg);
    mockDriverInit(TEST_NODE_ID);
}

void tearDown(void)
{
}

void testStreamHandshakeAndDataTransfer(void)
{
    // 1. Create Server
    danpSocket_t *serverSock = danpSocket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(serverSock);
    TEST_ASSERT_EQUAL(0, danpBind(serverSock, SERVER_PORT));
    danpListen(serverSock, 5);

    // 2. Create Client
    danpSocket_t *clientSock = danpSocket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(clientSock);
    danpBind(clientSock, CLIENT_PORT); // Bind specific port to predict srcPort

    // 3. Client Connects (Blocking)
    // The Mock Loopback driver handles the SYN -> SYN-ACK -> ACK cycle synchronously!
    int32_t connRes = danpConnect(clientSock, TEST_NODE_ID, SERVER_PORT);
    TEST_ASSERT_EQUAL(0, connRes);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, clientSock->state);

    // 4. Server Accepts
    danpSocket_t *acceptedSock = danpAccept(serverSock, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(acceptedSock);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, acceptedSock->state);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, acceptedSock->remoteNode);
    TEST_ASSERT_EQUAL(CLIENT_PORT, acceptedSock->remotePort);

    // 5. Send Data Client -> Server (Reliable)
    const char *payload = "SecureData";
    int32_t sent = danpSend(clientSock, (void *)payload, 10);
    TEST_ASSERT_EQUAL(10, sent);

    // 6. Server Recv
    char buffer[32];
    int32_t received = danpRecv(acceptedSock, buffer, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, received);
    buffer[received] = 0;
    TEST_ASSERT_EQUAL_STRING("SecureData", buffer);

    // 7. Verify Sequence Numbers (RDP Internal Check)
    // Client sent 1 packet, so TxSeq should be 1
    TEST_ASSERT_EQUAL(1, clientSock->txSeq);
    // Server received 1 packet, so ExpectedSeq should be 1
    TEST_ASSERT_EQUAL(1, acceptedSock->rxExpectedSeq);
}

void testStreamCloseTriggersRst(void)
{
    // 1. Setup Connected Pair
    danpSocket_t *serverSock = danpSocket(DANP_TYPE_STREAM);
    danpBind(serverSock, 12);
    danpListen(serverSock, 5);

    danpSocket_t *clientSock = danpSocket(DANP_TYPE_STREAM);
    danpBind(clientSock, 13);
    int32_t res = danpConnect(clientSock, TEST_NODE_ID, 12);
    TEST_ASSERT_EQUAL(0, res);

    danpSocket_t *acceptedSock = danpAccept(serverSock, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(acceptedSock);
    TEST_ASSERT_EQUAL(DANP_SOCK_ESTABLISHED, acceptedSock->state);

    // 2. Close Client
    danpClose(clientSock);

    // 3. Verify Server Socket receives RST and Closes
    // The RST packet is loopbacked immediately.
    // However, we need to pump the input or wait?
    // The mock driver is synchronous, so danpInput is called within danpClose->danpSendControl->...->mockTx
    // So acceptedSock should be CLOSED immediately.

    TEST_ASSERT_EQUAL(DANP_SOCK_CLOSED, acceptedSock->state);

    // Cleanup
    danpClose(serverSock);
}

void testStreamSocketCreationAndStates(void)
{
    // Test socket creation and state transitions

    danpSocket_t *sock = danpSocket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQUAL(DANP_TYPE_STREAM, sock->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, sock->state);

    // Bind to port
    int32_t bindRes = danpBind(sock, 99);
    TEST_ASSERT_EQUAL(0, bindRes);
    TEST_ASSERT_EQUAL_UINT16(99, sock->localPort);

    // Listen
    int32_t listenRes = danpListen(sock, 5);
    TEST_ASSERT_EQUAL(0, listenRes);
    TEST_ASSERT_EQUAL(DANP_SOCK_LISTENING, sock->state);

    danpClose(sock);
}

void testStreamBidirectionalCommunication(void)
{
    // Test bidirectional data transfer

    danpSocket_t *serverSock = danpSocket(DANP_TYPE_STREAM);
    danpBind(serverSock, 14);
    danpListen(serverSock, 5);

    danpSocket_t *clientSock = danpSocket(DANP_TYPE_STREAM);
    danpBind(clientSock, 15);
    danpConnect(clientSock, TEST_NODE_ID, 14);

    danpSocket_t *acceptedSock = danpAccept(serverSock, DANP_WAIT_FOREVER);
    TEST_ASSERT_NOT_NULL(acceptedSock);

    // Client sends to server
    const char *clientMsg = "ClientData";
    int32_t sent1 = danpSend(clientSock, (void *)clientMsg, 10);
    TEST_ASSERT_EQUAL(10, sent1);

    char buffer1[32];
    int32_t recv1 = danpRecv(acceptedSock, buffer1, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, recv1);
    buffer1[recv1] = '\0';
    TEST_ASSERT_EQUAL_STRING("ClientData", buffer1);

    // Server sends to client
    const char *serverMsg = "ServerData";
    int32_t sent2 = danpSend(acceptedSock, (void *)serverMsg, 10);
    TEST_ASSERT_EQUAL(10, sent2);

    char buffer2[32];
    int32_t recv2 = danpRecv(clientSock, buffer2, 32, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(10, recv2);
    buffer2[recv2] = '\0';
    TEST_ASSERT_EQUAL_STRING("ServerData", buffer2);

    danpClose(clientSock);
    danpClose(serverSock);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(testStreamHandshakeAndDataTransfer);
    RUN_TEST(testStreamCloseTriggersRst);
    RUN_TEST(testStreamSocketCreationAndStates);
    RUN_TEST(testStreamBidirectionalCommunication);
    return UNITY_END();
}
