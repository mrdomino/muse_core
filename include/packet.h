/* Copyright 2014 InteraXon, Inc. */

/*
 * include <stddef.h> for size_t
 * include <stdint.h> for sized ints
 * include "defs.h" for SO_EXPORT
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

typedef struct _ix_packet ix_packet;

// TODO(soon): use ix_result everywhere
typedef struct {
  int16_t  err;   /* error message, or 0 if successful */
  uint32_t nread; /* number of bytes consumed */
  uint16_t npacs; /* number of parsed packets written to pacs */
} ix_pp_ret;


/*
 * Get the type of a packet.
 */
SO_EXPORT
ix_packet_type
ix_packet_get_type(const ix_packet* pac);

/*
 * Parses a sequence of packets from a buffer.
 *
 * Returns a struct describing the results. If r.err == 0, the operation
 * succeeded, r.nread is the offset of the first byte not consumed and r.npacs
 * is the number of packets parsed and returned.
 */
SO_EXPORT
ix_pp_ret
ix_packet_parse(const uint8_t* buf, size_t len,
                ix_packet* pacs, size_t cpacs);
