/**
 * @file main.c
 * @brief RTOS Demo Application
 *
 * Demonstrates RTOS features:
 * - Multiple tasks with different priorities
 * - Preemptive scheduling
 * - Priority inheritance for mutexes
 * - Message queues
 * - Soft timers
 */

#include "rtos.h"
#include "hal.h"
#include "stm32f4xx.h"

/*---------------------------------------------------------------------------*/
/* Task Stack and TCB Definitions */
/*---------------------------------------------------------------------------*/

#define TASK_STACK_SIZE     256     /* Stack size in words */

/* Task 1 - High priority (5ms period) */
static uint32_t task1_stack[TASK_STACK_SIZE];
static rtos_tcb_t task1_tcb;

/* Task 2 - Medium priority (20ms period) */
static uint32_t task2_stack[TASK_STACK_SIZE];
static rtos_tcb_t task2_tcb;

/* Task 3 - Low priority (background logger) */
static uint32_t task3_stack[TASK_STACK_SIZE];
static rtos_tcb_t task3_tcb;

/*---------------------------------------------------------------------------*/
/* Synchronization Objects */
/*---------------------------------------------------------------------------*/

static rtos_mutex_t shared_mutex;
static rtos_sem_t sync_sem;

/* Message queue for task communication */
#define QUEUE_SIZE          8
#define MSG_SIZE            sizeof(uint32_t)
static uint8_t queue_buffer[QUEUE_SIZE * MSG_SIZE];
static rtos_queue_t msg_queue;

/*---------------------------------------------------------------------------*/
/* Timer */
/*---------------------------------------------------------------------------*/

static rtos_timer_t heartbeat_timer;

/*---------------------------------------------------------------------------*/
/* Statistics */
/*---------------------------------------------------------------------------*/

static volatile uint32_t task1_count = 0;
static volatile uint32_t task2_count = 0;
static volatile uint32_t task3_count = 0;

/*---------------------------------------------------------------------------*/
/* Timer Callback */
/*---------------------------------------------------------------------------*/

static void heartbeat_callback(void *arg) {
    (void)arg;

    /* Toggle LED */
    hal_gpio_toggle(GPIOA, 5);
}

/*---------------------------------------------------------------------------*/
/* Task 1 - High Priority Fast Task (5ms period) */
/*---------------------------------------------------------------------------*/

static void task1_fn(void *arg) {
    (void)arg;

    uint32_t last_wake = rtos_now();
    uint32_t msg;

    hal_printf("[T1] Started (prio=1)\n");

    while (1) {
        task1_count++;

        /* Periodic work */
        uint32_t tick = rtos_now();
        int32_t jitter = (int32_t)(tick - last_wake) - 5;

        /* Send tick to queue for T3 */
        msg = tick;
        rtos_queue_send(&msg_queue, &msg, RTOS_NO_WAIT);

        /* Print every 200 iterations (1 second) */
        if (task1_count % 200 == 0) {
            hal_printf("[T1] tick=%u, runs=%u, jitter=%d\n",
                       tick, task1_count, jitter);
        }

        /* Wait until next period */
        last_wake += 5;
        rtos_delay_until(last_wake);
    }
}

/*---------------------------------------------------------------------------*/
/* Task 2 - Medium Priority Slow Task (20ms period) */
/*---------------------------------------------------------------------------*/

static void task2_fn(void *arg) {
    (void)arg;

    uint32_t last_wake = rtos_now();

    hal_printf("[T2] Started (prio=2)\n");

    while (1) {
        task2_count++;

        /* Acquire shared mutex - demonstrates priority inheritance */
        rtos_mutex_lock(&shared_mutex, RTOS_WAIT_FOREVER);

        /* Simulate some work while holding mutex */
        uint32_t tick = rtos_now();

        /* Print every 50 iterations (1 second) */
        if (task2_count % 50 == 0) {
            hal_printf("[T2] tick=%u, runs=%u\n", tick, task2_count);
        }

        rtos_mutex_unlock(&shared_mutex);

        /* Wait until next period */
        last_wake += 20;
        rtos_delay_until(last_wake);
    }
}

/*---------------------------------------------------------------------------*/
/* Task 3 - Low Priority Background Logger */
/*---------------------------------------------------------------------------*/

static void task3_fn(void *arg) {
    (void)arg;

    uint32_t msg;
    uint32_t last_report = 0;

    hal_printf("[T3] Started (prio=3)\n");

    while (1) {
        task3_count++;

        /* Wait for message from T1 */
        if (rtos_queue_recv(&msg_queue, &msg, 100) == RTOS_OK) {
            /* Process message - just count them */
        }

        /* Print statistics every second */
        uint32_t now = rtos_now();
        if (now - last_report >= 1000) {
            last_report = now;

#if RTOS_ENABLE_STATS
            hal_printf("[STATS] tick=%u, ctx_sw=%u, idle=%u%%\n",
                       now,
                       rtos_stats_context_switches(),
                       (rtos_stats_idle_ticks() * 100) / now);
#else
            hal_printf("[T3] tick=%u, msgs_processed=%u\n", now, task3_count);
#endif
        }
    }
}

/*---------------------------------------------------------------------------*/
/* Priority Inversion Demo */
/*---------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------*/
/* Main Entry Point */
/*---------------------------------------------------------------------------*/

int main(void) {
    /* Initialize HAL (clocks, GPIO, UART) */
    hal_system_init();

    hal_printf("\n");
    hal_printf("========================================\n");
    hal_printf("  Custom RTOS for ARM Cortex-M4\n");
    hal_printf("  Running on QEMU netduinoplus2\n");
    hal_printf("========================================\n");
    hal_printf("[BOOT] RTOS starting, tick rate: %d Hz\n", RTOS_TICK_RATE_HZ);

    /* Initialize RTOS */
    rtos_init();

    /* Initialize synchronization objects */
    rtos_mutex_init(&shared_mutex);
    rtos_sem_init(&sync_sem, 0);
    rtos_queue_init(&msg_queue, queue_buffer, MSG_SIZE, QUEUE_SIZE);

    /* Initialize and start heartbeat timer (500ms period) */
    rtos_timer_init(&heartbeat_timer);
    rtos_timer_start(&heartbeat_timer, 500, heartbeat_callback, NULL);

    /* Create demo tasks */
    hal_printf("[TASK] Creating T1 (prio=1, period=5ms)\n");
    rtos_task_create(task1_fn, "T1", 1,
                     task1_stack, TASK_STACK_SIZE,
                     &task1_tcb, NULL);

    hal_printf("[TASK] Creating T2 (prio=2, period=20ms)\n");
    rtos_task_create(task2_fn, "T2", 2,
                     task2_stack, TASK_STACK_SIZE,
                     &task2_tcb, NULL);

    hal_printf("[TASK] Creating T3 (prio=3, background)\n");
    rtos_task_create(task3_fn, "T3", 3,
                     task3_stack, TASK_STACK_SIZE,
                     &task3_tcb, NULL);

    hal_printf("[SCHED] Starting scheduler\n");
    hal_printf("----------------------------------------\n");

    /* Start the RTOS scheduler - this never returns */
    rtos_start();

    /* Should never reach here */
    while (1);

    return 0;
}
