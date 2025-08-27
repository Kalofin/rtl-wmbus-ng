#include "config.h"

#include <getopt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

t_config config;

static void print_version() { puts("Version: " GIT_VERSION); }

static void print_usage(const char *exe_name) {
  printf("%s [-i <input filename>] [-h] [-V]\n", exe_name);
  printf("\n");

  printf(
      "  -i <input filename> : Name of the file containing 8bit I/Q samples."
      " Default: STDIN\n");
  printf("  -h                  : This help\n");
  printf("  -v                  : Increase log level (Default: ERROR, can be WARNING, INFO, DEBUG\n");
  printf("  -V                  : Print the version\n");
}

void config_init() {
  // Initialize all field in the configuration struct
  config.input_fn = "";
  config.loglevel = WARNING;
}

void config_parse_cmdline_args(int argc, char *argv[]) {
  int option;

  while ((option = getopt(argc, argv, "i:hvV")) != -1) {
    switch (option) {
      case 'i':
        if (strcmp(optarg, "-") == 0) {
          config.input_fn = "";
        } else {
          config.input_fn = optarg;
        }
        break;

      case 'h':
        print_usage(argv[0]);
        exit(EXIT_SUCCESS);
        break;

      case 'v':
        config.loglevel++;
        break;

      case 'V':
        print_version();
        exit(EXIT_SUCCESS);
        break;

      default:
        print_usage(argv[0]);
        exit(EXIT_FAILURE);
    }
  }
}
