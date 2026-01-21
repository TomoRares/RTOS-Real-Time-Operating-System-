/**
 * @file rtos_kernel.c
 * @brief RTOS Kernel Core Implementation
 *
 * Contains the scheduler, list operations, and kernel initialization.
 */

#include "rtos.h"
#include "rtos_internal.h"
#include "stm32f4xx.h"
#include "hal.h"
#include <string.h>

/*---------------------------------------------------------------------------*/
/* Global Kernel Instance */
/*---------------------------------------------------------------------------*/
rtos_kernel_t g_kernel;

/*---------------------------------------------------------------------------*/
/* Idle Task Resources */
/*---------------------------------------------------------------------------*/
static rtos_tcb_t idle_tcb;
static uint32_t idle_stack[RTOS_IDLE_STACK_SIZE];

/*---------------------------------------------------------------------------*/
/* List Operations */
/*---------------------------------------------------------------------------*/

void rtos_list_init(rtos_list_t *list) {
    list->head = NULL;
    list->tail = NULL;
}

uint8_t rtos_list_is_empty(const rtos_list_t *list) {
    return (list->head == NULL) ? 1 : 0;
}

void rtos_list_add_tail(rtos_list_t *list, rtos_tcb_t *tcb) {
    tcb->next = NULL;
    tcb->prev = list->tail;

    if (list->tail != NULL) {
        list->tail->next = tcb;
    } else {
        list->head = tcb;
    }
    list->tail = tcb;
}

void rtos_list_add_head(rtos_list_t *list, rtos_tcb_t *tcb) {
    tcb->prev = NULL;
    tcb->next = list->head;

    if (list->head != NULL) {
        list->head->prev = tcb;
    } else {
        list->tail = tcb;
    }
    list->head = tcb;
}

void rtos_list_add_priority(rtos_list_t *list, rtos_tcb_t *tcb) {
    /* Insert in priority order (lower priority value = higher priority) */
    if (list->head == NULL) {
        /* Empty list */
        tcb->next = NULL;
        tcb->prev = NULL;
        list->head = tcb;
        list->tail = tcb;
        return;
    }

    /* Find insertion point */
    rtos_tcb_t *current = list->head;
    while (current != NULL && current->priority <= tcb->priority) {
        current = current->next;
    }

    if (current == NULL) {
        /* Insert at tail */
        rtos_list_add_tail(list, tcb);
    } else if (current == list->head) {
        /* Insert at head */
        rtos_list_add_head(list, tcb);
    } else {
        /* Insert before current */
        tcb->next = current;
        tcb->prev = current->prev;
        current->prev->next = tcb;
        current->prev = tcb;
    }
}

void rtos_list_remove(rtos_list_t *list, rtos_tcb_t *tcb) {
    if (tcb->prev != NULL) {
        tcb->prev->next = tcb->next;
    } else {
        list->head = tcb->next;
    }

    if (tcb->next != NULL) {
        tcb->next->prev = tcb->prev;
    } else {
        list->tail = tcb->prev;
    }

    tcb->next = NULL;
    tcb->prev = NULL;
}

rtos_tcb_t *rtos_list_pop_head(rtos_list_t *list) {
    rtos_tcb_t *tcb = list->head;

    if (tcb != NULL) {
        list->head = tcb->next;
        if (list->head != NULL) {
            list->head->prev = NULL;
        } else {
            list->tail = NULL;
        }
        tcb->next = NULL;
        tcb->prev = NULL;
    }

    return tcb;
}

/*---------------------------------------------------------------------------*/
/* Ready List Operations */
/*---------------------------------------------------------------------------*/

void rtos_add_ready(rtos_tcb_t *tcb) {
    uint32_t priority = tcb->priority;

    /* Add to the tail of the ready list for this priority */
    rtos_list_add_tail(&g_kernel.ready_list[priority], tcb);

    /* Set the bit in the priority bitmap */
    g_kernel.priority_bitmap |= (1UL << (31 - priority));

    tcb->state = RTOS_TASK_READY;
}

void rtos_remove_ready(rtos_tcb_t *tcb) {
    uint32_t priority = tcb->priority;

    /* Remove from ready list */
    rtos_list_remove(&g_kernel.ready_list[priority], tcb);

    /* Clear bitmap bit if list is now empty */
    if (rtos_list_is_empty(&g_kernel.ready_list[priority])) {
        g_kernel.priority_bitmap &= ~(1UL << (31 - priority));
    }
}

rtos_tcb_t *rtos_get_highest_priority_task(void) {
    if (g_kernel.priority_bitmap == 0) {
        return NULL;
    }

    /* Use CLZ to find highest priority in O(1) */
    uint32_t highest_priority = __CLZ(g_kernel.priority_bitmap);

    /* Return head of that priority's list */
    return g_kernel.ready_list[highest_priority].head;
}

/*---------------------------------------------------------------------------*/
/* Delay List Operations */
/*---------------------------------------------------------------------------*/

