
#include "acquisition.h"
#include "ads1115.h"
#include "mux.h"
#include "acquisition.h"
#include "calibration.h"

#include <stdint.h>

#define ACQUISITION_CHANNEL_COUNT       8
#define ACQUISITION_DIFFERENTIAL_COUNT  4
#define MOVING_AVERAGE_WINDOW           1

typedef struct
{
    int32_t buffer[MOVING_AVERAGE_WINDOW];
    int32_t sum;
    uint8_t index;
    uint8_t count;
} moving_average_t;

static moving_average_t channel_filter[ACQUISITION_CHANNEL_COUNT] = {0};

static int32_t acquisition_filtered_channel_code[ACQUISITION_CHANNEL_COUNT] = {0};

volatile int16_t acquisition_differential_ended_channel_code[ACQUISITION_DIFFERENTIAL_COUNT] = {0};

volatile int32_t acquisition_voltage_mv[ACQUISITION_DIFFERENTIAL_COUNT] = {0};

uint8_t acquisition_single_ended_channel_index = 0;

static uint8_t acquisition_filter_ready = 0;

/* Forward declaration. */
static int32_t acquisition_get_differential_voltage_mv(uint8_t channel, int16_t code);

static int32_t moving_average_update(moving_average_t *filter, int32_t sample)
{
    filter->sum -= filter->buffer[filter->index];

    filter->buffer[filter->index] = sample;
    filter->sum += sample;

    filter->index++;

    if(filter->index >= MOVING_AVERAGE_WINDOW)
    {
        filter->index = 0;
    }

    if(filter->count < MOVING_AVERAGE_WINDOW)
    {
        filter->count++;
    }

    return filter->sum / filter->count;
}

static uint8_t acquisition_check_filter_ready(void)
{
    uint8_t channel;

    for(channel = 0; channel < ACQUISITION_CHANNEL_COUNT; channel++)
    {
        if(channel_filter[channel].count < MOVING_AVERAGE_WINDOW)
        {
            return 0;
        }
    }

    return 1;
}

static void acquisition_update_differential_values(void)
{
    uint8_t channel;
    int32_t differential_code;

    int16_t new_differential_code[ACQUISITION_DIFFERENTIAL_COUNT];
    int32_t new_voltage_mv[ACQUISITION_DIFFERENTIAL_COUNT];

    for(channel = 0; channel < ACQUISITION_DIFFERENTIAL_COUNT; channel++)
    {
        differential_code = \
        acquisition_filtered_channel_code[channel * 2] - acquisition_filtered_channel_code[(channel * 2) + 1];

        new_differential_code[channel] = (int16_t)differential_code;

        new_voltage_mv[channel] =
            acquisition_get_differential_voltage_mv( channel,
                                                     new_differential_code[channel]
                                                    );
    }

    __disable_interrupt();

    for(channel = 0; channel < ACQUISITION_DIFFERENTIAL_COUNT; channel++)
    {
        acquisition_differential_ended_channel_code[channel] = new_differential_code[channel];
        acquisition_voltage_mv[channel] = new_voltage_mv[channel];
    }

    __enable_interrupt();
}

static void acquisition_callback(uint16_t sample, void *args)
{
    uint8_t completed_channel;

    (void)args;

    completed_channel = acquisition_single_ended_channel_index;

    /* Update the moving average for the current MUX channel. */
    acquisition_filtered_channel_code[completed_channel] =
        moving_average_update(&channel_filter[completed_channel], (int32_t)(int16_t)sample);

    /*
     * Advance to the next single-ended channel.
     */
    acquisition_single_ended_channel_index++;

    if(acquisition_single_ended_channel_index >= ACQUISITION_CHANNEL_COUNT)
    {
        acquisition_single_ended_channel_index = 0;

        if(!acquisition_filter_ready)
        {
            acquisition_filter_ready = acquisition_check_filter_ready();
        }

        if(acquisition_filter_ready)
        {
            acquisition_update_differential_values();
        }
    }

    mux_select(acquisition_single_ended_channel_index);
}

/*
 * Initialize the acquisition module.
 */
void acquisition_init(void)
{
    uint8_t channel;

    acquisition_single_ended_channel_index = 0;
    acquisition_filter_ready = 0;

    for(channel = 0; channel < ACQUISITION_CHANNEL_COUNT; channel++)
    {
        channel_filter[channel].sum = 0;
        channel_filter[channel].index = 0;
        channel_filter[channel].count = 0;

        acquisition_filtered_channel_code[channel] = 0;
    }

    for(channel = 0;
        channel < ACQUISITION_DIFFERENTIAL_COUNT;
        channel++)
    {
        acquisition_differential_ended_channel_code[channel] = 0;
        acquisition_voltage_mv[channel] = 0;
    }

    mux_select(0);

    ads1115_set_callback(acquisition_callback, 0);
}

/*
 * Get the latest differential input code.
 */
int16_t acquisition_get_differential_input_code(uint8_t channel)
{
    int16_t result;

    if(channel >= ACQUISITION_DIFFERENTIAL_COUNT)
    {
        return 0;
    }

    __disable_interrupt();
    result = acquisition_differential_ended_channel_code[channel];
    __enable_interrupt();

    return result;
}

/*
 * Convert differential ADC code to microvolts.
 */
static int32_t acquisition_get_differential_voltage_mv( uint8_t channel, int16_t code )
{
    static uint16_t index_1;
    static uint16_t index_2;
    static int16_t code_1;
    static int16_t code_2;
    static int32_t voltage_1;
    static int32_t voltage_2;
    int32_t result;

    calibration_get_calibration_indices(channel, code, &index_1, &index_2);

    calibration_read(channel, index_1, &code_1, &voltage_1);
    calibration_read(channel, index_2, &code_2, &voltage_2);

    result = (int32_t)( (int64_t)voltage_1 + \
            (( ((int64_t)voltage_2 - (int64_t)voltage_1) * ((int64_t)code - (int64_t)code_1) ) / ( (int64_t)code_2 - (int64_t)code_1 )) );

    return result;
}

int32_t acquisition_get_input_voltage(uint8_t channel)
{
    int32_t result;

    if(channel >= ACQUISITION_DIFFERENTIAL_COUNT)
    {
        return 0;
    }

    __disable_interrupt();
    result = acquisition_voltage_mv[channel];
    __enable_interrupt();

    return result;
}
