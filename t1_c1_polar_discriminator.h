#ifndef __T1_C1_POLAR_DISCRIMINATOR_H__
#define __T1_C1_POLAR_DISCRIMINATOR_H__

#include "fifo.h"

typedef struct {
  t_fifo* fifo_iq_sample; // input
  t_fifo* fifo_phi_rssi;  // output
} t_t1_c1_polar_discriminator_args;

void* t1_c1_polar_discriminator(void* args);



#endif
