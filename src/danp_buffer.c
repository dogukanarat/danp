/* danpBuffer.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "errno.h"
#include "osal/osal.h"
#include "danp/danp.h"
#include "danp_debug.h"

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief Semaphore to protect the packet pool. */
static osalMutexHandle_t buffer_mutex;

/** @brief Pool of packets for allocation. */
static danp_packet_t packet_pool[DANP_POOL_SIZE];

/** @brief Map of free packets in the pool. */
static bool packet_free_map[DANP_POOL_SIZE];

/* Functions */

int32_t danp_buffer_init(void)
{
    int32_t status = 0;
    osalMutexAttr_t sem_attr = {
        .name = "DanpPoolLock",
        .attrBits = OSAL_MUTEX_PRIO_INHERIT,
        .cbMem = NULL,
        .cbSize = 0,
    };

    for (;;)
    {
        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            packet_free_map[i] = true;
        }

        buffer_mutex = osalMutexCreate(&sem_attr);
        if (!buffer_mutex)
        {
            danp_log_message(DANP_LOG_ERROR, "Failed to create packet pool mutex");
            status = -ENOMEM;
        }

        danp_log_message(DANP_LOG_INFO, "DANP packet pool initialized");

        break;
    }

    return status;
}

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to the allocated packet, or NULL if pool is empty.
 */
danp_packet_t *danp_buffer_allocate(void)
{
    danp_packet_t *pkt = NULL;
    bool is_mutex_taken = false;

    for (;;)
    {
        if (0 != osalMutexLock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        is_mutex_taken = true;

        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            if (packet_free_map[i])
            {
                packet_free_map[i] = false;
                pkt = &packet_pool[i];
                break;
            }
        }
        if (!pkt)
        {
            danp_log_message(DANP_LOG_ERROR, "Packet pool out of memory");
            break;
        }

        danp_log_message(DANP_LOG_VERBOSE, "Allocated packet from pool");

        break;
    }

    if (is_mutex_taken)
    {
        osalMutexUnlock(buffer_mutex);
    }

    return pkt;
}

/**
 * @brief Free a packet back to the pool.
 * @param pkt Pointer to the packet to free.
 */
void danp_buffer_free(danp_packet_t *pkt)
{
    bool is_mutex_taken = false;
    int32_t index;

    for (;;)
    {
        if (!pkt)
        {
            danp_log_message(DANP_LOG_WARN, "Attempted to free NULL packet");
            break;
        }

        if (0 != osalMutexLock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        is_mutex_taken = true;

        index = pkt - packet_pool;
        if (index < 0 || index >= DANP_POOL_SIZE)
        {
            danp_log_message(DANP_LOG_ERROR, "Attempted to free invalid packet");
            break;
        }

        if (packet_free_map[index])
        {
            danp_log_message(DANP_LOG_WARN, "Attempted to free already free packet");
            break;
        }

        packet_free_map[index] = true;

        danp_log_message(DANP_LOG_VERBOSE, "Freed packet back to pool");

        break;
    }

    if (is_mutex_taken)
    {
        osalMutexUnlock(buffer_mutex);
    }
}

/**
 * @brief Get the number of free packets in the pool.
 * @return Number of free packets.
 */
size_t danp_buffer_get_free_count(void)
{
    size_t free_count = 0;
    bool is_mutex_taken = false;

    for (;;)
    {
        if (0 != osalMutexLock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        is_mutex_taken = true;

        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            if (packet_free_map[i])
            {
                free_count++;
            }
        }

        break;
    }

    if (is_mutex_taken)
    {
        osalMutexUnlock(buffer_mutex);
    }

    return free_count;
}
