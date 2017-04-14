#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "includes/assert.h"
void error_and_exit(char *msg, ...) {
  va_list args;
  va_start(args, msg);
  vfprintf(stderr, msg, args);
  va_end(args);
  exit(-1);
}