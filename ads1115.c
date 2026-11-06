
#include "ads1115.h"

static ads1115_callback ads1115_callback_function;
static void* ads1115_callback_args;
static volatile uint8_t ads1115_data_available;

void ads1115_init()
{
    uint32_t start_time = 1000000;

    P2DIR &= ~BIT5;
    P2SEL &= ~BIT5;
    P2SEL2 &= ~BIT5;

    P2IES |= BIT5;
    P2IFG &= ~BIT5;

    P2IE |= BIT5;

    i2c_init();

    while(start_time--)
    {
        __no_operation();
    }

    while(i2c_dev_reg_write_16(0x48, 0x02, 0x0000));
    while(i2c_dev_reg_write_16(0x48, 0x03, 0x8000));
}

int ads1115_read_raw(uint16_t* value)
{
    return i2c_dev_reg_read_16(0x48, 0, value);
}

void ads1115_set_callback(ads1115_callback callback, void* args)
{
    ads1115_callback_function = callback;
    ads1115_callback_args = args;
}

void ads1115_process()
{
    static uint16_t result;
    int ret;

    if(ads1115_data_available)
    {
        ads1115_data_available = 0;

        ret = ads1115_read_raw(&result);

        if(ads1115_callback_function && !ret)
        {
            ads1115_callback_function(result, ads1115_callback_args);
        }
    }
}

static volatile uint8_t ads1115_sample_count = 0;

#pragma vector=PORT2_VECTOR
__interrupt void PORT2_ISR(void)
{

    ads1115_sample_count++;

    if( (ads1115_data_available == 0) && (ads1115_sample_count == 2) )
    {
        ads1115_sample_count = 0;
        ads1115_data_available = 1;
    }

    P2IFG &= ~BIT5;
}
