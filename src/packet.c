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
  TT_ix_packet = TT_USER
};


static HParser *parser;


// TODO(soon): finish the packet parser
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
act_packet_acc(const HParseResult* p, void* user_data)
{
  HParsedToken **fields, **channels;
  ix_packet *pac;

  IX_UNUSED(user_data);

  /* TODO(soon): clean up and use glue.h macros */
  assert(p->ast->seq->used >= 3);
  fields = h_seq_elements(p->ast);
  assert(fields[2]->seq->used == 3);
  channels = h_seq_elements(fields[2]);

  pac = H_ALLOC(ix_packet);
  pac->type = IX_PAC_ACCELEROMETER;
  pac->acc.ch1 = H_CAST_UINT(channels[0]);
  pac->acc.ch2 = H_CAST_UINT(channels[1]);
  pac->acc.ch3 = H_CAST_UINT(channels[2]);

  return H_MAKE(ix_packet, pac);
}

static HParsedToken*
act_packet_eeg4(const HParseResult* p, void* user_data)
{
  HParsedToken **fields, **channels;
  ix_packet *pac;

  IX_UNUSED(user_data);

  assert(p->ast->seq->used >= 3);
  fields = h_seq_elements(p->ast);
  assert(fields[2]->seq->used == 4);
  channels = h_seq_elements(fields[2]);

  pac = H_ALLOC(ix_packet);
  pac->type = IX_PAC_UNCOMPRESSED_EEG;
  pac->eeg.ch1 = H_CAST_UINT(channels[0]);
  pac->eeg.ch2 = H_CAST_UINT(channels[1]);
  pac->eeg.ch3 = H_CAST_UINT(channels[2]);
  pac->eeg.ch4 = H_CAST_UINT(channels[3]);

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

  H_ARULE(packet_acc,
          h_sequence(type_acc,
                     flags,
                     h_repeat_n(sample, 3),
                     h_ignore(h_bits(2, false)),
                     NULL));

  /* TODO(soon): configurable number of EEG channels */
  H_ARULE(packet_eeg4,
          h_sequence(type_eeg, flags, h_repeat_n(sample, 4), NULL));
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
