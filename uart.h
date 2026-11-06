
#ifndef UART_H_
#define UART_H_

#include <stdint.h>

#include "clock.h"

typedef void (*uart_rx_callback_t)(uint8_t, void*);

void uart_init();
extern void uart_send(uint8_t* data, int size);
extern void uart_set_rx_callback(uart_rx_callback_t callback, void* args);

#endif /* UART_H_ */
