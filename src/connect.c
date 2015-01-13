/* Copyright 2015 Steven Dee. */

#ifndef IX_MUSE_CORE_H_
#include <stddef.h>
#include <stdint.h>
#include "defs.h"
#include "result.h"
#include "packet.h"
#include "connect.h"
#endif

#ifndef IX_INTERNAL_H_
#include "r.h"
#endif

#include <stdlib.h>

struct _ix_connection {
  int        fd;
  ix_event_f on_event;
  void*      user_data;
};

ix_result
ix_connect_connect(int fd, ix_event_f on_event, void* user_data)
{
  ix_connection* ret = (ix_connection*)malloc(sizeof *ret);

  ret->fd = fd;
  ret->on_event = on_event;
  ret->user_data = user_data;
  return ix_r_ptr(ret);
}
