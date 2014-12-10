/* Copyright 2014 InteraXon, Inc. */

/*
 * include "packet.h" for ix_packet
 * include "result.h" for ix_result
 */

typedef enum {
  IX_CON_UNKNOWN,
  IX_CON_WAIT_DRAIN,
  IX_CON_WAIT_VERSION,
  IX_CON_WAIT_STATUS,
  IX_CON_STREAMING,
  IX_CON_HALT
} ix_con_state;

typedef struct _ix_connection ix_connection;

typedef struct {
  enum {
    IX_EV_PACKET = 1,
    IX_EV_STATE_CHANGE,
    IX_EV_DISPOSE
  } tag;
  union {
    ix_packet*   pkt;
    ix_con_state new_state;
  } data;
} ix_event;

typedef void (*ix_event_f)(const ix_event* ev, void* user_data);

SO_EXPORT
ix_result
ix_connect_connect(int fd, ix_event_f on_event, void* user_data);

SO_EXPORT
void
ix_connect_disconnect(ix_connection* con);

SO_EXPORT
ix_con_state
ix_connect_state(ix_connection* con);

SO_EXPORT
void
ix_connect_wake(ix_connection* con);
