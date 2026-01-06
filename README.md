# DANP - Data and Network Protocol Library

A lightweight, embedded-friendly network protocol library providing socket-based communication with both reliable (TCP-like) and unreliable (UDP-like) transport modes.

## Overview

DANP is a C99 network protocol implementation designed for resource-constrained embedded systems. It provides a familiar socket API with support for datagram and stream sockets, network routing, zero-copy operations, and packet fragmentation.

**Key Features:**
- ✅ **Dual Socket Types**: DGRAM (unreliable, UDP-like) and STREAM (reliable, TCP-like with handshake)
- ✅ **Network Routing**: Static routing table with multiple interface support
- ✅ **Zero-Copy Operations**: Efficient packet buffer management
- ✅ **Packet Fragmentation**: SFP (Small Fragmentation Protocol) for large messages
- ✅ **Portable**: C99 standard, POSIX-compatible
- ✅ **Embedded-Friendly**: Fixed-size memory pools, no dynamic allocation
- ✅ **Comprehensive Testing**: 50+ unit tests using Unity framework
- ✅ **Clean API**: Familiar socket-like interface

## Protocol Features

### Socket Types

**DGRAM (Datagram) Socket**
- Unreliable, connectionless communication (like UDP)
- No handshaking or connection establishment
- Fast, low overhead
- Suitable for status updates, telemetry, sensor data

**STREAM (Stream) Socket**
- Reliable, connection-oriented communication (like TCP)
- Three-way handshake (SYN → SYN-ACK → ACK)
- Connection establishment and termination (RST)
- Acknowledgments for received data
- Suitable for command/control, file transfer, critical data

### Network Features

- **Multi-Interface Support**: Register multiple network interfaces (CAN, UART, RF, etc.)
- **Static Routing**: Route packets based on destination node via routing table
- **Loopback Support**: Local node communication for testing
- **Priority Levels**: Normal and high priority packets
- **MTU Enforcement**: Per-interface Maximum Transmission Unit validation

### Buffer Management

- **Fixed Memory Pools**: Pre-allocated packet buffers (configurable pool size)
- **Zero-Copy Operations**: Direct buffer manipulation without copying
- **Packet Chaining**: Link multiple buffers for large messages
- **Fragmentation**: Automatic fragmentation/reassembly for oversized packets

## Quick Start

### Building the Library

```bash
# Basic build
cmake -B build
cmake --build build

# Build with tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build

# Build with test logging (for debugging)
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
cmake --build build
```

### Installation

```bash
# System-wide installation
cmake -B build
cmake --build build
sudo cmake --install build

# User installation (no sudo)
cmake -B build -DCMAKE_INSTALL_PREFIX=~/.local
cmake --build build
cmake --install build
```

### Basic Usage Example

```c
#include <danp/danp.h>

/* Initialize DANP */
danp_config_t config = {
    .local_node = 1,
    .log_function = NULL
};
danp_init(&config);

/* Register a network interface */
danp_interface_t my_interface = {
    .name = "CAN0",
    .address = 1,
    .mtu = 64,
    .tx_func = my_transmit_function
};
danp_register_interface(&my_interface);

/* Create and bind a datagram socket */
danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
danp_bind(sock, 10);

/* Send data to another node */
const char *message = "Hello";
danp_send_to(sock, message, 5, 2, 20);  // Send to node 2, port 20

/* Receive data */
char buffer[64];
uint16_t src_node, src_port;
int32_t len = danp_recv_from(sock, buffer, sizeof(buffer),
                              &src_node, &src_port, DANP_WAIT_FOREVER);

/* Cleanup */
danp_close(sock);
```

## API Overview

### Core Functions

```c
/* Initialization */
void danp_init(const danp_config_t *config);

/* Socket Management */
danp_socket_t *danp_socket(danp_socket_type_t type);
int32_t danp_bind(danp_socket_t *socket, uint16_t port);
int32_t danp_close(danp_socket_t *socket);

/* Datagram Operations */
int32_t danp_send_to(danp_socket_t *socket, const void *data, uint16_t length,
                     uint16_t dest_node, uint16_t dest_port);
int32_t danp_recv_from(danp_socket_t *socket, void *buffer, uint16_t buffer_size,
                       uint16_t *src_node, uint16_t *src_port, uint32_t timeout_ms);

/* Stream Operations */
int32_t danp_listen(danp_socket_t *socket, uint16_t port);
danp_socket_t *danp_accept(danp_socket_t *socket, uint32_t timeout_ms);
int32_t danp_connect(danp_socket_t *socket, uint16_t dest_node, uint16_t dest_port);
int32_t danp_send(danp_socket_t *socket, const void *data, uint16_t length);
int32_t danp_recv(danp_socket_t *socket, void *buffer, uint16_t buffer_size, uint32_t timeout_ms);

/* Network Interface */
void danp_register_interface(danp_interface_t *iface);
void danp_input(danp_interface_t *iface, const void *frame, uint16_t length);

/* Routing */
int32_t danp_route_table_load(const char *table_str);
int32_t danp_route_tx(danp_packet_t *packet);

/* Zero-Copy Operations */
danp_packet_t *danp_send_packet(danp_socket_t *socket, danp_packet_t *packet);
danp_packet_t *danp_recv_packet(danp_socket_t *socket, uint32_t timeout_ms);

/* SFP (Small Fragmentation Protocol) */
int32_t danp_sfp_send(danp_socket_t *socket, const void *data, uint32_t length);
int32_t danp_sfp_recv(danp_socket_t *socket, void *buffer, uint32_t buffer_size, uint32_t timeout_ms);
```

## Directory Structure

