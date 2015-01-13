// Copyright 2015 Steven Dee.

#include "internal_usage.h"

// Make sure to include hammer as extern "C" at the toplevel namespace in
// order for its symbols to match.
#ifdef __cplusplus
extern "C" {
#endif
#include <hammer/glue.h>
#include <hammer/hammer.h>
#ifdef __cplusplus
}
#endif

namespace interaxon {
namespace _ {

// IX_INTERNAL_H_ tells the individual source files that the internal
// includes have already been included elsewhere. We use Plan9-style
// includes, so the headers don't have their own include guards.
//
// IX_MUSE_CORE_H_ works similarly for the public API.
#define IX_INTERNAL_H_

#include "../src/defs_internal.h"
#include "../src/r.h"

#include "../src/connect.c"
#include "../src/packet.c"
#include "../src/r.c"
#include "../src/result.c"
#include "../src/version.c"

}   // namespace _
}   // namespace interaxon
