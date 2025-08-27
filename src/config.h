#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <stdbool.h>

#include "logging.h"

typedef struct {
  const char* input_fn;
  t_loglevel loglevel;
} t_config;

extern t_config config;

void config_init();
void config_parse_cmdline_args(int argc, char* argv[]);

#endif  //__CONFIG_H__
