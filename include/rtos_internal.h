/**
 * @file rtos_internal.h
 * @brief Internal RTOS Kernel Structures
 */

#ifndef RTOS_INTERNAL_H
#define RTOS_INTERNAL_H

#include <stdint.h>
#include "rtos_config.h"

/*---------------------------------------------------------------------------*/
/* Task States */
/*---------------------------------------------------------------------------*/
typedef enum {
    RTOS_TASK_READY     = 0,    /* Task is ready to run */
    RTOS_TASK_RUNNING   = 1,    /* Task is currently running */
    RTOS_TASK_BLOCKED   = 2,    /* Task is blocked (waiting for resource/delay) */
    RTOS_TASK_SUSPENDED = 3     /* Task is suspended */
} rtos_task_state_t;

/*---------------------------------------------------------------------------*/
/* Status Codes */
/*---------------------------------------------------------------------------*/
typedef enum {
    RTOS_OK             = 0,    /* Operation successful */
    RTOS_ERR_PARAM      = -1,   /* Invalid parameter */
    RTOS_ERR_TIMEOUT    = -2,   /* Operation timed out */
    RTOS_ERR_RESOURCE   = -3,   /* Resource not available */
    RTOS_ERR_STATE      = -4,   /* Invalid state for operation */
    RTOS_ERR_NO_MEM     = -5,   /* Out of memory */
    RTOS_ERR_ISR        = -6    /* Called from ISR when not allowed */
} rtos_status_t;

/*---------------------------------------------------------------------------*/
/* Forward Declarations */
/*---------------------------------------------------------------------------*/
typedef struct rtos_tcb rtos_tcb_t;
typedef struct rtos_list rtos_list_t;
typedef struct rtos_sem rtos_sem_t;
typedef struct rtos_mutex rtos_mutex_t;
typedef struct rtos_queue rtos_queue_t;
typedef struct rtos_timer rtos_timer_t;

/*---------------------------------------------------------------------------*/
/* Linked List Node */
/*---------------------------------------------------------------------------*/
struct rtos_list {
    rtos_tcb_t *head;           /* First element in list */
    rtos_tcb_t *tail;           /* Last element in list */
};

/*---------------------------------------------------------------------------*/
/* Task Control Block (TCB) */
/*---------------------------------------------------------------------------*/
struct rtos_tcb {
    uint32_t *stack_ptr;        /* Current stack pointer (MUST be first for asm) */
    uint32_t priority;          /* Current task priority (0 = highest) */
    uint32_t base_priority;     /* Original priority (for priority inheritance) */
    rtos_task_state_t state;    /* Current task state */
    uint32_t wake_tick;         /* Tick count when task should wake (for delay) */
    struct rtos_tcb *next;      /* Next task in ready/wait list */
    struct rtos_tcb *prev;      /* Previous task in ready/wait list */
    char name[16];              /* Task name for debugging */
    uint32_t *stack_base;       /* Stack base address (for overflow detection) */
    uint32_t stack_size;        /* Stack size in words */
    void *wait_object;          /* Object task is waiting on (sem/mutex/queue) */

#if RTOS_ENABLE_STATS
    uint32_t run_count;         /* Number of times task has run */
    uint32_t total_ticks;       /* Total ticks task has been running */
#endif
};

/*---------------------------------------------------------------------------*/
/* Binary Semaphore */
/*---------------------------------------------------------------------------*/
struct rtos_sem {
    volatile uint32_t count;    /* Current count (0 or 1 for binary) */
    rtos_list_t wait_list;      /* List of blocked tasks */
};

/*---------------------------------------------------------------------------*/
/* Mutex with Priority Inheritance */
/*---------------------------------------------------------------------------*/
struct rtos_mutex {
    rtos_tcb_t *owner;          /* Current owner (NULL if unlocked) */
    uint8_t original_priority;  /* Owner's original priority */
    uint8_t lock_count;         /* Recursive lock count */
    rtos_list_t wait_list;      /* List of blocked tasks (priority-sorted) */
};

/*---------------------------------------------------------------------------*/
/* Message Queue */
/*---------------------------------------------------------------------------*/
struct rtos_queue {
    uint8_t *buffer;            /* Ring buffer storage */
    uint32_t msg_size;          /* Size of each message in bytes */
    uint32_t capacity;          /* Maximum number of messages */
    volatile uint32_t head;     /* Write index */
    volatile uint32_t tail;     /* Read index */
    volatile uint32_t count;    /* Current message count */
    rtos_list_t send_wait;      /* Tasks waiting to send (queue full) */
    rtos_list_t recv_wait;      /* Tasks waiting to receive (queue empty) */
};

