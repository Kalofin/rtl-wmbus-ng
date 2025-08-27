#ifndef __BUFFERS_H__
#define __BUFFERS_H__

#include <stdint.h>

#define DEFAULT_FIFO_SIZE 4
#define DEFAULT_BUFFER_SIZE 4096

typedef struct {
  uint8_t data[DEFAULT_BUFFER_SIZE*2];
  int stop;  // signal that this ist the last buffer
  uint64_t timestamp; // timestamp of the first sample in the buffer
} t_raw_sample_buffer;

typedef struct {
  t_raw_sample_buffer buffers[DEFAULT_FIFO_SIZE];
} t_raw_sample_buffers;

extern t_raw_sample_buffers raw_sample_buffers;

typedef struct {
  struct {
    float i;
    float q;
  } data[DEFAULT_BUFFER_SIZE];
  int stop;  // signal that this ist the last buffer
  uint64_t timestamp; // timestamp of the first sample in the buffer
} t_decimated_sample_buffer;

typedef struct {
  t_decimated_sample_buffer buffers[DEFAULT_FIFO_SIZE];
} t_decimated_sample_buffers;

extern t_decimated_sample_buffers decimated_sample_buffers;

typedef struct {
  struct {
    float phi;
    float rssi;
  } data[DEFAULT_BUFFER_SIZE];
  int stop;
  uint64_t timestamp; // timestamp of the first sample in the buffer
} t_phi_rssi_buffer;

typedef struct {
  t_phi_rssi_buffer buffers[DEFAULT_FIFO_SIZE];
} t_phi_rssi_buffers;

extern t_phi_rssi_buffers phi_rssi_buffers;

#endif
