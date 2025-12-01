/**
 * @file testCore.c
 * @brief Core functionality tests for DANP library
 *
 * This file contains unit tests for core DANP functionality including:
 * - Header packing/unpacking operations
 * - Memory pool management
 * - Initialization and configuration
 */

#include "danp/danp.h"
#include "unity.h"
#include <string.h>

/* ============================================================================
 * External Function Declarations
 * ============================================================================
 * These functions are internal to DANP but exposed for testing purposes
 */
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

/* ============================================================================
 * Test Setup and Teardown
 * ============================================================================
 */

/**
 * @brief Setup function called before each test
 *
 * Initializes the DANP library with a default local node address of 1
 */
void setUp(void)
{
    danpConfig_t cfg = {.localNode = 1};
    danpInit(&cfg);
}

/**
 * @brief Teardown function called after each test
 */
void tearDown(void)
{
    /* No cleanup needed for current tests */
}

/* ============================================================================
 * Header Packing Tests
 * ============================================================================
 */

/**
 * @brief Test that header packing and unpacking preserves all field values
 *
 * This test verifies that when we pack header fields into a 32-bit integer
 * and then unpack them, all original values are correctly preserved.
 */
void test_header_packing_preserves_values(void)
{
    /* Prepare input values for header fields */
    uint8_t prio_in = DANP_PRIORITY_HIGH;
    uint16_t dst_in = 0xAB;  /* Destination node: 171 */
    uint16_t src_in = 0x12;  /* Source node: 18 */
    uint8_t dst_port_in = 45;
    uint8_t src_port_in = 12;
    uint8_t flags_in = DANP_FLAG_SYN;

    /* Pack the header into raw 32-bit format */
    uint32_t raw_header = danpPackHeader(
        prio_in, dst_in, src_in, dst_port_in, src_port_in, flags_in);

    /* Prepare variables to receive unpacked values */
    uint16_t dst_out, src_out;
    uint8_t dst_port_out, src_port_out, flags_out;

    /* Unpack the header back into individual fields */
    danpUnpackHeader(raw_header, &dst_out, &src_out, &dst_port_out, &src_port_out, &flags_out);

    /* Verify all fields match the original input values */
    TEST_ASSERT_EQUAL_UINT16(dst_in, dst_out);
    TEST_ASSERT_EQUAL_UINT16(src_in, src_out);
    TEST_ASSERT_EQUAL_UINT8(dst_port_in, dst_port_out);
    TEST_ASSERT_EQUAL_UINT8(src_port_in, src_port_out);
    TEST_ASSERT_EQUAL_UINT8(flags_in, flags_out);
}

/**
 * @brief Test header packing with edge cases and various flag combinations
 *
 * This test verifies header packing works correctly with:
 * - Minimum values (all zeros)
 * - ACK flag
 * - RST flag with non-zero values
 */
void test_header_packing_handles_edge_cases(void)
{
    /* Test Case 1: All fields set to minimum (zero) values */
    uint32_t raw_header_1 = danpPackHeader(0, 0, 0, 0, 0, 0);
    uint16_t dst_1, src_1;
    uint8_t dst_port_1, src_port_1, flags_1;

    danpUnpackHeader(raw_header_1, &dst_1, &src_1, &dst_port_1, &src_port_1, &flags_1);

    TEST_ASSERT_EQUAL_UINT16(0, dst_1);
    TEST_ASSERT_EQUAL_UINT16(0, src_1);
    TEST_ASSERT_EQUAL_UINT8(0, dst_port_1);
    TEST_ASSERT_EQUAL_UINT8(0, src_port_1);
    TEST_ASSERT_EQUAL_UINT8(0, flags_1);

    /* Test Case 2: Normal priority with ACK flag */
    uint32_t raw_header_2 = danpPackHeader(DANP_PRIORITY_NORMAL, 10, 20, 30, 40, DANP_FLAG_ACK);
    uint16_t dst_2, src_2;
    uint8_t dst_port_2, src_port_2, flags_2;

    danpUnpackHeader(raw_header_2, &dst_2, &src_2, &dst_port_2, &src_port_2, &flags_2);

    TEST_ASSERT_EQUAL_UINT8(DANP_FLAG_ACK, flags_2);

    /* Test Case 3: High priority with RST flag and larger node addresses */
    uint32_t raw_header_3 = danpPackHeader(DANP_PRIORITY_HIGH, 100, 200, 5, 6, DANP_FLAG_RST);
    uint16_t dst_3, src_3;
    uint8_t dst_port_3, src_port_3, flags_3;

    danpUnpackHeader(raw_header_3, &dst_3, &src_3, &dst_port_3, &src_port_3, &flags_3);

    TEST_ASSERT_EQUAL_UINT16(100, dst_3);
    TEST_ASSERT_EQUAL_UINT16(200, src_3);
    TEST_ASSERT_EQUAL_UINT8(5, dst_port_3);
    TEST_ASSERT_EQUAL_UINT8(6, src_port_3);
    TEST_ASSERT_EQUAL_UINT8(DANP_FLAG_RST, flags_3);
}

