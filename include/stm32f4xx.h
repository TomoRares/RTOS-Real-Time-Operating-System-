/**
 * @file stm32f4xx.h
 * @brief STM32F4xx Register Definitions
 */

#ifndef STM32F4XX_H
#define STM32F4XX_H

#include <stdint.h>

/* Memory map base addresses */
#define PERIPH_BASE             0x40000000UL
#define APB1PERIPH_BASE         PERIPH_BASE
#define APB2PERIPH_BASE         (PERIPH_BASE + 0x00010000UL)
#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)

/* Core peripherals base addresses */
#define SCS_BASE                0xE000E000UL
#define SYSTICK_BASE            (SCS_BASE + 0x0010UL)
#define NVIC_BASE               (SCS_BASE + 0x0100UL)
#define SCB_BASE                (SCS_BASE + 0x0D00UL)

/* Peripheral base addresses */
#define GPIOA_BASE              (AHB1PERIPH_BASE + 0x0000UL)
#define GPIOB_BASE              (AHB1PERIPH_BASE + 0x0400UL)
#define GPIOC_BASE              (AHB1PERIPH_BASE + 0x0800UL)
#define GPIOD_BASE              (AHB1PERIPH_BASE + 0x0C00UL)
#define RCC_BASE                (AHB1PERIPH_BASE + 0x3800UL)
#define USART2_BASE             (APB1PERIPH_BASE + 0x4400UL)
#define USART1_BASE             (APB2PERIPH_BASE + 0x1000UL)

/*---------------------------------------------------------------------------*/
/* System Control Block (SCB) */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t CPUID;        /* CPUID Base Register */
    volatile uint32_t ICSR;         /* Interrupt Control and State Register */
    volatile uint32_t VTOR;         /* Vector Table Offset Register */
    volatile uint32_t AIRCR;        /* Application Interrupt and Reset Control */
    volatile uint32_t SCR;          /* System Control Register */
    volatile uint32_t CCR;          /* Configuration Control Register */
    volatile uint8_t  SHP[12];      /* System Handlers Priority Registers */
    volatile uint32_t SHCSR;        /* System Handler Control and State */
    volatile uint32_t CFSR;         /* Configurable Fault Status Register */
    volatile uint32_t HFSR;         /* HardFault Status Register */
    volatile uint32_t DFSR;         /* Debug Fault Status Register */
    volatile uint32_t MMFAR;        /* MemManage Fault Address Register */
    volatile uint32_t BFAR;         /* BusFault Address Register */
    volatile uint32_t AFSR;         /* Auxiliary Fault Status Register */
} SCB_Type;

#define SCB                     ((SCB_Type *)SCB_BASE)

/* SCB ICSR bit definitions */
#define SCB_ICSR_PENDSVSET_Pos  28
#define SCB_ICSR_PENDSVSET_Msk  (1UL << SCB_ICSR_PENDSVSET_Pos)
#define SCB_ICSR_PENDSVCLR_Pos  27
#define SCB_ICSR_PENDSVCLR_Msk  (1UL << SCB_ICSR_PENDSVCLR_Pos)

/* SCB SHP indices for exception priorities */
#define SCB_SHP_PENDSV_IDX      10      /* PendSV priority index in SHP array */
#define SCB_SHP_SYSTICK_IDX     11      /* SysTick priority index in SHP array */

/*---------------------------------------------------------------------------*/
/* SysTick Timer */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t CTRL;         /* Control and Status Register */
    volatile uint32_t LOAD;         /* Reload Value Register */
    volatile uint32_t VAL;          /* Current Value Register */
    volatile uint32_t CALIB;        /* Calibration Register */
} SysTick_Type;

#define SysTick                 ((SysTick_Type *)SYSTICK_BASE)

