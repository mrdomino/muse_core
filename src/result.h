/* Copyright 2014 Interaxon, Inc. */

/*
 * include <stdint.h> for sized int types.
 * include "defs.h" for SO_EXPORT
 */

/*
 * Universal result code across all ix calls.
 *
 * IX_OK always means the call succeeded. Which codes are returned in what
 * circumstances is documented at individual function declaration sites.
 */
typedef enum _ix_err {
  IX_OK,
  IX_EMOREDATA,
  IX_EBADSTR,
  IX_EBADVER,
} ix_err;

/*
 * Return type of a function that could fail.
 *
 * If err is IX_OK, the function succeeded. Otherwise, the function failed. The
 * contents of res are dependent on the function that produced the result in
 * question.
 */
typedef struct {
  int8_t                    err;
  union {
    int32_t                   sin;
    uint32_t                  uin;
    struct _ix_connection*    con;
    void*                     ptr;
  }                         res;
} ix_result;

SO_EXPORT
const char*
ix_strerror(ix_err e);
