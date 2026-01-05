# DANP Example Applications

This directory contains example applications demonstrating how to use the DANP (Data and Network Protocol) library in real-world scenarios.

## Overview

DANP examples showcase common networking patterns using both DGRAM (unreliable, UDP-like) and STREAM (reliable, TCP-like) sockets. Each example demonstrates proper initialization, interface registration, routing, and communication.

## Building Examples

To build the example applications, enable the `BUILD_EXAMPLES` option when configuring CMake:

```bash
cmake -B build -DBUILD_EXAMPLES=ON
cmake --build build
```

## Planned Examples

### 1. Echo Server (DGRAM)

**Purpose:** Demonstrates basic unreliable communication

```c
#include <danp/danp.h>

int main(void)
{
    /* Initialize DANP */
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    /* Register interface (CAN, UART, etc.) */
    danp_interface_t my_interface = {
        .name = "CAN0",
        .address = 1,
        .mtu = 64,
        .tx_func = my_tx_function
    };
    danp_register_interface(&my_interface);

    /* Create and bind DGRAM socket to echo port */
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 7);  /* Echo port */

    /* Echo loop */
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

**Features:**
- Simple DGRAM socket
- Echo server pattern
- Interface registration
- Send/receive operations

---

### 2. Stream Server/Client

**Purpose:** Demonstrates reliable connection-oriented communication

**Server:**
```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    /* Register interface */
    danp_register_interface(&my_interface);

    /* Create STREAM socket and listen */
    danp_socket_t *server = danp_socket(DANP_TYPE_STREAM);
    danp_listen(server, 8080);

    while (1)
    {
        /* Accept connection (blocks until client connects) */
        danp_socket_t *client = danp_accept(server, DANP_WAIT_FOREVER);

        if (client)
        {
            char buffer[128];
            int32_t len = danp_recv(client, buffer, sizeof(buffer), DANP_WAIT_FOREVER);

            if (len > 0)
            {
                danp_send(client, buffer, len);  /* Echo back */
            }

            danp_close(client);
        }
    }

    return 0;
}
```

**Client:**
```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 2, .log_function = NULL};
    danp_init(&config);

    danp_register_interface(&my_interface);

    /* Create STREAM socket and connect */
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    danp_connect(sock, 1, 8080);  /* Connect to node 1, port 8080 */

    /* Send data */
    const char *message = "Hello, Server!";
    danp_send(sock, message, strlen(message));

    /* Receive response */
    char buffer[128];
    int32_t len = danp_recv(sock, buffer, sizeof(buffer), DANP_WAIT_FOREVER);

    if (len > 0)
    {
        buffer[len] = '\0';
        printf("Received: %s\n", buffer);
    }

    danp_close(sock);
    return 0;
}
```

**Features:**
- Three-way handshake
- Connection establishment
- Reliable data transfer
- Acknowledgments

---

### 3. Multi-Interface Routing

**Purpose:** Demonstrates routing packets across multiple interfaces

```c
#include <danp/danp.h>

/* Two different network interfaces */
static danp_interface_t can_interface = {
    .name = "CAN0",
    .address = 1,
    .mtu = 64,
    .tx_func = can_tx_function
};

static danp_interface_t uart_interface = {
    .name = "UART0",
    .address = 1,
    .mtu = 128,
    .tx_func = uart_tx_function
};

