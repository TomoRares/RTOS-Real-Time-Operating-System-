/**
 * @file rtos_timer.c
 * @brief Soft Timer Implementation
 *
 * Provides software timers that run callbacks on expiry.
 */

#include "rtos.h"
#include "rtos_internal.h"
#include "stm32f4xx.h"

/*---------------------------------------------------------------------------*/
/* External References */
/*---------------------------------------------------------------------------*/
extern rtos_kernel_t g_kernel;

/*---------------------------------------------------------------------------*/
/* Timer Initialization */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_timer_init(rtos_timer_t *timer) {
    if (timer == NULL) {
        return RTOS_ERR_PARAM;
    }

    timer->period_ticks = 0;
    timer->next_expiry = 0;
    timer->callback = NULL;
    timer->arg = NULL;
    timer->active = 0;
    timer->one_shot = 0;
    timer->next = NULL;

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Helper: Insert Timer into Sorted List */
/*---------------------------------------------------------------------------*/
static void timer_insert(rtos_timer_t *timer) {
    timer->next = NULL;

    if (g_kernel.timer_list == NULL) {
        g_kernel.timer_list = timer;
        return;
    }

    /* Insert sorted by next_expiry */
    rtos_timer_t *prev = NULL;
    rtos_timer_t *curr = g_kernel.timer_list;

    while (curr != NULL && (int32_t)(curr->next_expiry - timer->next_expiry) <= 0) {
        prev = curr;
        curr = curr->next;
    }

    if (prev == NULL) {
        /* Insert at head */
        timer->next = g_kernel.timer_list;
        g_kernel.timer_list = timer;
    } else {
        /* Insert after prev */
        timer->next = prev->next;
        prev->next = timer;
    }
}

/*---------------------------------------------------------------------------*/
/* Helper: Remove Timer from List */
/*---------------------------------------------------------------------------*/
static void timer_remove(rtos_timer_t *timer) {
    if (g_kernel.timer_list == NULL) {
        return;
    }

    if (g_kernel.timer_list == timer) {
        g_kernel.timer_list = timer->next;
        timer->next = NULL;
        return;
    }

    rtos_timer_t *prev = g_kernel.timer_list;
    while (prev->next != NULL && prev->next != timer) {
        prev = prev->next;
    }

    if (prev->next == timer) {
        prev->next = timer->next;
        timer->next = NULL;
    }
}

/*---------------------------------------------------------------------------*/
/* Start Periodic Timer */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_timer_start(rtos_timer_t *timer, uint32_t period_ms,
                                rtos_timer_cb_t callback, void *arg) {
    if (timer == NULL || callback == NULL || period_ms == 0) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Remove from list if already active */
    if (timer->active) {
        timer_remove(timer);
    }

    /* Configure timer */
    timer->period_ticks = (period_ms * RTOS_TICK_RATE_HZ) / 1000;
    if (timer->period_ticks == 0) {
        timer->period_ticks = 1;
    }
    timer->next_expiry = g_kernel.tick_count + timer->period_ticks;
    timer->callback = callback;
    timer->arg = arg;
    timer->active = 1;
    timer->one_shot = 0;

    /* Insert into timer list */
    timer_insert(timer);

    rtos_exit_critical(state);

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Start One-Shot Timer */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_timer_start_once(rtos_timer_t *timer, uint32_t delay_ms,
                                     rtos_timer_cb_t callback, void *arg) {
    if (timer == NULL || callback == NULL || delay_ms == 0) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    /* Remove from list if already active */
    if (timer->active) {
        timer_remove(timer);
    }

    /* Configure timer */
    timer->period_ticks = (delay_ms * RTOS_TICK_RATE_HZ) / 1000;
    if (timer->period_ticks == 0) {
        timer->period_ticks = 1;
    }
    timer->next_expiry = g_kernel.tick_count + timer->period_ticks;
    timer->callback = callback;
    timer->arg = arg;
    timer->active = 1;
    timer->one_shot = 1;

    /* Insert into timer list */
    timer_insert(timer);

    rtos_exit_critical(state);

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Stop Timer */
/*---------------------------------------------------------------------------*/

rtos_status_t rtos_timer_stop(rtos_timer_t *timer) {
    if (timer == NULL) {
        return RTOS_ERR_PARAM;
    }

    uint32_t state = rtos_enter_critical();

    if (timer->active) {
        timer_remove(timer);
        timer->active = 0;
    }

    rtos_exit_critical(state);

    return RTOS_OK;
}

/*---------------------------------------------------------------------------*/
/* Check Timer Active */
/*---------------------------------------------------------------------------*/

uint8_t rtos_timer_is_active(rtos_timer_t *timer) {
    if (timer == NULL) {
        return 0;
    }
    return timer->active;
}

/*---------------------------------------------------------------------------*/
/* Timer Tick Processing (called from SysTick ISR) */
/*---------------------------------------------------------------------------*/

void rtos_timer_tick(void) {
    /* Process expired timers */
    while (g_kernel.timer_list != NULL) {
        rtos_timer_t *timer = g_kernel.timer_list;

        /* Check if timer expired */
        if ((int32_t)(g_kernel.tick_count - timer->next_expiry) >= 0) {
            /* Remove from list */
            g_kernel.timer_list = timer->next;
            timer->next = NULL;

            /* Call callback */
            if (timer->callback != NULL) {
                timer->callback(timer->arg);
            }

            /* Reschedule if periodic */
            if (!timer->one_shot && timer->active) {
                timer->next_expiry = g_kernel.tick_count + timer->period_ticks;
                timer_insert(timer);
            } else {
                timer->active = 0;
            }
        } else {
            /* List is sorted, no more expired timers */
            break;
        }
    }
}
