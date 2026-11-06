
#ifndef ACQUISITION_H_
#define ACQUISITION_H_

#include <stdint.h>
#include <msp430.h>

extern void acquisition_init();
extern int32_t acquisition_get_input_voltage(uint8_t channel);
extern int16_t acquisition_get_differential_input_code(uint8_t channel);

#endif /* ACQUISITION_H_ */