int main(void)
{
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    /* Register both interfaces */
    danp_register_interface(&can_interface);
    danp_register_interface(&uart_interface);

    /* Load routing table */
    /* Nodes 10-19 via CAN, nodes 20-29 via UART */
    const char *routes =
        "10:CAN0,"
        "11:CAN0,"
        "20:UART0,"
        "21:UART0";
    danp_route_table_load(routes);

    /* Create socket */
    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 100);

    /* Send to different nodes (auto-routed) */
    danp_send_to(sock, "CAN message", 11, 10, 100);    /* Goes via CAN */
    danp_send_to(sock, "UART message", 12, 20, 100);   /* Goes via UART */

    return 0;
}
```

**Features:**
- Multiple interface registration
- Static routing table
- Automatic route selection
- Per-interface MTU

---

### 4. Zero-Copy Large Message Transfer (SFP)

**Purpose:** Demonstrates efficient large message transfer with fragmentation

```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    danp_register_interface(&my_interface);

    /* Create STREAM socket */
    danp_socket_t *sock = danp_socket(DANP_TYPE_STREAM);
    danp_connect(sock, 2, 9000);

    /* Send large message (>256 bytes, auto-fragmented) */
    uint8_t large_data[1024];
    /* ... fill with data ... */

    int32_t sent = danp_sfp_send(sock, large_data, sizeof(large_data));

    if (sent > 0)
    {
        printf("Sent %d bytes (fragmented)\n", sent);
    }

    danp_close(sock);
    return 0;
}
```

**Receiver:**
```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 2, .log_function = NULL};
    danp_init(&config);

    danp_register_interface(&my_interface);

    danp_socket_t *server = danp_socket(DANP_TYPE_STREAM);
    danp_listen(server, 9000);

    danp_socket_t *client = danp_accept(server, DANP_WAIT_FOREVER);

    /* Receive large message (auto-reassembled) */
    uint8_t buffer[2048];
    int32_t received = danp_sfp_recv(client, buffer, sizeof(buffer), DANP_WAIT_FOREVER);

    if (received > 0)
    {
        printf("Received %d bytes (reassembled from fragments)\n", received);
    }

    danp_close(client);
    return 0;
}
```

**Features:**
- SFP (Small Fragmentation Protocol)
- Automatic fragmentation (TX)
- Automatic reassembly (RX)
- Zero-copy buffer management
- Handles messages larger than MTU

---

### 5. Telemetry System (DGRAM)

**Purpose:** Real-world example of sensor data collection

```c
#include <danp/danp.h>

/* Sensor node (sends telemetry) */
typedef struct {
    uint16_t sensor_id;
    float temperature;
    float humidity;
    uint32_t timestamp;
} telemetry_packet_t;

int main(void)
{
    danp_config_t config = {.local_node = 10, .log_function = NULL};
    danp_init(&config);

    danp_register_interface(&can_interface);
    danp_route_table_load("1:CAN0");  /* Gateway at node 1 */

    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 5000);

    while (1)
    {
        telemetry_packet_t data = {
            .sensor_id = 10,
            .temperature = read_temperature(),
            .humidity = read_humidity(),
            .timestamp = get_timestamp()
        };

        /* Send to gateway node 1, port 5000 */
        danp_send_to(sock, &data, sizeof(data), 1, 5000);

        sleep_ms(1000);  /* Send every second */
    }

    return 0;
}
```

**Gateway (collects telemetry):**
```c
#include <danp/danp.h>

int main(void)
{
    danp_config_t config = {.local_node = 1, .log_function = NULL};
    danp_init(&config);

    danp_register_interface(&can_interface);

    danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
    danp_bind(sock, 5000);

    while (1)
    {
        telemetry_packet_t data;
        uint16_t src_node, src_port;

        int32_t len = danp_recv_from(sock, &data, sizeof(data),
                                      &src_node, &src_port, DANP_WAIT_FOREVER);

        if (len == sizeof(data))
        {
            printf("Sensor %u: temp=%.1f°C, humidity=%.1f%%\n",
                   data.sensor_id, data.temperature, data.humidity);
        }
    }

    return 0;
}
```

**Features:**
- Unreliable (fast) telemetry
- Structured data packets
- Periodic transmission
- Multiple sensor support

---

## Common Patterns

### Interface Implementation

All examples require implementing a transmit function for your hardware interface:

```c
static int32_t can_tx(void *iface_common, danp_packet_t *packet)
{
    /* Get CAN hardware handle */
    can_handle_t *can = (can_handle_t *)iface_common;

    /* Build frame: header + payload */
    uint8_t frame[DANP_HEADER_SIZE + DANP_MAX_PACKET_SIZE];
    memcpy(frame, &packet->header_raw, DANP_HEADER_SIZE);

    if (packet->length > 0)
    {
        memcpy(frame + DANP_HEADER_SIZE, packet->payload, packet->length);
    }

    /* Transmit via hardware */
    return can_transmit(can, frame, DANP_HEADER_SIZE + packet->length);
}
```

### Receiving Packets

When data arrives from hardware, call `danp_input()`:

```c
void can_rx_interrupt(void)
{
    uint8_t frame[128];
    uint16_t length;

    /* Get frame from CAN hardware */
    if (can_receive(frame, &length) == 0)
    {
        /* Feed to DANP input */
        danp_input(&can_interface, frame, length);
    }
}
```

### Error Handling

All DANP functions return status codes:

```c
int32_t rc = danp_send_to(sock, data, len, dest_node, dest_port);

