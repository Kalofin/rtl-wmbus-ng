#ifndef __DECIMATION_H__
#define __DECIMATION_H__

#include "fifo.h"

typedef struct {
  t_fifo* fifo_raw_sample;
  t_fifo* fifo_decimated_sample;
} t_decimation_t1_c1_args;

void* t1_c1_decimator(void* args);



#endif
