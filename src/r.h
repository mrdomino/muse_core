/* Copyright 2014 Interaxon, Inc. */

/*
 * include <stdint.h> for sized int types
 * include "result.h" for ix_result
 */

/*
 * Shorthand functions to return ix_results.
 *
 * Internal use only. This should not show up in the user-facing include dir.
 */
ix_result ix_r_err(enum _ix_err err);
ix_result ix_r_sin(int32_t      sin);
ix_result ix_r_uin(uint32_t     uin);
ix_result ix_r_ptr(void*        ptr);
