#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>

static _Thread_local char iso_datetime_buffer[64];

_Thread_local char thread_name[64];

// TODO: make this thread safe!

const char* get_iso_datetime() {
  struct timespec ts;
  struct tm* timeinfo;

  timespec_get(&ts, TIME_UTC);
  timeinfo = gmtime(&ts.tv_sec);

  strftime(iso_datetime_buffer, sizeof(iso_datetime_buffer),
           "%Y-%m-%dT%H:%M:%S", timeinfo);
  snprintf(iso_datetime_buffer + 19, sizeof(iso_datetime_buffer) - 19,
           ".%06ldZ", ts.tv_nsec / 1000);

  return iso_datetime_buffer;
}

void get_iso_datetime_thread_safe(char* buffer, size_t buffer_size) {
  struct timespec ts;
  struct tm timeinfo;

  timespec_get(&ts, TIME_UTC);
  gmtime_r(&ts.tv_sec, &timeinfo);

  strftime(buffer, buffer_size, "%Y-%m-%dT%H:%M:%S", &timeinfo);
  snprintf(buffer + 19, buffer_size - 19, ".%06ldZ", ts.tv_nsec / 1000);
}

void set_thread_name(const char* name) {
  strncpy(thread_name, name, sizeof(thread_name));
#ifdef _GNU_SOURCE
  pthread_setname_np(pthread_self(), name);
#endif
}
const char* get_thread_name() { return thread_name; }
