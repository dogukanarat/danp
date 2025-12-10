#include "danp/danp.h"
#include "unity.h"

// Forward declare mock init
void mockDriverInit(uint16_t nodeId);

#define TEST_NODE_ID 10
#define PORT_A 20
#define PORT_B 21

void setUp(void)
{
    // 1. Init Core
    danpConfig_t cfg = {.localNode = TEST_NODE_ID};
    danpInit(&cfg);

    // 2. Init Mock Driver (Loopback mode)
    // Any packet sent to TEST_NODE_ID will be fed back to RX
    mockDriverInit(TEST_NODE_ID);
}

void tearDown(void)
{
}

void testDgramSendRecvSameNode(void)
{
    // Scenario: Socket A sends to Socket B (both on same node via loopback)

    // 1. Create Sockets
    danpSocket_t *sockA = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockA, PORT_A);

    danpSocket_t *sockB = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockB, PORT_B);

    // 2. Send A -> B
    const char *msg = "HelloUnity";
    int32_t sent = danpSendTo(sockA, (void *)msg, 10, TEST_NODE_ID, PORT_B);
    TEST_ASSERT_EQUAL(10, sent);

    // 3. Receive at B
    char buffer[32];
    uint16_t srcNode, srcPort;
    // We expect immediate availability because Mock Driver is synchronous
    int32_t received = danpRecvFrom(sockB, buffer, 32, &srcNode, &srcPort, DANP_WAIT_FOREVER);

    TEST_ASSERT_EQUAL(10, received);
    buffer[received] = '\0';
    TEST_ASSERT_EQUAL_STRING("HelloUnity", buffer);

    // Verify Sender Info
    TEST_ASSERT_EQUAL(TEST_NODE_ID, srcNode);
    TEST_ASSERT_EQUAL(PORT_A, srcPort);

    // Cleanup
    danpClose(sockA);
    danpClose(sockB);
}

void testDgramMultipleMessages(void)
{
    // Test sending multiple messages in sequence

    danpSocket_t *sockA = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockA, PORT_A);

    danpSocket_t *sockB = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockB, PORT_B);

    // Send multiple messages
    const char *msg1 = "First";
    const char *msg2 = "Second";
    const char *msg3 = "Third";

    danpSendTo(sockA, (void *)msg1, 5, TEST_NODE_ID, PORT_B);
    danpSendTo(sockA, (void *)msg2, 6, TEST_NODE_ID, PORT_B);
    danpSendTo(sockA, (void *)msg3, 5, TEST_NODE_ID, PORT_B);

    // Receive all messages
    char buffer[32];
    uint16_t srcNode, srcPort;

    int32_t r1 = danpRecvFrom(sockB, buffer, 32, &srcNode, &srcPort, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(5, r1);
    buffer[r1] = '\0';
    TEST_ASSERT_EQUAL_STRING("First", buffer);

    int32_t r2 = danpRecvFrom(sockB, buffer, 32, &srcNode, &srcPort, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(6, r2);
    buffer[r2] = '\0';
    TEST_ASSERT_EQUAL_STRING("Second", buffer);

    int32_t r3 = danpRecvFrom(sockB, buffer, 32, &srcNode, &srcPort, DANP_WAIT_FOREVER);
    TEST_ASSERT_EQUAL(5, r3);
    buffer[r3] = '\0';
    TEST_ASSERT_EQUAL_STRING("Third", buffer);

    danpClose(sockA);
    danpClose(sockB);
}

void testDgramSocketCreationAndBinding(void)
{
    // Test basic socket creation and binding

    danpSocket_t *sock = danpSocket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQUAL(DANP_TYPE_DGRAM, sock->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, sock->state);

    // Bind to a port
    int32_t bindResult = danpBind(sock, 100);
    TEST_ASSERT_EQUAL(0, bindResult);
    TEST_ASSERT_EQUAL_UINT16(100, sock->localPort);

    danpClose(sock);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(testDgramSendRecvSameNode);
    RUN_TEST(testDgramMultipleMessages);
    RUN_TEST(testDgramSocketCreationAndBinding);
    return UNITY_END();
}
