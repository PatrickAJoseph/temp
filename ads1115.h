
#ifndef ADS1115_H_
#define ADS1115_H_

#include "i2c.h"

typedef void (*ads1115_callback)(uint16_t, void* args);

extern void ads1115_init();
int ads1115_read_raw(uint16_t* value);
extern void ads1115_set_callback(ads1115_callback callback, void* args);
extern void ads1115_process();

#endif /* ADS1115_H_ */
