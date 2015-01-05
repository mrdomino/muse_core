/* Copyright 2014 Interaxon, Inc. */

#include <stddef.h>
#include <stdint.h>
#include "defs.h"
#include "result.h"
#include "packet.h"

#include "r.h"
#include "defs_internal.h"

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

struct _ix_packet {
  ix_pac_type  type;
  union {
    struct {
      uint16_t     dropped_samples;
      ix_samples_n samples;
    };
    uint32_t error;
  };
};

enum {
  TT_ix_packet = TT_USER,
  TT_ix_samples_n,
};


SO_EXPORT HParser *g_ix_packet;

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

#define ACT_VALIDATE_TYPE(N, T, C)                          \
  H_VALIDATE_APPLY(validate_type_ ##N, _uint_const_attr, C) \
  H_ACT_APPLY(act_type_ ##N, _make_uint_const, T)

ACT_VALIDATE_TYPE(drlref, IX_PAC_DRLREF, 0x9)
ACT_VALIDATE_TYPE(acc, IX_PAC_ACCELEROMETER, 0xa)
ACT_VALIDATE_TYPE(battery, IX_PAC_BATTERY, 0xb)
ACT_VALIDATE_TYPE(error, IX_PAC_ERROR, 0xd)
ACT_VALIDATE_TYPE(eeg, IX_PAC_EEG, 0xe)

#undef ACT_VALIDATE_TYPE

H_VALIDATE_APPLY(validate_flags_dropped, _uint_const_attr, 0x8)
H_VALIDATE_APPLY(validate_flags_no_dropped, _uint_const_attr, 0)
H_VALIDATE_APPLY(validate_packet_sync, _uint_const_attr, 0x55aaffff)

H_ACT_APPLY(act_prefix_no_dropped, _make_uint_const, 0)
H_ACT_APPLY(act_prefix_dropped, _make_uint_const, H_FIELD_UINT(1))

static HParsedToken*
act_ix_samples_n(const HParseResult* p, void* user_data)
{
  ix_samples_n *out;
  uint8_t      i;

  IX_UNUSED(user_data);
  out = H_ALLOC(ix_samples_n);
  out->n = h_seq_len(p->ast);
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
  if (has_dropped_samples) pac->dropped_samples = H_FIELD_UINT(1);
  if (has_data) {
    switch (H_INDEX_TOKEN(p->ast, 2)->token_type) {
    case TT_ix_samples_n: pac->samples = *H_FIELD(ix_samples_n, 2); break;
    case TT_UINT: pac->error = H_FIELD_UINT(2); break;
    default: assert(false);
    }
  }
  return H_MAKE(ix_packet, pac);
}

H_ACT_APPLY(_make_packet, _make_packet_generic,
            (ix_pac_type)H_FIELD_UINT(0), true, false)
H_ACT_APPLY(_make_packet_dropped, _make_packet_generic,
            (ix_pac_type)H_FIELD_UINT(0), true, true)

H_ACT_APPLY(act_packet_sync, _make_packet_generic, IX_PAC_SYNC, false, false)
static HAction act_packet_error = _make_packet;
static HAction act_packet_battery = _make_packet;
static HAction act_packet_drlref = _make_packet;
static HAction act_packet_acc = _make_packet_dropped;
static HAction act_packet_eeg4 = _make_packet_dropped;

IX_INITIALIZER(_pp_init_parser)
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

#define _SRULE(N, P) H_RULE(N, h_action(P, act_ix_samples_n, NULL))

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

#undef _SRULE

  H_ARULE(packet_acc,
          h_sequence(type_acc, prefix_maybe_dropped, samples_acc, NULL));

  /* TODO(soon): configurable number of EEG channels */
  H_ARULE(packet_eeg4,
          h_sequence(type_eeg, prefix_maybe_dropped, samples_eeg4, NULL));

  /* TODO(soon): compressed EEG */

  H_ARULE(packet_drlref,
          h_sequence(type_drlref, prefix_no_dropped, samples_drlref, NULL));
  H_ARULE(packet_battery,
          h_sequence(type_battery, prefix_no_dropped, data_battery, NULL));
  H_ARULE(packet_error,
          h_sequence(type_error, prefix_no_dropped, word, NULL));
  H_AVRULE(packet_sync, word);

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

ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet_fn pac_f,
                void* user_data)
{
  HParseResult *p = h_parse(g_ix_packet, buf, len);
  ix_result    ret;

  if (p) {
    assert(p->bit_length % 8 == 0);
    ret = ix_r_uin(p->bit_length / 8);
    pac_f(H_CAST(ix_packet, p->ast), user_data);
    h_parse_result_free(p);
  }
  else ret = ix_r_err(IX_EBADSTR);
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
  assert(channel < p->samples.n);
  return p->samples.data[channel];
}

uint16_t
ix_packet_dropped_samples(const ix_packet* p)
{
  assert(p->type == IX_PAC_ACCELEROMETER || p->type == IX_PAC_EEG);
  return p->dropped_samples;
}
