
#ifndef I2C_H_
#define I2C_H_

#include <stdint.h>
#include <msp430.h>

extern void i2c_init();
extern int i2c_dev_reg_write_16(uint8_t address, uint8_t reg, uint16_t data);
extern int i2c_dev_reg_read_16(uint8_t address, uint8_t reg, uint16_t* value);

#endif /* I2C_H_ */
