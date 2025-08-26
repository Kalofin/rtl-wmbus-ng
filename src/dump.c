#include "dump.h"

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>

t_float_buffer dumpbuf_phi;
t_float_iq_buffer dumpbuf_filtered_iq_samples;

static void zero_iq_buffer(t_float_iq_buffer* buf) {
  for (int i = 0; i < DUMP_BUFSIZE; i++) {
    buf->buffer[i].i = 0;
    buf->buffer[i].q = 0;
  }
  buf->idx = 0;
}

static void zero_float_buffer(t_float_buffer* buf) {
  for (int i = 0; i < DUMP_BUFSIZE; i++) {
    buf->buffer[i] = 0;
  }
  buf->idx = 0;
}

void init_dump() {
  // initialize all dump buffers
  zero_iq_buffer(&dumpbuf_filtered_iq_samples);
  zero_float_buffer(&dumpbuf_phi);
}

void add_iq_sample(t_float_iq_buffer* buf, uint64_t timestamp, float i,
                   float q) {
  buf->timestamps[buf->idx] = timestamp;
  buf->buffer[buf->idx].i = i;
  buf->buffer[buf->idx].q = q;
  buf->idx++;
  if (buf->idx >= DUMP_BUFSIZE) buf->idx = 0;
}

void dump_iq_samples(t_float_iq_buffer* buf, const char* fn) {
  FILE* fh;
  fh = fopen(fn, "w+");
  if (fh == NULL) {
    perror("Unable to open dump file");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < DUMP_BUFSIZE; i++) {
    fprintf(fh, "%" PRIu64 " %f %f\n", buf->timestamps[buf->idx],
            buf->buffer[buf->idx].i, buf->buffer[buf->idx].q);
    buf->idx++;
    if (buf->idx >= DUMP_BUFSIZE) buf->idx = 0;
  }
  fclose(fh);
}

void add_float_sample(t_float_buffer* buf, uint64_t timestamp, float f) {
  buf->timestamps[buf->idx] = timestamp;
  buf->buffer[buf->idx] = f;
  buf->idx++;
  if (buf->idx >= DUMP_BUFSIZE) buf->idx = 0;
}

void dump_float_samples(t_float_buffer* buf, const char* fn) {
  FILE* fh;
  fh = fopen(fn, "w+");
  if (fh == NULL) {
    perror("Unable to open dump file");
    exit(EXIT_FAILURE);
  }
  for (int i = 0; i < DUMP_BUFSIZE; i++) {
    fprintf(fh, "%" PRIu64 " %f\n", buf->timestamps[buf->idx], buf->buffer[buf->idx]);
    buf->idx++;
    if (buf->idx >= DUMP_BUFSIZE) buf->idx = 0;
  }
  fclose(fh);
}