if (rc < 0)
{
    printf("Send failed\n");
    /* Handle error */
}
else
{
    printf("Sent %d bytes\n", rc);
}
```

---

## Integration with RTOS

DANP works well with FreeRTOS, Zephyr, and other RTOS:

```c
/* Task 1: Receive and process */
void task_network_rx(void *params)
{
    danp_socket_t *sock = (danp_socket_t *)params;

    while (1)
    {
        char buffer[128];
        uint16_t src_node, src_port;

        int32_t len = danp_recv_from(sock, buffer, sizeof(buffer),
                                      &src_node, &src_port, 100);  /* 100ms timeout */

        if (len > 0)
        {
            process_message(buffer, len, src_node);
        }
    }
}

/* Task 2: Periodic transmit */
void task_network_tx(void *params)
{
    danp_socket_t *sock = (danp_socket_t *)params;

    while (1)
    {
        send_status_update(sock);
        vTaskDelay(pdMS_TO_TICKS(5000));  /* Every 5 seconds */
    }
}
```

---

## Dependencies

Examples require:
- **DANP Library**: Installed via `./ci/install.sh`
- **OSAL**: Operating System Abstraction Layer
- **Hardware Interface**: CAN, UART, SPI, etc. (user-provided)

---

## Building Your Own Application

1. **Include DANP headers**
   ```c
   #include <danp/danp.h>
   ```

2. **Link against DANP**
   ```cmake
   find_package(Danp REQUIRED)
   target_link_libraries(myapp PRIVATE Danp::Danp)
   ```

3. **Initialize library**
   ```c
   danp_config_t config = {.local_node = YOUR_NODE_ID};
   danp_init(&config);
   ```

4. **Register interface(s)**
   ```c
   danp_register_interface(&your_interface);
   ```

5. **Load routing table** (optional)
   ```c
   danp_route_table_load("node:interface,...");
   ```

6. **Create socket and communicate**
   ```c
   danp_socket_t *sock = danp_socket(DANP_TYPE_DGRAM);
   danp_bind(sock, port);
   /* ... send/receive ... */
   ```

---

## Debugging Tips

1. **Enable test logging during development**
   ```bash
   cmake -B build -DBUILD_TESTS=ON -DBUILD_TESTS_WITH_LOGS=ON
   ```

2. **Check routing table**
   ```c
   danp_print_stats(printf);  /* Shows routing and buffer stats */
   ```

3. **Verify interface registration**
   - Ensure `tx_func` is not NULL
   - Verify MTU is set correctly
   - Check interface name matches routing table

4. **Monitor buffer pool**
   ```c
   uint32_t free_count = danp_buffer_get_free_count();
   printf("Free buffers: %u/%u\n", free_count, DANP_POOL_SIZE);
   ```

---

## Performance Considerations

- **DGRAM**: Faster, no handshake overhead
- **STREAM**: Reliable but slower (handshake + ACKs)
- **SFP**: Adds fragmentation overhead for large messages
- **Zero-Copy**: Eliminates memcpy for large transfers
- **Buffer Pool**: Pre-allocated, no malloc in runtime

---

## Further Reading

- [DANP API Documentation](../include/danp/danp.h)
- [Test Examples](../test/) - Real usage patterns
- [Main README](../README.md) - Complete API reference
- [CI Scripts](../ci/README.md) - Build and install guide

---

## Repository

GitHub: [dogukanarat/danp](https://github.com/dogukanarat/danp)
