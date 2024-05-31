#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "iob_versat.h"

int sysfs_read_file(const char *filename, uint32_t *read_value) {
  return 0;
}

int sysfs_write_file(const char *filename, uint32_t write_value) {
  return 0;
}

int versat_reset() {
  return 0;
}

int versat_init() {
  return 0;
}

int versat_print_version() {
  return 0;
}

int versat_get_count(uint64_t *count) {
  return 0;
}

int main(int argc, char *argv[]) {
  printf("[User] IOb-Versat application\n");

  if (versat_init()) {
    perror("[User] Failed to initialize versat");

    return EXIT_FAILURE;
  }

  if (versat_print_version()) {
    perror("[User] Failed to print version");

    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
