
#include "t1_c1_polar_discriminator.h"

#include <complex.h>

#include "atan2.h"
#include "buffers.h"
#include "fifo.h"

static inline float _t1_c1_polar_discriminator(float i, float q) {
  static float complex s_last;
  const float complex s = i + q * _Complex_I;
  const float complex y = s * conjf(s_last);
  const float delta_phi = atan2_libm(y);
  s_last = s;
  return delta_phi;
}

static float rssi_filter_t1_c1(float sample) {
  static float old_sample;

#define ALPHA 0.6789f
  old_sample = ALPHA * sample + (1.0f - ALPHA) * old_sample;
#undef ALPHA

  return old_sample;
}

// execute the polar discriminator and the RSSI calculation

void* t1_c1_polar_discriminator(void* args) {
#ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), "T1 C1 polar discriminator");
#endif

  // extract arguments
  t_fifo* fifo_in = ((t_t1_c1_polar_discriminator_args*)args)->fifo_iq_sample;
  t_fifo* fifo_out = ((t_t1_c1_polar_discriminator_args*)args)->fifo_phi_rssi;

  int stop = 0;

  t_phi_rssi_buffer* phi_rssi_buffer;
  phi_rssi_buffer = &phi_rssi_buffers.buffers[fifo_get_write_idx(fifo_out)];

  unsigned int wr_idx = 0;

  while (!stop) {
    // get the input buffer
    t_decimated_sample_buffer* iq_sample_buffer;
    iq_sample_buffer =
        &decimated_sample_buffers.buffers[fifo_get_read_idx(fifo_in)];

    // if we are signalled to stop, return
    if (iq_sample_buffer->stop) {
      // signal stop also downstream
      phi_rssi_buffer->stop = 1;
      fifo_release_write_idx(fifo_out);
      return NULL;
    }

    // process the full buffer
    for (size_t k = 0;
         k < sizeof(iq_sample_buffer->data) / sizeof(iq_sample_buffer->data[0]);
         k++) {
      const float i_t1_c1 = iq_sample_buffer->data[k].i;
      const float q_t1_c1 = iq_sample_buffer->data[k].q;

      // Demodulate.
      phi_rssi_buffer->data[wr_idx].phi =
          _t1_c1_polar_discriminator(i_t1_c1, q_t1_c1);

      // We are using one simple filter to rssi value in order to
      // prevent unexpected "splashes" in signal power.
      const float rssi_t1_c1 = sqrtf(i_t1_c1 * i_t1_c1 + q_t1_c1 * q_t1_c1);
      phi_rssi_buffer->data[wr_idx].rssi = rssi_filter_t1_c1(rssi_t1_c1);

      if (++wr_idx >= (sizeof(iq_sample_buffer->data) /
                       sizeof(iq_sample_buffer->data[0]))) {
        wr_idx = 0;
        phi_rssi_buffer->stop = 0;
        fifo_release_write_idx(fifo_out);
        phi_rssi_buffer =
            &phi_rssi_buffers.buffers[fifo_get_write_idx(fifo_out)];
      }
    }
    fifo_release_read_idx(fifo_in);
  }
  return NULL;
}
