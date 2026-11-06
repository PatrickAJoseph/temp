
#ifndef CALIBRATION_H_
#define CALIBRATION_H_

#include <stdint.h>
#include "flash.h"

#define CALIBRATION_START 0xD800U
#define CALIBRATION_SIZE  0x2600U
#define CALIBRATION_END   (CALIBRATION_START + CALIBRATION_SIZE)

extern void calibration_write(uint8_t channel, uint16_t index, int16_t code, int32_t voltage);
extern void calibration_read(uint8_t channel, uint16_t index, int16_t* code, int32_t* voltage);
extern void calibration_get_calibration_indices(uint8_t channel, int16_t code, uint16_t* index_1, uint16_t* index_2);

#endif /* CALIBRATION_H_ */
