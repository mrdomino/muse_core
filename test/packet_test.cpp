#include <cstddef>
#include <cstdint>

extern "C" {
#include <muse_core/defs.h>
#include <muse_core/result.h>
#include <muse_core/packet.h>
}

#include <exception>
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

constexpr bool has_dropped_samples(ix_pac_type t) {
  return t == IX_PAC_EEG || t == IX_PAC_ACCELEROMETER;
}

struct IxPacket {
  IxPacket(const ix_packet* p):
    type(ix_packet_type(p)),
    dropped_samples(has_dropped_samples(type)?
                    ix_packet_dropped_samples(p) : 0)
  {
    if (type == IX_PAC_ACCELEROMETER) {
      samples.reserve(3);
      samples.push_back(ix_packet_acc_ch1(p));
      samples.push_back(ix_packet_acc_ch2(p));
      samples.push_back(ix_packet_acc_ch3(p));
    }
    else if (type == IX_PAC_EEG) {
      samples.reserve(4);
      samples.push_back(ix_packet_eeg_ch1(p));
      samples.push_back(ix_packet_eeg_ch2(p));
      samples.push_back(ix_packet_eeg_ch3(p));
      samples.push_back(ix_packet_eeg_ch4(p));
    }
  }

  ix_pac_type type;
  uint16_t dropped_samples;
  std::vector<uint16_t> samples;
};

struct PacketParseError : ::std::exception {};

pair<uint32_t, std::vector<IxPacket>> test_parse(string const& buf) {
  auto pacs = std::vector<IxPacket>();
  ix_packet_fn pac_f = [](const ix_packet* p, void* user_data) {
    auto pacs = static_cast<std::vector<IxPacket>*>(user_data);
    pacs->push_back(IxPacket(p));
  };
  auto r = ix_packet_parse((uint8_t*)buf.c_str(), buf.size(), pac_f, &pacs);
  if (r.err == IX_OK) {
    return make_pair(r.res.uin, pacs);
  }
  else {
    throw PacketParseError();
  }
}

TEST(PacketTest, ParseFailures) {
  EXPECT_ANY_THROW(test_parse("asdf"));
  EXPECT_ANY_THROW(test_parse(""));
}

TEST(PacketTest, ParsesSync) {
  auto r = test_parse(sync_packet());
  ASSERT_EQ(1u, r.second.size());
  EXPECT_EQ(IX_PAC_SYNC, r.second[0].type);
  EXPECT_EQ(4u, r.first);
}

TEST(PacketTest, ParsesAcc) {
  auto r = test_parse(acc_packet(1u, 2u, 3u));
  auto p = r.second.at(0);
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(1u, p.samples[0]);
  EXPECT_EQ(2u, p.samples[1]);
  EXPECT_EQ(3u, p.samples[2]);

  auto maxval = (1u << 10) - 1u;

  r = test_parse(acc_packet(maxval, maxval, maxval));
  p = r.second.at(0);
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(maxval, p.samples[0]);
  EXPECT_EQ(maxval, p.samples[1]);
  EXPECT_EQ(maxval, p.samples[2]);
}

TEST(PacketTest, ParsesEeg) {
  auto r = test_parse(eeg_packet(1u, 2u, 3u, 4u));
  auto p = r.second.at(0);
  ASSERT_EQ(IX_PAC_EEG, p.type);
  EXPECT_EQ(1u, p.samples[0]);
  EXPECT_EQ(2u, p.samples[1]);
  EXPECT_EQ(3u, p.samples[2]);
  EXPECT_EQ(4u, p.samples[3]);

  auto maxval = (1u << 10) - 1u;

  r = test_parse(eeg_packet(maxval, maxval, maxval, maxval));
  p = r.second.at(0);
  EXPECT_EQ(maxval, p.samples[0]);
  EXPECT_EQ(maxval, p.samples[1]);
  EXPECT_EQ(maxval, p.samples[2]);
  EXPECT_EQ(maxval, p.samples[3]);
}

TEST(PacketTest, ParseMultiplePackets) {
  auto buf = sync_packet() + acc_packet(0, 0, 0) + eeg_packet(511, 512, 53, 400)
                           + acc_packet(737, 20, 3)
                           + eeg_packet(1023, 1022, 1021, 1000)
                           + sync_packet();
  auto pacs = std::vector<IxPacket>();
  auto begin = buf.cbegin();
  auto end = buf.cend();
  while (begin != end) {
    auto r = test_parse(string(begin, end));
    auto v = r.second;
    pacs.insert(pacs.end(), v.begin(), v.end());
    begin += r.first;
  }
  EXPECT_EQ(end, begin);

  ASSERT_EQ(6u, pacs.size());

  EXPECT_EQ(IX_PAC_SYNC, pacs[0].type);
  EXPECT_EQ(IX_PAC_EEG, pacs[2].type);
  EXPECT_EQ(53u, pacs[2].samples[2]);
  EXPECT_EQ(IX_PAC_ACCELEROMETER, pacs[3].type);
  EXPECT_EQ(737u, pacs[3].samples[0]);
  EXPECT_EQ(1022u, pacs[4].samples[1]);
  EXPECT_EQ(IX_PAC_SYNC, pacs[5].type);
}

// TODO(soon): drl_ref, battery, error, compressed eeg
// TODO(soon): dropped samples
// TODO(soon): NEED MORE vs BAD STR

}  // namespace
