/* Copyright 2014 Interaxon, Inc. */

#ifndef IX_MUSE_CORE_H_
#include <stdint.h>
#include "defs.h"
#include "result.h"
#endif

#ifndef IX_INTERNAL_H_
#include "r.h"
#endif

ix_result
ix_r_err(ix_err err)
{
  ix_result r;

  r.err = err;
  r.res.sin = -1;
  return r;
}

ix_result
ix_r_uin(uint32_t uin)
{
  ix_result r;

  r.err = IX_OK;
  r.res.uin = uin;
  return r;
}

// TODO(soon): write ix_result helpers
ix_result
ix_r_ptr(void* ptr)
{
  ix_result r;

  r.err = IX_OK;
  r.res.ptr = ptr;
  return r;
}
