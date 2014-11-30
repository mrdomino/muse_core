#include "all.h"


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
static const char proto[] = { 'P', 'R', 'O', 'T', 'O' };


#define MUSE_MINVER "MUSE APP HW-0.0 FW-0.0.0 BL-0.0.0 FW_BUILD-0 " \
                    "FW_TARGET_HW-0.0 FW_TYPE- PROTO-2"


ssize_t
ix_find_version_start(const char* buf, size_t len)
{
  ssize_t ret;

  for (ret = 0; ret + sizeof muse_spc < len; ++ret) {
    if (memcmp(buf + ret, muse_spc, sizeof muse_spc) == 0) {
      return ret;
    }
  }
  return -1;
}

static ix_pv_ret
_parse_version_xy(const char* buf, size_t len, void* p)
{
  ix_version_xy* ver = (ix_version_xy*)p;

  (void)buf;
  (void)len;
  (void)ver;
  return (ix_pv_ret){ .err = IX_PV_FAIL };
}

static ix_pv_ret
_parse_version_xyz(const char* buf, size_t len, void* p)
{
  ix_version_xyz* ver = (ix_version_xyz*)p;

  (void)buf;
  (void)len;
  (void)ver;
  return (ix_pv_ret){ .err = IX_PV_FAIL };
}

static ix_pv_ret
_parse_uint16_t(const char* buf, size_t len, void* p)
{
  uint16_t* x = (uint16_t*)p;

  (void)buf;
  (void)len;
  (void)x;
  return (ix_pv_ret){ .err = IX_PV_FAIL };
}

static ix_pv_ret
_parse_fw_type(const char* buf, size_t len, void* p)
{
  ix_fw_type* fwt = (ix_fw_type*)p;

  (void)buf;
  (void)len;
  (void)fwt;
  return (ix_pv_ret){ .err = IX_PV_FAIL };
}

static ix_pv_ret
_parse_label_dash(const char* buf, size_t len,
                  const char label[], size_t label_len,
                  ix_pv_ret (*parse_fn)(const char*, size_t, void*),
                  void* p)
{
  ssize_t   fed = 0;
  ix_pv_ret r;

  if (len < label_len + 1) {
    return (ix_pv_ret){ .err = IX_PV_NEED_MORE };
  }
  if (memcmp(buf, label, label_len) != 0) {
    return (ix_pv_ret){ .err = IX_PV_BAD_STR };
  }
  fed += label_len;
  if (buf[fed] != '-') {
    return (ix_pv_ret){ .err = IX_PV_BAD_STR };
  }
  fed += 1;
  r = parse_fn(buf + fed, len - fed, p);
  if (r.end < 0) {
    return r;
  }
  else return (ix_pv_ret){ .end = fed + r.end };
}

ix_pv_ret
ix_parse_version(const char* buf, size_t len, ix_muse_config* cfg)
{
  ssize_t fed = 5;

  assert(len > 5);

  switch (buf[fed]) {
  default:
    return (ix_pv_ret){ .err = IX_PV_BAD_STR };
# define PV_MATCH(c, rest, typ)                           \
  case c:                                                 \
    if (len < fed + 1 + sizeof rest) {                    \
      return (ix_pv_ret){ .err = IX_PV_NEED_MORE };       \
    }                                                     \
    if (memcmp(rest, buf + fed + 1, sizeof rest) != 0) {  \
      return (ix_pv_ret){ .err = IX_PV_BAD_STR };         \
    }                                                     \
    cfg->img_type = typ;                                  \
    fed += 1 + sizeof rest;                               \
    break

  PV_MATCH('A', pp_spc, IX_IMG_APP);
  PV_MATCH('B', oot_spc, IX_IMG_BOOT);
  PV_MATCH('T', est_spc, IX_IMG_TEST);
# undef PV_MATCH
  }

  {
    ix_pv_ret r;

#   define PV_LABEL_DASH(label, parse_fn, addr)   \
    r = _parse_label_dash(buf + fed, len - fed,   \
                          label, sizeof label,    \
                          parse_fn, addr);        \
    if (r.end < 0) {                              \
      return r;                                   \
    }                                             \
    else fed += r.end

    PV_LABEL_DASH(hw, _parse_version_xy, &cfg->hw_version);
    PV_LABEL_DASH(fw, _parse_version_xyz, &cfg->fw_version);
    PV_LABEL_DASH(bl, _parse_version_xyz, &cfg->bl_version);
    PV_LABEL_DASH(fw_build, _parse_uint16_t, &cfg->build_number);
    PV_LABEL_DASH(fw_target_hw, _parse_version_xy, &cfg->target_hw_version);
    PV_LABEL_DASH(fw_type, _parse_fw_type, &cfg->fw_type);
    {
      uint16_t pv;

      PV_LABEL_DASH(proto, _parse_uint16_t, &pv);
      if (pv != 2) {
        return (ix_pv_ret){ .err = IX_PV_BAD_VER };
      }
    }
#   undef PV_LABEL_DASH
  }

  return (ix_pv_ret){ .end = fed };
}

void
ix_print_version(FILE* fp, const ix_muse_config* cfg)
{
  (void)cfg;
  fprintf(fp, "tsst\n");
}
