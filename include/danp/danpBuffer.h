/* danpBuffer.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_BUFFER_H
#define INC_DANP_BUFFER_H

/* Includes */

#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */


/* External Declarations */

extern int32_t danp_buffer_init(void);
extern danp_packet_t *danp_buffer_allocate(void);
extern void danp_buffer_free(danp_packet_t *pkt);
extern size_t danp_buffer_get_free_count(void);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_BUFFER_H */