/* SysTick CTRL bit definitions */
#define SYSTICK_CTRL_ENABLE_Pos     0
#define SYSTICK_CTRL_ENABLE_Msk     (1UL << SYSTICK_CTRL_ENABLE_Pos)
#define SYSTICK_CTRL_TICKINT_Pos    1
#define SYSTICK_CTRL_TICKINT_Msk    (1UL << SYSTICK_CTRL_TICKINT_Pos)
#define SYSTICK_CTRL_CLKSOURCE_Pos  2
#define SYSTICK_CTRL_CLKSOURCE_Msk  (1UL << SYSTICK_CTRL_CLKSOURCE_Pos)
#define SYSTICK_CTRL_COUNTFLAG_Pos  16
#define SYSTICK_CTRL_COUNTFLAG_Msk  (1UL << SYSTICK_CTRL_COUNTFLAG_Pos)

/*---------------------------------------------------------------------------*/
/* NVIC (Nested Vectored Interrupt Controller) */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t ISER[8];      /* Interrupt Set Enable Register */
    uint32_t RESERVED0[24];
    volatile uint32_t ICER[8];      /* Interrupt Clear Enable Register */
    uint32_t RESERVED1[24];
    volatile uint32_t ISPR[8];      /* Interrupt Set Pending Register */
    uint32_t RESERVED2[24];
    volatile uint32_t ICPR[8];      /* Interrupt Clear Pending Register */
    uint32_t RESERVED3[24];
    volatile uint32_t IABR[8];      /* Interrupt Active Bit Register */
    uint32_t RESERVED4[56];
    volatile uint8_t  IP[240];      /* Interrupt Priority Register */
    uint32_t RESERVED5[644];
    volatile uint32_t STIR;         /* Software Trigger Interrupt Register */
} NVIC_Type;

#define NVIC                    ((NVIC_Type *)NVIC_BASE)

/*---------------------------------------------------------------------------*/
/* GPIO */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t MODER;        /* Mode Register */
    volatile uint32_t OTYPER;       /* Output Type Register */
    volatile uint32_t OSPEEDR;      /* Output Speed Register */
    volatile uint32_t PUPDR;        /* Pull-up/Pull-down Register */
    volatile uint32_t IDR;          /* Input Data Register */
    volatile uint32_t ODR;          /* Output Data Register */
    volatile uint32_t BSRR;         /* Bit Set/Reset Register */
    volatile uint32_t LCKR;         /* Configuration Lock Register */
    volatile uint32_t AFR[2];       /* Alternate Function Register */
} GPIO_TypeDef;

#define GPIOA                   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB                   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC                   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD                   ((GPIO_TypeDef *)GPIOD_BASE)

/* GPIO Mode definitions */
#define GPIO_MODE_INPUT         0x00
#define GPIO_MODE_OUTPUT        0x01
#define GPIO_MODE_AF            0x02
#define GPIO_MODE_ANALOG        0x03

/* GPIO Output Type definitions */
#define GPIO_OTYPE_PP           0x00    /* Push-pull */
#define GPIO_OTYPE_OD           0x01    /* Open-drain */

/* GPIO Speed definitions */
#define GPIO_SPEED_LOW          0x00
#define GPIO_SPEED_MEDIUM       0x01
#define GPIO_SPEED_HIGH         0x02
#define GPIO_SPEED_VERY_HIGH    0x03

/* GPIO Pull-up/Pull-down definitions */
#define GPIO_PUPD_NONE          0x00
#define GPIO_PUPD_UP            0x01
#define GPIO_PUPD_DOWN          0x02

/*---------------------------------------------------------------------------*/
/* USART */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t SR;           /* Status Register */
    volatile uint32_t DR;           /* Data Register */
    volatile uint32_t BRR;          /* Baud Rate Register */
    volatile uint32_t CR1;          /* Control Register 1 */
    volatile uint32_t CR2;          /* Control Register 2 */
    volatile uint32_t CR3;          /* Control Register 3 */
    volatile uint32_t GTPR;         /* Guard Time and Prescaler Register */
} USART_TypeDef;

#define USART1                  ((USART_TypeDef *)USART1_BASE)
#define USART2                  ((USART_TypeDef *)USART2_BASE)

