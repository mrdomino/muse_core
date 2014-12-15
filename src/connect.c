#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include "defs.h"
#include "result.h"
#include "packet.h"
#include "connect.h"
#include "r.h"

struct _ix_connection {
  int        fd;
  ix_event_f on_event;
  void*      user_data;
};

ix_result
ix_connect_connect(int fd, ix_event_f on_event, void* user_data)
{
  ix_connection* ret = malloc(sizeof *ret);

  ret->fd = fd;
  ret->on_event = on_event;
  ret->user_data = user_data;
  return ix_r_ptr(ret);
}