/*---------------------------------------------------------------------------*/
/* Soft Timer */
/*---------------------------------------------------------------------------*/
typedef void (*rtos_timer_cb_t)(void *arg);

struct rtos_timer {
    uint32_t period_ticks;      /* Timer period in ticks */
    uint32_t next_expiry;       /* Next expiry tick count */
    rtos_timer_cb_t callback;   /* Callback function */
    void *arg;                  /* Callback argument */
    uint8_t active;             /* Timer active flag */
    uint8_t one_shot;           /* One-shot or periodic */
    struct rtos_timer *next;    /* Next timer in list */
};

/*---------------------------------------------------------------------------*/
/* Kernel State */
/*---------------------------------------------------------------------------*/
typedef struct {
    uint32_t priority_bitmap;                           /* Bitmap of ready priorities */
    rtos_list_t ready_list[RTOS_MAX_PRIORITIES];       /* Per-priority ready lists */
    rtos_tcb_t *current_task;                          /* Currently running task */
    rtos_tcb_t *next_task;                             /* Next task to run */
    volatile uint32_t tick_count;                       /* System tick counter */
    uint8_t scheduler_running;                          /* Scheduler started flag */
    uint8_t scheduler_locked;                           /* Scheduler lock count */
    rtos_list_t delay_list;                            /* Tasks waiting on delay */
    rtos_timer_t *timer_list;                          /* Active timer list */

#if RTOS_ENABLE_STATS
    uint32_t context_switches;                         /* Total context switches */
    uint32_t idle_ticks;                               /* Ticks spent in idle */
#endif
} rtos_kernel_t;

/*---------------------------------------------------------------------------*/
/* Global Kernel Instance (defined in rtos_kernel.c) */
/*---------------------------------------------------------------------------*/
extern rtos_kernel_t g_kernel;

/*---------------------------------------------------------------------------*/
/* Internal Functions */
/*---------------------------------------------------------------------------*/

/* List operations */
void rtos_list_init(rtos_list_t *list);
void rtos_list_add_tail(rtos_list_t *list, rtos_tcb_t *tcb);
void rtos_list_add_head(rtos_list_t *list, rtos_tcb_t *tcb);
void rtos_list_add_priority(rtos_list_t *list, rtos_tcb_t *tcb);
void rtos_list_remove(rtos_list_t *list, rtos_tcb_t *tcb);
rtos_tcb_t *rtos_list_pop_head(rtos_list_t *list);
uint8_t rtos_list_is_empty(const rtos_list_t *list);

/* Scheduler operations */
void rtos_schedule(void);
void rtos_add_ready(rtos_tcb_t *tcb);
void rtos_remove_ready(rtos_tcb_t *tcb);
rtos_tcb_t *rtos_get_highest_priority_task(void);

/* Delay list operations */
void rtos_add_to_delay_list(rtos_tcb_t *tcb, uint32_t ticks);
void rtos_check_delayed_tasks(void);

/* Timer operations */
void rtos_timer_tick(void);

/* Critical section helpers */
uint32_t rtos_enter_critical(void);
void rtos_exit_critical(uint32_t state);

/* Context switch trigger */
void rtos_trigger_context_switch(void);

/* Port-specific functions */
void rtos_port_init(void);
void rtos_port_start_first_task(void);
uint32_t *rtos_port_init_stack(uint32_t *stack_top, void (*task_fn)(void *), void *arg);

/* Idle task */
void rtos_idle_task(void *arg);

/*---------------------------------------------------------------------------*/
/* Stack Frame Structure (for reference) */
/*---------------------------------------------------------------------------*/
/*
 * Hardware-saved frame (pushed by exception entry):
 * +------+
 * | xPSR |  <- High address
 * | PC   |
 * | LR   |
 * | R12  |
 * | R3   |
 * | R2   |
 * | R1   |
 * | R0   |
 * +------+
 *
 * Software-saved frame (pushed by PendSV):
 * +------+
 * | R11  |
 * | R10  |
 * | R9   |
 * | R8   |
 * | R7   |
 * | R6   |
 * | R5   |
 * | R4   |  <- stack_ptr points here (low address)
 * +------+
 */

/* Stack frame offsets */
#define STACK_FRAME_R0      8
#define STACK_FRAME_R1      9
#define STACK_FRAME_R2      10
#define STACK_FRAME_R3      11
#define STACK_FRAME_R12     12
#define STACK_FRAME_LR      13
#define STACK_FRAME_PC      14
#define STACK_FRAME_XPSR    15

/* xPSR Thumb bit must be set */
#define XPSR_INIT_VALUE     0x01000000

/* EXC_RETURN values */
#define EXC_RETURN_PSP_UNPRIV   0xFFFFFFFD  /* Return to Thread mode, use PSP */

#endif /* RTOS_INTERNAL_H */
