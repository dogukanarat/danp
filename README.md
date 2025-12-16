# DANP

A lightweight, portable networking library implementing reliable and unreliable communication protocols for embedded systems and distributed applications.

[![License](https://img.shields.io/badge/license-All%20Rights%20Reserved-red.svg)]()
[![C Standard](https://img.shields.io/badge/C-C99-blue.svg)]()
[![Platform](https://img.shields.io/badge/platform-POSIX%20%7C%20FreeRTOS-green.svg)]()

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Design Criteria](#design-criteria)
- [Architecture](#architecture)
- [Requirements](#requirements)
- [Building](#building)
- [Usage](#usage)
- [API Documentation](#api-documentation)
- [Examples](#examples)
- [Testing](#testing)
- [Platform Support](#platform-support)
- [License](#license)

## Overview

DANP is a custom network protocol library designed for resource-constrained environments where standard TCP/IP stacks may be too heavy or unavailable. It provides both reliable (TCP-like) and unreliable (UDP-like) communication with a familiar BSD socket-style API.

### Key Characteristics

- **Lightweight**: Minimal memory footprint with static allocation
- **Portable**: OS abstraction layer supports POSIX and FreeRTOS
- **Flexible**: Supports multiple transport layers (ZeroMQ, custom drivers)
- **Reliable**: Stop-and-wait ARQ for guaranteed delivery
- **Socket API**: Familiar BSD-style socket interface

## Features

### Protocol Features

- **Dual Socket Types**:
  - `DANP_TYPE_DGRAM`: Unreliable, connectionless (UDP-like)
  - `DANP_TYPE_STREAM`: Reliable, connection-oriented (TCP-like)

- **Connection Management**:
  - Three-way handshake (SYN, SYN-ACK, ACK)
  - Graceful connection termination (RST)
  - Connection state machine

- **Reliability Mechanisms** (STREAM sockets):
  - Stop-and-wait ARQ protocol
  - Sequence numbering
  - Acknowledgments with retransmission
  - Configurable timeout and retry limits

- **Addressing**:
  - 256 nodes (8-bit node addresses)
  - 64 ports per node (6-bit port numbers)
  - Ephemeral port allocation

### Implementation Features

- **Static Memory Allocation**: No dynamic memory, predictable behavior
- **Packet Pool**: Pre-allocated packet buffers with semaphore protection
- **Multi-Interface Support**: Register multiple network interfaces
- **Priority Support**: High/normal packet priorities
- **Logging Framework**: Configurable logging with multiple levels

## Design Criteria

### 1. Portability

**Requirement**: Run on multiple platforms without modification.

**Implementation**:
- OS abstraction layer (`danpArch.h`)
- Platform-specific implementations:
  - `danpPosix.c`: POSIX threads, mutexes, queues
  - `danpFreeRTOS.c`: FreeRTOS tasks, semaphores, queues
- Weak linkage for easy platform extension

### 2. Resource Efficiency

**Requirement**: Minimal memory footprint for embedded systems.

**Implementation**:
- Static packet pool (configurable size)
- Fixed-size socket pool (20 sockets)
- No dynamic allocation
- Compact 32-bit header format

### 3. Reliability

**Requirement**: Guaranteed delivery for critical data.

**Implementation**:
- Stop-and-wait ARQ for STREAM sockets
- Sequence numbers and acknowledgments
- Configurable retransmission (default: 3 retries, 500ms timeout)
- Connection state management

### 4. Simplicity

**Requirement**: Easy to use and understand.

**Implementation**:
- Familiar BSD socket API
- Clear state machine
- Comprehensive documentation
- Example applications

### 5. Flexibility

**Requirement**: Support various transport layers.

**Implementation**:
- Driver abstraction (`danpInterface_t`)
- ZeroMQ driver for IPC/network
- Mock driver for testing
- Easy to add custom drivers

## Architecture

### Layer Structure

```
┌─────────────────────────────────────┐
│     Application Layer               │
│  (Socket API: connect, send, recv)  │
├─────────────────────────────────────┤
│     DANP Protocol Layer             │
│  (Packet handling, reliability)     │
├─────────────────────────────────────┤
│     Driver Layer                    │
│  (ZeroMQ, Custom drivers)           │
├─────────────────────────────────────┤
│     OS Abstraction Layer            │
│  (POSIX, FreeRTOS)                  │
└─────────────────────────────────────┘
```

### Packet Format

```
 0                   1                   2                   3
 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|R|P|  Dst Node (8)   |  Src Node (8)   | DstPort(6)| SrcPort(6)|
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
|    Flags (2)  |                 Payload (0-128 bytes)         |
+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

R: RST flag (1 bit)
P: Priority (1 bit)
Flags: SYN, ACK (2 bits)
```

### State Machine (STREAM Sockets)

```
CLOSED → [connect] → SYN_SENT → [SYN-ACK] → ESTABLISHED
                         ↓
CLOSED ← [listen] → LISTENING → [SYN] → SYN_RECEIVED → [ACK] → ESTABLISHED
                                                            ↓
                                                      ESTABLISHED → [RST] → CLOSED
```

## Requirements

### Build Requirements

- **CMake**: 3.14 or higher
- **C Compiler**: C99 compatible (GCC, Clang)
- **Threads**: POSIX threads (for POSIX builds)

### Optional Dependencies

- **ZeroMQ**: 4.x (for ZMQ driver support)
- **Doxygen**: 1.9+ (for documentation)
- **Sphinx**: 8.x + Breathe (for modern documentation)
- **Graphviz**: For call graphs

### Runtime Requirements

- **POSIX**: Linux, macOS, or POSIX-compliant OS
- **FreeRTOS**: FreeRTOS 10.x or compatible

## Building

### Dependency Fetching

The build uses CMake's `FetchContent` to pull required dependencies automatically:

- **OSAL**: `https://github.com/dogukanarat/osal`
- **Unity** (tests): `https://github.com/ThrowTheSwitch/Unity`

Override the OSAL revision by setting `-DOSAL_GIT_TAG=<ref>` during configuration.

### Basic Build

```bash
mkdir build
cd build
cmake ..
make
```

### Build Options

```bash
# Enable/disable POSIX support (default: ON)
cmake -DDANP_ARCH_POSIX=ON ..

# Enable/disable FreeRTOS support (default: OFF)
cmake -DDANP_ARCH_FREERTOS=ON ..

# Enable/disable ZeroMQ driver (default: ON)
cmake -DDANP_ZMQ_SUPPORT=ON ..

# Build examples
cmake -DBUILD_EXAMPLES=ON ..

# Build tests
cmake -DBUILD_TESTS=ON ..
```

### CMake Presets

```bash
# Development build
cmake --preset=Debug
cmake --build --preset=Debug

# Release build
cmake --preset=Release
cmake --build --preset=Release

# Coverage build
cmake --preset=Coverage
cmake --build --preset=Coverage
```

### Documentation

```bash
# Generate Sphinx documentation (modern)
./generate_sphinx_docs.sh
```

## Installation

### System-Wide Installation

```bash
# Configure and build
cmake -B build -S .
cmake --build build

# Install (requires sudo for system directories)
sudo cmake --install build
```

### Custom Installation Prefix

```bash
# Install to custom location
cmake -B build -S . -DCMAKE_INSTALL_PREFIX=/opt/danp
cmake --build build
cmake --install build
```

### Using the Installed Library

After installation, use `find_package()` in your CMakeLists.txt:

```cmake
find_package(danp REQUIRED)
target_link_libraries(myapp PRIVATE danp::danp)
```

For detailed installation instructions, see [INSTALL.md](INSTALL.md).


## Usage

### Basic Example (DGRAM Socket)

```c
#include "danp/danp.h"

// Initialize DANP
danpConfig_t config = {
    .localNode = 1,
    .logFunction = myLogCallback
};
danpInit(&config);

// Register network interface
danpInterface_t iface;
danpZmqInit(&iface, "tcp://localhost:5555");
danpRegisterInterface(&iface);

// Create and bind socket
danpSocket_t* sock = danpSocket(DANP_TYPE_DGRAM);
danpBind(sock, 8080);

// Send data
char msg[] = "Hello, DANP!";
danpSendTo(sock, msg, sizeof(msg), 2, 9090);

// Receive data
char buffer[128];
uint16_t srcNode, srcPort;
int len = danpRecvFrom(sock, buffer, sizeof(buffer), &srcNode, &srcPort, 1000);

// Clean up
danpClose(sock);
```

### Basic Example (STREAM Socket)

```c
// Server
danpSocket_t* server = danpSocket(DANP_TYPE_STREAM);
danpBind(server, 8080);
danpListen(server, 5);

danpSocket_t* client = danpAccept(server, DANP_WAIT_FOREVER);
char buffer[128];
int len = danpRecv(client, buffer, sizeof(buffer), 5000);
danpSend(client, "ACK", 3);
danpClose(client);

// Client
danpSocket_t* sock = danpSocket(DANP_TYPE_STREAM);
danpConnect(sock, 1, 8080);  // Connect to node 1, port 8080
danpSend(sock, "Hello", 5);
danpRecv(sock, buffer, sizeof(buffer), 5000);
danpClose(sock);
```

## API Documentation

### Core Functions

- `danpInit()`: Initialize the library
- `danpRegisterInterface()`: Register network interface
- `danpAllocPacket()` / `danpFreePacket()`: Packet pool management

### Socket API

- `danpSocket()`: Create socket
- `danpBind()`: Bind to local port
- `danpListen()`: Listen for connections (STREAM)
- `danpAccept()`: Accept connection (STREAM)
- `danpConnect()`: Connect to remote (STREAM)
- `danpSend()` / `danpRecv()`: Connected I/O
- `danpSendTo()` / `danpRecvFrom()`: Connectionless I/O
- `danpClose()`: Close socket

### Configuration Constants

```c
#define DANP_MAX_PACKET_SIZE    128   // Max payload size
#define DANP_POOL_SIZE          20    // Packet pool size
#define DANP_RETRY_LIMIT        3     // Max retransmissions
#define DANP_ACK_TIMEOUT_MS     500   // ACK timeout
#define DANP_MAX_PORTS          64    // Ports per node
#define DANP_MAX_NODES          256   // Max nodes
```

For complete API documentation, see:
- **Doxygen HTML**: `docs/html/index.html`
- **Sphinx HTML**: `sphinx/index.html`

## Examples

### Available Examples

- **DGRAM Client/Server**: Unreliable communication
  - `example/dgram/client.c`
  - `example/dgram/server.c`

- **STREAM Client/Server**: Reliable communication
  - `example/stream/client.c`
  - `example/stream/server.c`

### Building Examples

```bash
cmake -DBUILD_EXAMPLES=ON ..
make

# Run examples
./example/dgram/danp_dgram_server
./example/dgram/danp_dgram_client

./example/stream/danp_stream_server
./example/stream/danp_stream_client
```

## Testing

### Unit Tests

```bash
cmake -S . -B build -DBUILD_TESTS=ON
cmake --build build
ctest --test-dir build --output-on-failure
```

### Test Coverage

```bash
# After configuring with -DENABLE_COVERAGE=ON and BUILD_TESTS=ON
cmake --build build --target coverage
```

Artifacts:
- `build/coverage.info.cleaned`: textual summary
- `build/coverage_html/index.html`: HTML report

The CI workflow publishes both files as artifacts for every run.

- Core packet handling
- DGRAM socket operations
- STREAM socket operations
- Connection management
- Reliability mechanisms

## Continuous Integration

- GitHub Actions workflow: `.github/workflows/ci.yml`
- Runs on pushes and pull requests targeting `main` or `master`
- Installs Ninja/lcov, then configures the project (FetchContent automatically downloads OSAL/Unity)
- Configures CMake with coverage + test flags, then builds and runs `ctest`
- Generates the `coverage` target and uploads both LCOV data and HTML reports as artifacts

## Platform Support

### POSIX (Linux, macOS)

- **Threading**: pthreads
- **Synchronization**: pthread mutexes, semaphores
- **Queues**: Custom implementation with condition variables
- **Status**: ✅ Fully supported

### FreeRTOS

- **Threading**: FreeRTOS tasks
- **Synchronization**: FreeRTOS semaphores, mutexes
- **Queues**: FreeRTOS queues
- **Status**: ⚠️ Implementation available, testing in progress

### Adding New Platforms

1. Implement OS abstraction functions in `src/arch/danpYourOS.c`
2. Implement required functions from `danpArch.h`:
   - Thread/task management
   - Mutex operations
   - Semaphore operations
   - Queue operations
   - Time functions
3. Add CMake option and source file to `CMakeLists.txt`

## Project Structure

```
danp/
├── include/danp/          # Public headers
│   ├── danp.h            # Main API
│   ├── danpArch.h        # OS abstraction
│   └── danpTypes.h       # Common types
├── src/                   # Implementation
│   ├── danp.c            # Core protocol
│   ├── danpSocket.c      # Socket layer
│   ├── arch/             # Platform implementations
│   │   ├── danpPosix.c
│   │   └── danpFreeRTOS.c
│   └── driver/           # Network drivers
│       └── danpZmq.c
├── example/               # Example applications
├── test/                  # Unit tests
├── docs/                 # Documentation sources and XML output
│   ├── api.rst           # API reference page
│   ├── conf.py           # Sphinx config
│   ├── index.rst         # Documentation entry point
│   └── xml/              # Generated Doxygen XML
├── CMakeLists.txt        # Build configuration
├── Doxyfile              # Doxygen config
└── README.md             # This file
```

## Contributing

This is a proprietary project. All rights reserved.

## License

All Rights Reserved. Unauthorized copying, modification, or distribution is prohibited.

## Authors

DANP Team

## Acknowledgments

- Inspired by BSD sockets and TCP/IP
- Uses ZeroMQ for transport layer abstraction
- Built with CMake for cross-platform support
