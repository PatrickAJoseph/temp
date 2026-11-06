
#include "calibration.h"
#include "flash.h"

#include <msp430.h>

#define CALIBRATION_CHANNEL_ENTRY_SIZE              (CALIBRATION_SIZE >> 4)
#define CALIBRATION_NUMBER_OF_ENTRIES_PER_CHANNEL   (CALIBRATION_CHANNEL_ENTRY_SIZE/6)

/**
 *  Calibration entries are organized as 6 byte tuples:
 *  adc_code: 2 bytes
 *  voltage: 4 bytes
 */

#define CALIBRATION_ADC_CODE_ADDRESS(channel, index)                      \
        (CALIBRATION_START + (channel * CALIBRATION_CHANNEL_ENTRY_SIZE) + (6*index) )

#define CALIBRATION_VOLTAGE_ADDRESS(channel, index)                      \
        (CALIBRATION_START + (channel * CALIBRATION_CHANNEL_ENTRY_SIZE) + (6*index) + 2 )

void calibration_write(uint8_t channel, uint16_t index, int16_t code, int32_t voltage)
{
    FCTL3 = FWKEY;                            // Clear Lock bit
    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation

    volatile uint8_t* ptr1 = (volatile uint8_t*)CALIBRATION_ADC_CODE_ADDRESS(channel, index);
    volatile uint8_t* ptr2 = (volatile uint8_t*)CALIBRATION_VOLTAGE_ADDRESS(channel, index);

    ptr1[0] = (uint8_t)code;
    ptr1[1] = (uint8_t)( code >> 8 );

    ptr2[0] = (uint8_t)voltage;
    ptr2[1] = (uint8_t)( voltage >> 8 );
    ptr2[2] = (uint8_t)( voltage >> 16 );
    ptr2[3] = (uint8_t)( voltage >>24 );

    while (FCTL3 & BUSY);

    FCTL1 = FWKEY;                            // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}

void calibration_read(uint8_t channel, uint16_t index, int16_t* code, int32_t* voltage)
{
    volatile uint8_t* ptr1 = (volatile uint8_t*)CALIBRATION_ADC_CODE_ADDRESS(channel, index);
    volatile uint8_t* ptr2 = (volatile uint8_t*)CALIBRATION_VOLTAGE_ADDRESS(channel, index);

    *code = *((int16_t*)ptr1);
    *voltage = *((int32_t*)ptr2);
}

uint16_t low_indices[4];
uint16_t high_indices[4];

void calibration_get_calibration_indices(uint8_t channel, int16_t code, uint16_t* index_1, uint16_t* index_2)
{
    uint16_t low;
    uint16_t high;
    uint16_t mid;
    int16_t current_code;

    volatile int16_t* adc_code = (volatile int16_t*)CALIBRATION_ADC_CODE_ADDRESS(channel, 0);
    volatile int32_t* input_voltage = (volatile int32_t*)CALIBRATION_VOLTAGE_ADDRESS(channel, 0);

    if( (*adc_code == (int16_t)0xFFFFU) && (*input_voltage == (int32_t)0xFFFFFFFFU) )
    {
        __no_operation();
        return;
    }

    low = 0;
    high = 0;

    while(( (adc_code[high] != (int16_t)0xFFFFU) && (input_voltage[high] != (int32_t)0xFFFFFFFFU) ))
    {
        high++;
    }

    while(low < high)
    {
        mid = low + ((high - low) / 2);

        current_code = *((int16_t*)CALIBRATION_ADC_CODE_ADDRESS(channel, mid));

        if( code > current_code )
        {
            low = mid + 1;
        }
        else {
            high = mid;
        }
    }

    *index_1 = low - 1;
    *index_2 = low;

    low_indices[channel] = *index_1;
    high_indices[channel] = *index_2;
}
