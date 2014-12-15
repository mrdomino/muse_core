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

typedef struct _ix_packet ix_packet;


/*
 * Get the type of a packet.
 */
SO_EXPORT
ix_packet_type
ix_packet_get_type(const ix_packet* pac);

/*
 * Find the offset of the next sync packet in buf.
 *
 * Returns err == IX_OK, res.uin == first byte past the sync if one was found.
 * If no sync, then err == IX_EMOREDATA and res is undefined.
 */
SO_EXPORT
ix_result
ix_packet_find_sync(const uint8_t* buf, size_t len);

/*
 * Parse a sequence of packets from a buffer.
 *
 * Returns err == IX_OK, res.uin == offset of first unparsed byte on successful
 * parse. Parsed packets are returned via pacs, which is an array of cpacs
 * packet pointers. npacs is set to the number of successfully parsed packets.
 *
 * If err == IX_ECAPACITY, this could have parsed more packets than it did, but
 * was limited by cpacs. In this case, res.uin is still the offset of the first
 * unparsed byte, and pacs / npacs still contain the partial results. It need
 * not be the case that npacs == cpacs here; for instance, if a certain
 * sequence of bytes would have led to multiple packets being parsed such that
 * the total would be greater than npacs, then no packets would be parsed from
 * that byte sequence. The reasonable action to take in this case is to act on
 * the already-parsed packets (if any), increase cpacs (if npacs was 0), and
 * call again starting from buf + res.uin.
 *
 * If err == EBADSTR, the parse failed, and all the data in buf should be
 * treated as garbage up until the next sync packet.
 */
SO_EXPORT
ix_result
ix_packet_parse(const uint8_t* buf, size_t len,
                ix_packet** pacs, size_t cpacs, size_t* npacs);
