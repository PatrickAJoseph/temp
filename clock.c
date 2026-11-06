
#include "clock.h"

#include <stdint.h>
#include <msp430.h>

void clock_init()
{
    int i;

    /* Lowest DCOx and MODx settings. */
    DCOCTL = 0;

    /* Set DCO and BCS settings for 16MHz */
    BCSCTL1 = CALBC1_16MHZ;
    DCOCTL = CALDCO_16MHZ;

    for(i = 0 ; i < 1000 ; i++)
    {
        __no_operation();
    }
}
