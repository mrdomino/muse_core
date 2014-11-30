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
  uint8_t x;
  uint8_t y;
} ix_version_xy;

typedef struct {
  uint8_t x;
  uint8_t y;
  uint8_t z;
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
} ix_muse_config;

enum _ix_pv_err {
  IX_PV_FAIL = -1,
  IX_PV_NEED_MORE = -2,
  IX_PV_BAD_STR = -3,
  IX_PV_BAD_VER = -4
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
  enum _ix_pv_err  err;
} ix_pv_ret;


/*
 * Looks for a possible Muse version string in buf.
 *
 * Returns the start of the string if found, or -1 if none.
 */
ssize_t
ix_find_version_start(const char* buf, size_t len);

/*
 * Parses a muse configuration string.
 *
 * This is designed to be used with ix_find_version_start, i.e. it
 * doesn't bother verifying that the version string begins with "MUSE ".
 */
ix_pv_ret
ix_parse_version(const char* buf, size_t len, ix_muse_config* cfg);

/*
 * Prints a text representation of cfg to fp.
 */
void
ix_print_version(FILE* fp, const ix_muse_config* cfg);
