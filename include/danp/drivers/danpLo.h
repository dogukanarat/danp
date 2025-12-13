/* danpLo.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_LO_H
#define INC_DANP_LO_H

/* Includes */

#include "danp/danp.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */

typedef struct danp_lo_interface_s
{
    danp_interface_t common;
    void *context;
} danp_lo_interface_t;

/* External Declarations */

extern int32_t danp_lo_init(danp_lo_interface_t *iface, uint16_t address);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_LO_H */
