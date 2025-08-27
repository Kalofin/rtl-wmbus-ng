#include <errno.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buffers.h"
#include "config.h"
#include "dump.h"
#include "fifo.h"
#include "logging.h"
#include "t1_c1_decoder.h"

int main(int argc, char *argv[]) {
  set_thread_name("Read");

  config_init();

  config_parse_cmdline_args(argc, argv);

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
  debug("Spawn T1/C1 decoder thread");
  if (pthread_create(&thread_t1_c1, NULL, t1_c1_decoder, &t1_c1_decoder_args) !=
      0) {
    perror("pthread_create");
    exit(EXIT_FAILURE);
  }

  // open the input file, stdin by default
  FILE *input = stdin;
  if ((strcmp(config.input_fn, "") != 0) &&
      (strcmp(config.input_fn, "-") != 0)) {
    info("Opening input file \"%s\"", config.input_fn);
    input = fopen(config.input_fn, "rb");
  } else {
    info("Reading from STDIN");
  }
  if (input == NULL) {
    error("Failed to open input file \"%s\": %s", config.input_fn,
          strerror(errno));
    return EXIT_FAILURE;
  }

  // Read from the input file and fill raw sample buffers for the decimator_t1_c1

  int stop = 0;
  uint64_t timestamp = 0;
  while (!stop) {
    t_raw_sample_buffer *raw_sample_buffer;
    // pick the first free buffer
    raw_sample_buffer =
        &raw_sample_buffers
             .buffers[fifo_get_write_idx(&fifo_raw_sample_buffers)];
    raw_sample_buffer->timestamp = timestamp;

    // if there is no more adta to read, stop.
    if (feof(input)) {
      debug("EOF at input file");
      stop = 1;
    } else {
      // try to read a full buffer
      size_t read_items = fread(raw_sample_buffer->data,
                                sizeof(raw_sample_buffer->data), 1, input);
      timestamp += sizeof(raw_sample_buffer->data);

      // stop if we could not read a full buffer
      if (1 != read_items) {
        if (feof(input)) {
          debug("EOF at input file");
        }
         debug("Failed to read a full buffer from the input file");
        stop = 1;
      }

      raw_sample_buffer->stop = stop;
      // mark write buffer as full so it can be read
      debug2("push one input sample buffer");
      fifo_release_write_idx(&fifo_raw_sample_buffers);
    }
  }

  // close the input file
  if (input != stdin) {
    debug("Close input file");
    fclose(input);
  }

  // join threads
  debug("Join T1/C1 decoder thread")
  pthread_join(thread_t1_c1, NULL);

  debug("Exit...")
  return EXIT_SUCCESS;
}
