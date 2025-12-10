#include "danp/danp.h"
#include "unity.h"
#include <string.h>

// Helper to access internals if needed
extern uint32_t danpPackHeader(
    uint8_t prio,
    uint16_t dst,
    uint16_t src,
    uint8_t dstPort,
    uint8_t srcPort,
    uint8_t flags);
extern void danpUnpackHeader(
    uint32_t raw,
    uint16_t *dst,
    uint16_t *src,
    uint8_t *dstPort,
    uint8_t *srcPort,
    uint8_t *flags);

void setUp(void)
{
    danpConfig_t cfg = {.localNode = 1};
    danpInit(&cfg);
}

void tearDown(void)
{
}

void test_HeaderPacking_Should_PreserveValues(void)
{
    uint8_t prioIn = DANP_PRIORITY_HIGH;
    uint16_t dstIn = 0xAB; // 171
    uint16_t srcIn = 0x12; // 18
    uint8_t dPortIn = 45;
    uint8_t sPortIn = 12;
    uint8_t flagsIn = DANP_FLAG_SYN;

    uint32_t raw = danpPackHeader(prioIn, dstIn, srcIn, dPortIn, sPortIn, flagsIn);

    uint16_t dstOut, srcOut;
    uint8_t dPortOut, sPortOut, flagsOut;

    danpUnpackHeader(raw, &dstOut, &srcOut, &dPortOut, &sPortOut, &flagsOut);

    TEST_ASSERT_EQUAL_UINT16(dstIn, dstOut);
    TEST_ASSERT_EQUAL_UINT16(srcIn, srcOut);
    TEST_ASSERT_EQUAL_UINT8(dPortIn, dPortOut);
    TEST_ASSERT_EQUAL_UINT8(sPortIn, sPortOut);
    TEST_ASSERT_EQUAL_UINT8(flagsIn, flagsOut);
}

void test_MemoryPool_Should_AllocateUntilExhaustion(void)
{
    danpPacket_t *packets[DANP_POOL_SIZE];

    // Allocate all
    for (int i = 0; i < DANP_POOL_SIZE; i++)
    {
        packets[i] = danpAllocPacket();
        TEST_ASSERT_NOT_NULL(packets[i]);
    }

    // Next one should fail
    danpPacket_t *failPkt = danpAllocPacket();
    TEST_ASSERT_NULL(failPkt);

    // Free one
    danpFreePacket(packets[0]);

    // Should succeed again
    danpPacket_t *retryPkt = danpAllocPacket();
    TEST_ASSERT_NOT_NULL(retryPkt);

    // Cleanup
    danpFreePacket(retryPkt);
    for (int i = 1; i < DANP_POOL_SIZE; i++)
    {
        danpFreePacket(packets[i]);
    }
}

void test_HeaderPacking_Should_HandleEdgeCases(void)
{
    // Test with minimum values
    uint32_t raw1 = danpPackHeader(0, 0, 0, 0, 0, 0);
    uint16_t dst1, src1;
    uint8_t dPort1, sPort1, flags1;
    danpUnpackHeader(raw1, &dst1, &src1, &dPort1, &sPort1, &flags1);
    TEST_ASSERT_EQUAL_UINT16(0, dst1);
    TEST_ASSERT_EQUAL_UINT16(0, src1);
    TEST_ASSERT_EQUAL_UINT8(0, dPort1);
    TEST_ASSERT_EQUAL_UINT8(0, sPort1);
    TEST_ASSERT_EQUAL_UINT8(0, flags1);

    // Test with various flags
    uint32_t raw2 = danpPackHeader(DANP_PRIORITY_NORMAL, 10, 20, 30, 40, DANP_FLAG_ACK);
    uint16_t dst2, src2;
    uint8_t dPort2, sPort2, flags2;
    danpUnpackHeader(raw2, &dst2, &src2, &dPort2, &sPort2, &flags2);
    TEST_ASSERT_EQUAL_UINT8(DANP_FLAG_ACK, flags2);

    // Test with RST flag
    uint32_t raw3 = danpPackHeader(DANP_PRIORITY_HIGH, 100, 200, 5, 6, DANP_FLAG_RST);
    uint16_t dst3, src3;
    uint8_t dPort3, sPort3, flags3;
    danpUnpackHeader(raw3, &dst3, &src3, &dPort3, &sPort3, &flags3);
    TEST_ASSERT_EQUAL_UINT16(100, dst3);
    TEST_ASSERT_EQUAL_UINT16(200, src3);
    TEST_ASSERT_EQUAL_UINT8(5, dPort3);
    TEST_ASSERT_EQUAL_UINT8(6, sPort3);
    TEST_ASSERT_EQUAL_UINT8(DANP_FLAG_RST, flags3);
}

void test_PacketAllocation_Should_ReturnDifferentPackets(void)
{
    danpPacket_t *pkt1 = danpAllocPacket();
    danpPacket_t *pkt2 = danpAllocPacket();

    TEST_ASSERT_NOT_NULL(pkt1);
    TEST_ASSERT_NOT_NULL(pkt2);
    TEST_ASSERT_NOT_EQUAL(pkt1, pkt2);

    danpFreePacket(pkt1);
    danpFreePacket(pkt2);
}

void test_Init_Should_SetLocalNode(void)
{
    danpConfig_t cfg = {.localNode = 42, .logFunction = NULL};
    danpInit(&cfg);

    // After init, any socket created should use this node
    danpSocket_t *sock = danpSocket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQUAL_UINT16(42, sock->localNode);
    danpClose(sock);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_HeaderPacking_Should_PreserveValues);
    RUN_TEST(test_MemoryPool_Should_AllocateUntilExhaustion);
    RUN_TEST(test_HeaderPacking_Should_HandleEdgeCases);
    RUN_TEST(test_PacketAllocation_Should_ReturnDifferentPackets);
    RUN_TEST(test_Init_Should_SetLocalNode);
    return UNITY_END();
}