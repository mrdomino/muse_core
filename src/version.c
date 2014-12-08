/* Copyright 2014 Interaxon, Inc. */

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
static const char pp_spc[] = { 'P', 'P', ' ' };
static const char oot_spc[] = { 'O', 'O', 'T', ' ' };
static const char est_spc[] = { 'E', 'S', 'T', ' ' };
static const char hw[] = { 'H', 'W' };
static const char fw[] = { 'F', 'W' };
static const char bl[] = { 'B', 'L' };
static const char fw_build[] = { 'F', 'W', '_', 'B', 'U', 'I', 'L', 'D' };
static const char fw_target_hw[] = {
  'F', 'W', '_', 'T', 'A', 'R', 'G', 'E', 'T', '_', 'H', 'W'
};
static const char fw_type[] = { 'F', 'W', '_', 'T', 'Y', 'P', 'E' };
static const char onsumer[] = { 'o', 'n', 's', 'u', 'm', 'e', 'r' };
static const char esearch[] = { 'e', 's', 'e', 'a', 'r', 'c', 'h' };
static const char proto[] = { 'P', 'R', 'O', 'T', 'O' };


static HParser *parser;
static ix_muse_version parsed_version;


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

static HParsedToken*
act_version(const HParseResult* p, void* user_data)
{
  (void)p;
  (void)user_data;
  return NULL;
}

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

  H_ADRULE(version, h_sequence(muse, spc, img, end, NULL), &parsed_version);

  parser = version;
}

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

static ix_vp_ret
_vp_fail(enum _ix_vp_err code)
{
  ix_vp_ret r;

  r.err = code;
  r.end = 0;
  return r;
}

static ix_vp_ret
_vp_ok(size_t fed)
{
  ix_vp_ret r;

  r.err = 0;
  r.end = fed;
  return r;
}

ix_vp_ret
ix_version_parse(const char* buf, size_t len, ix_muse_version* out)
{
  HParseResult *r;

  assert(parser);
  r = h_parse(parser, (const uint8_t*)buf, len);

  if (r) {
    assert(r->bit_length % 8 == 0);
    memcpy(out, &parsed_version, sizeof parsed_version);
    return _vp_ok(r->bit_length / 8);
  }
  else return _vp_fail(IX_VP_BAD_STR);
}
