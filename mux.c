
#include "mux.h"

void mux_init()
{
    P2SEL &= ~(BIT0 | BIT1 | BIT2);
    P2SEL2 &= ~(BIT0 | BIT1 | BIT2);
    P2DIR |= (BIT0 + BIT1 + BIT2);
    P2OUT |= (BIT0 + BIT1 + BIT2);
}

void mux_select(uint8_t input)
{
    uint8_t value = P2OUT;

    input ^= 0xFF;

    value &= ~(BIT0 | BIT1 | BIT2);
    value |= input;

    P2OUT = value;
}
