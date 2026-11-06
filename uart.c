#include "clock.h"
#include "uart.h"
#include <msp430.h>

#define UART_BR_HIGH(x)         (uint8_t)(((1000ULL * 1000ULL * CLOCK_FREQ_MHZ) / (x)) >> 8)
#define UART_BR_LOW(x)          (uint8_t)((1000ULL * 1000ULL * CLOCK_FREQ_MHZ) / (x))

static uart_rx_callback_t uart_callback_function;
static void* uart_callback_args;

void uart_init()
{
    /* Set P1.1 as RXD and P1.2 as TXD. */
    P1SEL |= (BIT1 + BIT2);
    P1SEL2 |= (BIT1 + BIT2);

    /* Use SMCLK as clock source for USCI0. */
    UCA0CTL1 |= UCSSEL_2;

    /* Set UART rate to 57600. */
    UCA0BR0 = UART_BR_LOW(9600);
    UCA0BR1 = UART_BR_HIGH(9600);

    /* Enable over-sampling */
    UCA0MCTL = UCBRS0;

    /* Reset USCI state machine. */
    UCA0CTL1 &= ~UCSWRST;

    /* Enable receive interrupt. */
    IE2 |= UCA0RXIE;
}

void uart_send(uint8_t* data, int size)
{
    while(size)
    {
        /* Wait for pending transmit events to complete */
        while (!(IFG2&UCA0TXIFG));

        /* Load data into TX register. */
        UCA0TXBUF = *data;

        /* Increase user-application buffer pointer and decrease count. */
        data++;
        size--;
    }
}

void uart_set_rx_callback(uart_rx_callback_t callback, void* args)
{
    uart_callback_function = callback;
    uart_callback_args = args;
}


#if defined(__TI_COMPILER_VERSION__) || defined(__IAR_SYSTEMS_ICC__)
#pragma vector=USCIAB0RX_VECTOR
__interrupt void USCI0RX_ISR(void)
#elif defined(__GNUC__)
void __attribute__ ((interrupt(USCIAB0RX_VECTOR))) USCI0RX_ISR (void)
#else
#error Compiler not supported!
#endif
{
    uint8_t rx_byte;

    rx_byte = UCA0RXBUF;

    if(uart_callback_function)
    {
        uart_callback_function(rx_byte, uart_callback_args);
    }
}
