
#include "i2c.h"
#include "clock.h"

#define I2C_CLOCK_FREQUENCY_KHZ         (45ULL)
#define I2C_CLOCK_FREQUENCY_HZ          ( 1000ULL * I2C_CLOCK_FREQUENCY_KHZ )

void i2c_bus_recovery(void);

#define I2C_WRITE_TIMEOUT       (10000U)

int i2c_dev_reg_write_16(uint8_t address, uint8_t reg, uint16_t data)
{
    uint16_t timeout;

    UCB0CTL1 |= UCTR + UCTXSTT;

    timeout = I2C_WRITE_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(!timeout)
    {
        i2c_bus_recovery();
        return -1;
    }

    UCB0TXBUF = reg;

    timeout = I2C_WRITE_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(!timeout)
    {
        i2c_bus_recovery();
        return -1;
    }

    UCB0TXBUF = (uint8_t)( data >> 8 );

    timeout = I2C_WRITE_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(!timeout)
    {
        i2c_bus_recovery();
        return -1;
    }


    UCB0TXBUF = (uint8_t)( data & 0xFF );

    timeout = I2C_WRITE_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(!timeout)
    {
        i2c_bus_recovery();
        return -1;
    }


    UCB0CTL1 |= UCTXSTP;                    // I2C stop condition

    while (UCB0CTL1 & UCTXSTP);

    return 0;
}


#define I2C_READ_TIMEOUT        (10000)

int i2c_dev_reg_read_16(uint8_t address, uint8_t reg, uint16_t* value)
{
    uint16_t data = 0;
    uint16_t timeout = I2C_READ_TIMEOUT;

    UCB0CTL1 |= UCTR + UCTXSTT;

    timeout = I2C_READ_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(timeout == 0)
    {
        i2c_bus_recovery();
        return -1;
    }

    UCB0TXBUF = reg;

    timeout = I2C_READ_TIMEOUT;

    while(!(IFG2 & UCB0TXIFG) && timeout)
    {
        timeout--;
    }

    if(timeout == 0)
    {
        i2c_bus_recovery();
        return -1;
    }

    UCB0CTL1 &= ~UCTR;
    UCB0CTL1 |= UCTXSTT;

    //while(UCB0CTL1 & UCTXSTT);

    timeout = I2C_READ_TIMEOUT;

    while(!(IFG2 & UCB0RXIFG) && timeout)
    {
        timeout--;
    }

    if(timeout == 0)
    {
        i2c_bus_recovery();
        return -1;
    }


    UCB0CTL1 |= UCTXSTP;                    // I2C stop condition

    data |= ((uint16_t)(UCB0RXBUF)) << 8;

    timeout = I2C_READ_TIMEOUT;

    while(!(IFG2 & UCB0RXIFG) && timeout)
    {
        timeout--;
    }

    if(timeout == 0)
    {
        i2c_bus_recovery();
        return -1;
    }


    data |= ((uint16_t)(UCB0RXBUF)) << 0;

    IFG2 &= ~UCB0RXIFG;                     // Clear USCI_B0 TX int flag

    while (UCB0CTL1 & UCTXSTP);

    *value = data;
    return 0;
}

void delay_cycles(int i)
{
    while(i)
    {
        i--;
        __no_operation();
    }
}

void i2c_bus_recovery(void)
{
    uint8_t i;

    UCB0CTL1 |= UCSWRST;

    P1SEL  &= ~(BIT6 | BIT7);
    P1SEL2 &= ~(BIT6 | BIT7);

    // Release SDA and SCL
    P1DIR &= ~(BIT6 | BIT7);

    for (i = 0; i < 9; i++)
    {
        // Pull SCL LOW
        P1OUT &= ~BIT6;
        P1DIR |= BIT6;
        delay_cycles(1000);

        // Release SCL
        P1DIR &= ~BIT6;
        delay_cycles(1000);

        // Has SDA been released?
        if (P1IN & BIT7)
            break;
    }

    // Restore I2C configuration
    i2c_init();
}

void i2c_init()
{
    uint16_t divider = (CLOCK_FREQ_HZ / I2C_CLOCK_FREQUENCY_HZ);

    P1SEL |= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
    P1SEL2|= BIT6 + BIT7;                     // Assign I2C pins to USCI_B0
    UCB0CTL1 |= UCSWRST;                      // Enable SW reset
    UCB0CTL0 = UCMST + UCMODE_3 + UCSYNC;     // I2C Master, synchronous mode
    UCB0CTL1 = UCSSEL_2 + UCSWRST;            // Use SMCLK, keep SW reset
    UCB0BR0 = (uint8_t)(divider & 0xFF);                             // fSCL = SMCLK/12 = ~100kHz
    UCB0BR1 = (uint8_t)(divider >> 8);
    UCB0I2CSA = 0x48;                         // Slave Address is 048h
    UCB0CTL1 &= ~UCSWRST;                     // Clear SW reset, resume operations
}
