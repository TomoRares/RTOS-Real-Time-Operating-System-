/**
 * @file startup.c
 * @brief Startup Code for STM32F407 (Cortex-M4)
 *
 * Contains vector table, Reset_Handler, and default exception handlers.
 */

#include <stdint.h>
#include "include/stm32f4xx.h"

/*---------------------------------------------------------------------------*/
/* External Symbols from Linker Script */
/*---------------------------------------------------------------------------*/
extern uint32_t _estack;        /* Initial stack pointer (top of SRAM) */
extern uint32_t _sidata;        /* Start of .data section in Flash */
extern uint32_t _sdata;         /* Start of .data section in RAM */
extern uint32_t _edata;         /* End of .data section in RAM */
extern uint32_t _sbss;          /* Start of .bss section */
extern uint32_t _ebss;          /* End of .bss section */

/*---------------------------------------------------------------------------*/
/* Function Prototypes */
/*---------------------------------------------------------------------------*/
void Reset_Handler(void);
void Default_Handler(void);
void NMI_Handler(void) __attribute__((weak, alias("Default_Handler")));
void HardFault_Handler(void);
void MemManage_Handler(void) __attribute__((weak, alias("Default_Handler")));
void BusFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void UsageFault_Handler(void) __attribute__((weak, alias("Default_Handler")));
void SVC_Handler(void) __attribute__((weak, alias("Default_Handler")));
void DebugMon_Handler(void) __attribute__((weak, alias("Default_Handler")));
void PendSV_Handler(void);      /* Implemented in rtos_port.c */
void SysTick_Handler(void);     /* Implemented in rtos_port.c */

/* External interrupt handlers (weak aliases) */
void WWDG_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void PVD_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void TAMP_STAMP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RTC_WKUP_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void FLASH_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void RCC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void EXTI4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream0_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream4_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream5_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void DMA1_Stream6_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void ADC_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART1_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART2_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));
void USART3_IRQHandler(void) __attribute__((weak, alias("Default_Handler")));

/* Main function prototype */
extern int main(void);

/*---------------------------------------------------------------------------*/
/* Vector Table */
/*---------------------------------------------------------------------------*/
__attribute__((section(".isr_vector")))
const void *vector_table[] = {
    /* Stack pointer initial value */
    &_estack,

    /* Cortex-M4 Core Exceptions */
    Reset_Handler,          /* Reset Handler */
    NMI_Handler,            /* NMI Handler */
    HardFault_Handler,      /* Hard Fault Handler */
    MemManage_Handler,      /* MPU Fault Handler */
    BusFault_Handler,       /* Bus Fault Handler */
    UsageFault_Handler,     /* Usage Fault Handler */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    0,                      /* Reserved */
    SVC_Handler,            /* SVCall Handler */
    DebugMon_Handler,       /* Debug Monitor Handler */
    0,                      /* Reserved */
    PendSV_Handler,         /* PendSV Handler */
    SysTick_Handler,        /* SysTick Handler */

    /* STM32F4 External Interrupts */
    WWDG_IRQHandler,            /* Window WatchDog */
    PVD_IRQHandler,             /* PVD through EXTI Line detection */
    TAMP_STAMP_IRQHandler,      /* Tamper and TimeStamps */
    RTC_WKUP_IRQHandler,        /* RTC Wakeup */
    FLASH_IRQHandler,           /* FLASH */
    RCC_IRQHandler,             /* RCC */
    EXTI0_IRQHandler,           /* EXTI Line0 */
    EXTI1_IRQHandler,           /* EXTI Line1 */
    EXTI2_IRQHandler,           /* EXTI Line2 */
    EXTI3_IRQHandler,           /* EXTI Line3 */
    EXTI4_IRQHandler,           /* EXTI Line4 */
    DMA1_Stream0_IRQHandler,    /* DMA1 Stream 0 */
    DMA1_Stream1_IRQHandler,    /* DMA1 Stream 1 */
    DMA1_Stream2_IRQHandler,    /* DMA1 Stream 2 */
    DMA1_Stream3_IRQHandler,    /* DMA1 Stream 3 */
    DMA1_Stream4_IRQHandler,    /* DMA1 Stream 4 */
    DMA1_Stream5_IRQHandler,    /* DMA1 Stream 5 */
    DMA1_Stream6_IRQHandler,    /* DMA1 Stream 6 */
    ADC_IRQHandler,             /* ADC1, ADC2, ADC3 */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    0,                          /* Reserved */
    USART1_IRQHandler,          /* USART1 */
    USART2_IRQHandler,          /* USART2 */
    USART3_IRQHandler,          /* USART3 */
    /* ... more interrupts can be added as needed ... */
};

/*---------------------------------------------------------------------------*/
/* Reset Handler - Entry Point */
/*---------------------------------------------------------------------------*/
void Reset_Handler(void) {
    uint32_t *src, *dst;

    /* Copy .data section from Flash to RAM */
    src = &_sidata;
    dst = &_sdata;
    while (dst < &_edata) {
        *dst++ = *src++;
    }

    /* Zero fill .bss section */
    dst = &_sbss;
    while (dst < &_ebss) {
        *dst++ = 0;
    }

    /* Enable FPU (Cortex-M4 with FPU) */
    /* SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2)); */
    /* Note: Not using FPU in this RTOS for simplicity */

    /* Call main() */
    main();

    /* Should never reach here */
    while (1) {
        __WFI();
    }
}

/*---------------------------------------------------------------------------*/
/* Default Handler */
/*---------------------------------------------------------------------------*/
void Default_Handler(void) {
    /* Infinite loop for unhandled interrupts */
    while (1) {
        __asm volatile ("nop");
    }
}

/*---------------------------------------------------------------------------*/
/* Hard Fault Handler */
/*---------------------------------------------------------------------------*/
void HardFault_Handler(void) {
    /* Capture fault information for debugging */
    volatile uint32_t cfsr = SCB->CFSR;
    volatile uint32_t hfsr = SCB->HFSR;
    volatile uint32_t mmfar = SCB->MMFAR;
    volatile uint32_t bfar = SCB->BFAR;

    /* Prevent compiler warnings */
    (void)cfsr;
    (void)hfsr;
    (void)mmfar;
    (void)bfar;

    /* Infinite loop - attach debugger to inspect */
    while (1) {
        __asm volatile ("nop");
    }
}

/*---------------------------------------------------------------------------*/
/* System Initialization (called by HAL if needed) */
/*---------------------------------------------------------------------------*/
void SystemInit(void) {
    /* Reset RCC configuration to default state */
    /* This is optional as QEMU starts with a known state */

    /* Set vector table offset to Flash base */
    SCB->VTOR = 0x08000000;
}
