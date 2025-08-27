
#include "t1_c1_decimator.h"

#include "buffers.h"
#include "dump.h"
#include "fifo.h"
#include "moving_average_filter.h"
#include "logging.h"

static inline float moving_average_t1_c1(float sample, size_t i_or_q) {
#define COEFFS 8
  static int i_hist[COEFFS];
  static int q_hist[COEFFS];

  static MAVGI_FILTER filter[2] =  // i/q
      {
          {.length = COEFFS, .hist = i_hist},  //  0
          {.length = COEFFS, .hist = q_hist}   //  1
      };
#undef COEFFS

  return mavgi(sample, &filter[i_or_q]);
}

void* t1_c1_decimator(void* args) {
  set_thread_name("T1/C1 decimator");
  // extract arguments
  t_fifo* fifo_raw_sample = ((t_decimation_t1_c1_args*)args)->fifo_raw_sample;
  t_fifo* fifo_decimates_sample =
      ((t_decimation_t1_c1_args*)args)->fifo_decimated_sample;

  int stop = 0;
  t_decimated_sample_buffer* decimated_sample_buffer;
  decimated_sample_buffer =
      &decimated_sample_buffers
           .buffers[fifo_get_write_idx(fifo_decimates_sample)];

  unsigned int wr_idx = 0;
  int decimation_rate_index = 0;

  uint64_t timestamp = 0;
  decimated_sample_buffer->timestamp = timestamp;
  while (!stop) {
    // get the buffer
    t_raw_sample_buffer* raw_sample_buffer =
        &raw_sample_buffers.buffers[fifo_get_read_idx(fifo_raw_sample)];

    // if we are signalled to stop, return
    if (raw_sample_buffer->stop) {
      // signal stop also downstream
      decimated_sample_buffer->stop = 1;
      fifo_release_write_idx(fifo_decimates_sample);
      return NULL;
    }

    // process the full buffer
    for (size_t k = 0; k < sizeof(raw_sample_buffer->data) /
                               sizeof(raw_sample_buffer->data[0]);
         k += 2)  // +2 : i and q interleaved
    {
      const float i_unfilt = ((float)(raw_sample_buffer->data[k]) - 127.5f);
      const float q_unfilt = ((float)(raw_sample_buffer->data[k + 1]) - 127.5f);

      // Low-Pass-Filtering before decimation is necessary, to ensure
      // that i and q signals don't contain frequencies above new sample
      // rate. Moving average can be viewed as a low pass filter.
      // store decimated samples in buffer
      decimated_sample_buffer->data[wr_idx].i =
          moving_average_t1_c1(i_unfilt, 0);
      decimated_sample_buffer->data[wr_idx].q =
          moving_average_t1_c1(q_unfilt, 1);

      ++decimation_rate_index;
      // skip the next steps until we averaged over enough (2) samples
      if (decimation_rate_index < 2) continue;

      add_iq_sample(&dumpbuf_filtered_iq_samples, timestamp,
                    decimated_sample_buffer->data[wr_idx].i,
                    decimated_sample_buffer->data[wr_idx].q);

      timestamp++;
      decimation_rate_index = 0;
      wr_idx++;
      if (wr_idx >= (sizeof(decimated_sample_buffer->data) /
                     sizeof(decimated_sample_buffer->data[0]))) {
        wr_idx = 0;
        decimated_sample_buffer->stop = 0;
        debug2("push one decimated sample buffer");
        fifo_release_write_idx(fifo_decimates_sample);
        decimated_sample_buffer =
            &decimated_sample_buffers
                 .buffers[fifo_get_write_idx(fifo_decimates_sample)];
        decimated_sample_buffer->timestamp = timestamp;
      }
    }
    fifo_release_read_idx(fifo_raw_sample);
  }
  return NULL;
}
