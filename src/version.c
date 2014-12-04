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


ssize_t
ix_version_find_start(const char* buf, size_t len)
{
  ssize_t ret;

  for (ret = 0; ret + sizeof muse_spc < len; ++ret) {
    if (memcmp(buf + ret, muse_spc, sizeof muse_spc) == 0) {
      return ret;
    }
  }
  return -1;
}

static ix_vp_ret
_vp_fail(enum _ix_vp_err code)
{
  return (ix_vp_ret){ .end = -1, .err = code };
}

/* TODO move me */
/*
 * Parse a base-10 number into a uint16_t.
 *
 * Returns first character after the number, or -1 if there was an
 * error. In the case of an error, errno is used to communicate the
 * condition.
 *
 * Possible errors:
 *    - EAGAIN    The buffer ended with a digit, so we might not have
 *                the whole number.
 *
 *    - EBADMSG   The buffer didn't start with a digit.
 *
 *    - EOVERFLOW The number would overflow a uint16.
 */
static ssize_t
_parse_uint16(const char* buf, size_t len, uint16_t* out)
{
  /*
   * State machine states. START and SAW DIGIT lead to EAGAIN. ERR leads
   * to EBADMSG. OVER leads to EOVERFLOW. SAW NONDIGIT is success.
   *
   *   +-------+   isdigit(buf[0])  +-----------+ would overflow +------+
   *   | START |------------------->| SAW DIGIT |--------------->| OVER |
   *   +-------+                    +-----------+L               +------+
   *       |                         /         \_/ isdigit(buf[i])
   *       | !isdigit(buf[0])      /
   *       v                     / !isdigit(buf[i])
   *    +------+           +--------------+
   *    | ERR  |           | SAW NONDIGIT |
   *    +------+           +--------------+
   */
  enum {
    _PU_START,
    _PU_ERR,
    _PU_OVER,
    _PU_SAW_DIGIT,
    _PU_SAW_NONDIGIT
  }        state = _PU_START;
  uint16_t acc = 0;
  ssize_t  red = 0;
  uint8_t  digit;

  while (len-- > 0) {
    switch (state) {
    default: assert(0);
    case _PU_START:
      if (!isdigit(*buf)) {
        state = _PU_ERR;
        break;
      }
      else state = _PU_SAW_DIGIT;
      /* fallthrough */
    case _PU_SAW_DIGIT:
      if (isdigit(*buf)) {
        digit = *buf - '0';
        /*  acc * 10 + digit > UINT16_MAX =>
            acc * 10 > UINT16_MAX - digit => */
        if (acc > (UINT16_MAX - digit) / 10) {
          state = _PU_OVER;
        }
        else {
          acc = acc * 10 + digit;
          break;
        }
      }
      else state = _PU_SAW_NONDIGIT;
      /* fallthrough unless we consumed a digit */
    case _PU_OVER:
    case _PU_SAW_NONDIGIT:
    case _PU_ERR:
      break;
    }

    assert(state != _PU_START);
    if (state == _PU_SAW_NONDIGIT ||
        state == _PU_OVER ||
        state == _PU_ERR)
    {
      break;
    }
    else {
      red++;
      buf++;
    }
  }

  switch (state) {
  default: assert(0);
  case _PU_START:
  case _PU_SAW_DIGIT:
    errno = EAGAIN;
    return -1;
  case _PU_ERR:
    errno = EBADMSG;
    return -1;
  case _PU_OVER:
    errno = EOVERFLOW;
    return -1;
  case _PU_SAW_NONDIGIT:
    *out = acc;
    return red;
  }
}

#define PARSE_UINT16(fed, len, buf, addr) do {              \
  ssize_t ret = _parse_uint16(buf + fed, len - fed, addr);  \
  if (fed + ret == len || (ret == -1 && errno == EAGAIN)) { \
    return _vp_fail(IX_VP_NEED_MORE);                       \
  }                                                         \
  if (ret == -1) {                                          \
    return _vp_fail(IX_VP_FAIL);                            \
  }                                                         \
  fed += ret;                                               \
} while (0)

#define PARSE_DOT(fed, len, buf) do {           \
  if (buf[fed] != '.') {                        \
    return _vp_fail(IX_VP_BAD_STR);             \
  }                                             \
  fed++;                                        \
} while (0)



static ix_vp_ret
_parse_version_xy(const char* buf, size_t len, void* p)
{
  ix_version_xy* ver = (ix_version_xy*)p;
  ssize_t        fed = 0, sen;

  if (len > SSIZE_MAX) {
    return _vp_fail(IX_VP_FAIL);
  }
  else sen = (ssize_t)len;

  PARSE_UINT16(fed, sen, buf, &ver->x);
  PARSE_DOT(fed, sen, buf);
  PARSE_UINT16(fed, sen, buf, &ver->y);

  return (ix_vp_ret){ .end = fed };
}

