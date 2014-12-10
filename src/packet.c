#include <assert.h>
#include <hammer/glue.h>
#include <hammer/hammer.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include "defs.h"

#include "packet.h"


struct _ix_packet {
  ix_packet_type type;
};

static HParser *parser;


// TODO(soon): finish the packet parser
static bool
validate_type(HParseResult* p, void* user_data)
{
  (void)user_data;
  switch (p->ast->uint) {
  case 0xf:
  case 0xe:
  case 0xd:
  case 0xc:
  case 0xb:
  case 0xa:
  case 0x9:
    return true;
  case 0x0:
    return true; /* XXX ??? */
  default:
    return false;
  }
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

__attribute__((constructor))
static void
_pp_init_parser()
{
#ifndef NDEBUG
  static int inited;

  assert(!inited);
  inited = 1;
#endif
  H_VRULE(type, h_bits(4, false));
  H_VRULE(flags_dropped, h_bits(4, false));
  H_VRULE(flags_ndropped, h_bits(4, false));
  H_RULE(dropped_samples, h_uint16());
  H_RULE(dropped, h_choice(h_sequence(flags_dropped, dropped_samples, NULL),
                           flags_ndropped,
                           NULL));

  H_RULE(packet, h_sequence(type, dropped, NULL));
  parser = packet;
}


ix_packet_type
ix_packet_get_type(const ix_packet* pac)
{
  return pac->type;
}

// TODO(soon): use ix_result everywhere, remove these
static ix_pp_ret
_pp_fail(int16_t err)
{
  ix_pp_ret r;

  assert(err != 0);
  r.err = err;
  return r;
}

static ix_pp_ret
_pp_ok(uint32_t nread, uint16_t npacs)
{
  ix_pp_ret r;

  r.err = 0;
  r.nread = nread;
  r.npacs = npacs;
  return r;
}

ix_pp_ret
ix_packet_parse(const uint8_t* buf, size_t len, ix_packet* pacs, size_t cpacs)
{
  HParseResult *r = h_parse(parser, buf, len);

  (void)pacs;
  (void)cpacs;

  if (r) {
    assert(r->bit_length % 8 == 0);
    h_parse_result_free(r);
    return _pp_ok(r->bit_length / 8, 0);
  }
  else return _pp_fail(-1);
}