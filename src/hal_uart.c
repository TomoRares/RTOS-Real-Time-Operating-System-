/**
 * @file hal_uart.c
 * @brief UART HAL Implementation
 *
 * Provides UART initialization and basic I/O for STM32F4.
 */

#include "hal.h"
#include "stm32f4xx.h"
#include <stdarg.h>

/*---------------------------------------------------------------------------*/
/* UART Initialization */
/*---------------------------------------------------------------------------*/

void hal_uart_init(const hal_uart_config_t *config) {
    if (config == NULL || config->uart == NULL) {
        return;
    }

    USART_TypeDef *uart = config->uart;

    /* Enable USART clock */
    if (uart == USART1) {
        RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    } else if (uart == USART2) {
        RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    }

    /* Calculate baud rate divisor */
    /* For 16MHz clock and 115200 baud: 16000000 / 115200 = 138.89 */
    /* BRR = mantissa << 4 | fraction */
    uint32_t pclk = RTOS_CPU_CLOCK_HZ;  /* Assume peripheral clock = CPU clock */
    uint32_t div = (pclk + config->baud / 2) / config->baud;

    uart->BRR = div;

    /* Configure CR1: 8N1, no parity, TX and RX enable */
    uart->CR1 = 0;

    if (config->word_length == 9) {
        uart->CR1 |= USART_CR1_M;
    }

    if (config->parity == HAL_UART_PARITY_EVEN) {
        uart->CR1 |= USART_CR1_PCE;
    } else if (config->parity == HAL_UART_PARITY_ODD) {
        uart->CR1 |= USART_CR1_PCE | USART_CR1_PS;
    }

    /* Enable TX and RX */
    uart->CR1 |= USART_CR1_TE | USART_CR1_RE;

    /* Configure CR2: stop bits */
    uart->CR2 = 0;
    if (config->stop_bits == 2) {
        uart->CR2 |= (2 << 12);  /* 2 stop bits */
    }

    /* Configure CR3: no flow control */
    uart->CR3 = 0;

    /* Enable USART */
    uart->CR1 |= USART_CR1_UE;
}

/*---------------------------------------------------------------------------*/
/* UART Character I/O */
/*---------------------------------------------------------------------------*/

void hal_uart_putc(USART_TypeDef *uart, char ch) {
    /* Wait for TX empty */
    while (!(uart->SR & USART_SR_TXE)) {
        /* Busy wait */
    }
    uart->DR = (uint8_t)ch;
}

char hal_uart_getc(USART_TypeDef *uart) {
    /* Wait for RX not empty */
    while (!(uart->SR & USART_SR_RXNE)) {
        /* Busy wait */
    }
    return (char)(uart->DR & 0xFF);
}

void hal_uart_puts(USART_TypeDef *uart, const char *str) {
    while (*str) {
        if (*str == '\n') {
            hal_uart_putc(uart, '\r');
        }
        hal_uart_putc(uart, *str++);
    }
}

uint8_t hal_uart_rx_available(USART_TypeDef *uart) {
    return (uart->SR & USART_SR_RXNE) ? 1 : 0;
}

uint8_t hal_uart_tx_ready(USART_TypeDef *uart) {
    return (uart->SR & USART_SR_TXE) ? 1 : 0;
}

/*---------------------------------------------------------------------------*/
/* Simple Printf Implementation */
/*---------------------------------------------------------------------------*/

static USART_TypeDef *g_printf_uart = USART2;

/* Semihosting for QEMU output */
static void semihosting_putc(char c) {
    /* ARM semihosting SYS_WRITEC */
    __asm volatile (
        "mov r0, #0x03\n"   /* SYS_WRITEC */
        "mov r1, %0\n"
        "bkpt #0xAB\n"
        :
        : "r" (&c)
        : "r0", "r1", "memory"
      );
}
static void print_char(char c) {
    semihosting_putc(c);
}

static void print_string(const char *s) {
    while (*s) {
        if (*s == '\n') {
            print_char('\r');
        }
        print_char(*s++);
    }
}

static void print_uint(uint32_t val, int base, int min_width, char pad) {
    char buf[12];
    int i = 0;
    const char *digits = "0123456789ABCDEF";

    if (val == 0) {
        buf[i++] = '0';
    } else {
        while (val > 0) {
            buf[i++] = digits[val % base];
            val /= base;
        }
    }

    /* Pad if necessary */
    while (i < min_width) {
        buf[i++] = pad;
    }

    /* Print in reverse */
    while (i > 0) {
        print_char(buf[--i]);
    }
}

static void print_int(int32_t val, int min_width, char pad) {
    if (val < 0) {
        print_char('-');
        val = -val;
        if (min_width > 0) min_width--;
    }
    print_uint((uint32_t)val, 10, min_width, pad);
}

void hal_printf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);

    while (*fmt) {
        if (*fmt == '%') {
            fmt++;

            /* Parse flags */
            char pad = ' ';
            int width = 0;

            if (*fmt == '0') {
                pad = '0';
                fmt++;
            }

            /* Parse width */
            while (*fmt >= '0' && *fmt <= '9') {
                width = width * 10 + (*fmt - '0');
                fmt++;
            }

            switch (*fmt) {
                case 'd':
                case 'i':
                    print_int(va_arg(args, int32_t), width, pad);
                    break;

                case 'u':
                    print_uint(va_arg(args, uint32_t), 10, width, pad);
                    break;

                case 'x':
                case 'X':
                    print_uint(va_arg(args, uint32_t), 16, width, pad);
                    break;

                case 'p':
                    print_string("0x");
                    print_uint(va_arg(args, uint32_t), 16, 8, '0');
                    break;

                case 's':
                    print_string(va_arg(args, const char *));
                    break;

                case 'c':
                    print_char((char)va_arg(args, int));
                    break;

                case '%':
                    print_char('%');
                    break;

                default:
                    print_char('%');
                    print_char(*fmt);
                    break;
            }
        } else {
            if (*fmt == '\n') {
                print_char('\r');
            }
            print_char(*fmt);
        }
        fmt++;
    }

    va_end(args);
}

void hal_debug(const char *tag, const char *msg) {
    hal_printf("[%s] %s\n", tag, msg);
}
