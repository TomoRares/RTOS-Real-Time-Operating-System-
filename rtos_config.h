/**
 * @file rtos_config.h
 * @brief RTOS Configuration Parameters
 */

#ifndef RTOS_CONFIG_H
#define RTOS_CONFIG_H

/* System clock configuration */
#define RTOS_CPU_CLOCK_HZ       16000000    /* 16 MHz (default for STM32F4 on QEMU) */
#define RTOS_TICK_RATE_HZ       1000        /* 1kHz tick rate (1ms period) */

/* Task configuration */
#define RTOS_MAX_TASKS          8           /* Maximum concurrent tasks */
#define RTOS_MAX_PRIORITIES     4           /* Priority levels (0-3, 0 = highest) */
#define RTOS_DEFAULT_STACK_SIZE 256         /* Default stack size in words (1KB) */
#define RTOS_IDLE_STACK_SIZE    128         /* Idle task stack in words (512B) */

/* Timer configuration */
#define RTOS_MAX_TIMERS         8           /* Maximum soft timers */

/* Synchronization configuration */
#define RTOS_MAX_SEMAPHORES     8           /* Maximum semaphores */
#define RTOS_MAX_MUTEXES        8           /* Maximum mutexes */
#define RTOS_MAX_QUEUES         4           /* Maximum message queues */

/* Feature flags */
#define RTOS_ENABLE_STATS       1           /* Enable timing statistics */
#define RTOS_ENABLE_STACK_CHECK 1           /* Enable stack overflow detection */
#define RTOS_ENABLE_PRIORITY_INHERITANCE 1  /* Enable priority inheritance for mutexes */

/* HAL configuration */
#define RTOS_UART_BAUD          115200      /* UART baud rate */

/* Debug configuration */
#define RTOS_DEBUG_PRINT        1           /* Enable debug printing */

/* Calculated values - do not modify */
#define RTOS_TICK_PERIOD_MS     (1000 / RTOS_TICK_RATE_HZ)
#define RTOS_SYSTICK_RELOAD     ((RTOS_CPU_CLOCK_HZ / RTOS_TICK_RATE_HZ) - 1)

/* Priority bitmap width (must be >= RTOS_MAX_PRIORITIES) */
#define RTOS_PRIORITY_BITMAP_WIDTH  32

/* Timeout values */
#define RTOS_NO_WAIT            0
#define RTOS_WAIT_FOREVER       0xFFFFFFFF

#endif /* RTOS_CONFIG_H */
