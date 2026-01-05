/* danp_defs.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_DEFS_H
#define INC_DANP_DEFS_H

/* Includes */


#ifdef __cplusplus
extern "C" {
#endif


/* Configurations */


/* Definitions */

/** @brief Maximum size of a DANP packet payload in bytes. */
#define DANP_MAX_PACKET_SIZE 128

/** @brief Size of the packet pool. */
#define DANP_POOL_SIZE 20

/** @brief Size of the DANP header in bytes. */
#define DANP_HEADER_SIZE 4

/** @brief Maximum number of retries for reliable transmission. */
#define DANP_RETRY_LIMIT 3

/** @brief Acknowledgment timeout in milliseconds. */
#define DANP_ACK_TIMEOUT_MS 500

/** @brief Maximum number of supported ports. */
#define DANP_MAX_PORTS 64

/** @brief Maximum number of supported nodes. */
#define DANP_MAX_NODES 256

/** @brief Constant for infinite wait. */
#define DANP_WAIT_FOREVER 0xFFFFFFFFU

/** @brief High priority for packets. */
#define DANP_PRIORITY_HIGH 1

/** @brief Normal priority for packets. */
#define DANP_PRIORITY_NORMAL 0

/** @brief Maximum user data per SFP fragment (MTU - Header - SFP header = 128 - 4 - 1 = 123). */
#define DANP_SFP_MAX_DATA_PER_FRAGMENT (DANP_MAX_PACKET_SIZE - DANP_HEADER_SIZE - 1)

/** @brief Maximum number of fragments per message. */
#define DANP_SFP_MAX_FRAGMENTS 255

/** @brief SFP flag: More fragments follow. */
#define DANP_SFP_FLAG_MORE 0x80

/** @brief SFP flag: First fragment. */
#define DANP_SFP_FLAG_BEGIN 0x40

/* Types */


/* External Declarations */


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_DEFS_H */