/* danp_radio.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_RADIO_H
#define INC_DANP_RADIO_H

/* Includes */

#include "danp/danp.h"
#include "zephyr/drivers/radio_ctrl.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */

typedef struct danp_radio_interface_s
{
    danp_interface_t common;
    void *context;
} danp_radio_interface_t;

/* External Declarations */

int32_t danp_radio_init (
    danp_radio_interface_t *iface,
    const struct device *radio_dev,
    const ralf_params_lora_t *rx_params,
    const ralf_params_lora_t *tx_params,
    const ralf_params_lora_cad_t *cad_params,
    int16_t address);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_RADIO_H */
