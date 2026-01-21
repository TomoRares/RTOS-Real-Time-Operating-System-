/**
 * @file rtos_port.c
 * @brief ARM Cortex-M4 Port Implementation
 *
 * Contains architecture-specific code for context switching,
 * interrupt handling, and stack initialization.
 */

#include "rtos.h"
#include "rtos_internal.h"
#include "stm32f4xx.h"
#include "hal.h"

/*---------------------------------------------------------------------------*/
/* External References */
/*---------------------------------------------------------------------------*/
extern rtos_kernel_t g_kernel;

/*---------------------------------------------------------------------------*/
/* Port Configuration */
/*---------------------------------------------------------------------------*/
void rtos_task_exit(void);

/* Priority configuration for PendSV and SysTick */
/* PendSV must be lowest priority to allow other interrupts to preempt */
#define PENDSV_PRIORITY     0xFF    /* Lowest priority (255) */
#define SYSTICK_PRIORITY    0xFF    /* Same low priority */

/*---------------------------------------------------------------------------*/
/* Port Initialization */
/*---------------------------------------------------------------------------*/
void rtos_port_init(void) {
    /* Set PendSV to lowest priority */
    SCB->SHP[SCB_SHP_PENDSV_IDX] = PENDSV_PRIORITY;

    /* Set SysTick to lowest priority */
    SCB->SHP[SCB_SHP_SYSTICK_IDX] = SYSTICK_PRIORITY;

    /* Configure SysTick for 1kHz tick rate */
    SysTick->LOAD = RTOS_SYSTICK_RELOAD;
    SysTick->VAL = 0;
    SysTick->CTRL = SYSTICK_CTRL_CLKSOURCE_Msk |    /* Use processor clock */
                    SYSTICK_CTRL_TICKINT_Msk |       /* Enable interrupt */
                    SYSTICK_CTRL_ENABLE_Msk;         /* Enable SysTick */
}

/*---------------------------------------------------------------------------*/
/* Stack Initialization */
/*---------------------------------------------------------------------------*/
uint32_t *rtos_port_init_stack(uint32_t *stack_top, void (*task_fn)(void *), void *arg) {
    uint32_t *sp = stack_top;

    /* Stack grows downward on ARM */

    /* Hardware-saved frame (exception entry will pop these) */
    *(--sp) = XPSR_INIT_VALUE;          /* xPSR - Thumb bit set */
    *(--sp) = (uint32_t)task_fn;        /* PC - Task entry point */
    *(--sp) = (uint32_t)rtos_task_exit; /* LR - Return address (task exit handler) */
    *(--sp) = 0x12121212;               /* R12 */
    *(--sp) = 0x03030303;               /* R3 */
    *(--sp) = 0x02020202;               /* R2 */
    *(--sp) = 0x01010101;               /* R1 */
    *(--sp) = (uint32_t)arg;            /* R0 - Task argument */

    /* Software-saved frame (PendSV will pop these on first context switch) */
    *(--sp) = 0x11111111;               /* R11 */
    *(--sp) = 0x10101010;               /* R10 */
    *(--sp) = 0x09090909;               /* R9 */
    *(--sp) = 0x08080808;               /* R8 */
    *(--sp) = 0x07070707;               /* R7 */
    *(--sp) = 0x06060606;               /* R6 */
    *(--sp) = 0x05050505;               /* R5 */
    *(--sp) = 0x04040404;               /* R4 */

    return sp;
}

/*---------------------------------------------------------------------------*/
/* Task Exit Handler (should never be called) */
/*---------------------------------------------------------------------------*/
void rtos_task_exit(void) {
    /* Task returned from its function - this should not happen */
    /* Suspend the task */
    __disable_irq();
    rtos_task_suspend(NULL);
    __enable_irq();

    /* If we get here, just spin */
    while (1) {
        __WFI();
    }
}

/*---------------------------------------------------------------------------*/
/* Trigger Context Switch */
/*---------------------------------------------------------------------------*/
void rtos_trigger_context_switch(void) {
    /* Set PendSV pending bit to trigger context switch */
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    __DSB();
    __ISB();
}

