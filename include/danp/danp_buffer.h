/* danpBuffer.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_BUFFER_H
#define INC_DANP_BUFFER_H

/* Includes */

#include "danp/danp_types.h"

#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */


/* Types */


/* External Declarations */

/**
 * @brief Initialize the packet buffer pool.
 * @return 0 on success, negative on error.
 */
extern int32_t danp_buffer_init(void);

/**
 * @brief Allocate a packet buffer.
 * @return Pointer to allocated packet, or NULL if pool is full.
 */
extern danp_packet_t *danp_buffer_get(void);

/**
 * @brief Free a packet back to the pool.
 * @param pkt Pointer to the packet to free.
 */
extern void danp_buffer_free(danp_packet_t *pkt);

/**
 * @brief Get the number of free packets in the pool.
 * @return Number of free packets.
 */
extern size_t danp_buffer_get_free_count(void);

/**
 * @brief Free a packet chain (frees all linked packets).
 * @param pkt Pointer to first packet in chain.
 */
 void danp_buffer_free_chain(danp_packet_t *pkt);

#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_BUFFER_H */
