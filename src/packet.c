#include <stddef.h>
#include <stdint.h>
#include "defs.h"
#include "result.h"
#include "packet.h"

#include "r.h"
#include "initializer.h"

#include <assert.h>
#include <hammer/glue.h>
#include <hammer/hammer.h>
#include <string.h>


enum {
  TT_ix_packet = TT_USER,
  TT_ix_samples_n,
};

static HParser *parser;


#define VALIDATE_TYPE(x, n)                             \
  static bool                                           \
  validate_type_ ##x(HParseResult* p, void* user_data)  \
  {                                                     \
    IX_UNUSED(user_data);                               \
    return H_CAST_UINT(p->ast) == (n);                  \
  }                                                     \
  static bool validate_type_ ##x(HParseResult* p, void* user_data)

VALIDATE_TYPE(drl_ref, 0x9);
VALIDATE_TYPE(acc,     0xa);
VALIDATE_TYPE(battery, 0xb);
VALIDATE_TYPE(error,   0xd);
VALIDATE_TYPE(eeg,     0xe);

#undef VALIDATE_TYPE

static bool
validate_flags_dropped(HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return (H_CAST_UINT(p->ast) & 0x8) == 0x8;
}

static bool
validate_flags_ndropped(HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return H_CAST_UINT(p->ast) == 0;
}

static bool
validate_packet_sync(HParseResult* p, void* user_data)
{
  IX_UNUSED(user_data);
  return p->ast->uint == 0x55aaffff;
}

static HParsedToken*
act_ix_samples_n(const HParseResult* p, HParsedToken** sam, uint8_t n,
                 void* user_data)
{
  ix_samples_n *out;
  uint8_t      i;

  IX_UNUSED(user_data);

  out = H_ALLOC(ix_samples_n);
  out->n = n;
  for (i = 0; i < n; i++) {
    out->data[i] = H_CAST_UINT(sam[i]);
  }
  return H_MAKE(ix_samples_n, out);
}

static HParsedToken*
act_samples_acc(const HParseResult* p, void* user_data)
{
  return act_ix_samples_n(p, h_seq_elements(h_seq_index(p->ast, 0)), 3,
                          user_data);
}

static HParsedToken*
act_samples_eeg4(const HParseResult* p, void* user_data)
{
  return act_ix_samples_n(p, h_seq_elements(p->ast), 4, user_data);
}

static HParsedToken*
act_packet_acc(const HParseResult* p, void* user_data)
{
  ix_packet *pac;
  ix_samples_n *sam;

  IX_UNUSED(user_data);

  sam = H_CAST(ix_samples_n, h_seq_index(p->ast, 2));
  pac = H_ALLOC(ix_packet);
  pac->type = IX_PAC_ACCELEROMETER;
  pac->samples = *sam;
  return H_MAKE(ix_packet, pac);
}

static HParsedToken*
act_packet_eeg4(const HParseResult* p, void* user_data)
{
  ix_packet *pac;
  ix_samples_n *sam;

  IX_UNUSED(user_data);

  sam = H_CAST(ix_samples_n, h_seq_index(p->ast, 2));
  pac = H_ALLOC(ix_packet);
  pac->type = IX_PAC_UNCOMPRESSED_EEG;
  pac->samples = *sam;
  return H_MAKE(ix_packet, pac);
}

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

  H_VRULE(type_acc, nibble);
  H_VRULE(type_eeg, nibble);
  H_VRULE(type_drl_ref, nibble);
  H_VRULE(type_battery, nibble);
  H_VRULE(type_error, nibble);

  H_VRULE(flags_dropped, nibble);
  H_VRULE(flags_ndropped, nibble);
  H_RULE(dropped_samples, short_);
  H_RULE(flags,
         h_choice(h_sequence(flags_dropped, dropped_samples, NULL),
                  flags_ndropped,
                  NULL));

  H_ARULE(samples_acc,
          h_sequence(h_repeat_n(sample, 3), h_ignore(h_bits(2, false)), NULL));
  H_ARULE(samples_eeg4, h_repeat_n(sample, 4));

  H_ARULE(packet_acc, h_sequence(type_acc, flags, samples_acc, NULL));

  /* TODO(soon): configurable number of EEG channels */
  H_ARULE(packet_eeg4, h_sequence(type_eeg, flags, samples_eeg4, NULL));

  /* TODO(soon): compressed EEG */

  H_RULE(packet_drl_ref,
         h_sequence(type_drl_ref, flags_ndropped, h_repeat_n(sample, 2),
                    NULL));
  H_RULE(packet_battery,
         h_sequence(type_battery, flags_ndropped, h_repeat_n(short_, 4),
                    NULL));
  H_RULE(packet_error,
         h_sequence(type_error, flags_ndropped, word_, NULL));
  H_VRULE(packet_sync, word_);

  H_RULE(packet,
         h_choice(packet_acc,
                  packet_eeg4,
                  packet_drl_ref,
                  packet_battery,
                  packet_error,
                  packet_sync,
                  NULL));
  parser = packet;
}


ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet* pac)
{
  HParseResult *r = h_parse(parser, buf, len);
  ix_result    ret;

  if (r) {
    if (r->ast->token_type == TT_UINT && r->ast->uint == 0x55aaffff) {
      pac->type = IX_PAC_SYNC;
    }
    else if (r->ast->token_type == (HTokenType)TT_ix_packet) {
      memcpy(pac, r->ast->user, sizeof(ix_packet));
    }
    assert(r->bit_length % 8 == 0);
    ret = ix_r_uin(r->bit_length / 8);
    h_parse_result_free(r);
    return ret;
  }
  else return ix_r_err(IX_EBADSTR);
}
