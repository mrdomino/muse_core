/* Copyright 2014 InteraXon, Inc. */

#include <assert.h>
#include <ctype.h>
#include <hammer/glue.h>
#include <hammer/hammer.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "defs.h"
#include "except.h"
#include "version.h"


static const char muse_spc[] = { 'M', 'U', 'S', 'E', ' ' };

static HParser *parser;
/* TODO(soon): figure out how to make this not global. */
static ix_muse_version parsed_version;


int32_t
ix_version_find_start(const char* buf, size_t len)
{
  int32_t i;

  for (i = 0; i + sizeof muse_spc < len; ++i) {
    if (memcmp(buf + i, muse_spc, sizeof muse_spc) == 0) {
      return i;
    }
  }

  for (i = 1; i < (int32_t)sizeof muse_spc; ++i) {
    if (memcmp(buf + len - i, muse_spc, i) == 0) {
      return i;
    }
  }

  return -1;
}

/*
 * Action to take on parsing an ix_img_type token.
 */
static HParsedToken*
act_img(const HParseResult* p, void* user_data)
{
  ix_muse_version* ver = (ix_muse_version*)user_data;

  assert(p->ast->bytes.len > 0);
  switch (p->ast->bytes.token[0]) {
  case 'A':
    ver->img_type = IX_IMG_APP;
    break;
  case 'B':
    ver->img_type = IX_IMG_BOOT;
    break;
  case 'T':
    ver->img_type = IX_IMG_TEST;
    break;
  default:
    ver->img_type = IX_IMG_UNKNOWN;
  }
  return NULL;
}

/*
 * Library constructor -- initialize the parser.
 */
__attribute__((constructor))
static void
_vp_init_parser()
{
#ifndef NDEBUG
  static int inited;

  assert(!inited);
  inited = 1;
#endif

  H_RULE(muse, h_token((uint8_t*)"MUSE", 4));

  H_ADRULE(img, h_choice(h_token((uint8_t*)"APP", 3),
                         h_token((uint8_t*)"BOOT", 4),
                         h_token((uint8_t*)"TEST", 4),
                         NULL), &parsed_version);

  H_RULE(spc, h_ignore(h_ch(' ')));
  H_RULE(end, h_choice(h_ch('\n'), h_ch('\r'), NULL));

  H_RULE(version, h_sequence(muse, spc, img, end, NULL));
  parser = version;
}

/*
 * Parse failure -- no contents, return an error.
 */
static ix_vp_ret
_vp_fail(enum _ix_vp_err code)
{
  ix_vp_ret r;

  assert(code != 0);
  r.err = code;
  r.end = 0;
  return r;
}

/*
 * Parse success -- return next byte.
 */
static ix_vp_ret
_vp_ok(size_t fed)
{
  ix_vp_ret r;

  r.err = 0;
  r.end = fed;
  return r;
}

ix_vp_ret
ix_version_parse(const char* buf, size_t len, ix_muse_version* ver)
{
  HParseResult *r;

  assert(parser);
  r = h_parse(parser, (const uint8_t*)buf, len);

  if (r) {
    assert(r->bit_length % 8 == 0);
    memcpy(ver, &parsed_version, sizeof parsed_version);
    h_parse_result_free(r);
    return _vp_ok(r->bit_length / 8);
  }
  else return _vp_fail(IX_VP_BAD_STR);
}
