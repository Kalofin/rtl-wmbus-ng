#ifndef __LOGGING_H__
#define __LOGGING_H__

#include <stddef.h>
#include <stdio.h>

typedef enum {
  NONE = 0,
  ERROR = 1,
  WARNING = 2,
  INFO = 3,
  DEBUG = 4,
  DEBUG2 = 5,
} t_loglevel;

#include "config.h"


void set_thread_name(const char *);
const char* get_thread_name();

const char* get_iso_datetime();
void get_iso_datetime_thread_safe(char *buffer, size_t buffer_size);

#define log(level, format, ...) {\
  fprintf(stderr, "%s [" level "][%s] " format "\n", get_iso_datetime(),  get_thread_name(), ##__VA_ARGS__); \
}

#define error(format, ...)      \
  if (config.loglevel >= ERROR) \
    log("ERROR", format, ##__VA_ARGS__)

#define warning(format, ...)      \
  if (config.loglevel >= WARNING) \
    log("WARN ", format, ##__VA_ARGS__)

#define info(format, ...)      \
  if (config.loglevel >= INFO) \
    log("INFO ", format, ##__VA_ARGS__)

#define debug(format, ...)      \
  if (config.loglevel >= DEBUG) \
    log("DEBUG", format, ##__VA_ARGS__)

#define debug2(format, ...)      \
  if (config.loglevel >= DEBUG2) \
    log("DBG2 ", format, ##__VA_ARGS__)

#endif  // __LOGGING_H__
