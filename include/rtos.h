/**
 * @file rtos.h
 * @brief Public RTOS API
 *
 * This header provides the public API for the RTOS including:
 * - Task creation and management
 * - Synchronization primitives (semaphores, mutexes, queues)
 * - Soft timers
 * - Time management
 */

#ifndef RTOS_H
#define RTOS_H

#include <stdint.h>
#include <stddef.h>
#include "rtos_config.h"
#include "rtos_internal.h"

#ifdef __cplusplus
extern "C" {
#endif

/*---------------------------------------------------------------------------*/
/* Type Definitions */
/*---------------------------------------------------------------------------*/

/**
 * @brief Task function prototype
 */
typedef void (*rtos_task_fn_t)(void *arg);

/*---------------------------------------------------------------------------*/
/* Kernel API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize the RTOS kernel
 * @note Must be called before any other RTOS functions
 */
void rtos_init(void);

/**
 * @brief Start the RTOS scheduler
 * @note This function does not return
 */
void rtos_start(void);

/**
 * @brief Get current tick count
 * @return Current system tick count
 */
uint32_t rtos_now(void);

/**
 * @brief Check if scheduler is running
 * @return 1 if running, 0 otherwise
 */
uint8_t rtos_is_running(void);

/**
 * @brief Check if currently in ISR context
 * @return 1 if in ISR, 0 otherwise
 */
uint8_t rtos_in_isr(void);

/*---------------------------------------------------------------------------*/
/* Task API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Create a new task
 * @param fn Task function
 * @param name Task name (max 15 chars)
 * @param priority Task priority (0 = highest, RTOS_MAX_PRIORITIES-1 = lowest)
 * @param stack Stack memory (must be word-aligned)
 * @param stack_size Stack size in words
 * @param tcb Pointer to TCB storage
 * @param arg Argument passed to task function
 * @return RTOS_OK on success, error code otherwise
 */
rtos_status_t rtos_task_create(rtos_task_fn_t fn, const char *name,
                                uint8_t priority, uint32_t *stack,
                                uint32_t stack_size, rtos_tcb_t *tcb,
                                void *arg);

/**
 * @brief Yield CPU to another task of same priority
 */
void rtos_yield(void);

/**
 * @brief Delay current task for specified milliseconds
 * @param ms Delay in milliseconds
 */
void rtos_delay(uint32_t ms);

/**
 * @brief Delay current task until specified tick count
 * @param wake_tick Absolute tick count to wake at
 */
void rtos_delay_until(uint32_t wake_tick);

/**
 * @brief Suspend a task
 * @param tcb Task to suspend (NULL for current task)
 * @return RTOS_OK on success
 */
rtos_status_t rtos_task_suspend(rtos_tcb_t *tcb);

/**
 * @brief Resume a suspended task
 * @param tcb Task to resume
 * @return RTOS_OK on success
 */
rtos_status_t rtos_task_resume(rtos_tcb_t *tcb);

/**
 * @brief Get current task TCB
 * @return Pointer to current task's TCB
 */
rtos_tcb_t *rtos_task_current(void);

/**
 * @brief Get task name
 * @param tcb Task TCB
 * @return Task name string
 */
const char *rtos_task_name(rtos_tcb_t *tcb);

/**
 * @brief Get task priority
 * @param tcb Task TCB
 * @return Task priority
 */
uint8_t rtos_task_priority(rtos_tcb_t *tcb);

/*---------------------------------------------------------------------------*/
/* Semaphore API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize a binary semaphore
 * @param sem Pointer to semaphore structure
 * @param initial Initial count (0 or 1)
 * @return RTOS_OK on success
 */
rtos_status_t rtos_sem_init(rtos_sem_t *sem, uint32_t initial);

/**
 * @brief Wait on a semaphore (decrement/take)
 * @param sem Semaphore to wait on
 * @param timeout_ms Timeout in ms (RTOS_WAIT_FOREVER for infinite)
 * @return RTOS_OK on success, RTOS_ERR_TIMEOUT on timeout
 */
rtos_status_t rtos_sem_wait(rtos_sem_t *sem, uint32_t timeout_ms);

/**
 * @brief Post to a semaphore (increment/give)
 * @param sem Semaphore to post
 * @return RTOS_OK on success
 */
rtos_status_t rtos_sem_post(rtos_sem_t *sem);

/**
 * @brief Try to take semaphore without blocking
 * @param sem Semaphore to try
 * @return RTOS_OK if taken, RTOS_ERR_RESOURCE if not available
 */
rtos_status_t rtos_sem_try(rtos_sem_t *sem);

/*---------------------------------------------------------------------------*/
/* Mutex API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize a mutex
 * @param mtx Pointer to mutex structure
 * @return RTOS_OK on success
 */
rtos_status_t rtos_mutex_init(rtos_mutex_t *mtx);

/**
 * @brief Lock a mutex
 * @param mtx Mutex to lock
 * @param timeout_ms Timeout in ms (RTOS_WAIT_FOREVER for infinite)
 * @return RTOS_OK on success, RTOS_ERR_TIMEOUT on timeout
 * @note Supports priority inheritance
 */
rtos_status_t rtos_mutex_lock(rtos_mutex_t *mtx, uint32_t timeout_ms);

/**
 * @brief Unlock a mutex
 * @param mtx Mutex to unlock
 * @return RTOS_OK on success, RTOS_ERR_STATE if not owner
 */
rtos_status_t rtos_mutex_unlock(rtos_mutex_t *mtx);

/**
 * @brief Try to lock mutex without blocking
 * @param mtx Mutex to try
 * @return RTOS_OK if locked, RTOS_ERR_RESOURCE if not available
 */
rtos_status_t rtos_mutex_try(rtos_mutex_t *mtx);

/*---------------------------------------------------------------------------*/
/* Queue API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize a message queue
 * @param q Pointer to queue structure
 * @param buffer Storage buffer
 * @param msg_size Size of each message in bytes
 * @param capacity Maximum number of messages
 * @return RTOS_OK on success
 */
rtos_status_t rtos_queue_init(rtos_queue_t *q, void *buffer,
                               uint32_t msg_size, uint32_t capacity);

/**
 * @brief Send message to queue
 * @param q Queue to send to
 * @param msg Pointer to message data
 * @param timeout_ms Timeout in ms (RTOS_WAIT_FOREVER for infinite)
 * @return RTOS_OK on success, RTOS_ERR_TIMEOUT on timeout
 */
rtos_status_t rtos_queue_send(rtos_queue_t *q, const void *msg, uint32_t timeout_ms);

/**
 * @brief Receive message from queue
 * @param q Queue to receive from
 * @param msg Buffer to store received message
 * @param timeout_ms Timeout in ms (RTOS_WAIT_FOREVER for infinite)
 * @return RTOS_OK on success, RTOS_ERR_TIMEOUT on timeout
 */
rtos_status_t rtos_queue_recv(rtos_queue_t *q, void *msg, uint32_t timeout_ms);

/**
 * @brief Get number of messages in queue
 * @param q Queue to check
 * @return Number of messages currently in queue
 */
uint32_t rtos_queue_count(rtos_queue_t *q);

/**
 * @brief Check if queue is empty
 * @param q Queue to check
 * @return 1 if empty, 0 otherwise
 */
uint8_t rtos_queue_is_empty(rtos_queue_t *q);

/**
 * @brief Check if queue is full
 * @param q Queue to check
 * @return 1 if full, 0 otherwise
 */
uint8_t rtos_queue_is_full(rtos_queue_t *q);

/*---------------------------------------------------------------------------*/
/* Timer API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize a timer structure
 * @param timer Pointer to timer structure
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_init(rtos_timer_t *timer);

/**
 * @brief Start a periodic timer
 * @param timer Timer to start
 * @param period_ms Period in milliseconds
 * @param callback Function to call on expiry
 * @param arg Argument passed to callback
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_start(rtos_timer_t *timer, uint32_t period_ms,
                                rtos_timer_cb_t callback, void *arg);

/**
 * @brief Start a one-shot timer
 * @param timer Timer to start
 * @param delay_ms Delay in milliseconds
 * @param callback Function to call on expiry
 * @param arg Argument passed to callback
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_start_once(rtos_timer_t *timer, uint32_t delay_ms,
                                     rtos_timer_cb_t callback, void *arg);

/**
 * @brief Stop a timer
 * @param timer Timer to stop
 * @return RTOS_OK on success
 */
rtos_status_t rtos_timer_stop(rtos_timer_t *timer);

/**
 * @brief Check if timer is active
 * @param timer Timer to check
 * @return 1 if active, 0 otherwise
 */
uint8_t rtos_timer_is_active(rtos_timer_t *timer);

/*---------------------------------------------------------------------------*/
/* Critical Section API */
/*---------------------------------------------------------------------------*/

/**
 * @brief Enter critical section (disable interrupts)
 * @return Previous interrupt state (for nesting)
 */
uint32_t rtos_critical_enter(void);

/**
 * @brief Exit critical section (restore interrupts)
 * @param state Previous interrupt state from rtos_critical_enter
 */
void rtos_critical_exit(uint32_t state);

/*---------------------------------------------------------------------------*/
/* Statistics API (if enabled) */
/*---------------------------------------------------------------------------*/

#if RTOS_ENABLE_STATS
/**
 * @brief Get total context switch count
 * @return Number of context switches since boot
 */
uint32_t rtos_stats_context_switches(void);

/**
 * @brief Get idle tick count
 * @return Number of ticks spent in idle task
 */
uint32_t rtos_stats_idle_ticks(void);

/**
 * @brief Get task run count
 * @param tcb Task TCB
 * @return Number of times task has been scheduled
 */
uint32_t rtos_stats_task_runs(rtos_tcb_t *tcb);
#endif

#ifdef __cplusplus
}
#endif

#endif /* RTOS_H */