/* ============================================================================
 * Memory Pool Tests
 * ============================================================================
 */

/**
 * @brief Test memory pool allocation until exhaustion
 *
 * This test verifies:
 * 1. All packets in the pool can be allocated
 * 2. Allocation fails when pool is exhausted
 * 3. Freeing a packet allows reallocation
 */
void test_memory_pool_allocates_until_exhaustion(void)
{
    danpPacket_t *packets[DANP_POOL_SIZE];

    /* Step 1: Allocate all packets from the pool */
    for (int i = 0; i < DANP_POOL_SIZE; i++)
    {
        packets[i] = danpAllocPacket();
        TEST_ASSERT_NOT_NULL(packets[i]);
    }

    /* Step 2: Attempt to allocate beyond pool capacity - should fail */
    danpPacket_t *fail_packet = danpAllocPacket();
    TEST_ASSERT_NULL(fail_packet);

    /* Step 3: Free one packet to make room in the pool */
    danpFreePacket(packets[0]);

    /* Step 4: Allocation should now succeed again */
    danpPacket_t *retry_packet = danpAllocPacket();
    TEST_ASSERT_NOT_NULL(retry_packet);

    /* Cleanup: Free all remaining packets */
    danpFreePacket(retry_packet);
    for (int i = 1; i < DANP_POOL_SIZE; i++)
    {
        danpFreePacket(packets[i]);
    }
}

/**
 * @brief Test that allocated packets have different memory addresses
 *
 * This test verifies that the memory pool returns distinct packet
 * instances rather than reusing the same memory address.
 */
void test_packet_allocation_returns_different_packets(void)
{
    /* Allocate two packets from the pool */
    danpPacket_t *packet_1 = danpAllocPacket();
    danpPacket_t *packet_2 = danpAllocPacket();

    /* Both allocations should succeed */
    TEST_ASSERT_NOT_NULL(packet_1);
    TEST_ASSERT_NOT_NULL(packet_2);

    /* The packets should have different memory addresses */
    TEST_ASSERT_NOT_EQUAL(packet_1, packet_2);

    /* Cleanup */
    danpFreePacket(packet_1);
    danpFreePacket(packet_2);
}

/* ============================================================================
 * Initialization Tests
 * ============================================================================
 */

/**
 * @brief Test that initialization correctly sets the local node address
 *
 * This test verifies that when we initialize DANP with a specific local
 * node address, all subsequently created sockets inherit that address.
 */
void test_init_sets_local_node(void)
{
    /* Initialize DANP with a specific local node address */
    danpConfig_t config = {.localNode = 42, .logFunction = NULL};
    danpInit(&config);

    /* Create a socket and verify it has the configured local node address */
    danpSocket_t *socket = danpSocket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL_UINT16(42, socket->localNode);

    /* Cleanup */
    danpClose(socket);
}

/* ============================================================================
 * Test Runner
 * ============================================================================
 */

/**
 * @brief Main test runner
 *
 * Executes all core functionality tests in sequence
 */
int main(void)
{
    UNITY_BEGIN();

    /* Header packing tests */
    RUN_TEST(test_header_packing_preserves_values);
    RUN_TEST(test_header_packing_handles_edge_cases);

    /* Memory pool tests */
    RUN_TEST(test_memory_pool_allocates_until_exhaustion);
    RUN_TEST(test_packet_allocation_returns_different_packets);

    /* Initialization tests */
    RUN_TEST(test_init_sets_local_node);

    return UNITY_END();
}
