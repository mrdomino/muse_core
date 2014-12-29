/* Copyright 2014 InteraXon, Inc. */

/*
 * include <assert.h> to use the getter macros
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
 * Accelerometer type-specific accessors.
 *
 * These assert that ix_packet_type(p) == IX_PAC_ACCELEROMETER. It is an error
 * to call them on any other packet type.
 */
SO_EXPORT uint16_t ix_packet_acc_ch1(const ix_packet* p);
SO_EXPORT uint16_t ix_packet_acc_ch2(const ix_packet* p);
SO_EXPORT uint16_t ix_packet_acc_ch3(const ix_packet* p);

/*
 * EEG type-specific accessors.
 *
 * These assert that ix_packet_type(p) == IX_PAC_EEG. It is an error to call
 * them on any other packet type.
 */
SO_EXPORT uint16_t ix_packet_eeg_ch1(const ix_packet* p);
SO_EXPORT uint16_t ix_packet_eeg_ch2(const ix_packet* p);
SO_EXPORT uint16_t ix_packet_eeg_ch3(const ix_packet* p);
SO_EXPORT uint16_t ix_packet_eeg_ch4(const ix_packet* p);

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
