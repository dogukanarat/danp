/* danpBuffer.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include <errno.h>
#include <stdint.h>

#include "osal/osal_mutex.h"

#include "danp/danp.h"
#include "danp_debug.h"

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief Semaphore to protect the packet pool. */
static osal_mutex_handle_t buffer_mutex;

/** @brief Pool of packets for allocation. */
static danp_packet_t packet_pool[DANP_POOL_SIZE];

/** @brief Map of free packets in the pool. */
static bool packet_free_map[DANP_POOL_SIZE];

/* Functions */

int32_t danp_buffer_init(void)
{
    int32_t status = 0;

    // Reset packet pool state
    for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
    {
        packet_free_map[i] = true;
    }

    // Skip mutex creation if already initialized
    if (buffer_mutex != NULL)
    {
        return 0;
    }

    osal_mutex_attr_t sem_attr = {
        .name = "DanpPoolLock",
        .attr_bits = OSAL_MUTEX_RECURSIVE,
        .cb_mem = NULL,
        .cb_size = 0,
    };

    buffer_mutex = osal_mutex_create(&sem_attr);
    if (!buffer_mutex)
    {
        /* LCOV_EXCL_START */
        DANP_LOG_ERR("Failed to create packet pool mutex");
        status = -ENOMEM;
        /* LCOV_EXCL_STOP */
    }

    return status;
}

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to the allocated packet, or NULL if pool is empty.
 */
danp_packet_t *danp_buffer_get(void)
{
    danp_packet_t *pkt = NULL;
    bool is_mutex_taken = false;

    for (;;)
    {
        if (0 != osal_mutex_lock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            /* LCOV_EXCL_START */
            break;
            /* LCOV_EXCL_STOP */
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
            DANP_LOG_ERR("Packet pool out of memory");
            break;
        }

        DANP_LOG_VER(
            "Buffer allocated: %p, %d/%d free",
            pkt,
            danp_buffer_get_free_count(),
            DANP_POOL_SIZE);

        break;
    }

    if (is_mutex_taken)
    {
        osal_mutex_unlock(buffer_mutex);
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
    const uintptr_t pool_start = (uintptr_t)&packet_pool[0];
    const uintptr_t pool_end = (uintptr_t)(&packet_pool[DANP_POOL_SIZE]);
    const uintptr_t pkt_addr = (uintptr_t)pkt;

    for (;;)
    {
        if (!pkt)
        {
            DANP_LOG_WRN("Attempted to free NULL packet");
            break;
        }

        if (0 != osal_mutex_lock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            /* LCOV_EXCL_START */
            break;
            /* LCOV_EXCL_STOP */
        }
        is_mutex_taken = true;

        if (pkt_addr < pool_start || pkt_addr >= pool_end)
        {
            DANP_LOG_ERR("Attempted to free invalid packet");
            break;
        }

        index = (int32_t)(pkt - packet_pool);
        if (index < 0 || index >= DANP_POOL_SIZE)
        {
            DANP_LOG_ERR("Attempted to free invalid packet");
            break;
        }

        if (packet_free_map[index])
        {
            DANP_LOG_WRN("Attempted to free already free packet");
            break;
        }

        packet_free_map[index] = true;

        DANP_LOG_VER(
            "Buffer freed: %p, %d/%d free",
            pkt,
            danp_buffer_get_free_count(),
            DANP_POOL_SIZE);

        break;
    }

    if (is_mutex_taken)
    {
        osal_mutex_unlock(buffer_mutex);
    }
}

size_t danp_buffer_get_free_count(void)
{
    size_t free_count = 0;
    bool is_mutex_taken = false;

    for (;;)
    {
        if (0 != osal_mutex_lock(buffer_mutex, OSAL_WAIT_FOREVER))
        {
            /* LCOV_EXCL_START */
            break;
            /* LCOV_EXCL_STOP */
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
        osal_mutex_unlock(buffer_mutex);
    }

    return free_count;
}

/**
 * @brief Free a packet chain (frees all linked packets).
 * @param pkt Pointer to first packet in chain.
 */
void danp_buffer_free_chain(danp_packet_t *pkt)
{
    danp_packet_t *current = pkt;
    danp_packet_t *next_pkt;

    for (;;)
    {
        if (!current)
        {
            break;
        }

        next_pkt = current->next;
        danp_buffer_free(current);
        current = next_pkt;

        if (!next_pkt)
        {
            break;
        }
    }
}
