#include <stdint.h>
#include "defs.h"
#include "result.h"

#include <stddef.h>
#include "util.h"

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