/* USART SR bit definitions */
#define USART_SR_PE             (1 << 0)    /* Parity Error */
#define USART_SR_FE             (1 << 1)    /* Framing Error */
#define USART_SR_NF             (1 << 2)    /* Noise Flag */
#define USART_SR_ORE            (1 << 3)    /* Overrun Error */
#define USART_SR_IDLE           (1 << 4)    /* IDLE Line Detected */
#define USART_SR_RXNE           (1 << 5)    /* Read Data Register Not Empty */
#define USART_SR_TC             (1 << 6)    /* Transmission Complete */
#define USART_SR_TXE            (1 << 7)    /* Transmit Data Register Empty */
#define USART_SR_LBD            (1 << 8)    /* LIN Break Detection Flag */
#define USART_SR_CTS            (1 << 9)    /* CTS Flag */

/* USART CR1 bit definitions */
#define USART_CR1_SBK           (1 << 0)    /* Send Break */
#define USART_CR1_RWU           (1 << 1)    /* Receiver Wakeup */
#define USART_CR1_RE            (1 << 2)    /* Receiver Enable */
#define USART_CR1_TE            (1 << 3)    /* Transmitter Enable */
#define USART_CR1_IDLEIE        (1 << 4)    /* IDLE Interrupt Enable */
#define USART_CR1_RXNEIE        (1 << 5)    /* RXNE Interrupt Enable */
#define USART_CR1_TCIE          (1 << 6)    /* Transmission Complete Interrupt Enable */
#define USART_CR1_TXEIE         (1 << 7)    /* TXE Interrupt Enable */
#define USART_CR1_PEIE          (1 << 8)    /* PE Interrupt Enable */
#define USART_CR1_PS            (1 << 9)    /* Parity Selection */
#define USART_CR1_PCE           (1 << 10)   /* Parity Control Enable */
#define USART_CR1_WAKE          (1 << 11)   /* Wakeup Method */
#define USART_CR1_M             (1 << 12)   /* Word Length */
#define USART_CR1_UE            (1 << 13)   /* USART Enable */
#define USART_CR1_OVER8         (1 << 15)   /* Oversampling Mode */

/*---------------------------------------------------------------------------*/
/* RCC (Reset and Clock Control) */
/*---------------------------------------------------------------------------*/
typedef struct {
    volatile uint32_t CR;           /* Clock Control Register */
    volatile uint32_t PLLCFGR;      /* PLL Configuration Register */
    volatile uint32_t CFGR;         /* Clock Configuration Register */
    volatile uint32_t CIR;          /* Clock Interrupt Register */
    volatile uint32_t AHB1RSTR;     /* AHB1 Peripheral Reset Register */
    volatile uint32_t AHB2RSTR;     /* AHB2 Peripheral Reset Register */
    volatile uint32_t AHB3RSTR;     /* AHB3 Peripheral Reset Register */
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;     /* APB1 Peripheral Reset Register */
    volatile uint32_t APB2RSTR;     /* APB2 Peripheral Reset Register */
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;      /* AHB1 Peripheral Clock Enable Register */
    volatile uint32_t AHB2ENR;      /* AHB2 Peripheral Clock Enable Register */
    volatile uint32_t AHB3ENR;      /* AHB3 Peripheral Clock Enable Register */
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;      /* APB1 Peripheral Clock Enable Register */
    volatile uint32_t APB2ENR;      /* APB2 Peripheral Clock Enable Register */
    uint32_t RESERVED3[2];
    volatile uint32_t AHB1LPENR;    /* AHB1 Peripheral Clock Enable in Low Power Mode */
    volatile uint32_t AHB2LPENR;    /* AHB2 Peripheral Clock Enable in Low Power Mode */
    volatile uint32_t AHB3LPENR;    /* AHB3 Peripheral Clock Enable in Low Power Mode */
    uint32_t RESERVED4;
    volatile uint32_t APB1LPENR;    /* APB1 Peripheral Clock Enable in Low Power Mode */
    volatile uint32_t APB2LPENR;    /* APB2 Peripheral Clock Enable in Low Power Mode */
    uint32_t RESERVED5[2];
    volatile uint32_t BDCR;         /* Backup Domain Control Register */
    volatile uint32_t CSR;          /* Clock Control & Status Register */
    uint32_t RESERVED6[2];
    volatile uint32_t SSCGR;        /* Spread Spectrum Clock Generation Register */
    volatile uint32_t PLLI2SCFGR;   /* PLLI2S Configuration Register */
} RCC_TypeDef;

