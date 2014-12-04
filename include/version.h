#define MUSE_MINVER "MUSE APP HW-0.0 FW-0.0.0 BL-0.0.0 FW_BUILD-0 " \
                    "FW_TARGET_HW-0.0 FW_TYPE- PROTO-2"


typedef enum {
  IX_IMG_UNKNOWN,
  IX_IMG_BOOT,
  IX_IMG_APP,
  IX_IMG_TEST
} ix_img_type;

typedef enum {
  IX_FW_UNKNOWN,
  IX_FW_CONSUMER,
  IX_FW_RESEARCH
} ix_fw_type;

typedef struct {
  uint16_t x;
  uint16_t y;
} ix_version_xy;

typedef struct {
  uint16_t x;
  uint16_t y;
  uint16_t z;
} ix_version_xyz;

/*
 * Configuration for proto v2 Muse.
 */
typedef struct {
  ix_img_type     img_type;
  ix_version_xy   hw_version;
  ix_version_xyz  fw_version;
  ix_version_xyz  bl_version;
  uint16_t        build_number;
  ix_version_xy   target_hw_version;
  ix_fw_type      fw_type;
} ix_muse_version;

enum _ix_vp_err {
  IX_VP_FAIL = -1,
  IX_VP_NEED_MORE = -2,
  IX_VP_BAD_STR = -3,
  IX_VP_BAD_VER = -4
};

/*
 * Return of parse_version.
 *
 * If end >= 0, then the parse was successful and end points to the byte
 * after the version string. If end < 0, then there was a parse error
 * described by err.
 */
typedef union {
  ssize_t          end;
  enum _ix_vp_err  err;
} ix_vp_ret;


/*
 * Looks for a possible Muse version string in buf.
 *
 * Returns the start of the string if found, or -1 if none.
 */
ssize_t
ix_version_find_start(const char* buf, size_t len);

/*
 * Parses a muse version string.
 *
 * This is designed to be used with ix_find_version_start, i.e. it
 * doesn't bother verifying that the version string begins with "MUSE ".
 *
 * If this returns negative, the contents of cfg are undefined.
 */
ix_vp_ret
ix_version_parse(const char* buf, size_t len, ix_muse_version* cfg);

/*
 * Prints a text representation of cfg to fp.
 */
void
ix_version_print(FILE* fp, const ix_muse_version* cfg);