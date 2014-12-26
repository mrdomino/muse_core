#include <cstddef>
#include <cstdint>

extern "C" {
#include <muse_core/defs.h>
#include <muse_core/result.h>
#include <muse_core/packet.h>
}

#include <gtest/gtest.h>
#include <string>
#include <utility>

using std::make_pair;
using std::pair;
using std::string;

// TODO(soon): test cases for all packet types + failures
// TODO(soon): + a helper to produce arbitrary packet-like things

namespace {

inline string sync_packet() {
  string ret;

  ret.push_back(0xff);
  ret.push_back(0xff);
  ret.push_back(0xaa);
  ret.push_back(0x55);
  return ret;
}

inline string acc_packet(uint16_t ch1, uint16_t ch2, uint16_t ch3) {
  string ret;
  uint8_t vals[5];

  // [11111111]
  // [11222222]
  // [22223333]
  // [333333xx]

  vals[0] = 0xa << 4;
  vals[1] = ch1 >> 2;
  vals[2] = (ch1 << 6 & 0xc0) | (ch2 >> 4 & 0x3f);
  vals[3] = (ch2 << 4 & 0xf0) | (ch3 >> 6 & 0x0f);
  vals[4] = (ch3 << 2 & 0xfc);

  for (auto b : vals) { ret.push_back(b); }
  return ret;
}

pair<uint32_t, ix_packet> test_parse(string const& buf) {
  ix_packet p;
  auto r = ix_packet_parse((uint8_t*)buf.c_str(), buf.size(), &p);
  if (r.err == IX_OK) {
    return make_pair(r.res.uin, p);
  }
  else {
    throw "";
  }
}

TEST(PacketTest, ParseFailures) {
  EXPECT_ANY_THROW(test_parse("asdf"));
  EXPECT_ANY_THROW(test_parse(""));
}

TEST(PacketTest, ParsesSync) {
  auto r = test_parse(sync_packet());
  EXPECT_EQ(IX_PAC_SYNC, r.second.type);
  EXPECT_EQ(4u, r.first);
}

TEST(PacketTest, ParsesAcc) {
  auto r = test_parse(acc_packet(0x1, 0x2, 0x3));
  auto p = r.second;
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(1u, p.acc.ch1);
  EXPECT_EQ(2u, p.acc.ch2);
  EXPECT_EQ(3u, p.acc.ch3);

  auto maxval = (1u << 10) - 1u;

  r = test_parse(acc_packet(maxval, maxval, maxval));
  p = r.second;
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(maxval, p.acc.ch1);
  EXPECT_EQ(maxval, p.acc.ch2);
  EXPECT_EQ(maxval, p.acc.ch3);
}

}  // namespace
