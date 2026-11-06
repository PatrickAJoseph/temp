
#ifndef MUX_H_
#define MUX_H_

#include <msp430.h>
#include <stdint.h>

extern void mux_init();
extern void mux_select(uint8_t input);

#endif /* MUX_H_ */
