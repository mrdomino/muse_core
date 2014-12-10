#include <cstddef>
#include <cstdint>

extern "C" {
#include "defs.h"
#include "packet.h"
}

#include <gtest/gtest.h>
#include <string>

// TODO(soon): test cases for all packet types + failures
// TODO(soon): + a helper to produce arbitrary packet-like things

namespace {

TEST(PacketTest, ParsesDroppedSampleFlag) {
  uint8_t buf[80];
  buf[0] = 0xf8;
  buf[1] = 3;
  buf[2] = 4;
  {
    auto r = ix_packet_parse(buf, sizeof buf, NULL, 0);
    EXPECT_EQ(0, r.err);
    EXPECT_EQ(3u, r.nread);
  }

  buf[0] = 0xf0;
  {
    auto r = ix_packet_parse(buf, sizeof buf, NULL, 0);
    EXPECT_EQ(0, r.err);
    EXPECT_EQ(1u, r.nread);
  }
}

}  // namespace
