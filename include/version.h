/* Copyright 2014 Interaxon, Inc. */

/*
 * include <stddef.h>   for size_t
 * include <stdint.h>   for sized int types
 * include "defs.h"     for SO_EXPORT
 */

/*
 * Smallest possible valid muse version string.
 */
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
  uint16_t z;
} ix_version_xyz;

/*
 * Configuration for proto v2 Muse.
 */
typedef struct {
  ix_img_type     img_type;
  ix_version_xyz  hw_version;
  ix_version_xyz  fw_version;
  ix_version_xyz  bl_version;
  uint16_t        build_number;
  ix_version_xyz  target_hw_version;
  ix_fw_type      fw_type;
} ix_muse_version;

enum _ix_vp_err {
  IX_VP_FAIL = -1,
  IX_VP_NEED_MORE = -2,
  IX_VP_BAD_STR = -3,
  IX_VP_BAD_VER = -4
};

/*
 * Return of version_parse.
 *
 * If err == 0, end is the first byte after the version string and trailing
 * newline. If err is nonzero, then it describes what went wrong.
 */
typedef struct {
  size_t          end;
  enum _ix_vp_err err;
} ix_vp_ret;


/*
 * Looks for a possible Muse version string in buf.
 *
 * Returns the start of the string if found, or -1 if none.
 *
 * Thread-safe (reentrant).
 */
SO_EXPORT
int32_t
ix_version_find_start(const char* buf, size_t len);

/*
 * Parses a muse version string.
 *
 * Thread-hostile -- uses a global data structure to store intermediate data.
 */
SO_EXPORT
ix_vp_ret
ix_version_parse(const char* buf, size_t len, ix_muse_version* ver);
