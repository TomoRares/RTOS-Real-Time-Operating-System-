/**
 * @file rtos_task.c
 * @brief Task Management Implementation
 *
 * Contains task creation, deletion, suspension, and delay functions.
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
/* Stack Marker for Overflow Detection */
/*---------------------------------------------------------------------------*/
#define STACK_MARKER    0xDEADBEEF

/*---------------------------------------------------------------------------*/
/* Task Creation */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_task_create(rtos_task_fn_t fn, const char *name,
                                uint8_t priority, uint32_t *stack,
                                uint32_t stack_size, rtos_tcb_t *tcb,
                                void *arg) {
    /* Validate parameters */
    if (fn == NULL || stack == NULL || tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    if (priority >= RTOS_MAX_PRIORITIES) {
        return RTOS_ERR_PARAM;
    }

    if (stack_size < 32) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Initialize TCB */
    memset(tcb, 0, sizeof(rtos_tcb_t));

    /* Copy task name */
    if (name != NULL) {
        strncpy(tcb->name, name, sizeof(tcb->name) - 1);
        tcb->name[sizeof(tcb->name) - 1] = '\0';
    } else {
        strcpy(tcb->name, "unnamed");
    }

    /* Set priorities */
    tcb->priority = priority;
    tcb->base_priority = priority;

    /* Set stack information */
    tcb->stack_base = stack;
    tcb->stack_size = stack_size;

#if RTOS_ENABLE_STACK_CHECK
    /* Fill stack with marker pattern for overflow detection */
    for (uint32_t i = 0; i < stack_size; i++) {
        stack[i] = STACK_MARKER;
    }
#endif

    /* Initialize stack (stack grows downward) */
    uint32_t *stack_top = stack + stack_size;
    tcb->stack_ptr = rtos_port_init_stack(stack_top, fn, arg);

    /* Set initial state */
    tcb->state = RTOS_TASK_READY;

    /* Add to ready list */
    rtos_add_ready(tcb);

    rtos_exit_critical(state);

    /* If scheduler is running and new task has higher priority, yield */
    if (g_kernel.scheduler_running && priority < g_kernel.current_task->priority) {
        rtos_yield();
    }

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Task Yield */
/*---------------------------------------------------------------------------*/

void rtos_yield(void) {
    if (!g_kernel.scheduler_running) {
        return;
    }

    /* Mark current task as ready (will be put back in ready list by scheduler) */
    g_kernel.current_task->state = RTOS_TASK_READY;

    /* Trigger context switch */
    rtos_trigger_context_switch();
}

/*---------------------------------------------------------------------------*/
/* Task Delay */
/*---------------------------------------------------------------------------*/

void rtos_delay(uint32_t ms) {
    if (ms == 0 || !g_kernel.scheduler_running) {
        return;
    }

    /* Convert ms to ticks */
    uint32_t ticks = (ms * RTOS_TICK_RATE_HZ) / 1000;
    if (ticks == 0) {
        ticks = 1;
    }

    uint32_t state = rtos_enter_critical();

    /* Remove from ready list if needed */
    if (g_kernel.current_task->state == RTOS_TASK_RUNNING) {
        /* Task will be added to delay list, not ready list */
        g_kernel.current_task->state = RTOS_TASK_BLOCKED;
    }

    /* Add to delay list */
    rtos_add_to_delay_list(g_kernel.current_task, ticks);

    rtos_exit_critical(state);

    /* Trigger context switch */
    rtos_trigger_context_switch();
}

void rtos_delay_until(uint32_t wake_tick) {
    if (!g_kernel.scheduler_running) {
        return;
    }

    uint32_t state = rtos_enter_critical();

    /* Calculate ticks until wake */
    int32_t ticks = (int32_t)(wake_tick - g_kernel.tick_count);

    if (ticks > 0) {
        /* Remove from ready list if needed */
        if (g_kernel.current_task->state == RTOS_TASK_RUNNING) {
            g_kernel.current_task->state = RTOS_TASK_BLOCKED;
        }

        /* Set absolute wake tick */
        g_kernel.current_task->wake_tick = wake_tick;

        /* Add to delay list */
        rtos_add_to_delay_list(g_kernel.current_task, ticks);

        rtos_exit_critical(state);

        /* Trigger context switch */
        rtos_trigger_context_switch();
    } else {
        /* Already past wake time */
        rtos_exit_critical(state);
    }
}

/*---------------------------------------------------------------------------*/
/* Task Suspend/Resume */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_task_suspend(rtos_tcb_t *tcb) {
    uint32_t state = rtos_enter_critical();

    /* If NULL, suspend current task */
    if (tcb == NULL) {
        tcb = g_kernel.current_task;
    }

    if (tcb == NULL) {
        rtos_exit_critical(state);
        return RTOS_ERR_PARAM;
    }

    /* Can't suspend if already suspended */
    if (tcb->state == RTOS_TASK_SUSPENDED) {
        rtos_exit_critical(state);
        return RTOS_ERR_STATE;
    }

    /* Remove from ready list if it's there */
    if (tcb->state == RTOS_TASK_READY) {
        rtos_remove_ready(tcb);
    }

    /* Remove from delay list if blocked on delay */
    if (tcb->state == RTOS_TASK_BLOCKED && tcb->wait_object == NULL) {
        rtos_list_remove(&g_kernel.delay_list, tcb);
    }

    tcb->state = RTOS_TASK_SUSPENDED;

    rtos_exit_critical(state);

    /* If we suspended ourselves, yield */
    if (tcb == g_kernel.current_task) {
        rtos_trigger_context_switch();
    }

    return RTOS_OK;
}

rtos_status_t rtos_task_resume(rtos_tcb_t *tcb) {
    if (tcb == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Can only resume suspended tasks */
    if (tcb->state != RTOS_TASK_SUSPENDED) {
        rtos_exit_critical(state);
        return RTOS_ERR_STATE;
    }

    /* Add back to ready list */
    rtos_add_ready(tcb);

    rtos_exit_critical(state);

    /* If resumed task has higher priority, yield */
    if (g_kernel.scheduler_running && tcb->priority < g_kernel.current_task->priority) {
        rtos_trigger_context_switch();
    }

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Task Information */
/*---------------------------------------------------------------------------*/

rtos_tcb_t *rtos_task_current(void) {
    return g_kernel.current_task;
}

const char *rtos_task_name(rtos_tcb_t *tcb) {
    if (tcb == NULL) {
        tcb = g_kernel.current_task;
    }
    return tcb ? tcb->name : "none";
}

uint8_t rtos_task_priority(rtos_tcb_t *tcb) {
    if (tcb == NULL) {
        tcb = g_kernel.current_task;
    }
    return tcb ? tcb->priority : RTOS_MAX_PRIORITIES;
}

/*---------------------------------------------------------------------------*/
/* Stack Check */
/*---------------------------------------------------------------------------*/

#if RTOS_ENABLE_STACK_CHECK
uint32_t rtos_task_stack_unused(rtos_tcb_t *tcb) {
    if (tcb == NULL || tcb->stack_base == NULL) {
        return 0;
    }

    uint32_t *stack = tcb->stack_base;
    uint32_t unused = 0;

    /* Count unused stack words from bottom */
    for (uint32_t i = 0; i < tcb->stack_size; i++) {
        if (stack[i] == STACK_MARKER) {
            unused++;
        } else {
            break;
        }
    }

    return unused * sizeof(uint32_t);  /* Return in bytes */
}

uint8_t rtos_task_stack_overflow(rtos_tcb_t *tcb) {
    if (tcb == NULL || tcb->stack_base == NULL) {
        return 0;
    }

    /* Check if bottom of stack was overwritten */
    return (tcb->stack_base[0] != STACK_MARKER) ? 1 : 0;
}
#endif
