/* Copyright 2014 InteraXon, Inc. */

/*
 * include <stddef.h> for size_t
 * include <stdint.h> for sized ints
 * include "defs.h" for SO_EXPORT
 * include "result.h" for ix_result
 */

/*
 * Possible packet types.
 *
 * Stable -- none of the existing values are expected to change.
 */
typedef enum {
  IX_PAC_SYNC = 1,
  IX_PAC_UNCOMPRESSED_EEG,
  IX_PAC_ERROR,
  IX_PAC_EEG,
  IX_PAC_BATTERY,
  IX_PAC_ACCELEROMETER,
  IX_PAC_DRLREF
} ix_pac_type;

/* TODO(soon): decide whether to make these opaque and provide an allocator */

/*
 * Accelerometer channels.
 *
 * Unstable -- may change between releases.
 */
struct _pac_accelerometer {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
};

/*
 * EEG channels.
 *
 * Unstable -- may change between releases.
 */
struct _pac_eeg4 {
  uint16_t ch1;
  uint16_t ch2;
  uint16_t ch3;
  uint16_t ch4;
};

/*
 * Structure covering all packet types.
 *
 * Unstable -- may change between releases.
 */
typedef struct {
  ix_pac_type type;
  union {
    struct _pac_accelerometer acc;
    struct _pac_eeg4          eeg;
  };
} ix_packet;


/*
 * Getter macros for packet fields.
 *
 * These assert the appropriate packet type, and should not be called on
 * packets of unknown or incorrect type.
 *
 * Code that refers to these symbols, and not directly to ix_packet fields,
 * should not need to be modified -- only recompiled -- if the layout of
 * ix_packet changes.
 */
#define ix_packet_type(p) (p)->type
#define ix_assert(t, p, f) (assert(ix_packet_type(p) == (t)), f)
#define ix_packet_acc(p) ix_assert(IX_PAC_ACCELEROMETER, p, (p)->acc)
#define ix_packet_acc_ch1(p) ix_packet_acc(p).ch1
#define ix_packet_acc_ch2(p) ix_packet_acc(p).ch2
#define ix_packet_acc_ch3(p) ix_packet_acc(p).ch3
#define ix_packet_eeg(p) ix_assert(IX_PAC_UNCOMPRESSED_EEG, p, (p)->eeg)
#define ix_packet_eeg_ch1(p) ix_packet_eeg(p).ch1
#define ix_packet_eeg_ch2(p) ix_packet_eeg(p).ch2
#define ix_packet_eeg_ch3(p) ix_packet_eeg(p).ch3
#define ix_packet_eeg_ch4(p) ix_packet_eeg(p).ch4

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
