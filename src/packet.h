/* Copyright 2014 Interaxon, Inc. */

/*
 * include <assert.h> for accessors
 * include <stddef.h> for size_t
 * include <stdint.h> for sized ints
 * include "defs.h" for SO_EXPORT
 * include "result.h" for ix_result
 */

/*
 * Possible packet types.
 */
typedef enum {
  IX_PAC_SYNC = 1,
  IX_PAC_ERROR,
  IX_PAC_EEG,
  IX_PAC_BATTERY,
  IX_PAC_ACCELEROMETER,
  IX_PAC_DRLREF
} ix_pac_type;


/*
 * Generic packet structure.
 */
typedef struct _ix_packet ix_packet;

/*
 * Packet callback function type.
 */
typedef void (*ix_packet_fn)(const ix_packet* p, void* user_data);


/*
 * Return the type of the passed packet.
 */
SO_EXPORT ix_pac_type ix_packet_type(const ix_packet* p);

/*
 * Packet channel accessor.
 *
 * Provides access to the sample at a specific channel for a given
 * accelerometer or EEG packet.
 *
 * WARNING: channel bound checks are implemented with assertions that
 * compile to no-ops if NDEBUG is defined. Always validate user-supplied
 * channel offsets before calling ix_packet_*_ch, or else you may
 * inadvertently give users access to arbitrary program memory.
 */
SO_EXPORT uint16_t ix_packet_ch(const ix_packet* p, size_t channel);

/*
 * Return the error code of the passed packet.
 *
 * Must be called only on packets of type IX_PAC_ERROR.
 */
SO_EXPORT uint32_t ix_packet_error(const ix_packet* p);

#define _ix_assert_type(p, t) (assert(ix_packet_type(p) == t), (p))

/*
 * DRL/REF accessors.
 */
#define ix_packet_drl(p) ix_packet_ch(_ix_assert_type(p, IX_PAC_DRLREF), 0)
#define ix_packet_ref(p) ix_packet_ch(_ix_assert_type(p, IX_PAC_DRLREF), 1)

/*
 * Accelerometer channel accessors.
 */
#define ix_packet_acc_ch(p, i) \
  ix_packet_ch(_ix_assert_type(p, IX_PAC_ACCELEROMETER), i)
#define ix_packet_acc_ch1(p) ix_packet_acc_ch(p, 0)
#define ix_packet_acc_ch2(p) ix_packet_acc_ch(p, 1)
#define ix_packet_acc_ch3(p) ix_packet_acc_ch(p, 2)

/*
 * EEG channel accessors.
 */
#define ix_packet_eeg_ch(p, i) \
  ix_packet_ch(_ix_assert_type(p, IX_PAC_EEG), i)
#define ix_packet_eeg_ch1(p) ix_packet_eeg_ch(p, 0)
#define ix_packet_eeg_ch2(p) ix_packet_eeg_ch(p, 1)
#define ix_packet_eeg_ch3(p) ix_packet_eeg_ch(p, 2)
#define ix_packet_eeg_ch4(p) ix_packet_eeg_ch(p, 3)

/*
 * Return the dropped samples value for this packet.
 *
 * This is the direct value sent with the specific packet in question -- in
 * other words, the number of dropped samples since the last successful packet
 * of the same type.
 *
 * This asserts that the packet is of a type that sends information on dropped
 * samples -- at this time, IX_PAC_ACCELEROMETER and IX_PAC_EEG. It is an error
 * to call it on any other packet type.
 */
SO_EXPORT uint16_t ix_packet_dropped_samples(const ix_packet* p);

/*
 * Parse a packet from a buffer.
 *
 * Returns err == IX_OK, res.uin == offset of first unparsed byte on successful
 * parse. Calls pac_f once per packet parsed with the packet and the supplied
 * user_data.
 */
SO_EXPORT
ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet_fn pac_f,
                void* user_data);
