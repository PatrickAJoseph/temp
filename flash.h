/*
 * flash.h
 *
 *  Created on: 01-Nov-2026
 *      Author: hp
 */

#ifndef FLASH_H_
#define FLASH_H_

#include "clock.h"

extern void flash_init();
extern void flash_write(uint32_t address, uint8_t* data, int size);

#endif /* FLASH_H_ */
