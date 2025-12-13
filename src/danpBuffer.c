/* danpBuffer.c - one line definition */

/* All Rights Reserved */

/* Includes */

#include "errno.h"
#include "osal/osal.h"
#include "danp/danp.h"
#include "danpDebug.h"

/* Imports */


/* Definitions */


/* Types */


/* Forward Declarations */


/* Variables */

/** @brief Semaphore to protect the packet pool. */
static osalMutexHandle_t BufferMutex;

/** @brief Pool of packets for allocation. */
static danpPacket_t PacketPool[DANP_POOL_SIZE];

/** @brief Map of free packets in the pool. */
static bool PacketFreeMap[DANP_POOL_SIZE];

/* Functions */

int32_t danpBufferInit(void)
{
    int32_t status = 0;
    osalMutexAttr_t semAttr = {
        .name = "DanpPoolLock",
        .attrBits = OSAL_MUTEX_PRIO_INHERIT,
        .cbMem = NULL,
        .cbSize = 0,
    };

    for (;;)
    {
        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            PacketFreeMap[i] = true;
        }

        BufferMutex = osalMutexCreate(&semAttr);
        if (!BufferMutex)
        {
            danpLogMessage(DANP_LOG_ERROR, "Failed to create packet pool mutex");
            status = -ENOMEM;
        }

        danpLogMessage(DANP_LOG_INFO, "DANP packet pool initialized");

        break;
    }

    return status;
}

/**
 * @brief Allocate a packet from the pool.
 * @return Pointer to the allocated packet, or NULL if pool is empty.
 */
danpPacket_t *danpBufferAllocate(void)
{
    danpPacket_t *pkt = NULL;
    bool isMutexTaken = false;

    for (;;)
    {
        if (0 != osalMutexLock(BufferMutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        isMutexTaken = true;

        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            if (PacketFreeMap[i])
            {
                PacketFreeMap[i] = false;
                pkt = &PacketPool[i];
                break;
            }
        }
        if (!pkt)
        {
            danpLogMessage(DANP_LOG_ERROR, "Packet pool out of memory");
            break;
        }

        danpLogMessage(DANP_LOG_VERBOSE, "Allocated packet from pool");

        break;
    }

    if (isMutexTaken)
    {
        osalMutexUnlock(BufferMutex);
    }

    return pkt;
}

/**
 * @brief Free a packet back to the pool.
 * @param pkt Pointer to the packet to free.
 */
void danpBufferFree(danpPacket_t *pkt)
{
    bool isMutexTaken = false;
    int32_t index;

    for (;;)
    {
        if (!pkt)
        {
            danpLogMessage(DANP_LOG_WARN, "Attempted to free NULL packet");
            break;
        }

        if (0 != osalMutexLock(BufferMutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        isMutexTaken = true;

        index = pkt - PacketPool;
        if (index < 0 || index >= DANP_POOL_SIZE)
        {
            danpLogMessage(DANP_LOG_ERROR, "Attempted to free invalid packet");
            break;
        }

        if (PacketFreeMap[index])
        {
            danpLogMessage(DANP_LOG_WARN, "Attempted to free already free packet");
            break;
        }

        PacketFreeMap[index] = true;

        danpLogMessage(DANP_LOG_VERBOSE, "Freed packet back to pool");

        break;
    }

    if (isMutexTaken)
    {
        osalMutexUnlock(BufferMutex);
    }
}

/**
 * @brief Get the number of free packets in the pool.
 * @return Number of free packets.
 */
size_t danpBufferGetFreeCount(void)
{
    size_t freeCount = 0;
    bool isMutexTaken = false;

    for (;;)
    {
        if (0 != osalMutexLock(BufferMutex, OSAL_WAIT_FOREVER))
        {
            break;
        }
        isMutexTaken = true;

        for (int32_t i = 0; i < DANP_POOL_SIZE; i++)
        {
            if (PacketFreeMap[i])
            {
                freeCount++;
            }
        }

        break;
    }

    if (isMutexTaken)
    {
        osalMutexUnlock(BufferMutex);
    }

    return freeCount;
}
