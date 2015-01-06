// Copyright 2015 Interaxon, Inc.
//
// This file exposes the Muse Core API under a namespace so that it can be
// called and compiled natively as C++.
//
// This is NOT the recommended way to use Muse Core. Generally, you should
// just include <muse_core/muse_core.h> at the top level namespace and link
// against the muse_core library either statically or dynamically. Doing so
// means your code will immediately benefit from minor and bugfix releases,
// including security updates, and will always interact well with other
// projects linked against muse_core.
//
// We use this pattern internally, and if you are developing an SDK you may
// consider doing so as well, in order to allow people to link against our
// SDK without having to also install muse_core while also allowing people to
// link against muse_core separately if they choose to do so. If you are
// considering doing this, drop us a line at <sdk@interaxon.ca> to let us know.
//

#pragma once

// We would just include <muse_core/muse_core.h> here directly, but it exports
// symbols extern "C", which sidesteps C++ namespaces. The whole point of this
// is to have the Muse Core symbols available without conflicting with those
// exposed by the library.
//
#define IX_MUSE_CORE_H_

#include <cstddef>
#include <cstdint>

namespace interaxon {
namespace _ {

#include <muse_core/defs.h>
#include <muse_core/packet.h>
#include <muse_core/result.h>
#include <muse_core/connect.h>
#include <muse_core/serial.h>
#include <muse_core/version.h>
#include <muse_core/util.h>

}   // namespace _
}   // namespace interaxon
