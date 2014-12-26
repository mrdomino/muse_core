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

inline string eeg_packet(uint16_t ch1, uint16_t ch2, uint16_t ch3,
                         uint16_t ch4) {
  string ret;
  uint8_t vals[6];

  // [11111111]
  // [11222222]
  // [22223333]
  // [33333344]
  // [44444444]

  vals[0] = 0xe << 4;
  vals[1] = ch1 >> 2;
  vals[2] = (ch1 << 6 & 0xc0) | (ch2 >> 4 & 0x3f);
  vals[3] = (ch2 << 4 & 0xf0) | (ch3 >> 6 & 0x0f);
  vals[4] = (ch3 << 2 & 0xfc) | (ch4 >> 8 & 0x03);
  vals[5] = (ch4      & 0xff);

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
  EXPECT_EQ(IX_PAC_SYNC, ix_packet_type(&r.second));
  EXPECT_EQ(4u, r.first);
}

TEST(PacketTest, ParsesAcc) {
  auto r = test_parse(acc_packet(1u, 2u, 3u));
  auto p = &r.second;
  ASSERT_EQ(IX_PAC_ACCELEROMETER, ix_packet_type(p));
  EXPECT_EQ(1u, ix_packet_acc_ch1(p));
  EXPECT_EQ(2u, ix_packet_acc_ch2(p));
  EXPECT_EQ(3u, ix_packet_acc_ch3(p));

  auto maxval = (1u << 10) - 1u;

  r = test_parse(acc_packet(maxval, maxval, maxval));
  p = &r.second;
  ASSERT_EQ(IX_PAC_ACCELEROMETER, ix_packet_type(p));
  EXPECT_EQ(maxval, ix_packet_acc_ch1(p));
  EXPECT_EQ(maxval, ix_packet_acc_ch2(p));
  EXPECT_EQ(maxval, ix_packet_acc_ch3(p));
}

TEST(PacketTest, ParsesEeg) {
  auto r = test_parse(eeg_packet(1u, 2u, 3u, 4u));
  auto p = &r.second;
  ASSERT_EQ(IX_PAC_UNCOMPRESSED_EEG, ix_packet_type(p));
  EXPECT_EQ(1u, ix_packet_eeg_ch1(p));
  EXPECT_EQ(2u, ix_packet_eeg_ch2(p));
  EXPECT_EQ(3u, ix_packet_eeg_ch3(p));
  EXPECT_EQ(4u, ix_packet_eeg_ch4(p));

  auto maxval = (1u << 10) - 1u;

  r = test_parse(eeg_packet(maxval, maxval, maxval, maxval));
  p = &r.second;
  EXPECT_EQ(maxval, ix_packet_eeg_ch1(p));
  EXPECT_EQ(maxval, ix_packet_eeg_ch2(p));
  EXPECT_EQ(maxval, ix_packet_eeg_ch3(p));
  EXPECT_EQ(maxval, ix_packet_eeg_ch4(p));
}

TEST(PacketTest, ParseMultiplePackets) {
  auto buf = sync_packet() + acc_packet(0, 0, 0) + eeg_packet(511, 512, 53, 400)
                           + acc_packet(737, 20, 3)
                           + eeg_packet(1023, 1022, 1021, 1000)
                           + sync_packet();
  auto begin = buf.cbegin();
  auto end = buf.cend();
  while (begin != end) {
    auto r = test_parse(string(begin, end));
    begin += r.first;
  }
  EXPECT_EQ(end, begin);
}

// TODO(soon): drl_ref, battery, error, compressed eeg
// TODO(soon): dropped samples
// TODO(soon): NEED MORE vs BAD STR

}  // namespace
