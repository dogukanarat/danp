/**
 * @file test_runner.c
 * @brief Unified test runner for all DANP test suites
 *
 * This file provides a single main() function that runs all test suites
 * in the DANP project. Each test suite is run sequentially and results
 * are collected and reported by Unity.
 */

#include "unity.h"

/* ============================================================================
 * External Test Suite Declarations
 * ============================================================================
 */

/* Declare the suite runner functions from each test file */
extern void run_core_tests(void);
extern void run_dgram_tests(void);
extern void run_route_tests(void);
extern void run_stream_tests(void);
extern void run_zerocopy_tests(void);

/* ============================================================================
 * Unity Setup and Teardown
 * ============================================================================
 */

/**
 * @brief Global setUp function required by Unity
 *
 * This function is called before each test. Since each test suite
 * has its own setUp function, this is left empty.
 */
void setUp(void)
{
    /* Each test suite handles its own setup */
}

/**
 * @brief Global tearDown function required by Unity
 *
 * This function is called after each test. Since each test suite
 * has its own tearDown function, this is left empty.
 */
void tearDown(void)
{
    /* Each test suite handles its own teardown */
}

/* ============================================================================
 * Main Test Runner
 * ============================================================================
 */

/**
 * @brief Main entry point for all DANP tests
 *
 * Executes all test suites in sequence:
 * 1. Core functionality tests
 * 2. Datagram socket tests
 * 3. Routing table tests
 * 4. Stream socket tests
 * 5. Zero-copy buffer tests
 *
 * @return 0 if all tests pass, non-zero otherwise
 */
int main(void)
{
    UNITY_BEGIN();

    /* Run all test suites */
    run_core_tests();
    run_dgram_tests();
    run_route_tests();
    run_stream_tests();
    run_zerocopy_tests();

    return UNITY_END();
}