static ix_vp_ret
_parse_version_xyz(const char* buf, size_t len, void* p)
{
  ix_version_xyz* ver = (ix_version_xyz*)p;
  ssize_t fed = 0, sen;

  if (len > SSIZE_MAX) {
    return _vp_fail(IX_VP_FAIL);
  }
  else sen = (ssize_t)len;

  PARSE_UINT16(fed, sen, buf, &ver->x);
  PARSE_DOT(fed, sen, buf);
  PARSE_UINT16(fed, sen, buf, &ver->y);
  PARSE_DOT(fed, sen, buf);
  PARSE_UINT16(fed, sen, buf, &ver->z);

  return (ix_vp_ret){ .end = fed };
}

static ix_vp_ret
_parse_uint16_t(const char* buf, size_t len, void* p)
{
  uint16_t* x = (uint16_t*)p;
  ssize_t   fed = 0, sen;

  if (len > SSIZE_MAX) {
    return _vp_fail(IX_VP_FAIL);
  }
  else sen = (ssize_t)len;

  PARSE_UINT16(fed, sen, buf, x);
  return (ix_vp_ret){ .end = fed };
}

#undef PARSE_UINT16
#undef PARSE_DOT

static ix_vp_ret
_parse_fw_type(const char* buf, size_t len, void* p)
{
  ix_fw_type* fwt = (ix_fw_type*)p;

  (void)buf;
  (void)len;
  (void)fwt;
  return _vp_fail(IX_VP_FAIL);
}

static ix_vp_ret
_parse_label_dash(const char* buf, size_t len,
                  const char label[], size_t label_len,
                  ix_vp_ret (*parse_fn)(const char*, size_t, void*),
                  void* p)
{
  ssize_t   fed = 0;
  ix_vp_ret r;

  if (len < label_len + 1) {
    return _vp_fail(IX_VP_NEED_MORE);
  }
  if (memcmp(buf, label, label_len) != 0) {
    return _vp_fail(IX_VP_BAD_STR);
  }
  fed += label_len;
  if (buf[fed] != '-') {
    return _vp_fail(IX_VP_BAD_STR);
  }
  fed += 1;
  r = parse_fn(buf + fed, len - fed, p);
  if (r.end < 0) {
    return r;
  }
  else return (ix_vp_ret){ .end = fed + r.end };
}

ix_vp_ret
ix_version_parse(const char* buf, size_t len, ix_muse_version* cfg)
{
  ssize_t fed = 5, sen;

  if (len >= SSIZE_MAX) {
    return _vp_fail(IX_VP_FAIL);
  }
  else sen = (ssize_t)len;

  if (sen <= fed) {
    return _vp_fail(IX_VP_NEED_MORE);
  }

  switch (buf[fed]) {
  default:
    return _vp_fail(IX_VP_BAD_STR);
# define VP_MATCH(c, rest, typ)                           \
  case c:                                                 \
    if (len < fed + 1 + sizeof rest) {                    \
      return _vp_fail(IX_VP_NEED_MORE);                   \
    }                                                     \
    if (memcmp(rest, buf + fed + 1, sizeof rest) != 0) {  \
      return _vp_fail(IX_VP_BAD_STR);                     \
    }                                                     \
    cfg->img_type = typ;                                  \
    fed += 1 + sizeof rest;                               \
    break

  VP_MATCH('A', pp_spc, IX_IMG_APP);
  VP_MATCH('B', oot_spc, IX_IMG_BOOT);
  VP_MATCH('T', est_spc, IX_IMG_TEST);
# undef VP_MATCH
  }

  {
    ix_vp_ret r;

#   define VP_LABEL_DASH(label, parse_fn, addr)     \
    assert(len < SSIZE_MAX && fed <= (ssize_t)len); \
    r = _parse_label_dash(buf + fed, sen - fed,     \
                          label, sizeof label,      \
                          parse_fn, addr);          \
    if (r.end < 0) {                                \
      return r;                                     \
    }                                               \
    else fed += r.end

#   define VP_SPACE()                   \
    if (len <= (size_t)fed) {           \
      return _vp_fail(IX_VP_NEED_MORE); \
    }                                   \
    else if (buf[fed] != ' ') {         \
      return _vp_fail(IX_VP_BAD_STR);   \
    }                                   \
    else fed += 1

    VP_LABEL_DASH(hw, _parse_version_xy, &cfg->hw_version);
    VP_SPACE();
    VP_LABEL_DASH(fw, _parse_version_xyz, &cfg->fw_version);
    VP_SPACE();
    VP_LABEL_DASH(bl, _parse_version_xyz, &cfg->bl_version);
    VP_SPACE();
    VP_LABEL_DASH(fw_build, _parse_uint16_t, &cfg->build_number);
    VP_SPACE();
    VP_LABEL_DASH(fw_target_hw, _parse_version_xy, &cfg->target_hw_version);
    VP_SPACE();
    VP_LABEL_DASH(fw_type, _parse_fw_type, &cfg->fw_type);
    VP_SPACE();
    {
      uint16_t pv;

      VP_LABEL_DASH(proto, _parse_uint16_t, &pv);
      if (pv != 2) {
        return _vp_fail(IX_VP_BAD_VER);
      }
    }
#   undef VP_SPACE
#   undef VP_LABEL_DASH
  }

  return (ix_vp_ret){ .end = fed };
}

void
ix_version_print(FILE* fp, const ix_muse_version* cfg)
{
  (void)cfg;
  fprintf(fp, "tsst\n");
}
