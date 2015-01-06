/* Copyright 2014 Interaxon, Inc. */

/*
 * This file defines parsers for all the Muse packet types.
 *
 * The best strategy for understanding it is to start with _ix_packet_init. The
 * various RULE macros are hammer preprocessor magic -- see hammer/glue.h or
 * the hammer docs if you're curious, or just read them all as saying that the
 * thing on the left is constructed out of the thing or things on the right.
 *
 * Then, compare the data types at the top of the file with the functions
 * and preprocessor macros before _ix_packet_init. These all describe how to
 * build an ix_packet structure out of the various top-level packet parsers.
 * Crucially, the actions only ever get called on successful parse results, so
 * we don't need to make sure that e.g. a DRL/REF packet has 2 sample fields --
 * it's defined to by construction.
 *
 * Given all that, ix_packet_parse is stupidly trivial. The rest of the file is
 * just accessors for use in user callbacks.
 */

#ifndef IX_MUSE_CORE_H_
#include <stddef.h>
#include <stdint.h>
#include "defs.h"
#include "result.h"
#include "packet.h"
#endif

#ifndef IX_INTERNAL_H_
#include "r.h"
#include "defs_internal.h"
#endif

#include <assert.h>
#include <hammer/glue.h>
#include <hammer/hammer.h>


enum {
  DRLREF_CHANNELS = 2u,
  ACC_CHANNELS = 3u,
  BAT_CHANNELS = 4u,
  EEG4_CHANNELS = 4u,
  MAX_CHANNELS = 4u
};

typedef struct {
  uint16_t n;
  uint16_t data[MAX_CHANNELS];
} ix_samples_n;

struct _samples_dropped {
  ix_samples_n samples;
  uint16_t     dropped;
};

struct _ix_packet {
  ix_pac_type  type;
  union {
    struct _samples_dropped samples_dropped;
    uint32_t                error;
  };
};

/*
 * Hammer token types -- used by H_MAKE, H_CAST, H_FIELD, etc.
 */
enum {
  TT_ix_packet = TT_USER,
  TT_ix_samples_n,
};

/*
 * Action and validation to match packet type codes with our own enum values.
 */
