/**
 * @file testCore.c
 * @brief Core functionality tests for DANP library
 *
 * This file contains unit tests for core DANP functionality including:
 * - Header packing/unpacking operations
 * - Memory pool management
 * - Initialization and configuration
 */

#include <stdarg.h>
#include <string.h>

#include "unity.h"

#include "danp/danp.h"
#include "danp/danp_buffer.h"

static int32_t core_loopback_tx(void *iface_common, danp_packet_t *packet)
{
    (void)iface_common;
    (void)packet;
    return 0;
}

static danp_interface_t core_loopback_iface = {
    .name = "CORE_LOOPBACK",
    .mtu = 128,
    .tx_func = core_loopback_tx,
};
static bool core_iface_registered = false;

static void ensure_core_interface(uint16_t address)
{
    core_loopback_iface.address = address;
    if (!core_iface_registered)
    {
        danp_register_interface(&core_loopback_iface);
        core_iface_registered = true;
    }
}

static int core_stats_print_calls = 0;

static void counting_print(const char *fmt, ...)
{
    (void)fmt;
    va_list args;
    va_start(args, fmt);
    va_end(args);
    core_stats_print_calls++;
}

/* ============================================================================
 * External Function Declarations
 * ============================================================================
 * These functions are internal to DANP but exposed for testing purposes
 */
extern uint32_t danp_pack_header(
    uint8_t prio,
    uint16_t dst,
    uint16_t src,
    uint8_t dst_port,
    uint8_t src_port,
    uint8_t flags);

