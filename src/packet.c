#include <assert.h>
#include <hammer/glue.h>
#include <hammer/hammer.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "defs.h"
#include "result.h"
#include "packet.h"
#include "r.h"
#include "initializer.h"


struct _ix_packet {
  ix_packet_type type;
};

static HParser *parser;


// TODO(soon): finish the packet parser
static bool
validate_type_eeg(HParseResult* p, void* user_data)
{
  (void)user_data;
  return p->ast->uint == 0xe;
}

static bool
validate_type_drl_ref(HParseResult* p, void* user_data)
{
  (void)user_data;
  return p->ast->uint == 0x9;
}

static bool
validate_type_battery(HParseResult* p, void* user_data)
{
  (void)user_data;
  return p->ast->uint == 0xb;
}

static bool
validate_type_error(HParseResult* p, void* user_data)
{
  (void)user_data;
  return p->ast->uint == 0xd;
}

static bool
validate_flags_dropped(HParseResult* p, void* user_data)
{
  (void)user_data;
  return (p->ast->uint & 0x8) == 0x8;
}

static bool
validate_flags_ndropped(HParseResult* p, void* user_data)
{
  (void)user_data;
  return (p->ast->uint & 0x8) == 0;
}

static bool
validate_packet_sync(HParseResult* p, void* user_data)
{
  (void)user_data;
  return p->ast->uint == 0x55aaffff;
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
  H_RULE(sample, h_with_endianness(MUSE_ENDIAN, h_bits(10, false)));
  H_RULE(word_, h_with_endianness(MUSE_ENDIAN, h_uint32()));
#undef MUSE_ENDIAN

  H_VRULE(type_eeg, nibble);
  H_VRULE(type_drl_ref, nibble);
  H_VRULE(type_battery, nibble);
  H_VRULE(type_error, nibble);

  H_VRULE(flags_dropped, nibble);
  H_VRULE(flags_ndropped, nibble);
  H_RULE(dropped_samples, short_);
  H_RULE(dropped,
         h_choice(h_sequence(flags_dropped, dropped_samples, NULL),
                  flags_ndropped,
                  NULL));

  /* TODO(soon): configurable number of EEG channels */
  H_RULE(packet_eeg,
         h_sequence(type_eeg, dropped, h_repeat_n(sample, 4), NULL));
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
         h_choice(packet_eeg,
                  packet_drl_ref,
                  packet_battery,
                  packet_error,
                  packet_sync,
                  NULL));
  parser = packet;
}


ix_packet_type
ix_packet_get_type(const ix_packet* pac)
{
  return pac->type;
}

ix_result
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet** pacs, size_t cpacs,
                size_t* npacs)
{
  HParseResult *r = h_parse(parser, buf, len);

  (void)cpacs;
  (void)pacs;
  (void)npacs;

  if (r) {
    assert(r->bit_length % 8 == 0);
    h_parse_result_free(r);
    return ix_r_uin(r->bit_length / 8);
  }
  else return ix_r_err(IX_EBADSTR);
}
