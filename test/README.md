# DANP Testing Guide

This directory contains comprehensive unit tests for the DANP (Data and Network Protocol) library using the [Unity Test Framework](https://github.com/ThrowTheSwitch/Unity).

## Test Architecture

DANP uses a unified test runner that manages all test suites in a single executable. This approach:
- ✅ Simplifies test execution (one binary for all tests)
- ✅ Enables per-suite setup/teardown isolation
- ✅ Provides consistent test environment
- ✅ Supports optional test logging for debugging

## Test Coverage

### Test Suites (50 tests total)

**Core Tests** (`test_core.c`) - 13 tests
- Header packing/unpacking validation
- Memory pool allocation and exhaustion
- Initialization and configuration
- Packet input validation
- Buffer management and free count tracking
- Port binding validation
- Statistics printing

**Datagram Tests** (`test_dgram.c`) - 5 tests
- DGRAM socket creation and binding
- Unreliable message send/receive (loopback)
- Multiple message handling
- Large payload rejection
- Receive timeout behavior

**Routing Tests** (`test_route.c`) - 6 tests
- Routing table loading and parsing
- Multi-interface packet routing
- MTU enforcement per interface
- Route table replacement
- Invalid route handling
- Interface validation

**Stream Tests** (`test_stream.c`) - 5 tests
- Three-way handshake (SYN → SYN-ACK → ACK)
- Reliable data transfer with ACKs
- Connection termination with RST
- Bidirectional communication
- Accept timeout handling

**Zero-Copy Tests** (`test_zerocopy.c`) - 21 tests
- Packet buffer get/free operations
- Packet chain management
- Zero-copy send/receive
- SFP (Small Fragmentation Protocol) operations
- Large message fragmentation/reassembly
- Error handling for invalid operations

## Building and Running Tests

### Basic Test Build

```bash
# Configure with tests enabled
cmake -B build -DBUILD_TESTS=ON
cmake --build build

# Run all tests via CTest
ctest --test-dir build

# Run test executable directly
./build/test/TestDanp
```

### Test Output (Without Logging)

```
test/test_runner.c:393:test_header_packing_preserves_values:PASS
test/test_runner.c:397:test_header_packing_handles_edge_cases:PASS
...
-----------------------
50 Tests 0 Failures 0 Ignored
OK
```

### Verbose Test Execution

```bash
# Run with CTest verbose output
ctest --test-dir build --verbose

# Direct execution (shows all test names)
./build/test/TestDanp
```

## Test Logging

DANP tests support optional logging output controlled by the `BUILD_TESTS_WITH_LOGS` CMake option.

### Without Logging (Default)

```bash
cmake -B build -DBUILD_TESTS=ON
cmake --build build
./build/test/TestDanp
```

Output: Only test pass/fail results (clean, fast)

### With Logging (For Debugging)

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
cmake --build build
./build/test/TestDanp
```

Output: Includes detailed color-coded log messages from stream socket tests:
- Initialization events
- Connection handshake steps
- Packet transmission/reception
- Buffer allocation/deallocation
- State transitions

The logging option:
- Compiles `log_message.c` when enabled
- Defines `ENABLE_TEST_LOGGING` preprocessor macro
- Shows color-coded output (Info=green, Debug=cyan, Warn=yellow, Error=red)
- Helps debug complex protocol interactions

## Test File Structure

### Unified Test Runner (`test_runner.c`)

```c
/* Main entry point */
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

/* Required Unity functions */
void setUp(void) { /* Global setup */ }
void tearDown(void) { /* Global teardown */ }
```

### Test Suite Example (`test_dgram.c`)

```c
/* Suite-specific setup/teardown */
static void setUp_dgram(void)
{
    danp_config_t config = {.local_node = TEST_NODE_ID};
    danp_init(&config);
    setup_loopback_interface();
}

static void tearDown_dgram(void)
{
    /* Cleanup after each test */
}

/* Test function */
void test_dgram_send_recv_same_node(void)
{
    danp_socket_t *sock_a = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock_a, PORT_A);

    danp_socket_t *sock_b = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock_b, PORT_B);

    const char *message = "HelloUnity";
    danp_send_to(sock_a, message, 10, TEST_NODE_ID, PORT_B);

    char buffer[32];
    uint16_t src_node, src_port;
    int32_t bytes = danp_recv_from(sock_b, buffer, 32,
                                    &src_node, &src_port, DANP_WAIT_FOREVER);

    TEST_ASSERT_EQUAL(10, bytes);
    buffer[bytes] = '\0';
    TEST_ASSERT_EQUAL_STRING("HelloUnity", buffer);
    TEST_ASSERT_EQUAL(TEST_NODE_ID, src_node);
    TEST_ASSERT_EQUAL(PORT_A, src_port);

    danp_close(sock_a);
    danp_close(sock_b);
}

/* Suite runner called by test_runner.c */
void run_dgram_tests(void)
{
    setUp_dgram();
    RUN_TEST(test_dgram_send_recv_same_node);
    tearDown_dgram();

    /* ... more tests ... */
}
```

## Common Unity Assertions Used in DANP Tests

```c
/* Integer comparisons */
TEST_ASSERT_EQUAL(expected, actual);
TEST_ASSERT_EQUAL_INT32(expected, actual);
TEST_ASSERT_EQUAL_UINT16(expected, actual);
TEST_ASSERT_EQUAL_UINT8(expected, actual);

