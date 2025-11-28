#include "unity.h"
#include "danp/danp.h"

// Forward declare mock init
void mockDriverInit(uint16_t nodeId);

#define TEST_NODE_ID 10
#define PORT_A 20
#define PORT_B 21

void setUp(void) {
    // 1. Init Core
    danpConfig_t cfg = { .localNode = TEST_NODE_ID };
    danpInit(&cfg);
    
    // 2. Init Mock Driver (Loopback mode)
    // Any packet sent to TEST_NODE_ID will be fed back to RX
    mockDriverInit(TEST_NODE_ID);
}

void tearDown(void) {
}

void test_DGRAM_SendRecv_SameNode(void) {
    // Scenario: Socket A sends to Socket B (both on same node via loopback)
    
    // 1. Create Sockets
    danpSocket_t* sockA = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockA, PORT_A);

    danpSocket_t* sockB = danpSocket(DANP_TYPE_DGRAM);
    danpBind(sockB, PORT_B);

    // 2. Send A -> B
    const char* msg = "HelloUnity";
    int32_t sent = danpSendTo(sockA, (void*)msg, 10, TEST_NODE_ID, PORT_B);
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
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_DGRAM_SendRecv_SameNode);
    return UNITY_END();
}