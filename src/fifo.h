#ifndef __FIFO_H__
#define __FIFO_H__

#include <pthread.h>

typedef struct {
  unsigned int number_of_elements;
  unsigned int write_index;
  unsigned int read_index;
  unsigned int count;
  const char * debug_str;

  pthread_mutex_t mutex;
  pthread_cond_t not_full;
  pthread_cond_t not_empty;
} t_fifo;

void fifo_reset(t_fifo* fifo, unsigned int number_of_elements, const char * debug_str);

// asks for the next write index to use.
// If there is no free write slot it blocks until one becomes available
unsigned int fifo_get_write_idx(t_fifo* fifo);

// signals that the current write index is now available for reading
void fifo_release_write_idx(t_fifo* fifo);

// asks for the next read index to use.
// If there is no available element for reading it blocks until one becomes
// available
unsigned int fifo_get_read_idx(t_fifo* fifo);

// signals that the current read index is now available to be filled with data
// again
void fifo_release_read_idx(t_fifo* fifo);
#endif
