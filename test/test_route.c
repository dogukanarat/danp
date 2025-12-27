/**
 * @file test_route.c
 * @brief Unit tests for the static routing table logic.
 */

#include "unity.h"
 
#include "danp/danp.h"

/* ============================================================================
 * Test Interface Configuration
 * ============================================================================
 */

static danp_interface_t iface_a;
static danp_interface_t iface_b;
static bool interfaces_registered = false;

static uint32_t iface_a_tx_count = 0;
static uint32_t iface_b_tx_count = 0;

static int32_t iface_a_tx(void *iface_common, danp_packet_t *packet)
{
    (void)iface_common;
    (void)packet;
    iface_a_tx_count++;
    return 0;
}

static int32_t iface_b_tx(void *iface_common, danp_packet_t *packet)
{
    (void)iface_common;
    (void)packet;
    iface_b_tx_count++;
    return 0;
}

static void init_test_interfaces(void)
{
    if (!interfaces_registered)
    {
        iface_a.name = "IFACE_A";
        iface_a.address = 1;
        iface_a.mtu = 128;
        iface_a.tx_func = iface_a_tx;
        iface_a.next = NULL;

        iface_b.name = "IFACE_B";
        iface_b.address = 2;
        iface_b.mtu = 128;
        iface_b.tx_func = iface_b_tx;
        iface_b.next = NULL;

        danp_register_interface(&iface_a);
        danp_register_interface(&iface_b);
        interfaces_registered = true;
    }
}

static void prepare_packet(danp_packet_t *pkt, uint16_t dest_node, uint16_t payload_len)
{
    memset(pkt, 0, sizeof(*pkt));
    pkt->header_raw = danp_pack_header(
        DANP_PRIORITY_NORMAL,
        dest_node,
        /* src */ 1,
        /* dst_port */ 10,
        /* src_port */ 20,
        DANP_FLAG_NONE);
    pkt->length = payload_len;
}

/* ============================================================================
 * Test Setup / Teardown
 * ============================================================================
 */

void setUp(void)
{
    danp_config_t cfg = {.local_node = 1};
    danp_init(&cfg);

    iface_a.mtu = 128;
    iface_b.mtu = 128;
    iface_a_tx_count = 0;
    iface_b_tx_count = 0;

    init_test_interfaces();
    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load(""));
}

void tearDown(void)
{
    /* Nothing to clean up between tests */
}

/* ============================================================================
 * Tests
 * ============================================================================
 */

void test_route_table_routes_packets_over_registered_interfaces(void)
{
    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load("100:IFACE_A, 200:IFACE_B\n150:IFACE_A"));

    danp_packet_t pkt_a;
    prepare_packet(&pkt_a, 100, 16);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt_a));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);
    TEST_ASSERT_EQUAL_UINT32(0, iface_b_tx_count);

    danp_packet_t pkt_b;
    prepare_packet(&pkt_b, 200, 12);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt_b));
    TEST_ASSERT_EQUAL_UINT32(1, iface_b_tx_count);

    danp_packet_t pkt_a_second;
    prepare_packet(&pkt_a_second, 150, 8);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt_a_second));
    TEST_ASSERT_EQUAL_UINT32(2, iface_a_tx_count);
}

void test_route_table_replaces_entries_and_clears_on_error(void)
{
    danp_packet_t pkt;

    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load("55:IFACE_A"));
    prepare_packet(&pkt, 55, 10);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);

    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load("55:IFACE_B"));
    prepare_packet(&pkt, 55, 10);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);
    TEST_ASSERT_EQUAL_UINT32(1, iface_b_tx_count);

    TEST_ASSERT_EQUAL_INT32(-1, danp_route_table_load("55:UNKNOWN_IFACE"));
    prepare_packet(&pkt, 55, 10);
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_tx(&pkt));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);
    TEST_ASSERT_EQUAL_UINT32(1, iface_b_tx_count);
}

void test_route_tx_enforces_mtu_limits(void)
{
    iface_a.mtu = 32;
    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load("77:IFACE_A"));

    const uint16_t max_payload = iface_a.mtu - DANP_HEADER_SIZE;

    danp_packet_t pkt_ok;
    prepare_packet(&pkt_ok, 77, max_payload);
    TEST_ASSERT_EQUAL_INT32(0, danp_route_tx(&pkt_ok));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);

    danp_packet_t pkt_too_large;
    prepare_packet(&pkt_too_large, 77, max_payload + 1);
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_tx(&pkt_too_large));
    TEST_ASSERT_EQUAL_UINT32(1, iface_a_tx_count);
}

void test_route_register_interface_validates_inputs(void)
{
    danp_register_interface(NULL);

    danp_interface_t missing_tx = {.name = "BAD_TX", .mtu = 32, .tx_func = NULL};
    danp_register_interface(&missing_tx);

    danp_interface_t missing_name = {.name = NULL, .mtu = 32, .tx_func = iface_a_tx};
    danp_register_interface(&missing_name);

    danp_interface_t zero_mtu = {.name = "ZERO_MTU", .mtu = 0, .tx_func = iface_a_tx};
    danp_register_interface(&zero_mtu);
}

void test_route_table_load_errors_and_whitespace(void)
{
    TEST_ASSERT_EQUAL_INT32(0, danp_route_table_load(",,300:IFACE_A   ,,"));

    TEST_ASSERT_EQUAL_INT32(-1, danp_route_table_load("400IFACE_A"));
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_table_load(":IFACE_A"));
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_table_load("500:"));
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_table_load("XYZ:IFACE_A"));
}

void test_route_tx_handles_missing_inputs(void)
{
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_tx(NULL));

    danp_packet_t pkt;
    prepare_packet(&pkt, 999, 4);
    TEST_ASSERT_EQUAL_INT32(-1, danp_route_tx(&pkt));
}

/* ============================================================================
 * Test Runner
 * ============================================================================
 */

int main(void)
{
    UNITY_BEGIN();

    RUN_TEST(test_route_table_routes_packets_over_registered_interfaces);
    RUN_TEST(test_route_table_replaces_entries_and_clears_on_error);
    RUN_TEST(test_route_tx_enforces_mtu_limits);
    RUN_TEST(test_route_register_interface_validates_inputs);
    RUN_TEST(test_route_table_load_errors_and_whitespace);
    RUN_TEST(test_route_tx_handles_missing_inputs);

    return UNITY_END();
}