/*---------------------------------------------------------------------------*/
/* Start First Task */
/*---------------------------------------------------------------------------*/
void rtos_port_start_first_task(void) {
    /* Get the first task's stack pointer */
    uint32_t *sp = g_kernel.current_task->stack_ptr;

    __asm volatile (
        /* Set PSP to first task's stack pointer */
        "msr psp, %0                \n"

        /* Switch to using PSP in Thread mode */
        /* CONTROL register: bit 1 = 1 -> use PSP */
        "mov r0, #2                 \n"
        "msr control, r0            \n"
        "isb                        \n"

        /* Pop registers from task stack */
        /* Pop R4-R11 (software saved) */
        "ldmia %0!, {r4-r11}        \n"

        /* Update PSP with new value */
        "msr psp, %0                \n"

        /* Enable interrupts */
        "cpsie i                    \n"

        /* Pop hardware frame and jump to task */
        /* This pops R0-R3, R12, LR, PC, xPSR */
        "pop {r0-r3, r12, lr}       \n"

        /* Skip to PC and xPSR manually */
        "pop {r0}                   \n"  /* Pop PC into R0 */
        "pop {r1}                   \n"  /* Pop xPSR into R1 */

        /* Restore xPSR */
        "msr apsr_nzcvq, r1         \n"

        /* Jump to task */
        "bx r0                      \n"

        :
        : "r" (sp)
        : "r0", "r1", "memory"
    );

    /* Should never reach here */
    while (1);
}

/*---------------------------------------------------------------------------*/
/* PendSV Handler - Context Switch */
/*---------------------------------------------------------------------------*/
__attribute__((naked))
void PendSV_Handler(void) {
    __asm volatile (
        /* Disable interrupts */
        "cpsid i                    \n"

        /* Get current PSP */
        "mrs r0, psp                \n"

        /* Save R4-R11 to current task's stack */
        "stmdb r0!, {r4-r11}        \n"

        /* Get address of current_task */
        "ldr r1, =g_kernel          \n"

        /* Save current stack pointer to current task's TCB */
        /* g_kernel.current_task is at offset 4*RTOS_MAX_PRIORITIES + 4 */
        /* But current_task pointer is after ready_list array */
        /* Actually, we need to load current_task pointer first */
        "ldr r2, [r1, %[curr_off]]  \n"  /* r2 = g_kernel.current_task */
        "str r0, [r2, #0]           \n"  /* tcb->stack_ptr = r0 */

        /* Call scheduler to select next task */
        "push {r1, lr}              \n"
        "bl rtos_schedule           \n"
        "pop {r1, lr}               \n"

        /* Load new current_task */
        "ldr r2, [r1, %[curr_off]]  \n"  /* r2 = g_kernel.current_task (updated) */

        /* Get new task's stack pointer */
        "ldr r0, [r2, #0]           \n"  /* r0 = tcb->stack_ptr */

        /* Restore R4-R11 from new task's stack */
        "ldmia r0!, {r4-r11}        \n"

        /* Set PSP to new task's stack */
        "msr psp, r0                \n"

        /* Enable interrupts */
        "cpsie i                    \n"

        /* Return to new task using PSP */
        "ldr lr, =0xFFFFFFFD        \n"  /* EXC_RETURN: Thread mode, PSP */
        "bx lr                      \n"

        :
        : [curr_off] "I" (offsetof(rtos_kernel_t, current_task))
        : "memory"
    );
}

/*---------------------------------------------------------------------------*/
/* SysTick Handler - System Tick */
/*---------------------------------------------------------------------------*/
void SysTick_Handler(void) {
    uint32_t state = rtos_enter_critical();

    /* Increment tick counter */
    g_kernel.tick_count++;

    /* Process timers */
    rtos_timer_tick();

    /* Wake up delayed tasks */
    rtos_check_delayed_tasks();

    /* Check if we need to switch tasks */
    if (g_kernel.scheduler_running && !g_kernel.scheduler_locked) {
        /* Get highest priority ready task */
        rtos_tcb_t *next = rtos_get_highest_priority_task();

        if (next != g_kernel.current_task) {
            /* Trigger context switch */
            rtos_trigger_context_switch();
        }
    }

    rtos_exit_critical(state);
}

/*---------------------------------------------------------------------------*/
/* Critical Section Implementation */
/*---------------------------------------------------------------------------*/
uint32_t rtos_enter_critical(void) {
    uint32_t primask = __get_PRIMASK();
    __disable_irq();
    return primask;
}

void rtos_exit_critical(uint32_t state) {
    __set_PRIMASK(state);
}

/* Public API wrappers */
uint32_t rtos_critical_enter(void) {
    return rtos_enter_critical();
}

void rtos_critical_exit(uint32_t state) {
    rtos_exit_critical(state);
}

/*---------------------------------------------------------------------------*/
/* ISR Detection */
/*---------------------------------------------------------------------------*/
uint8_t rtos_in_isr(void) {
    uint32_t ipsr;
    __asm volatile ("mrs %0, ipsr" : "=r" (ipsr));
    return (ipsr != 0) ? 1 : 0;
}