void rtos_add_to_delay_list(rtos_tcb_t *tcb, uint32_t ticks) {
    tcb->wake_tick = g_kernel.tick_count + ticks;
    tcb->state = RTOS_TASK_BLOCKED;

    /* Add to delay list sorted by wake_tick */
    if (g_kernel.delay_list.head == NULL) {
        tcb->next = NULL;
        tcb->prev = NULL;
        g_kernel.delay_list.head = tcb;
        g_kernel.delay_list.tail = tcb;
        return;
    }

    /* Find insertion point */
    rtos_tcb_t *current = g_kernel.delay_list.head;
    while (current != NULL && (int32_t)(current->wake_tick - tcb->wake_tick) <= 0) {
        current = current->next;
    }

    if (current == NULL) {
        /* Add at tail */
        tcb->next = NULL;
        tcb->prev = g_kernel.delay_list.tail;
        g_kernel.delay_list.tail->next = tcb;
        g_kernel.delay_list.tail = tcb;
    } else if (current == g_kernel.delay_list.head) {
        /* Add at head */
        tcb->prev = NULL;
        tcb->next = current;
        current->prev = tcb;
        g_kernel.delay_list.head = tcb;
    } else {
        /* Insert before current */
        tcb->next = current;
        tcb->prev = current->prev;
        current->prev->next = tcb;
        current->prev = tcb;
    }
}

void rtos_check_delayed_tasks(void) {
    rtos_tcb_t *tcb = g_kernel.delay_list.head;

    while (tcb != NULL) {
        /* Check if it's time to wake this task */
        if ((int32_t)(g_kernel.tick_count - tcb->wake_tick) >= 0) {
            rtos_tcb_t *next = tcb->next;

            /* Remove from delay list */
            rtos_list_remove(&g_kernel.delay_list, tcb);

            /* Add back to ready list */
            rtos_add_ready(tcb);

            tcb = next;
        } else {
            /* List is sorted, so no more tasks to wake */
            break;
        }
    }
}

/*---------------------------------------------------------------------------*/
/* Scheduler */
/*---------------------------------------------------------------------------*/

void rtos_schedule(void) {
    /* This is called from PendSV with interrupts disabled */

    /* Update statistics for current task */
#if RTOS_ENABLE_STATS
    if (g_kernel.current_task != NULL) {
        g_kernel.current_task->total_ticks++;
    }
    g_kernel.context_switches++;
#endif

    /* If current task is still running/ready, put it back in ready list */
    if (g_kernel.current_task != NULL &&
        g_kernel.current_task->state == RTOS_TASK_RUNNING) {
        g_kernel.current_task->state = RTOS_TASK_READY;
        rtos_add_ready(g_kernel.current_task);
    }

    /* Get highest priority ready task */
    rtos_tcb_t *next = rtos_get_highest_priority_task();

    if (next != NULL) {
        /* Remove from ready list */
        rtos_remove_ready(next);
        next->state = RTOS_TASK_RUNNING;

#if RTOS_ENABLE_STATS
        next->run_count++;
#endif
    }

    /* Switch to next task */
    g_kernel.current_task = next;
}

/*---------------------------------------------------------------------------*/
/* Idle Task */
/*---------------------------------------------------------------------------*/

void rtos_idle_task(void *arg) {
    (void)arg;

    while (1) {
#if RTOS_ENABLE_STATS
        g_kernel.idle_ticks++;
#endif
        /* Low power wait for interrupt */
        __WFI();
    }
}

/*---------------------------------------------------------------------------*/
/* Kernel API */
/*---------------------------------------------------------------------------*/

void rtos_init(void) {
    /* Initialize kernel state */
    memset(&g_kernel, 0, sizeof(g_kernel));

    /* Initialize all ready lists */
    for (int i = 0; i < RTOS_MAX_PRIORITIES; i++) {
        rtos_list_init(&g_kernel.ready_list[i]);
    }

    /* Initialize delay list */
    rtos_list_init(&g_kernel.delay_list);

    /* Initialize port (SysTick, PendSV priorities) */
    rtos_port_init();

    /* Create idle task at lowest priority */
    rtos_task_create(rtos_idle_task, "Idle",
                     RTOS_MAX_PRIORITIES - 1,
                     idle_stack, RTOS_IDLE_STACK_SIZE,
                     &idle_tcb, NULL);
}

void rtos_start(void) {
    /* Get first task to run */
    g_kernel.current_task = rtos_get_highest_priority_task();

    if (g_kernel.current_task == NULL) {
        /* No tasks - shouldn't happen as idle task was created */
        while (1);
    }

    /* Remove from ready list */
    rtos_remove_ready(g_kernel.current_task);
    g_kernel.current_task->state = RTOS_TASK_RUNNING;

    /* Mark scheduler as running */
    g_kernel.scheduler_running = 1;

#if RTOS_ENABLE_STATS
    g_kernel.current_task->run_count++;
#endif

    /* Start first task */
    rtos_port_start_first_task();

    /* Should never reach here */
    while (1);
}

uint32_t rtos_now(void) {
    return g_kernel.tick_count;
}

uint8_t rtos_is_running(void) {
    return g_kernel.scheduler_running;
}

/*---------------------------------------------------------------------------*/
/* Statistics API */
/*---------------------------------------------------------------------------*/

#if RTOS_ENABLE_STATS
uint32_t rtos_stats_context_switches(void) {
    return g_kernel.context_switches;
}

uint32_t rtos_stats_idle_ticks(void) {
    return g_kernel.idle_ticks;
}

uint32_t rtos_stats_task_runs(rtos_tcb_t *tcb) {
    return tcb->run_count;
}
#endif