#define RCC                     ((RCC_TypeDef *)RCC_BASE)

/* RCC AHB1ENR bit definitions */
#define RCC_AHB1ENR_GPIOAEN     (1 << 0)
#define RCC_AHB1ENR_GPIOBEN     (1 << 1)
#define RCC_AHB1ENR_GPIOCEN     (1 << 2)
#define RCC_AHB1ENR_GPIODEN     (1 << 3)

/* RCC APB1ENR bit definitions */
#define RCC_APB1ENR_USART2EN    (1 << 17)

/* RCC APB2ENR bit definitions */
#define RCC_APB2ENR_USART1EN    (1 << 4)

/*---------------------------------------------------------------------------*/
/* Interrupt Numbers */
/*---------------------------------------------------------------------------*/
typedef enum {
    /* Cortex-M4 Processor Exceptions */
    NonMaskableInt_IRQn     = -14,
    HardFault_IRQn          = -13,
    MemoryManagement_IRQn   = -12,
    BusFault_IRQn           = -11,
    UsageFault_IRQn         = -10,
    SVCall_IRQn             = -5,
    DebugMonitor_IRQn       = -4,
    PendSV_IRQn             = -2,
    SysTick_IRQn            = -1,

    /* STM32F4 Specific Interrupts */
    WWDG_IRQn               = 0,
    EXTI0_IRQn              = 6,
    EXTI1_IRQn              = 7,
    EXTI2_IRQn              = 8,
    EXTI3_IRQn              = 9,
    EXTI4_IRQn              = 10,
    USART1_IRQn             = 37,
    USART2_IRQn             = 38,
    USART3_IRQn             = 39,
} IRQn_Type;

/*---------------------------------------------------------------------------*/
/* Inline Functions for Interrupt Control */
/*---------------------------------------------------------------------------*/
static inline void __disable_irq(void) {
    __asm volatile ("cpsid i" ::: "memory");
}

static inline void __enable_irq(void) {
    __asm volatile ("cpsie i" ::: "memory");
}

static inline void __DSB(void) {
    __asm volatile ("dsb 0xF" ::: "memory");
}

static inline void __ISB(void) {
    __asm volatile ("isb 0xF" ::: "memory");
}

static inline void __WFI(void) {
    __asm volatile ("wfi");
}

static inline uint32_t __get_PRIMASK(void) {
    uint32_t result;
    __asm volatile ("MRS %0, primask" : "=r" (result));
    return result;
}

static inline void __set_PRIMASK(uint32_t primask) {
    __asm volatile ("MSR primask, %0" :: "r" (primask) : "memory");
}

static inline uint32_t __get_PSP(void) {
    uint32_t result;
    __asm volatile ("MRS %0, psp" : "=r" (result));
    return result;
}

static inline void __set_PSP(uint32_t psp) {
    __asm volatile ("MSR psp, %0" :: "r" (psp) : "memory");
}

static inline uint32_t __get_MSP(void) {
    uint32_t result;
    __asm volatile ("MRS %0, msp" : "=r" (result));
    return result;
}

static inline void __set_MSP(uint32_t msp) {
    __asm volatile ("MSR msp, %0" :: "r" (msp) : "memory");
}

static inline uint32_t __get_CONTROL(void) {
    uint32_t result;
    __asm volatile ("MRS %0, control" : "=r" (result));
    return result;
}

static inline void __set_CONTROL(uint32_t control) {
    __asm volatile ("MSR control, %0" :: "r" (control) : "memory");
    __ISB();
}

/* Count Leading Zeros - used for O(1) priority lookup */
static inline uint32_t __CLZ(uint32_t value) {
    uint32_t result;
    __asm volatile ("clz %0, %1" : "=r" (result) : "r" (value));
    return result;
}

#endif /* STM32F4XX_H */
