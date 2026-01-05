/* danpZmq.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_ZMQ_H
#define INC_DANP_ZMQ_H

/* Includes */

#include <stdint.h>
#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */

typedef struct danp_zmq_interface_s
{
    danp_interface_t common;
    void *pub_sock;
    void *sub_sock;
    uint32_t rx_thread_id;
} danp_zmq_interface_t;

/* External Declarations */

extern void danp_zmq_init(
    danp_zmq_interface_t *iface,
    const char *pub_bind_endpoint,
    const char **sub_connect_endpoints,
    int32_t sub_count,
    uint16_t node_id);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_ZMQ_H */
