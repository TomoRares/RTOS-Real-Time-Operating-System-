/**
 * @file hal_gpio.c
 * @brief GPIO HAL Implementation
 *
 * Provides GPIO initialization and control for STM32F4.
 */

#include "hal.h"
#include "stm32f4xx.h"

/*---------------------------------------------------------------------------*/
/* GPIO Clock Enable */
/*---------------------------------------------------------------------------*/

void hal_gpio_enable_clock(GPIO_TypeDef *port) {
    if (port == GPIOA) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    } else if (port == GPIOB) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    } else if (port == GPIOC) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    } else if (port == GPIOD) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    }
}

/*---------------------------------------------------------------------------*/
/* GPIO Initialization */
/*---------------------------------------------------------------------------*/

void hal_gpio_init(const hal_gpio_config_t *config) {
    if (config == NULL || config->port == NULL || config->pin > 15) {
        return;
    }

    GPIO_TypeDef *port = config->port;
    uint8_t pin = config->pin;

    /* Enable clock for this port */
    hal_gpio_enable_clock(port);

    /* Configure mode (2 bits per pin) */
    uint32_t pos = pin * 2;
    port->MODER &= ~(0x3 << pos);
    port->MODER |= (config->mode << pos);

    /* Configure output type (1 bit per pin) */
    if (config->mode == GPIO_MODE_OUTPUT || config->mode == GPIO_MODE_AF) {
        if (config->otype == GPIO_OTYPE_OD) {
            port->OTYPER |= (1 << pin);
        } else {
            port->OTYPER &= ~(1 << pin);
        }

        /* Configure speed (2 bits per pin) */
        port->OSPEEDR &= ~(0x3 << pos);
        port->OSPEEDR |= (config->speed << pos);
    }

    /* Configure pull-up/pull-down (2 bits per pin) */
    port->PUPDR &= ~(0x3 << pos);
    port->PUPDR |= (config->pupd << pos);

    /* Configure alternate function (4 bits per pin) */
    if (config->mode == GPIO_MODE_AF) {
        uint32_t afr_idx = pin >> 3;        /* 0 for pins 0-7, 1 for pins 8-15 */
        uint32_t afr_pos = (pin & 0x7) * 4; /* Position within AFR register */

        port->AFR[afr_idx] &= ~(0xF << afr_pos);
        port->AFR[afr_idx] |= (config->af << afr_pos);
    }
}

/*---------------------------------------------------------------------------*/
/* GPIO Output Control */
/*---------------------------------------------------------------------------*/

void hal_gpio_set(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1 << pin);  /* Set bit in lower 16 bits = set pin */
}

void hal_gpio_clear(GPIO_TypeDef *port, uint8_t pin) {
    port->BSRR = (1 << (pin + 16));  /* Set bit in upper 16 bits = reset pin */
}

void hal_gpio_toggle(GPIO_TypeDef *port, uint8_t pin) {
    port->ODR ^= (1 << pin);
}

uint8_t hal_gpio_read(GPIO_TypeDef *port, uint8_t pin) {
    return (port->IDR & (1 << pin)) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/* System Initialization */
/*---------------------------------------------------------------------------*/

void hal_system_init(void) {
    /* Initialize system clocks */
    /* QEMU starts with 16MHz HSI clock, which is fine for our purposes */

    /* Configure UART GPIO pins */
    /* USART2: PA2 (TX), PA3 (RX) */
    hal_gpio_config_t uart_tx = {
        .port = GPIOA,
        .pin = 2,
        .mode = GPIO_MODE_AF,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_HIGH,
        .pupd = GPIO_PUPD_NONE,
        .af = 7  /* AF7 = USART1-3 */
    };
    hal_gpio_init(&uart_tx);

    hal_gpio_config_t uart_rx = {
        .port = GPIOA,
        .pin = 3,
        .mode = GPIO_MODE_AF,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_HIGH,
        .pupd = GPIO_PUPD_UP,
        .af = 7
    };
    hal_gpio_init(&uart_rx);

    /* Initialize UART */
    hal_uart_config_t uart_config = {
        .uart = USART2,
        .baud = RTOS_UART_BAUD,
        .word_length = 8,
        .stop_bits = 1,
        .parity = HAL_UART_PARITY_NONE
    };
    hal_uart_init(&uart_config);

    /* Configure LED GPIO (PA5 for Netduino Plus 2) */
    hal_gpio_config_t led = {
        .port = GPIOA,
        .pin = 5,
        .mode = GPIO_MODE_OUTPUT,
        .otype = GPIO_OTYPE_PP,
        .speed = GPIO_SPEED_LOW,
        .pupd = GPIO_PUPD_NONE,
        .af = 0
    };
    hal_gpio_init(&led);
}

/*---------------------------------------------------------------------------*/
/* Simple Blocking Delay */
/*---------------------------------------------------------------------------*/

void hal_delay_ms(uint32_t ms) {
    /* Simple busy-wait delay */
    /* Approximate: 4 cycles per loop iteration at 16MHz = 4000 iterations/ms */
    volatile uint32_t count = ms * 4000;
    while (count--) {
        __asm volatile ("nop");
    }
}
