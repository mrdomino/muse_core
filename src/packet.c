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


typedef struct {
  uint16_t n;
  uint16_t data[4];
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

static HParser *parser;


#define VALIDATE_UINT_PRED(N, P)                  \
  static bool                                     \
  validate_ ##N(HParseResult* p, void* user_data) \
  {                                               \
    IX_UNUSED(user_data);                         \
    return P;                                     \
  }

#define ACT_VALIDATE_TYPE(N, T, C)                          \
  VALIDATE_UINT_PRED(type_ ##N, H_CAST_UINT(p->ast) == (C)) \
  static HParsedToken*                                      \
  act_type_ ##N(const HParseResult* p, void* user_data) {   \
    IX_UNUSED(user_data);                                   \
    return H_MAKE_UINT(T);                                  \
  }

ACT_VALIDATE_TYPE(drlref, IX_PAC_DRLREF, 0x9)
ACT_VALIDATE_TYPE(acc, IX_PAC_ACCELEROMETER, 0xa)
ACT_VALIDATE_TYPE(battery, IX_PAC_BATTERY, 0xb)
ACT_VALIDATE_TYPE(error, IX_PAC_ERROR, 0xd)
ACT_VALIDATE_TYPE(eeg, IX_PAC_EEG, 0xe)

VALIDATE_UINT_PRED(flags_dropped, (H_CAST_UINT(p->ast) & 0x8) == 0x8)
VALIDATE_UINT_PRED(flags_ndropped, (H_CAST_UINT(p->ast) & 0x8) == 0)
VALIDATE_UINT_PRED(packet_sync, H_CAST_UINT(p->ast) == 0x55aaffff)

#undef ACT_VALIDATE_TYPE
#undef VALIDATE_UINT_PRED

static HParsedToken*
act_flags_ndropped(const HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return H_MAKE_UINT(0);
}

static HParsedToken*
act_prefix_dropped(const HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return H_MAKE_UINT(H_FIELD_UINT(1));
}

static HParsedToken*
act_ix_samples_n(const HParsedToken* sam, const HParseResult* p,
                 void* user_data)
{
  ix_samples_n *out;
  uint8_t      i;

  IX_UNUSED(user_data);
  out = H_ALLOC(ix_samples_n);
  out->n = h_seq_len(sam);
  assert(out->n <= 4);
  for (i = 0; i < out->n; i++) {
    out->data[i] = H_INDEX_UINT(sam, i);
  }
  return H_MAKE(ix_samples_n, out);
}

H_ACT_APPLY(act_data_battery, act_ix_samples_n, p->ast)
H_ACT_APPLY(act_samples_drlref, act_ix_samples_n, h_seq_index(p->ast, 0))
H_ACT_APPLY(act_samples_acc, act_ix_samples_n, h_seq_index(p->ast, 0))
H_ACT_APPLY(act_samples_eeg4, act_ix_samples_n, p->ast)

static HParsedToken*
act_packet_sync(const HParseResult* p, void* user_data)
{
  ix_packet *pac;

  IX_UNUSED(user_data);
  pac = H_ALLOC(ix_packet);
  pac->type = IX_PAC_SYNC;
  return H_MAKE(ix_packet, pac);
}

static HParsedToken*
_make_packet(bool has_dropped_samples,
             const HParseResult* p, void* user_data)
{
  ix_packet *pac;

  IX_UNUSED(user_data);
  pac = H_ALLOC(ix_packet);
  pac->type = H_FIELD_UINT(0);
  if (has_dropped_samples) pac->dropped_samples = H_FIELD_UINT(1);
  switch (H_INDEX_TOKEN(p->ast, 2)->token_type) {
  case TT_ix_samples_n: pac->samples = *H_FIELD(ix_samples_n, 2); break;
  case TT_UINT: pac->error = H_FIELD_UINT(2); break;
  default: assert(false);
  }
  return H_MAKE(ix_packet, pac);
}

H_ACT_APPLY(_make_packet_dropped, _make_packet, true)
H_ACT_APPLY(_make_packet_ndropped, _make_packet, false)

static HAction act_packet_error = _make_packet_ndropped;
static HAction act_packet_battery = _make_packet_ndropped;
static HAction act_packet_drlref = _make_packet_ndropped;
static HAction act_packet_acc = _make_packet_dropped;
static HAction act_packet_eeg4 = _make_packet_dropped;

IX_INITIALIZER(_pp_init_parser)
{
#ifndef NDEBUG
  static int inited;

  assert(!inited);
  inited = 1;
#endif
#define MUSE_ENDIAN (BYTE_LITTLE_ENDIAN | BIT_BIG_ENDIAN)
  H_RULE(nibble, h_bits(4, false));
  H_RULE(short_, h_with_endianness(MUSE_ENDIAN, h_uint16()));
  H_RULE(sample, h_bits(10, false));
  H_RULE(word_, h_with_endianness(MUSE_ENDIAN, h_uint32()));
#undef MUSE_ENDIAN

  H_AVRULE(type_acc, nibble);
  H_AVRULE(type_eeg, nibble);
  H_AVRULE(type_drlref, nibble);
  H_AVRULE(type_battery, nibble);
  H_AVRULE(type_error, nibble);

  H_VRULE(flags_dropped, nibble);
  H_AVRULE(flags_ndropped, nibble);
  H_ARULE(prefix_dropped, h_sequence(flags_dropped, short_, NULL));
  H_RULE(flags, h_choice(prefix_dropped, flags_ndropped, NULL));

  H_ARULE(data_battery, h_repeat_n(short_, 4));
  H_ARULE(samples_drlref,
          h_sequence(h_repeat_n(sample, 2), h_ignore(h_bits(4, false)), NULL));
  H_ARULE(samples_acc,
          h_sequence(h_repeat_n(sample, 3), h_ignore(h_bits(2, false)), NULL));
  H_ARULE(samples_eeg4, h_repeat_n(sample, 4));

  H_ARULE(packet_acc, h_sequence(type_acc, flags, samples_acc, NULL));

  /* TODO(soon): configurable number of EEG channels */
  H_ARULE(packet_eeg4, h_sequence(type_eeg, flags, samples_eeg4, NULL));

  /* TODO(soon): compressed EEG */

  H_ARULE(packet_drlref,
          h_sequence(type_drlref, flags_ndropped, samples_drlref, NULL));
  H_ARULE(packet_battery,
          h_sequence(type_battery, flags_ndropped, data_battery, NULL));
  H_ARULE(packet_error,
          h_sequence(type_error, flags_ndropped, word_, NULL));
  H_AVRULE(packet_sync, word_);

  H_RULE(packet,
         h_choice(packet_acc,
                  packet_eeg4,
                  packet_drlref,
                  packet_battery,
                  packet_error,
                  packet_sync,
                  NULL));
  parser = packet;
}


ix_pac_type
ix_packet_type(const ix_packet* p)
{ return p->type; }

uint32_t
ix_packet_error(const ix_packet* p)
{
  assert(p->type == IX_PAC_ERROR);
  return p->error;
}

uint16_t
ix_packet_ch(const ix_packet* p, size_t channel)
{
  size_t nch;

  switch (ix_packet_type(p)) {
  case IX_PAC_BATTERY:       /*FALLTHROUGH*/
  case IX_PAC_EEG:           nch = 4; break;
  case IX_PAC_ACCELEROMETER: nch = 3; break;
  case IX_PAC_DRLREF:        nch = 2; break;
  default:                   nch = 0;
  }
  assert(channel < nch);
  return p->samples.data[channel];
}

uint16_t
ix_packet_dropped_samples(const ix_packet* p)
{
  assert(p->type == IX_PAC_ACCELEROMETER || p->type == IX_PAC_EEG);
  return p->dropped_samples;
}


ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet_fn pac_f,
                void* user_data)
{
  HParseResult *p = h_parse(parser, buf, len);
  ix_result    ret;

  if (p) {
    assert(p->ast->token_type == (HTokenType)TT_ix_packet);
    assert(p->bit_length % 8 == 0);
    ret = ix_r_uin(p->bit_length / 8);
    pac_f(H_CAST(ix_packet, p->ast), user_data);
    h_parse_result_free(p);
    return ret;
  }
  else return ix_r_err(IX_EBADSTR);
}
