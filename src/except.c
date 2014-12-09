/* Copyright 2014 InteraXon, Inc. */

#include "except.h"

#include <assert.h>
#include <stdlib.h>

void
ix_notreached()
{
  assert(0);
  exit(1);
}
