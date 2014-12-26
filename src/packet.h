/* Copyright 2014 InteraXon, Inc. */

/*
 * include <stddef.h> for size_t
 * include <stdint.h> for sized ints
 * include "defs.h" for SO_EXPORT
 * include "result.h" for ix_result
 */

typedef enum {
  IX_PAC_INVALID,
  IX_PAC_SYNC,
  IX_PAC_UNCOMPRESSED_EEG,
  IX_PAC_ERROR,
  IX_PAC_EEG,
  IX_PAC_BATTERY,
  IX_PAC_ACCELEROMETER,
  IX_PAC_DRLREF
} ix_packet_type;

struct _pac_accelerometer {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
};

typedef struct {
  ix_packet_type type;
  union {
    struct _pac_accelerometer acc;
  };
} ix_packet;


/*
 * Parse a packet from a buffer.
 *
 * Returns err == IX_OK, res.uin == offset of first unparsed byte on successful
 * parse. Parsed packets are returned via pac. If err != IX_OK, res is
 * undefined and pac is not modified.
 */
SO_EXPORT
ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet* pac);