```
danp/
├── include/danp/           # Public API headers
│   ├── danp.h              # Main API header
│   ├── danp_types.h        # Type definitions
│   ├── danp_defs.h         # Configuration and constants
│   ├── danp_buffer.h       # Buffer management API
│   └── danp_socket.h       # Socket API
├── src/                    # Implementation
│   ├── danp.c              # Core functions
│   ├── danp_buffer.c       # Memory pool and buffer management
│   ├── danp_route.c        # Routing table and interface management
│   ├── danp_socket.c       # Socket operations
│   └── danp_zerocopy.c     # Zero-copy and SFP implementation
├── test/                   # Unit tests (Unity framework)
│   ├── test_runner.c       # Main test runner
│   ├── test_core.c         # Core functionality tests
│   ├── test_dgram.c        # Datagram socket tests
│   ├── test_stream.c       # Stream socket tests
│   ├── test_route.c        # Routing tests
│   ├── test_zerocopy.c     # Zero-copy and SFP tests
│   ├── log_message.c       # Test logging (optional)
│   └── CMakeLists.txt      # Test configuration
├── cmake/                  # CMake modules
│   ├── DanpConfig.cmake.in # Package config template
│   └── FindOsal.cmake      # OSAL dependency finder
├── ci/                     # CI/CD scripts
│   ├── debug.sh            # Debug build with tests
│   ├── release.sh          # Release build
│   └── install.sh          # Installation script
└── CMakeLists.txt          # Build configuration
```

## Configuration

### Memory Pool Configuration (include/danp/danp_defs.h)

```c
#define DANP_POOL_SIZE 32           /* Number of packet buffers */
#define DANP_MAX_PACKET_SIZE 256    /* Maximum payload per packet */
#define DANP_MAX_PORTS 64           /* Maximum number of ports */
#define DANP_MAX_ROUTES 16          /* Maximum routing table entries */
#define DANP_MAX_INTERFACES 4       /* Maximum network interfaces */
```

### Priority Levels

```c
#define DANP_PRIORITY_NORMAL 0      /* Normal priority packets */
#define DANP_PRIORITY_HIGH 1        /* High priority packets */
```

## Testing

DANP includes comprehensive test coverage with 50+ unit tests:

### Running Tests

```bash
# Build and run all tests
cmake -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --verbose

# Run tests without logging (clean output)
cmake -B build -DBUILD_TESTS=ON
./build/test/TestDanp

# Run tests with verbose logging (for debugging)
cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
./build/test/TestDanp
```

### Test Suites

- **Core Tests** (13 tests): Header packing, memory pools, initialization, buffer management
- **Datagram Tests** (5 tests): DGRAM socket operations, send/receive, timeouts
- **Route Tests** (6 tests): Routing table, interface registration, MTU enforcement
- **Stream Tests** (5 tests): Connection handshake, data transfer, RST handling
- **Zero-Copy Tests** (21 tests): Buffer operations, SFP fragmentation, packet chaining

See [test/README.md](test/README.md) for detailed testing documentation.

## Dependencies

- **C Standard Library**: C99 standard
- **OSAL**: Operating System Abstraction Layer for semaphores and message queues
  - Provides platform-independent primitives for embedded systems
  - See [cmake/FindOsal.cmake](cmake/FindOsal.cmake) for configuration

## Using DANP in Your Project

### CMake Integration

```cmake
cmake_minimum_required(VERSION 3.14)
project(MyProject)

# Find the installed library
find_package(Danp REQUIRED)

# Create your executable
add_executable(myapp main.c)

# Link against the library
target_link_libraries(myapp PRIVATE Danp::Danp)
```

### Example: Simple Echo Server

```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 7);  /* Echo port */

    while (1)
    {
        char buffer[64];
        uint16_t src_node, src_port;

        int32_t len = danp_recv_from(sock, buffer, sizeof(buffer),
                                      &src_node, &src_port, DANP_WAIT_FOREVER);

        if (len > 0)
        {
            danp_send_to(sock, buffer, len, src_node, src_port);
        }
    }

    return 0;
}
```

## Build Options

```bash
# Shared library (default: static)
cmake -DBUILD_SHARED_LIBS=ON ..

# Enable tests
cmake -DBUILD_TESTS=ON ..

# Enable test logging
cmake -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON ..

# Debug build
cmake -DCMAKE_BUILD_TYPE=Debug ..

# Release build (default, optimized)
cmake -DCMAKE_BUILD_TYPE=Release ..
```

## Compiler Support

- **Standard**: C99
- **Tested Compilers**: GCC 8+, Clang 10+
- **Warnings**: Full warnings enabled (`-Wall -Wextra -Wpedantic`)
- **Platforms**: Linux, embedded RTOS

## Performance Characteristics

- **Zero-Copy**: Direct buffer access, no memcpy for large payloads
- **Fixed Memory**: No malloc/free, predictable memory usage
- **Interrupt-Safe**: Buffer operations suitable for ISR context
- **Lightweight**: Small code footprint (~15KB compiled)

## Limitations

- **Static Configuration**: Pool sizes set at compile time
- **No Dynamic Routing**: Routing table is static, loaded at runtime
- **Limited Ports**: Maximum 64 ports per node
- **No Congestion Control**: Stream sockets lack flow control/windowing
- **Single-Threaded**: Application responsible for thread safety

## Contributing

Contributions are welcome! Please ensure:
- Code follows existing style (see `.clang-format`)
- All tests pass (`ctest`)
- New features include tests
- Documentation is updated

## License

All Rights Reserved

## Repository

GitHub: [dogukanarat/danp](https://github.com/dogukanarat/danp)
