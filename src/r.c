#include <stdint.h>
#include "result.h"
#include "r.h"

// TODO(soon): write ix_result helpers
ix_result
ix_r_ptr(void* ptr)
{
  ix_result r;

  r.err = 0;
  r.ok.ptr = ptr;
  return r;
}