#define _ACT_VALIDATE_TYPE(N, T, C)                         \
  H_VALIDATE_APPLY(validate_type_ ##N, _uint_const_attr, C) \
  H_ACT_APPLY(act_type_ ##N, _make_uint_const, T)
#define _SRULE(N, P) H_RULE(N, h_action(P, act_ix_samples_n, NULL))
#define _PRULE(N, P) H_RULE(N, h_action(P, act_ix_packet_no_dropped, NULL))
#define _PRULE_D(N, P) H_RULE(N, h_action(P, act_ix_packet_maybe_dropped, NULL))

/*
 * Top-level packet parser. Exported for use in benchmarking code, but not
 * mentioned in the public API. Clients should never use this directly.
 */
SO_EXPORT HParser *g_ix_packet;


/*
 * Actions and validation functions.
 *
 * These are mostly reusable across a bunch of different parsers. If they have
 * arguments before the HParseResult, those arguments are designed to be filled
 * by H_ACT_APPLY or H_VALIDATE_APPLY.
 */

static bool
_uint_const_attr(uint64_t v, const HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return H_CAST_UINT(p->ast) == v;
}

static HParsedToken*
_make_uint_const(uint64_t x, const HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return H_MAKE_UINT(x);
}

static HParsedToken*
act_ix_samples_n(const HParseResult* p, void* user_data)
{
  ix_samples_n *out;
  uint8_t      i;

  IX_UNUSED(user_data);
  out = H_ALLOC(ix_samples_n);
  out->n = h_seq_len(p->ast);
  /*
   * Just a sanity check to make sure we haven't messed up in our parse rules,
   * e.g. by adding a wider packet type and forgetting to bump MAX_CHANNELS.
   */
  assert(out->n <= MAX_CHANNELS);
  for (i = 0; i < out->n; i++) {
    out->data[i] = H_FIELD_UINT(i);
  }
  return H_MAKE(ix_samples_n, out);
}

static HParsedToken*
_make_packet_generic(ix_pac_type type, bool has_data, bool has_dropped_samples,
                     const HParseResult* p, void* user_data)
{
  ix_packet *pac;

  IX_UNUSED(user_data);
  pac = H_ALLOC(ix_packet);
  pac->type = type;
  if (has_dropped_samples) pac->samples_dropped.dropped = H_FIELD_UINT(1);
  if (has_data) {
    switch (H_INDEX_TOKEN(p->ast, 2)->token_type) {
    case TT_ix_samples_n:
      pac->samples_dropped.samples = *H_FIELD(ix_samples_n, 2);
      break;
    case TT_UINT:
      pac->error = H_FIELD_UINT(2);
      break;
    default: assert(false);
    }
  }
  return H_MAKE(ix_packet, pac);
}

_ACT_VALIDATE_TYPE(drlref, IX_PAC_DRLREF, 0x9)
_ACT_VALIDATE_TYPE(acc, IX_PAC_ACCELEROMETER, 0xa)
_ACT_VALIDATE_TYPE(battery, IX_PAC_BATTERY, 0xb)
_ACT_VALIDATE_TYPE(error, IX_PAC_ERROR, 0xd)
_ACT_VALIDATE_TYPE(eeg, IX_PAC_EEG, 0xe)
H_VALIDATE_APPLY(validate_flags_dropped, _uint_const_attr, 0x8)
H_VALIDATE_APPLY(validate_flags_no_dropped, _uint_const_attr, 0)
H_VALIDATE_APPLY(validate_packet_sync, _uint_const_attr, 0x55aaffff)
H_ACT_APPLY(act_prefix_no_dropped, _make_uint_const, 0)
H_ACT_APPLY(act_prefix_dropped, _make_uint_const, H_FIELD_UINT(1))
H_ACT_APPLY(act_packet_sync, _make_packet_generic,
            IX_PAC_SYNC, false, false)
H_ACT_APPLY(act_ix_packet_no_dropped, _make_packet_generic,
            (ix_pac_type)H_FIELD_UINT(0), true, false)
H_ACT_APPLY(act_ix_packet_maybe_dropped, _make_packet_generic,
            (ix_pac_type)H_FIELD_UINT(0), true, true)

IX_INITIALIZER(_ix_packet_init)
{
#ifndef NDEBUG
  static int inited;

  assert(!inited);
  inited = 1;
#endif
  H_RULE(nibble,
         h_with_endianness(BIT_BIG_ENDIAN, h_bits(4, false)));
  H_RULE(short_,    /* TODO(someday): little endian shorts */
         h_with_endianness(BYTE_BIG_ENDIAN, h_uint16()));
  H_RULE(sample, h_bits(10, false));
  H_RULE(word, h_uint32());

  H_AVRULE(type_acc, nibble);
  H_AVRULE(type_eeg, nibble);
  H_AVRULE(type_drlref, nibble);
  H_AVRULE(type_battery, nibble);
  H_AVRULE(type_error, nibble);

  H_VRULE(flags_dropped, nibble);
  H_VRULE(flags_no_dropped, nibble);
  H_RULE(prefix_no_dropped, flags_no_dropped);
  H_ARULE(prefix_dropped, h_sequence(flags_dropped, short_, NULL));
  H_RULE(prefix_maybe_dropped,
         h_choice(h_action(prefix_no_dropped, act_prefix_no_dropped, NULL),
                  prefix_dropped, NULL));

  _SRULE(data_battery,
         h_repeat_n(short_, BAT_CHANNELS));
  _SRULE(samples_drlref,
         h_action(
             h_sequence(h_repeat_n(sample, DRLREF_CHANNELS),
                        h_ignore(h_bits(4, false)),
                        NULL),
             h_act_first, NULL));
  _SRULE(samples_acc,
         h_action(
             h_sequence(h_repeat_n(sample, ACC_CHANNELS),
                        h_ignore(h_bits(2, false)),
                        NULL),
             h_act_first, NULL));
  _SRULE(samples_eeg4,
         h_repeat_n(sample, EEG4_CHANNELS));

  H_AVRULE(packet_sync, word);
  _PRULE_D(packet_acc,
           h_sequence(type_acc, prefix_maybe_dropped, samples_acc, NULL));
  /* TODO(soon): configurable number of EEG channels */
  _PRULE_D(packet_eeg4,
           h_sequence(type_eeg, prefix_maybe_dropped, samples_eeg4, NULL));
  /* TODO(soon): compressed EEG */
  _PRULE(packet_drlref,
         h_sequence(type_drlref, prefix_no_dropped, samples_drlref, NULL));
  _PRULE(packet_battery,
          h_sequence(type_battery, prefix_no_dropped, data_battery, NULL));
  _PRULE(packet_error,
          h_sequence(type_error, prefix_no_dropped, word, NULL));

  H_RULE(packet,
         h_with_endianness(BIT_LITTLE_ENDIAN | BYTE_LITTLE_ENDIAN,
                           h_choice(packet_acc,
                                    packet_eeg4,
                                    packet_drlref,
                                    packet_sync,
                                    packet_battery,
                                    packet_error,
                                    NULL)));
  g_ix_packet = packet;
}

size_t
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet_fn pac_f,
                void* user_data)
{
  HParseResult *p = h_parse(g_ix_packet, buf, len);
  size_t       r;

  if (p) {
    assert(p->bit_length > 0);
    assert(p->bit_length % 8 == 0);
    r = p->bit_length / 8;
    pac_f(H_CAST(ix_packet, p->ast), user_data);
    h_parse_result_free(p);
  }
  else r = 0;
  return r;
}

size_t
ix_packet_est_len(const uint8_t* buf, size_t len)
{
  bool   has_dropped;
  size_t ret;

  if (len == 0) {
    return 2;
  }
  else {
    switch (*buf >> 4) {
    case 0xa: case 0xe: has_dropped = true; break;
    default: has_dropped = false;
    }
    switch (*buf >> 4) {
    case 0x9: ret = 4; break;
    case 0xa: ret = 5; break;
    case 0xb: ret = 9; break;
    /* TODO(soon): compressed EEG */
    case 0xd: ret = 5; break;
    case 0xe: ret = 6; break;
    case 0xf: ret = 4; break;
    default: return 0;
    }
  }
  if (has_dropped && (*buf & 0x8)) {
    ret += 2;
  }
  return ret;
}


ix_pac_type
ix_packet_type(const ix_packet* p)
{ return p->type; }

uint32_t
ix_packet_error(const ix_packet* p)
{ return _ix_assert_type(p, IX_PAC_ERROR)->error; }

uint16_t
ix_packet_ch(const ix_packet* p, size_t channel)
{
  assert(channel < p->samples_dropped.samples.n);
  return p->samples_dropped.samples.data[channel];
}

uint16_t
ix_packet_dropped_samples(const ix_packet* p)
{
  assert(p->type == IX_PAC_ACCELEROMETER || p->type == IX_PAC_EEG);
  return p->samples_dropped.dropped;
}