extern void danp_unpack_header(
    uint32_t raw,
    uint16_t *dst,
    uint16_t *src,
    uint8_t *dst_port,
    uint8_t *src_port,
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
    danp_config_t cfg = {.local_node = 1};
    danp_init(&cfg);
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
    uint32_t raw_header = danp_pack_header(
        prio_in, dst_in, src_in, dst_port_in, src_port_in, flags_in);

    /* Prepare variables to receive unpacked values */
    uint16_t dst_out, src_out;
    uint8_t dst_port_out, src_port_out, flags_out;

    /* Unpack the header back into individual fields */
    danp_unpack_header(raw_header, &dst_out, &src_out, &dst_port_out, &src_port_out, &flags_out);

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
    uint32_t raw_header_1 = danp_pack_header(0, 0, 0, 0, 0, 0);
    uint16_t dst_1, src_1;
    uint8_t dst_port_1, src_port_1, flags_1;

    danp_unpack_header(raw_header_1, &dst_1, &src_1, &dst_port_1, &src_port_1, &flags_1);

    TEST_ASSERT_EQUAL_UINT16(0, dst_1);
    TEST_ASSERT_EQUAL_UINT16(0, src_1);
    TEST_ASSERT_EQUAL_UINT8(0, dst_port_1);
    TEST_ASSERT_EQUAL_UINT8(0, src_port_1);
    TEST_ASSERT_EQUAL_UINT8(0, flags_1);

    /* Test Case 2: Normal priority with ACK flag */
    uint32_t raw_header_2 = danp_pack_header(DANP_PRIORITY_NORMAL, 10, 20, 30, 40, DANP_FLAG_ACK);
    uint16_t dst_2, src_2;
    uint8_t dst_port_2, src_port_2, flags_2;

    danp_unpack_header(raw_header_2, &dst_2, &src_2, &dst_port_2, &src_port_2, &flags_2);

    TEST_ASSERT_EQUAL_UINT8(DANP_FLAG_ACK, flags_2);

    /* Test Case 3: High priority with RST flag and larger node addresses */
    uint32_t raw_header_3 = danp_pack_header(DANP_PRIORITY_HIGH, 100, 200, 5, 6, DANP_FLAG_RST);
    uint16_t dst_3, src_3;
    uint8_t dst_port_3, src_port_3, flags_3;

    danp_unpack_header(raw_header_3, &dst_3, &src_3, &dst_port_3, &src_port_3, &flags_3);

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
    danp_packet_t *packets[DANP_POOL_SIZE];

    /* Step 1: Allocate all packets from the pool */
    for (int i = 0; i < DANP_POOL_SIZE; i++)
    {
        packets[i] = danp_buffer_get();
        TEST_ASSERT_NOT_NULL(packets[i]);
    }

    /* Step 2: Attempt to allocate beyond pool capacity - should fail */
    danp_packet_t *fail_packet = danp_buffer_get();
    TEST_ASSERT_NULL(fail_packet);

    /* Step 3: Free one packet to make room in the pool */
    danp_buffer_free(packets[0]);

    /* Step 4: Allocation should now succeed again */
    danp_packet_t *retry_packet = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(retry_packet);

    /* Cleanup: Free all remaining packets */
    danp_buffer_free(retry_packet);
    for (int i = 1; i < DANP_POOL_SIZE; i++)
    {
        danp_buffer_free(packets[i]);
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
    danp_packet_t *packet_1 = danp_buffer_get();
    danp_packet_t *packet_2 = danp_buffer_get();

    /* Both allocations should succeed */
    TEST_ASSERT_NOT_NULL(packet_1);
    TEST_ASSERT_NOT_NULL(packet_2);

    /* The packets should have different memory addresses */
    TEST_ASSERT_NOT_EQUAL(packet_1, packet_2);

    /* Cleanup */
    danp_buffer_free(packet_1);
    danp_buffer_free(packet_2);
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
    danp_config_t config = {.local_node = 42, .log_function = NULL};
    danp_init(&config);

    /* Create a socket and verify it has the configured local node address */
    danp_socket_t *socket = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL_UINT16(42, socket->local_node);

    /* Cleanup */
    danp_close(socket);
}

void test_danp_input_drops_short_packets(void)
{
    ensure_core_interface(1);
    TEST_ASSERT_EQUAL_UINT32(DANP_POOL_SIZE, danp_buffer_get_free_count());

    uint8_t frame[2] = {0x00, 0x00};
    danp_input(&core_loopback_iface, frame, sizeof(frame));

    TEST_ASSERT_EQUAL_UINT32(DANP_POOL_SIZE, danp_buffer_get_free_count());
}

void test_danp_input_handles_no_memory(void)
{
    ensure_core_interface(1);
    danp_packet_t *held[DANP_POOL_SIZE];

    for (uint32_t i = 0; i < DANP_POOL_SIZE; i++)
    {
        held[i] = danp_buffer_get();
        TEST_ASSERT_NOT_NULL(held[i]);
    }

    uint8_t frame[4];
    uint32_t header = danp_pack_header(DANP_PRIORITY_NORMAL, 1, 1, 1, 1, DANP_FLAG_NONE);
    memcpy(frame, &header, sizeof(header));
    danp_input(&core_loopback_iface, frame, sizeof(frame));

    TEST_ASSERT_NULL(danp_buffer_get());

    for (uint32_t i = 0; i < DANP_POOL_SIZE; i++)
    {
        danp_buffer_free(held[i]);
    }
}

void test_danp_input_drops_packets_for_other_nodes(void)
{
    ensure_core_interface(1);
    uint8_t frame[6] = {0};
    uint32_t header = danp_pack_header(DANP_PRIORITY_NORMAL, 2, 1, 1, 1, DANP_FLAG_NONE);
    memcpy(frame, &header, sizeof(header));
    frame[4] = 0xAA;
    frame[5] = 0xBB;

    danp_input(&core_loopback_iface, frame, sizeof(frame));

    TEST_ASSERT_EQUAL_UINT32(DANP_POOL_SIZE, danp_buffer_get_free_count());
}

void test_buffer_free_handles_invalid_and_double_free(void)
{
    danp_packet_t *pkt = danp_buffer_get();
    TEST_ASSERT_NOT_NULL(pkt);
    danp_buffer_free(pkt);
    danp_buffer_free(pkt);

    danp_packet_t bogus;
    danp_buffer_free(&bogus);
}

void test_buffer_get_free_count_tracks_allocations(void)
{
    danp_packet_t *first = danp_buffer_get();
    danp_packet_t *second = danp_buffer_get();
    TEST_ASSERT_EQUAL_UINT32(DANP_POOL_SIZE - 2, danp_buffer_get_free_count());

    danp_buffer_free(second);
    danp_buffer_free(first);
    TEST_ASSERT_EQUAL_UINT32(DANP_POOL_SIZE, danp_buffer_get_free_count());
}

void test_bind_rejects_invalid_port(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);
    TEST_ASSERT_EQUAL_INT32(-1, danp_bind(sock, DANP_MAX_PORTS));
    danp_close(sock);
}

void test_bind_detects_port_in_use(void)
{
    danp_socket_t *first = danp_socket(DANP_TYPE_DGRAM);
    danp_socket_t *second = danp_socket(DANP_TYPE_DGRAM);
    TEST_ASSERT_NOT_NULL(first);
    TEST_ASSERT_NOT_NULL(second);

    TEST_ASSERT_EQUAL_INT32(0, danp_bind(first, 5));
    TEST_ASSERT_EQUAL_INT32(-1, danp_bind(second, 5));

    danp_close(first);
    danp_close(second);
}

void test_danp_print_stats_invokes_callback(void)
{
    core_stats_print_calls = 0;
    danp_print_stats(counting_print);
    TEST_ASSERT_GREATER_THAN_INT(0, core_stats_print_calls);
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
    RUN_TEST(test_danp_input_drops_short_packets);
    RUN_TEST(test_danp_input_handles_no_memory);
    RUN_TEST(test_danp_input_drops_packets_for_other_nodes);
    RUN_TEST(test_buffer_free_handles_invalid_and_double_free);
    RUN_TEST(test_buffer_get_free_count_tracks_allocations);
    RUN_TEST(test_bind_rejects_invalid_port);
    RUN_TEST(test_bind_detects_port_in_use);
    RUN_TEST(test_danp_print_stats_invokes_callback);

    return UNITY_END();
}
