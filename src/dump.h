#ifndef __DUMP_H__
#define __DUMP_H__

// define the probe points

#include <stdint.h>

#define DUMP_BUFSIZE 80000

typedef struct {
  uint64_t timestamps[DUMP_BUFSIZE];
  float buffer[DUMP_BUFSIZE];
  unsigned int idx;
} t_float_buffer;

typedef struct {
  uint64_t timestamps[DUMP_BUFSIZE];
  struct {
    float i;
    float q;
  } buffer[DUMP_BUFSIZE];
  unsigned int idx;
} t_float_iq_buffer;

extern t_float_buffer dumpbuf_phi;
extern t_float_iq_buffer dumpbuf_filtered_iq_samples;

void init_dump();

void add_iq_sample(t_float_iq_buffer* buf, uint64_t timestamp, float i,
                   float q);
void dump_iq_samples(t_float_iq_buffer* buf, const char* fn);

void add_float_sample(t_float_buffer* buf, uint64_t timestamp, float f);
void dump_float_samples(t_float_buffer* buf, const char* fn);

#endif  // __DUMP_H__
