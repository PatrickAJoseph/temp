
#ifndef CLOCK_H_
#define CLOCK_H_

#define CLOCK_FREQ_MHZ  (16ULL)
#define CLOCK_FREQ_HZ   ( 1000ULL * 1000ULL * CLOCK_FREQ_MHZ )

extern void clock_init();

#endif /* CLOCK_H_ */
