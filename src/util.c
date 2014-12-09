/* Copyright 2014 InteraXon, Inc. */

#include <stddef.h>
#include "util.h"

size_t
ix_strlcpy(char* dst, const char* src, size_t size)
{
  const char *s = src;
  char *d = dst;
  size_t n = size;

  if (n != 0) {
    while (--n != 0) {
      if ((*d++ = *s++) == '\0') {
        break;
      }
    }
  }
  if (n == 0) {
    if (size != 0) {
      *d = '\0';
    }
    while (*s++ != '\0') {}
  }
  return s - src - 1;
}
