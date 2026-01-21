/**
 * @file hal.h
 * @brief Hardware Abstraction Layer Interface
 */

#ifndef HAL_H
#define HAL_H

#include <stdint.h>
#include <stddef.h>
#include "stm32f4xx.h"
#include "rtos_config.h"

/*---------------------------------------------------------------------------*/
/* GPIO HAL */
/*---------------------------------------------------------------------------*/

/**
 * @brief GPIO pin configuration structure
 */
typedef struct {
    GPIO_TypeDef *port;     /* GPIO port (GPIOA, GPIOB, etc.) */
    uint8_t pin;            /* Pin number (0-15) */
    uint8_t mode;           /* Mode (INPUT, OUTPUT, AF, ANALOG) */
    uint8_t otype;          /* Output type (PP, OD) */
    uint8_t speed;          /* Speed (LOW, MEDIUM, HIGH, VERY_HIGH) */
    uint8_t pupd;           /* Pull-up/Pull-down */
    uint8_t af;             /* Alternate function number (0-15) */
} hal_gpio_config_t;

/**
 * @brief Initialize GPIO pin
 * @param config GPIO configuration
 */
void hal_gpio_init(const hal_gpio_config_t *config);

/**
 * @brief Set GPIO pin high
 * @param port GPIO port
 * @param pin Pin number
 */
void hal_gpio_set(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief Set GPIO pin low
 * @param port GPIO port
 * @param pin Pin number
 */
void hal_gpio_clear(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief Toggle GPIO pin
 * @param port GPIO port
 * @param pin Pin number
 */
void hal_gpio_toggle(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief Read GPIO pin state
 * @param port GPIO port
 * @param pin Pin number
 * @return Pin state (0 or 1)
 */
uint8_t hal_gpio_read(GPIO_TypeDef *port, uint8_t pin);

/**
 * @brief Enable clock for GPIO port
 * @param port GPIO port
 */
void hal_gpio_enable_clock(GPIO_TypeDef *port);

/*---------------------------------------------------------------------------*/
/* UART HAL */
/*---------------------------------------------------------------------------*/

/**
 * @brief UART configuration structure
 */
typedef struct {
    USART_TypeDef *uart;    /* UART peripheral */
    uint32_t baud;          /* Baud rate */
    uint8_t word_length;    /* Word length (8 or 9 bits) */
    uint8_t stop_bits;      /* Stop bits (1 or 2) */
    uint8_t parity;         /* Parity (NONE, EVEN, ODD) */
} hal_uart_config_t;

/* Parity definitions */
#define HAL_UART_PARITY_NONE    0
#define HAL_UART_PARITY_EVEN    1
#define HAL_UART_PARITY_ODD     2

/**
 * @brief Initialize UART
 * @param config UART configuration
 */
void hal_uart_init(const hal_uart_config_t *config);

/**
 * @brief Send single character (blocking)
 * @param uart UART peripheral
 * @param ch Character to send
 */
void hal_uart_putc(USART_TypeDef *uart, char ch);

/**
 * @brief Receive single character (blocking)
 * @param uart UART peripheral
 * @return Received character
 */
char hal_uart_getc(USART_TypeDef *uart);

/**
 * @brief Send string (blocking)
 * @param uart UART peripheral
 * @param str String to send
 */
void hal_uart_puts(USART_TypeDef *uart, const char *str);

/**
 * @brief Check if receive data available
 * @param uart UART peripheral
 * @return 1 if data available, 0 otherwise
 */
uint8_t hal_uart_rx_available(USART_TypeDef *uart);

/**
 * @brief Check if transmit ready
 * @param uart UART peripheral
 * @return 1 if ready, 0 otherwise
 */
uint8_t hal_uart_tx_ready(USART_TypeDef *uart);

/*---------------------------------------------------------------------------*/
/* System HAL */
/*---------------------------------------------------------------------------*/

/**
 * @brief Initialize system clocks
 */
void hal_system_init(void);

/**
 * @brief Simple delay (blocking)
 * @param ms Milliseconds to delay
 */
void hal_delay_ms(uint32_t ms);

/*---------------------------------------------------------------------------*/
/* Debug Output */
/*---------------------------------------------------------------------------*/

/**
 * @brief Print formatted string to UART
 * @param fmt Format string
 * @param ... Arguments
 */
void hal_printf(const char *fmt, ...);

/**
 * @brief Print debug message with tag
 * @param tag Message tag (e.g., "BOOT", "TASK")
 * @param msg Message string
 */
void hal_debug(const char *tag, const char *msg);

#endif /* HAL_H */
