#include "fifo.h"

#include <stdio.h>
#include <stdlib.h>

void fifo_reset(t_fifo* fifo, unsigned int number_of_elements,
                const char* debug_str) {
  fifo->number_of_elements = number_of_elements;
  fifo->count = 0;  // FIFO is empty upon start
  fifo->read_index = 0;
  fifo->write_index = 0;
  fifo->debug_str = debug_str;

  pthread_mutex_init(&fifo->mutex, NULL);
  pthread_cond_init(&fifo->not_full, NULL);
  pthread_cond_init(&fifo->not_empty, NULL);
}

// asks for the next write index to use.
// If there is no free write slot it blocks until one becomes available
unsigned int fifo_get_write_idx(t_fifo* fifo) {
  pthread_mutex_lock(&fifo->mutex);
  if (fifo->count == fifo->number_of_elements) {
    if (fifo->debug_str != NULL)
      printf("[%s] fifo_get_write_idx stalled: %d\n", fifo->debug_str,
       fifo->write_index);
    pthread_cond_wait(&fifo->not_full, &fifo->mutex);
  }
  if (fifo->debug_str != NULL)
    printf("[%s] fifo_get_write_idx: %d\n", fifo->debug_str, fifo->write_index);
  pthread_mutex_unlock(&fifo->mutex);
  return fifo->write_index;
}

// signals that the current write index is now available for reading
void fifo_release_write_idx(t_fifo* fifo) {
  pthread_mutex_lock(&fifo->mutex);
  if (fifo->count == fifo->number_of_elements) {
    perror("Write index released while FIFO is already full!");
    exit(EXIT_FAILURE);
  }
  if (fifo->debug_str != NULL)
    printf("[%s] fifo_release_write_idx: %d\n", fifo->debug_str,
           fifo->write_index);
  fifo->count++;
  fifo->write_index = (fifo->write_index + 1) % fifo->number_of_elements;
  pthread_cond_signal(&fifo->not_empty);
  pthread_mutex_unlock(&fifo->mutex);
}

// asks for the next read index to use.
// If there is no available element for reading it blocks until one becomes
// available
unsigned int fifo_get_read_idx(t_fifo* fifo) {
  pthread_mutex_lock(&fifo->mutex);
  if (fifo->count == 0) {
    if (fifo->debug_str != NULL)
      printf("[%s] fifo_get_read_idx stalled: %d\n", fifo->debug_str,
             fifo->read_index);
    pthread_cond_wait(&fifo->not_empty, &fifo->mutex);
  }
  if (fifo->debug_str != NULL)
    printf("[%s] fifo_get_read_idx: %d\n", fifo->debug_str, fifo->read_index);
  pthread_mutex_unlock(&fifo->mutex);
  return fifo->read_index;
}

// signals that the current read index is now available to be filled with data
// again
void fifo_release_read_idx(t_fifo* fifo) {
  pthread_mutex_lock(&fifo->mutex);
  if (fifo->count == 0) {
    perror("Read index shall be released while FIFO is empty!");
    exit(EXIT_FAILURE);
  }
  if (fifo->debug_str != NULL)
    printf("[%s] fifo_release_read_idx: %d\n", fifo->debug_str,
           fifo->read_index);
  fifo->count--;
  fifo->read_index = (fifo->read_index + 1) % fifo->number_of_elements;
  pthread_cond_signal(&fifo->not_full);
  pthread_mutex_unlock(&fifo->mutex);
}
