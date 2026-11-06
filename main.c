
#include <msp430.h>
#include <stdint.h>

#include "clock.h"
#include "ads1115.h"
#include "uart.h"
#include "flash.h"
#include "mux.h"
#include "acquisition.h"
#include "remote.h"
#include "calibration.h"

int main(void)
{
  WDTCTL = WDTPW + WDTHOLD;

  clock_init();
  flash_init();
  uart_init();
  ads1115_init();
  mux_init();
  acquisition_init();
  remote_init();

  __bis_SR_register(GIE);

  while(i2c_dev_reg_write_16(0x48, 0x01, 0xc240));

  while (1)
  {
      remote_process();
      ads1115_process();
  }
}
