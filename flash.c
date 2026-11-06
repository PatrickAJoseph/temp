
#include <msp430.h>
#include <stdint.h>

#include "flash.h"

void flash_init()
{
    FCTL2 = FWKEY + FSSEL0 + 44;
}

void flash_write(uint32_t address, uint8_t* data, int size)
{
    uint8_t *ptr;                          // Flash pointer

    ptr = (uint8_t*)address;              // Initialize Flash pointer
    FCTL3 = FWKEY;                            // Clear Lock bit

    FCTL1 = FWKEY + WRT;                      // Set WRT bit for write operation

    while(size)
    {
      *ptr = *data;                   // Write value to flash
      ptr++;
      data++;
      size--;
    }

    FCTL1 = FWKEY;                            // Clear WRT bit
    FCTL3 = FWKEY + LOCK;                     // Set LOCK bit
}
