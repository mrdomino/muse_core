#include <cstddef>
#include <cstdint>

extern "C" {
#include <muse_core/defs.h>
#include <muse_core/result.h>
#include <muse_core/packet.h>
}

#include <cmath>

#include <array>
#include <exception>
#include <gtest/gtest.h>
#include <string>
#include <type_traits>
#include <utility>

using std::array;
using std::enable_if;
using std::exception;
using std::forward;
using std::make_pair;
using std::pair;
using std::remove_reference;
using std::string;
using std::vector;

namespace {


////////////////////////////////////////////////////////////////////////////////
//  Packet builders
////////////////////////////////////////////////////////////////////////////////

inline string sync_packet() {
  string ret;

  ret.push_back(0xff);
  ret.push_back(0xff);
  ret.push_back(0xaa);
  ret.push_back(0x55);
  return ret;
}

inline string error_packet(uint32_t error) {
  string ret;

  ret.push_back(0xd << 4);
  ret.push_back(error       & 0xff);
  ret.push_back(error >> 8  & 0xff);
  ret.push_back(error >> 16 & 0xff);
  ret.push_back(error >> 24 & 0xff);
  return ret;
}

template <typename... Args>
inline string bitpacked_samples(Args&&... args) {
  auto samples = std::array<uint16_t, sizeof...(Args)>{
    {static_cast<uint16_t>(args)...}};
  auto m = static_cast<size_t>(ceil(samples.size() * 10.0 / 8.0));
  auto values = std::vector<uint8_t>();
  values.reserve(m);
  // [11111111] | 0
  // [11000000] | [00222222]
  // [22220000] | [00003333]
  // [33333300] | [00000044]
  // [44444444] | 0
  auto sample = samples.cbegin();
  for (auto i = 0u; i < m; ++i) {
    uint8_t value, mask_b = 0, shift_b = 0;
    assert(sample != samples.cend());
    switch (i % 5) {
    case 0: value = 0,              mask_b = 0xff, shift_b = 2; break;
    case 1: value = *sample++ << 6, mask_b = 0x3f, shift_b = 4; break;
    case 2: value = *sample++ << 4, mask_b = 0x0f, shift_b = 6; break;
    case 3: value = *sample++ << 2, mask_b = 0x03, shift_b = 8; break;
    case 4: value = *sample++; break;
    default: assert(false);
    }
    if (mask_b && sample != samples.cend()) {
      value |= *sample >> shift_b & mask_b;
    }
    values.push_back(value);
  }
  assert(sample == samples.cend());
  return string(values.begin(), values.end());
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 3>::type* = nullptr>
inline string acc_packet(uint16_t dropped, Args&&... args) {
  string ret;
  uint8_t prefix[3];

  prefix[0] = 0xa << 4 | 1 << 3;
  prefix[1] = dropped & 0xff;
  prefix[2] = dropped >> 8;

  for (auto b : prefix) { ret.push_back(b); }
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 3>::type* = nullptr>
inline string acc_packet(Args&&... args) {
  uint8_t b = 0xa << 4;
  string ret;

  ret.push_back(b);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 4>::type* = nullptr>
inline string eeg_packet(uint16_t dropped, Args&&... args) {
  string ret;
  uint8_t prefix[3];

  prefix[0] = 0xe << 4 | 1 << 3;
  prefix[1] = dropped & 0xff;
  prefix[2] = dropped >> 8;
  for (auto b : prefix) { ret.push_back(b); }
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 4>::type* = nullptr>
inline string eeg_packet(Args&&... args) {
  uint8_t b = 0xe << 4;
  string ret;

  ret.push_back(b);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

////////////////////////////////////////////////////////////////////////////////
//  Parse wrapper
////////////////////////////////////////////////////////////////////////////////

constexpr bool has_dropped_samples(ix_pac_type t) {
  return t == IX_PAC_EEG || t == IX_PAC_ACCELEROMETER;
}

struct IxPacket {
  IxPacket(): type(static_cast<ix_pac_type>(0)), dropped_samples(0) {}
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
    else if (type == IX_PAC_ERROR) {
      error = ix_packet_error(p);
    }
  }

  ix_pac_type type;
  uint16_t dropped_samples;
  vector<uint16_t> samples;
  uint32_t error;
};

struct PacketParseError : ::exception {};

pair<uint32_t, vector<IxPacket>> test_parse(string const& buf) {
  auto pacs = vector<IxPacket>();
  ix_packet_fn pac_f = [](const ix_packet* p, void* user_data) {
    auto pacs = static_cast<vector<IxPacket>*>(user_data);
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

////////////////////////////////////////////////////////////////////////////////
//  Test fixtures
////////////////////////////////////////////////////////////////////////////////

class PacketTest : public ::testing::Test {
protected:
  virtual ~PacketTest() {}
  virtual void parse() {
    r = test_parse(buf);
    v = r.second;
    if (v.size()) p = v.at(0);
  }

  string buf;
  decltype(test_parse(buf)) r;
  decltype(r.second) v;
  remove_reference<decltype(v[0])>::type p;
};

////////////////////////////////////////////////////////////////////////////////
//  Test suite proper
////////////////////////////////////////////////////////////////////////////////

TEST_F(PacketTest, ParseFailures) {
  EXPECT_ANY_THROW(test_parse("asdf"));
  EXPECT_ANY_THROW(test_parse(""));
}

TEST_F(PacketTest, ParsesSync) {
  buf = sync_packet();
  parse();
  ASSERT_EQ(1u, v.size());
  EXPECT_EQ(IX_PAC_SYNC, v[0].type);
  EXPECT_EQ(4u, r.first);
}

TEST_F(PacketTest, ParsesAcc) {
  buf = acc_packet(1u, 2u, 3u);
  parse();
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(1u, p.samples[0]);
  EXPECT_EQ(2u, p.samples[1]);
  EXPECT_EQ(3u, p.samples[2]);

  auto maxval = (1u << 10) - 1u;

  buf = acc_packet(maxval, maxval, maxval);
  parse();
  ASSERT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(maxval, p.samples[0]);
  EXPECT_EQ(maxval, p.samples[1]);
  EXPECT_EQ(maxval, p.samples[2]);
}

TEST_F(PacketTest, ParsesEeg) {
  buf = eeg_packet(1u, 2u, 3u, 4u);
  parse();
  ASSERT_EQ(IX_PAC_EEG, p.type);
  EXPECT_EQ(1u, p.samples[0]);
  EXPECT_EQ(2u, p.samples[1]);
  EXPECT_EQ(3u, p.samples[2]);
  EXPECT_EQ(4u, p.samples[3]);

  auto maxval = (1u << 10) - 1u;

  buf = eeg_packet(maxval, maxval, maxval, maxval);
  parse();
  EXPECT_EQ(maxval, p.samples[0]);
  EXPECT_EQ(maxval, p.samples[1]);
  EXPECT_EQ(maxval, p.samples[2]);
  EXPECT_EQ(maxval, p.samples[3]);
}

TEST_F(PacketTest, ParseMultiplePackets) {
  buf = sync_packet() + acc_packet(0, 0, 0) + eeg_packet(511, 512, 53, 400)
                      + acc_packet(737, 20, 3)
                      + eeg_packet(1023, 1022, 1021, 1000)
                      + sync_packet();
  auto pacs = vector<IxPacket>();
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

TEST_F(PacketTest, DroppedSamples) {
  buf = acc_packet(3u, 1u, 2u, 3u);
  parse();
  EXPECT_EQ(IX_PAC_ACCELEROMETER, p.type);
  EXPECT_EQ(3u, p.dropped_samples);

  buf = acc_packet(321u, 7u, 8u, 9u);
  parse();
  EXPECT_EQ(321u, p.dropped_samples);

  buf = eeg_packet(1337u, 1u, 2u, 3u, 4u);
  parse();
  EXPECT_EQ(IX_PAC_EEG, p.type);
  EXPECT_EQ(1337u, p.dropped_samples);
}

TEST_F(PacketTest, ParsesErrorPacket) {
  buf = error_packet(0x12345678);
  parse();
  ASSERT_EQ(IX_PAC_ERROR, p.type);
  EXPECT_EQ(0x12345678u, p.error);
  buf = error_packet(0);
  parse();
  ASSERT_EQ(IX_PAC_ERROR, p.type);
  EXPECT_EQ(0u, p.error);
}

// TODO(soon): drl_ref, battery, error, compressed eeg
// TODO(soon): NEED MORE vs BAD STR

}  // namespace
