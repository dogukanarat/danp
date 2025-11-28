/* danpArch.h - one line definition */

/* All Rights Reserved */

#ifndef INC_DANP_ARCH_H
#define INC_DANP_ARCH_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes */

#include <stdbool.h>
#include <stdint.h>

/* Configurations */


/* Definitions */


/* Types */

// Opaque handles
/** @brief Handle for an OS mutex. */
typedef void* danpOsMutexHandle_t;

/** @brief Handle for an OS semaphore. */
typedef void* danpOsSemaphoreHandle_t;

/** @brief Handle for an OS queue. */
typedef void* danpOsQueueHandle_t;

/* External Declarations */

// Thread/Task creation (optional helper)
/**
 * @brief Thread function prototype.
 * @param arg Argument passed to the thread.
 */
typedef void (*DanpThreadFunc)(void* arg);

/**
 * @brief Create a new thread.
 * @param func Thread function.
 * @param arg Argument to pass to the thread function.
 * @param name Name of the thread.
 */
void danpOsThreadCreate(DanpThreadFunc func, void* arg, const char* name);

// Time

/**
 * @brief Delay execution for a specified number of milliseconds.
 * @param ms Milliseconds to delay.
 */
void danpOsDelayMs(int ms);

/**
 * @brief Get the current system tick in milliseconds.
 * @return Current system tick.
 */
uint32_t danpOsGetTickMs(void);

// Mutexes

/**
 * @brief Create a mutex.
 * @return Handle to the created mutex, or NULL on failure.
 */
danpOsMutexHandle_t danpOsMutexCreate(void);

/**
 * @brief Lock a mutex.
 * @param mutex Handle to the mutex.
 * @param timeoutMs Timeout in milliseconds.
 * @return true if locked successfully, false otherwise.
 */
bool danpOsMutexLock(danpOsMutexHandle_t mutex, int timeoutMs);

/**
 * @brief Unlock a mutex.
 * @param mutex Handle to the mutex.
 */
void danpOsMutexUnlock(danpOsMutexHandle_t mutex);

// Semaphores (Binary)

/**
 * @brief Create a binary semaphore.
 * @return Handle to the created semaphore, or NULL on failure.
 */
danpOsSemaphoreHandle_t danpOsSemCreate(void);

/**
 * @brief Take (wait for) a semaphore.
 * @param sem Handle to the semaphore.
 * @param timeoutMs Timeout in milliseconds.
 * @return true if taken successfully, false otherwise.
 */
bool danpOsSemTake(danpOsSemaphoreHandle_t sem, int timeoutMs);

/**
 * @brief Give (signal) a semaphore.
 * @param sem Handle to the semaphore.
 */
void danpOsSemGive(danpOsSemaphoreHandle_t sem);

// Queues

/**
 * @brief Create a queue.
 * @param depth Maximum number of items in the queue.
 * @param itemSize Size of each item in bytes.
 * @return Handle to the created queue, or NULL on failure.
 */
danpOsQueueHandle_t danpOsQueueCreate(int depth, int itemSize);

/**
 * @brief Send an item to a queue.
 * @param q Handle to the queue.
 * @param item Pointer to the item to send.
 * @param timeoutMs Timeout in milliseconds.
 * @return true if sent successfully, false otherwise.
 */
bool danpOsQueueSend(danpOsQueueHandle_t q, void* item, int timeoutMs);

/**
 * @brief Receive an item from a queue.
 * @param q Handle to the queue.
 * @param item Pointer to the buffer to store the received item.
 * @param timeoutMs Timeout in milliseconds.
 * @return true if received successfully, false otherwise.
 */
bool danpOsQueueReceive(danpOsQueueHandle_t q, void* item, int timeoutMs);


#ifdef __cplusplus
}
#endif

#endif /* INC_DANP_ARCH_H */
