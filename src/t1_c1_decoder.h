#ifndef __T1_C1_DECODER_H__
#define __T1_C1_DECODER_H__

#include "fifo.h"

typedef struct {
  t_fifo * fifo_raw_sample;
} t_t1_c1_decoder_args;

void* t1_c1_decoder(void * args);

#endif
