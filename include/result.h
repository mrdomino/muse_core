/*
 * include <stdint.h> for sized int types.
 */

/*
 * Universal result code across all ix calls.
 *
 * IX_OK always means the call succeeded. Which codes are returned in what
 * circumstances is documented at individual function declaration sites.
 */
typedef enum _ix_err {
  IX_OK,
  IX_ERR_NEED_MORE,
  IX_ERR_BAD_STR,
  IX_ERR_BAD_VER
} ix_err;

/*
 * Return type of a function that could fail.
 *
 * If err is IX_OK, the function succeeded. Otherwise, the function failed. The
 * contents of ok are dependent on the function that produced the result in
 * question.
 */
typedef struct {
  int8_t                    err;
  union {
    int32_t                   sin;
    uint32_t                  uin;
    struct _ix_connection*    con;
    void*                     ptr;
  }                         ok;
} ix_result;
