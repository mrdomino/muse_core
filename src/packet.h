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
 * High guess at the maximum valid packet size. Used in ix_packet_est_len.
 * Exposed here for possible use in computing buffer sizes.
 */
enum { IX_PAC_MAXSIZE = 4096u };


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
SO_EXPORT
ix_pac_type
ix_packet_type(const ix_packet* p);

/*
 * Packet channel accessor.
 *
 * Provides access to the sample at a specific channel for a given
 * accelerometer or EEG packet.
 *
 * WARNING: channel bound checks are implemented with assertions that
 * compile to no-ops if NDEBUG is defined. Always validate user-supplied
 * channel offsets before calling ix_packet_ch, or else you may inadvertently
 * give users access to arbitrary program memory.
 */
SO_EXPORT
uint16_t
ix_packet_ch(const ix_packet* p, size_t channel);

/*
 * Return the error code of the passed packet.
 *
 * Must be called only on packets of type IX_PAC_ERROR.
 */
SO_EXPORT
uint32_t
ix_packet_error(const ix_packet* p);

/*
 * Macro that asserts a certain packet type and passes the packet through.
 */
#define _ix_assert_type(p, t) (assert(ix_packet_type(p) == t), (p))

/*
 * Battery accessors.
 *
 * Although these are defined in terms of ix_packet_battery_ch, please always
 * use the symbolic names for clarity's sake.
 */
#define ix_packet_battery_ch(p, i) \
  ix_packet_ch(_ix_assert_type(p, IX_PAC_BATTERY), i)
#define ix_packet_battery_percent(p) ix_packet_battery_ch(p, 0)
#define ix_packet_battery_fuel_gauge_mv(p) ix_packet_battery_ch(p, 1)
#define ix_packet_battery_adc_mv(p) ix_packet_battery_ch(p, 2)
#define ix_packet_battery_temp_c(p) ((int16_t)ix_packet_battery_ch(p, 3))

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
SO_EXPORT
uint16_t
ix_packet_dropped_samples(const ix_packet* p);

/*
 * Parse a packet from a buffer.
 *
 * Returns the offset of the first unparsed byte on successful parse, or 0 on
 * parse error. If the parse was successful, this will have called pac_f once
 * per packet parsed with the parsed packet and the supplied user_data.
 *
 * An error could mean either a corrupt data stream or a partial packet at the
 * end of the buffer. Use ix_packet_est_len to disambiguate these two cases.
 *
 * Must be called on non-NULL buf.
 */
SO_EXPORT
size_t
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet_fn pac_f,
                void* user_data);

/*
 * Return an estimate of how many bytes are needed for the next full packet.
 *
 * This uses a conservative heuristic based on limited knowledge about packet
 * types and structure. The goal is to inform the caller, given a buffer that
 * contains a partial packet:
 *
 *   1. when to try to parse it again, and
 *
 *   2. whether a parse failure would indicate corruption or merely
 *      insufficient data.
 *
 * If the return value is less than or equal to the passed length and
 * ix_packet_parse of that buffer and length returns an error, the buffer is
 * definitely corrupt and it is time to restart the stream, report an error, or
 * start scanning for a sync packet, depending on the application logic. (This
 * implies that a return value of 0 always indicates corruption. N.B. 0 is not
 * returned if len == 0.)
 *
 * For a given buffer, either est_len(buf, n) <= est_len(buf, n + 1) or
 * est_len(buf, n + 1) == 0. I.e., est_len returns a conservative estimate. It
 * may underestimate, but it never overestimates. If (m = est_len(buf, n)) > n,
 * then at least m - n more bytes are needed before the buffer could contain
 * a valid packet.
 *
 * This assumes that the maximum valid packet size is less than or equal to
 * IX_PAC_MAXSIZE. If it sees a packet that looks like it is bigger than that,
 * it returns 0 instead.
 *
 * If buf is NULL, len must be 0. The return value in this case is positive
 * and less than or equal to the minimum possible packet length.
 */
SO_EXPORT
size_t
ix_packet_est_len(const uint8_t* buf, size_t len);
