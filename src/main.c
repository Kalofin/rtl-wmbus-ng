#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "buffers.h"
#include "fifo.h"
#include "t1_c1_decoder.h"
#include "dump.h"

int main(/*int argc, char *argv[]*/) {

  // read from stdin --> decimator_t1_c1

  // prepare dump buffers
  init_dump();

  // prepare queue for the raw_sample buffers
  t_fifo fifo_raw_sample_buffers;

  fifo_reset(&fifo_raw_sample_buffers,
             sizeof(raw_sample_buffers.buffers) /
                 sizeof(raw_sample_buffers.buffers[0]),
             NULL);

  // start the decimator thread for C1 and T1 frames
  t_t1_c1_decoder_args t1_c1_decoder_args = {&fifo_raw_sample_buffers};
  pthread_t thread_t1_c1;
  if (pthread_create(&thread_t1_c1, NULL, t1_c1_decoder,
                     &t1_c1_decoder_args) != 0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }
#ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), "Reader");
#endif

  // TODO, parse arguments

  FILE *input = stdin;
  // input = fopen("samples/samples2.bin", "rb");
  // input = fopen("samples/kamstrup.bin", "rb");
  // input = fopen("samples/c1_mode_b.bin", "rb");
  // input = fopen("samples/t1_c1a_mixed.bin", "rb");
  // input = fopen("rtlsdr_868.95M_1M6_amiplus_notdecoded.bin.002", "rb");
  // input = get_net("localhost", 14423);

  if (input == NULL) {
    fprintf(stderr, "File open error.\n");
    return EXIT_FAILURE;
  }

  // demod_out = fopen("demod.bin", "wb");
  // demod_out2_t1_c1 = fopen("demod2_t1_c1.bin", "wb");
  // demod_out2_s1 = fopen("demod2_s1.bin", "wb");
  // clock_out = fopen("clock.bin", "wb");
  // bits_out = fopen("bits.bin", "wb");
  // rawbits_out = fopen("rawbits.bin", "wb");

  // general initialization
  int stop = 0;
  uint64_t timestamp=0;
  while (!stop) {
    t_raw_sample_buffer *raw_sample_buffer;
    raw_sample_buffer =
        &raw_sample_buffers
             .buffers[fifo_get_write_idx(&fifo_raw_sample_buffers)];
    raw_sample_buffer->timestamp=timestamp;

    if (feof(input)) {
      stop = 1;
    } else {
      // try to read a full buffer
      size_t read_items = fread(raw_sample_buffer->data,
                                sizeof(raw_sample_buffer->data), 1, input);
      timestamp+=sizeof(raw_sample_buffer->data);

      if (1 != read_items) {
        stop = 1;
      }

      raw_sample_buffer->stop = stop;
      // mark write buffer as full so it can be read
      fifo_release_write_idx(&fifo_raw_sample_buffers);
    }
  }

  // joind threads

  if (input != stdin) fclose(input);

  pthread_join(thread_t1_c1, NULL);

  return EXIT_SUCCESS;
}
