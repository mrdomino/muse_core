/* Copyright 2014 Interaxon, Inc. */

#ifndef IX_MUSE_CORE_H_
#include <stdint.h>
#include "defs.h"
#include "result.h"
#endif

#ifndef IX_INTERNAL_H_
#include <stddef.h>
#include "util.h"
#endif

#include <assert.h>

static const char* const _errors[] = {
  "ok",
  "need more input data",
  "need more return capacity",
  "parse failure",
  "unsupported protocol number",
};

const char*
ix_strerror(ix_err e)
{
  assert((uint32_t)e < LEN(_errors));
  return _errors[e];
}
