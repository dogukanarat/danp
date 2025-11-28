/* client.c - STREAM Client Example */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <stdint.h>
#include <assert.h>
#include "danp/danp.h"
#include "../exampleDefinitions.h" 

extern void danpZmqInit(const char* pubBindEndpoint, const char** subConnectEndpoints, int subCount, uint16_t nodeId);
extern void danpLogMessageCallback(danpLogLevel_t level, const char *funcName, const char* message, va_list args);

void taskClient(void* arg) {
    int32_t ret = 0;
    danpOsDelayMs(1000); // Wait for server to boot
    printf("[Client] Starting Stream Client...\n");

    // 1. Create Socket (STREAM)
    danpSocket_t* sockStream = danpSocket(DANP_TYPE_STREAM);
    
    // 2. Connect to Server (STREAM)
    int32_t connectResult = danpConnect(sockStream, NODE_SERVER, STREAM_PORT);
    assert(connectResult == 0);
    
    for(int i=0; i<5; i++) {
        char msg[32];
        sprintf(msg, "Hello %d", i);
        printf("[Client] Sending: %s\n", msg);
        
        ret = danpSend(sockStream, msg, strlen(msg));
        if (ret < 0) {
            printf("[Client] Send Failed! Breaking stream loop.\n");
            break; // Break the loop on send failure
        }
        
        // Wait for reply
        char reply[64];
        ret = danpRecv(sockStream, reply, sizeof(reply)-1, 5000);
        if (ret < 0) {
            printf("[Client] Receive Failed! Breaking stream loop.\n");
            break; // Break the loop on receive failure
        }
        reply[ret] = '\0';
        printf("[Client] Got Reply: %s\n", reply);
        
        danpOsDelayMs(1000);
    }
    
    danpClose(sockStream); // Close the stream socket
    printf("[Client] Stream socket closed.\n");
}

int main() {
    const char* subEndpoints[] = { "tcp://localhost:5555" };
    danpZmqInit("tcp://*:5556", subEndpoints, 1, NODE_CLIENT);
    danpConfig_t config = 
    { 
        .localNode = NODE_CLIENT,
        .logFunction = danpLogMessageCallback,
    };
    danpInit(&config);
    danpOsThreadCreate(taskClient, NULL, "Client");

    while(1) {
        danpOsDelayMs(1000);
    }

    return 0;
}