/* Pointer checks */
TEST_ASSERT_NULL(pointer);
TEST_ASSERT_NOT_NULL(pointer);
TEST_ASSERT_NOT_EQUAL(value1, value2);

/* String comparisons */
TEST_ASSERT_EQUAL_STRING(expected, actual);

/* Range checks */
TEST_ASSERT_GREATER_THAN_INT(threshold, actual);
```

## Test Patterns and Best Practices

### Pattern 1: Loopback Testing

Most tests use a loopback interface to simulate network communication:

```c
static int32_t loopback_tx(void *iface_common, danp_packet_t *packet)
{
    danp_interface_t *iface = (danp_interface_t *)iface_common;
    uint8_t buffer[DANP_HEADER_SIZE + DANP_MAX_PACKET_SIZE];

    /* Copy header and payload */
    memcpy(buffer, &packet->header_raw, DANP_HEADER_SIZE);
    if (packet->length > 0)
    {
        memcpy(buffer + DANP_HEADER_SIZE, packet->payload, packet->length);
    }

    /* Feed back to input */
    danp_input(iface, buffer, DANP_HEADER_SIZE + packet->length);
    return 0;
}
```

### Pattern 2: Resource Cleanup

Each test properly cleans up resources:

```c
void test_socket_operations(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 10);

    /* ... test operations ... */

    danp_close(sock);  /* Always cleanup */
}
```

### Pattern 3: Error Case Testing

Tests verify error handling:

```c
void test_bind_rejects_invalid_port(void)
{
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(sock);

    /* Port >= DANP_MAX_PORTS should fail */
    TEST_ASSERT_EQUAL_INT32(-1, danp_bind(sock, DANP_MAX_PORTS));

    danp_close(sock);
}
```

### Pattern 4: State Validation

Tests check internal state transitions:

```c
void test_stream_socket_creation_and_states(void)
{
    danp_socket_t *socket = danp_socket(DANP_TYPE_STREAM);
    TEST_ASSERT_NOT_NULL(socket);
    TEST_ASSERT_EQUAL(DANP_TYPE_STREAM, socket->type);
    TEST_ASSERT_EQUAL(DANP_SOCK_OPEN, socket->state);

    danp_bind(socket, test_port);
    TEST_ASSERT_EQUAL_UINT16(test_port, socket->local_port);

    danp_close(socket);
}
```

## Adding New Tests

### 1. Add Test Function to Existing Suite

```c
/* In test_dgram.c */
void test_dgram_my_new_test(void)
{
    /* Setup */
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 50);

    /* Exercise */
    /* ... your test code ... */

    /* Verify */
    TEST_ASSERT_EQUAL(expected, actual);

    /* Cleanup */
    danp_close(sock);
}
```

### 2. Add to Suite Runner

```c
/* In run_dgram_tests() */
void run_dgram_tests(void)
{
    /* ... existing tests ... */

    setUp_dgram();
    RUN_TEST(test_dgram_my_new_test);
    tearDown_dgram();
}
```

### 3. Rebuild and Run

```bash
cmake --build build
./build/test/TestDanp
```

## Test Configuration

### Conditional Test Compilation

Some tests can be disabled via defines in `test_stream.c`:

```c
#define ENABLE_TEST_STREAM_HANDSHAKE 1
#define ENABLE_TEST_STREAM_CLOSE_RST 1
#define ENABLE_TEST_STREAM_SOCKET_STATES 1
#define ENABLE_TEST_STREAM_BIDIRECTIONAL 1
```

Set to `0` to disable specific stream tests during development.

## Debugging Test Failures

### 1. Enable Logging

```bash
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
cmake --build build
./build/test/TestDanp
```

### 2. Run Specific Test

Edit `test_runner.c` to comment out other test suites:

```c
int main(void)
{
    UNITY_BEGIN();

    // run_core_tests();
    run_dgram_tests();  /* Only run this suite */
    // run_route_tests();
    // run_stream_tests();
    // run_zerocopy_tests();

    return UNITY_END();
}
```

### 3. Use GDB

```bash
gdb ./build/test/TestDanp
(gdb) break test_dgram_send_recv_same_node
(gdb) run
(gdb) step
```

### 4. Check Return Values

All DANP functions return error codes. Tests validate these:

```c
int32_t rc = danp_bind(sock, port);
TEST_ASSERT_EQUAL_INT32(0, rc);  /* 0 = success, -1 = error */
```

## Continuous Integration

Tests are designed for CI/CD pipelines:

```bash
# In GitHub Actions / GitLab CI
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

CTest provides:
- Exit code 0 on success, non-zero on failure
- `--output-on-failure` shows details only when tests fail
- Test timeout enforcement (30 seconds per test)
- Test labels for filtering (`unit`, `danp`)

## Test Metrics

Current test coverage:
- **50 total tests** across 5 suites
- **100% pass rate** in continuous integration
- **~0.2 seconds** total execution time
- **Zero memory leaks** (all buffers freed)

## Resources

- [Unity Documentation](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityGettingStartedGuide.md)
- [Unity Assertions Reference](https://github.com/ThrowTheSwitch/Unity/blob/master/docs/UnityAssertionsReference.md)
- [DANP API Documentation](../include/danp/danp.h)
- [Main README](../README.md)
