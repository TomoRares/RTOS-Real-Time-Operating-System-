/**
 * @file rtos_sync.c
 * @brief Synchronization Primitives Implementation
 *
 * Contains semaphore, mutex (with priority inheritance), and message queue.
 */

#include "rtos.h"
#include "rtos_internal.h"
#include "stm32f4xx.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
/* External References */
/*---------------------------------------------------------------------------*/
extern rtos_kernel_t g_kernel;

/*---------------------------------------------------------------------------*/
/* Helper: Block Current Task on Wait List */
/*---------------------------------------------------------------------------*/
static rtos_status_t block_on_wait_list(rtos_list_t *wait_list, void *wait_obj,
                                         uint32_t timeout_ms) {
    rtos_tcb_t *current = g_kernel.current_task;

    /* Add to wait list (priority sorted for fair scheduling) */
    rtos_list_add_priority(wait_list, current);

    current->state = RTOS_TASK_BLOCKED;
    current->wait_object = wait_obj;

    if (timeout_ms != RTOS_WAIT_FOREVER) {
        /* Add to delay list for timeout */
        uint32_t ticks = (timeout_ms * RTOS_TICK_RATE_HZ) / 1000;
        if (ticks == 0) ticks = 1;
        current->wake_tick = g_kernel.tick_count + ticks;
    } else {
        current->wake_tick = 0;  /* No timeout */
    }

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Helper: Wake Task from Wait List */
/*---------------------------------------------------------------------------*/
static rtos_tcb_t *wake_highest_priority_waiter(rtos_list_t *wait_list) {
    rtos_tcb_t *tcb = rtos_list_pop_head(wait_list);

    if (tcb != NULL) {
        /* Remove from delay list if it was there */
        if (tcb->wake_tick != 0) {
            rtos_list_remove(&g_kernel.delay_list, tcb);
        }

        tcb->wait_object = NULL;
        rtos_add_ready(tcb);
    }

    return tcb;
}

/*---------------------------------------------------------------------------*/
/* Binary Semaphore */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_sem_init(rtos_sem_t *sem, uint32_t initial) {
    if (sem == NULL) {
        return RTOS_ERR_PARAM;
    }

    sem->count = initial ? 1 : 0;
    rtos_list_init(&sem->wait_list);

    return RTOS_OK;
}

rtos_status_t rtos_sem_wait(rtos_sem_t *sem, uint32_t timeout_ms) {
    if (sem == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Try to take semaphore */
    if (sem->count > 0) {
        sem->count--;
        rtos_exit_critical(state);
        return RTOS_OK;
    }

    /* Semaphore not available */
    if (timeout_ms == RTOS_NO_WAIT) {
        rtos_exit_critical(state);
        return RTOS_ERR_RESOURCE;
    }

    /* Block current task */
    block_on_wait_list(&sem->wait_list, sem, timeout_ms);

    rtos_exit_critical(state);

    /* Trigger context switch */
    rtos_trigger_context_switch();

    /* When we wake up, check if we got the semaphore or timed out */
    state = rtos_enter_critical();

    rtos_status_t result = RTOS_OK;

    /* If we're still on the wait list, we timed out */
    if (g_kernel.current_task->wait_object == sem) {
        rtos_list_remove(&sem->wait_list, g_kernel.current_task);
        g_kernel.current_task->wait_object = NULL;
        result = RTOS_ERR_TIMEOUT;
    }

    rtos_exit_critical(state);

    return result;
}

rtos_status_t rtos_sem_post(rtos_sem_t *sem) {
    if (sem == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Check if any task is waiting */
    if (!rtos_list_is_empty(&sem->wait_list)) {
        /* Wake highest priority waiter */
        rtos_tcb_t *woken = wake_highest_priority_waiter(&sem->wait_list);

        rtos_exit_critical(state);

        /* If woken task has higher priority, yield */
        if (woken != NULL && g_kernel.scheduler_running &&
            woken->priority < g_kernel.current_task->priority) {
            rtos_trigger_context_switch();
        }
    } else {
        /* No waiters, increment count */
        if (sem->count < 1) {  /* Binary semaphore max is 1 */
            sem->count++;
        }
        rtos_exit_critical(state);
    }

    return RTOS_OK;
}

rtos_status_t rtos_sem_try(rtos_sem_t *sem) {
    return rtos_sem_wait(sem, RTOS_NO_WAIT);
}

/*---------------------------------------------------------------------------*/
/* Mutex with Priority Inheritance */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_mutex_init(rtos_mutex_t *mtx) {
    if (mtx == NULL) {
        return RTOS_ERR_PARAM;
    }

    mtx->owner = NULL;
    mtx->original_priority = 0;
    mtx->lock_count = 0;
    rtos_list_init(&mtx->wait_list);

    return RTOS_OK;
}

rtos_status_t rtos_mutex_lock(rtos_mutex_t *mtx, uint32_t timeout_ms) {
    if (mtx == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    rtos_tcb_t *current = g_kernel.current_task;

    /* Check if mutex is free */
    if (mtx->owner == NULL) {
        /* Acquire mutex */
        mtx->owner = current;
        mtx->original_priority = current->priority;
        mtx->lock_count = 1;
        rtos_exit_critical(state);
        return RTOS_OK;
    }

    /* Check for recursive lock */
    if (mtx->owner == current) {
        mtx->lock_count++;
        rtos_exit_critical(state);
        return RTOS_OK;
    }

    /* Mutex is owned by another task */
    if (timeout_ms == RTOS_NO_WAIT) {
        rtos_exit_critical(state);
        return RTOS_ERR_RESOURCE;
    }

#if RTOS_ENABLE_PRIORITY_INHERITANCE
    /* Priority inheritance: boost owner's priority if needed */
    if (current->priority < mtx->owner->priority) {
        /* Current task has higher priority (lower number) */
        /* Boost owner's priority */
        rtos_tcb_t *owner = mtx->owner;

        /* Remove owner from ready list at old priority */
        if (owner->state == RTOS_TASK_READY) {
            rtos_remove_ready(owner);
            owner->priority = current->priority;
            rtos_add_ready(owner);
        } else {
            owner->priority = current->priority;
        }
    }
#endif

    /* Block current task */
    block_on_wait_list(&mtx->wait_list, mtx, timeout_ms);

    rtos_exit_critical(state);

    /* Trigger context switch */
    rtos_trigger_context_switch();

    /* When we wake up, check result */
    state = rtos_enter_critical();

    rtos_status_t result = RTOS_OK;

    if (g_kernel.current_task->wait_object == mtx) {
        /* Still on wait list = timed out */
        rtos_list_remove(&mtx->wait_list, g_kernel.current_task);
        g_kernel.current_task->wait_object = NULL;
        result = RTOS_ERR_TIMEOUT;
    }

    rtos_exit_critical(state);

    return result;
}

rtos_status_t rtos_mutex_unlock(rtos_mutex_t *mtx) {
    if (mtx == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    rtos_tcb_t *current = g_kernel.current_task;

    /* Check if we own the mutex */
    if (mtx->owner != current) {
        rtos_exit_critical(state);
        return RTOS_ERR_STATE;
    }

    /* Decrement lock count for recursive mutex */
    mtx->lock_count--;
    if (mtx->lock_count > 0) {
        rtos_exit_critical(state);
        return RTOS_OK;
    }

#if RTOS_ENABLE_PRIORITY_INHERITANCE
    /* Restore original priority */
    if (current->priority != mtx->original_priority) {
        /* Remove from ready list at boosted priority */
        if (current->state == RTOS_TASK_READY) {
            rtos_remove_ready(current);
            current->priority = mtx->original_priority;
            rtos_add_ready(current);
        } else {
            current->priority = mtx->original_priority;
        }
    }
#endif

    /* Release mutex */
    mtx->owner = NULL;

    /* Wake highest priority waiter if any */
    if (!rtos_list_is_empty(&mtx->wait_list)) {
        rtos_tcb_t *woken = rtos_list_pop_head(&mtx->wait_list);

        if (woken != NULL) {
            /* Remove from delay list if necessary */
            if (woken->wake_tick != 0) {
                rtos_list_remove(&g_kernel.delay_list, woken);
            }

            /* Transfer ownership to woken task */
            mtx->owner = woken;
            mtx->original_priority = woken->base_priority;
            mtx->lock_count = 1;

            woken->wait_object = NULL;
            rtos_add_ready(woken);

            rtos_exit_critical(state);

            /* Yield if woken task has higher priority */
            if (g_kernel.scheduler_running &&
                woken->priority < current->priority) {
                rtos_trigger_context_switch();
            }

            return RTOS_OK;
        }
    }

    rtos_exit_critical(state);
    return RTOS_OK;
}

rtos_status_t rtos_mutex_try(rtos_mutex_t *mtx) {
    return rtos_mutex_lock(mtx, RTOS_NO_WAIT);
}

/*---------------------------------------------------------------------------*/
/* Message Queue */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_queue_init(rtos_queue_t *q, void *buffer,
                               uint32_t msg_size, uint32_t capacity) {
    if (q == NULL || buffer == NULL || msg_size == 0 || capacity == 0) {
        return RTOS_ERR_PARAM;
    }

    q->buffer = (uint8_t *)buffer;
    q->msg_size = msg_size;
    q->capacity = capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    rtos_list_init(&q->send_wait);
    rtos_list_init(&q->recv_wait);

    return RTOS_OK;
}

rtos_status_t rtos_queue_send(rtos_queue_t *q, const void *msg, uint32_t timeout_ms) {
    if (q == NULL || msg == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Check if queue has space */
    if (q->count < q->capacity) {
        /* Copy message to queue */
        memcpy(&q->buffer[q->head * q->msg_size], msg, q->msg_size);
        q->head = (q->head + 1) % q->capacity;
        q->count++;

        /* Wake a waiting receiver if any */
        if (!rtos_list_is_empty(&q->recv_wait)) {
            rtos_tcb_t *woken = wake_highest_priority_waiter(&q->recv_wait);

            rtos_exit_critical(state);

            if (woken != NULL && g_kernel.scheduler_running &&
                woken->priority < g_kernel.current_task->priority) {
                rtos_trigger_context_switch();
            }

            return RTOS_OK;
        }

        rtos_exit_critical(state);
        return RTOS_OK;
    }

    /* Queue full */
    if (timeout_ms == RTOS_NO_WAIT) {
        rtos_exit_critical(state);
        return RTOS_ERR_RESOURCE;
    }

    /* Block on send wait list */
    block_on_wait_list(&q->send_wait, q, timeout_ms);

    rtos_exit_critical(state);
    rtos_trigger_context_switch();

    /* Check if we can send now or timed out */
    state = rtos_enter_critical();

    if (g_kernel.current_task->wait_object == q) {
        rtos_list_remove(&q->send_wait, g_kernel.current_task);
        g_kernel.current_task->wait_object = NULL;
        rtos_exit_critical(state);
        return RTOS_ERR_TIMEOUT;
    }

    /* Try to send again */
    if (q->count < q->capacity) {
        memcpy(&q->buffer[q->head * q->msg_size], msg, q->msg_size);
        q->head = (q->head + 1) % q->capacity;
        q->count++;
        rtos_exit_critical(state);
        return RTOS_OK;
    }

    rtos_exit_critical(state);
    return RTOS_ERR_RESOURCE;
}

rtos_status_t rtos_queue_recv(rtos_queue_t *q, void *msg, uint32_t timeout_ms) {
    if (q == NULL || msg == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Check if queue has messages */
    if (q->count > 0) {
        /* Copy message from queue */
        memcpy(msg, &q->buffer[q->tail * q->msg_size], q->msg_size);
        q->tail = (q->tail + 1) % q->capacity;
        q->count--;

        /* Wake a waiting sender if any */
        if (!rtos_list_is_empty(&q->send_wait)) {
            rtos_tcb_t *woken = wake_highest_priority_waiter(&q->send_wait);

            rtos_exit_critical(state);

            if (woken != NULL && g_kernel.scheduler_running &&
                woken->priority < g_kernel.current_task->priority) {
                rtos_trigger_context_switch();
            }

            return RTOS_OK;
        }

        rtos_exit_critical(state);
        return RTOS_OK;
    }

    /* Queue empty */
    if (timeout_ms == RTOS_NO_WAIT) {
        rtos_exit_critical(state);
        return RTOS_ERR_RESOURCE;
    }

    /* Block on receive wait list */
    block_on_wait_list(&q->recv_wait, q, timeout_ms);

    rtos_exit_critical(state);
    rtos_trigger_context_switch();

    /* Check if we can receive now or timed out */
    state = rtos_enter_critical();

    if (g_kernel.current_task->wait_object == q) {
        rtos_list_remove(&q->recv_wait, g_kernel.current_task);
        g_kernel.current_task->wait_object = NULL;
        rtos_exit_critical(state);
        return RTOS_ERR_TIMEOUT;
    }

    /* Try to receive again */
    if (q->count > 0) {
        memcpy(msg, &q->buffer[q->tail * q->msg_size], q->msg_size);
        q->tail = (q->tail + 1) % q->capacity;
        q->count--;
        rtos_exit_critical(state);
        return RTOS_OK;
    }

    rtos_exit_critical(state);
    return RTOS_ERR_RESOURCE;
}

uint32_t rtos_queue_count(rtos_queue_t *q) {
    if (q == NULL) return 0;
    return q->count;
}

uint8_t rtos_queue_is_empty(rtos_queue_t *q) {
    if (q == NULL) return 1;
    return (q->count == 0) ? 1 : 0;
}

uint8_t rtos_queue_is_full(rtos_queue_t *q) {
    if (q == NULL) return 0;
    return (q->count >= q->capacity) ? 1 : 0;
}
