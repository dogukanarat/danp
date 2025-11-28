#include "unity.h"
#include "danp/danp.h"
#include <string.h>

// Helper to access internals if needed
extern uint32_t danpPackHeader(uint8_t prio, uint16_t dst, uint16_t src, uint8_t dstPort, uint8_t srcPort, uint8_t flags);
extern void danpUnpackHeader(uint32_t raw, uint16_t* dst, uint16_t* src, uint8_t* dstPort, uint8_t* srcPort, uint8_t* flags);

void setUp(void) {
    danpConfig_t cfg = { .localNode = 1 };
    danpInit(&cfg);
}

void tearDown(void) {
}

void test_HeaderPacking_Should_PreserveValues(void) {
    uint8_t  prioIn = DANP_PRIORITY_HIGH;
    uint16_t dstIn  = 0xAB; // 171
    uint16_t srcIn  = 0x12; // 18
    uint8_t  dPortIn = 45;
    uint8_t  sPortIn = 12;
    uint8_t  flagsIn = DANP_FLAG_SYN;

    uint32_t raw = danpPackHeader(prioIn, dstIn, srcIn, dPortIn, sPortIn, flagsIn);

    uint16_t dstOut, srcOut;
    uint8_t  dPortOut, sPortOut, flagsOut;

    danpUnpackHeader(raw, &dstOut, &srcOut, &dPortOut, &sPortOut, &flagsOut);

    TEST_ASSERT_EQUAL_UINT16(dstIn, dstOut);
    TEST_ASSERT_EQUAL_UINT16(srcIn, srcOut);
    TEST_ASSERT_EQUAL_UINT8(dPortIn, dPortOut);
    TEST_ASSERT_EQUAL_UINT8(sPortIn, sPortOut);
    TEST_ASSERT_EQUAL_UINT8(flagsIn, flagsOut);
}

void test_MemoryPool_Should_AllocateUntilExhaustion(void) {
    danpPacket_t* packets[DANP_POOL_SIZE];
    
    // Allocate all
    for(int i=0; i < DANP_POOL_SIZE; i++) {
        packets[i] = danpAllocPacket();
        TEST_ASSERT_NOT_NULL(packets[i]);
    }

    // Next one should fail
    danpPacket_t* failPkt = danpAllocPacket();
    TEST_ASSERT_NULL(failPkt);

    // Free one
    danpFreePacket(packets[0]);

    // Should succeed again
    danpPacket_t* retryPkt = danpAllocPacket();
    TEST_ASSERT_NOT_NULL(retryPkt);
    
    // Cleanup
    danpFreePacket(retryPkt);
    for(int i=1; i < DANP_POOL_SIZE; i++) {
        danpFreePacket(packets[i]);
    }
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_HeaderPacking_Should_PreserveValues);
    RUN_TEST(test_MemoryPool_Should_AllocateUntilExhaustion);
    return UNITY_END();
}