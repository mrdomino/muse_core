#include <cstddef>
#include <cstdint>

extern "C" {
#include <muse_core/defs.h>
#include <muse_core/result.h>
#include <muse_core/packet.h>
}

#include <gtest/gtest.h>
#include <string>

// TODO(soon): test cases for all packet types + failures
// TODO(soon): + a helper to produce arbitrary packet-like things

namespace {

TEST(PacketTest, ParsesDroppedSampleFlag) {
  uint8_t buf[80];
  size_t  n;
  buf[0] = 0xf8;
  buf[1] = 3;
  buf[2] = 4;
  {
    auto r = ix_packet_parse(buf, sizeof buf, NULL, 0, &n);
    EXPECT_EQ(IX_OK, r.err);
    EXPECT_EQ(3u, r.res.uin);
  }

  buf[0] = 0xf0;
  {
    auto r = ix_packet_parse(buf, sizeof buf, NULL, 0, &n);
    EXPECT_EQ(IX_OK, r.err);
    EXPECT_EQ(1u, r.res.uin);
  }
}

}  // namespace
