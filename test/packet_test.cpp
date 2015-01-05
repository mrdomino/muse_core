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
#include <type_traits>
#include <utility>

using std::array;
using std::enable_if;
using std::exception;
using std::forward;
using std::make_pair;
using std::pair;
using std::remove_reference;
using std::vector;

using parse_input = vector<uint8_t>;

namespace {

parse_input operator+(parse_input const& lhs, parse_input const& rhs) {
  parse_input ret = lhs;
  ret.insert(ret.end(), rhs.begin(), rhs.end());
  return ret;
}


////////////////////////////////////////////////////////////////////////////////
//  Packet builders
////////////////////////////////////////////////////////////////////////////////

template <typename Container, typename Val>
inline void append_little_endian_bytes(Container* c, Val const& v) {
  for (auto i = 0u; i < sizeof(Val); ++i) {
    c->push_back(v >> (i * 8) & 0xff);
  }
}

template <typename Container, typename Val>
inline void append_big_endian_bytes(Container* c, Val const& v) {
  for (auto i = 0u; i < sizeof(Val); ++i) {
    c->push_back(v >> (sizeof(Val) - 1 - i) * 8 & 0xff);
  }
}

inline parse_input sync_packet() {
  parse_input ret;
  append_little_endian_bytes(&ret, 0x55aaffff);
  return ret;
}

inline parse_input error_packet(uint32_t error) {
  parse_input ret;
  ret.push_back(0xd << 4);
  append_little_endian_bytes(&ret, error);
  return ret;
}

inline parse_input battery_packet(uint16_t pct, uint16_t fuel_mv,
                                  uint16_t adc_mv, int16_t temp_c) {
  parse_input ret;
  ret.push_back(0xb << 4);
  append_big_endian_bytes(&ret, pct);
  append_big_endian_bytes(&ret, fuel_mv);
  append_big_endian_bytes(&ret, adc_mv);
  append_big_endian_bytes(&ret, temp_c);
  return ret;
}

inline parse_input bitpacked_samples(uint16_t ch1, uint16_t ch2) {
  parse_input ret;
  ret.push_back(ch1);
  ret.push_back(ch1 >> 8 | ch2 << 2);
  ret.push_back(ch2 >> 6);
  return ret;
}

inline parse_input bitpacked_samples(uint16_t ch1, uint16_t ch2, uint16_t ch3) {
  parse_input ret;
  ret.push_back(ch1);
  ret.push_back(ch1 >> 8 | ch2 << 2);
  ret.push_back(ch2 >> 6 | ch3 << 4);
  ret.push_back(ch3 >> 4);
  return ret;
}

inline parse_input bitpacked_samples(uint16_t ch1, uint16_t ch2, uint16_t ch3,
                                     uint16_t ch4) {
  parse_input ret;
  ret.push_back(ch1);
  ret.push_back(ch1 >> 8 | ch2 << 2);
  ret.push_back(ch2 >> 6 | ch3 << 4);
  ret.push_back(ch3 >> 4 | ch4 << 6);
  ret.push_back(ch4 >> 2);
  return ret;
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 3>::type* = nullptr>
inline parse_input acc_packet(uint16_t dropped, Args&&... args) {
  parse_input ret;
  ret.push_back(0xa << 4 | 1 << 3);
  append_big_endian_bytes(&ret, dropped);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 3>::type* = nullptr>
inline parse_input acc_packet(Args&&... args) {
  uint8_t b = 0xa << 4;
  parse_input ret;
  ret.push_back(b);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 4>::type* = nullptr>
inline parse_input eeg_packet(uint16_t dropped, Args&&... args) {
  parse_input ret;
  ret.push_back(0xe << 4 | 1 << 3);
  append_big_endian_bytes(&ret, dropped);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

template <typename... Args,
          typename enable_if<sizeof...(Args) == 4>::type* = nullptr>
inline parse_input eeg_packet(Args&&... args) {
  uint8_t b = 0xe << 4;
  parse_input ret;
  ret.push_back(b);
  return ret + bitpacked_samples(forward<Args>(args)...);
}

inline parse_input drlref_packet(uint16_t drl, uint16_t ref) {
  parse_input ret;
  ret.push_back(0x9 << 4);
  return ret + bitpacked_samples(drl, ref);
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
    else if (type == IX_PAC_DRLREF) {
      drl = ix_packet_drl(p);
      ref = ix_packet_ref(p);
    }
    else if (type == IX_PAC_BATTERY) {
      battery_pct = ix_packet_battery_percent(p);
      fuel_mv = ix_packet_battery_fuel_gauge_mv(p);
      adc_mv = ix_packet_battery_adc_mv(p);
      temp_c = ix_packet_battery_temp_c(p);
    }
  }

  ix_pac_type type;
  uint16_t dropped_samples;
  vector<uint16_t> samples;
  uint32_t error;
  uint16_t drl;
  uint16_t ref;
  uint16_t battery_pct;
  uint16_t fuel_mv;
  uint16_t adc_mv;
  int16_t temp_c;
};

struct PacketParseError : ::exception {};

pair<uint32_t, vector<IxPacket>> test_parse(parse_input const& buf) {
  auto pacs = vector<IxPacket>();
  ix_packet_fn pac_f = [](const ix_packet* p, void* user_data) {
    auto pacs = static_cast<vector<IxPacket>*>(user_data);
    pacs->push_back(IxPacket(p));
  };
  auto r = ix_packet_parse(buf.data(), buf.size(), pac_f, &pacs);
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

  parse_input buf;
  decltype(test_parse(buf)) r;
  decltype(r.second) v;
  remove_reference<decltype(v[0])>::type p;
};

////////////////////////////////////////////////////////////////////////////////
//  Test suite proper
////////////////////////////////////////////////////////////////////////////////

TEST_F(PacketTest, ParseFailures) {
  parse_input garbage;
  for (auto c : "hello world\n") {
      garbage.push_back(c);
  }
  EXPECT_ANY_THROW(test_parse(garbage));
  EXPECT_ANY_THROW(test_parse(parse_input()));
}

// Disabled: too slow
TEST(PacketDeathTest, DISABLED_SegfaultOnNullBuf) {
  ix_packet_fn nil_f = [](const ix_packet*, void*) {};
  EXPECT_DEATH(ix_packet_parse(NULL, 1, nil_f, NULL), "");
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
                      + sync_packet()
                      + drlref_packet(50, 60)
                      + battery_packet(1, 2, 3, -1)
                      + error_packet(0xabad1dea);
  auto pacs = vector<IxPacket>();
  auto begin = buf.cbegin();
  auto end = buf.cend();
  while (begin != end) {
    auto r = test_parse(parse_input(begin, end));
    auto v = r.second;
    pacs.insert(pacs.end(), v.begin(), v.end());
    begin += r.first;
  }
  EXPECT_EQ(end, begin);

  ASSERT_EQ(9u, pacs.size());

  EXPECT_EQ(IX_PAC_SYNC, pacs[0].type);
  EXPECT_EQ(IX_PAC_EEG, pacs[2].type);
  EXPECT_EQ(53u, pacs[2].samples[2]);
  EXPECT_EQ(IX_PAC_ACCELEROMETER, pacs[3].type);
  EXPECT_EQ(737u, pacs[3].samples[0]);
  EXPECT_EQ(1022u, pacs[4].samples[1]);
  EXPECT_EQ(IX_PAC_SYNC, pacs[5].type);
  EXPECT_EQ(IX_PAC_BATTERY, pacs[7].type);
  EXPECT_EQ(-1, pacs[7].temp_c);
  EXPECT_EQ(IX_PAC_ERROR, pacs[8].type);
  EXPECT_EQ(0xabad1dea, pacs[8].error);
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

  buf = eeg_packet(1u, 2u, 3u, 4u);
  parse();
  EXPECT_EQ(0u, p.dropped_samples);

  buf = acc_packet(1u, 2u, 3u);
  EXPECT_EQ(0u, p.dropped_samples);
}

TEST_F(PacketTest, ParsesDrlRef) {
  buf = drlref_packet(3, 2);
  parse();
  ASSERT_EQ(IX_PAC_DRLREF, p.type);
  EXPECT_EQ(3, p.drl);
  EXPECT_EQ(2, p.ref);

  buf = drlref_packet(1021, 1023);
  parse();
  ASSERT_EQ(IX_PAC_DRLREF, p.type);
  EXPECT_EQ(1021u, p.drl);
  EXPECT_EQ(1023u, p.ref);
}

TEST_F(PacketTest, ParsesBattery) {
  buf = battery_packet(8731, 1337, 1234, 451);
  parse();
  ASSERT_EQ(IX_PAC_BATTERY, p.type);
  EXPECT_EQ(8731u, p.battery_pct);
  EXPECT_EQ(1337u, p.fuel_mv);
  EXPECT_EQ(1234u, p.adc_mv);
  EXPECT_EQ(451, p.temp_c);
  buf = battery_packet(1, 2, 3, -270);
  parse();
  ASSERT_EQ(IX_PAC_BATTERY, p.type);
  EXPECT_EQ(1u, p.battery_pct);
  EXPECT_EQ(2u, p.fuel_mv);
  EXPECT_EQ(3u, p.adc_mv);
  EXPECT_EQ(-270, p.temp_c);
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

// TODO(soon): compressed eeg
// TODO(soon): NEED MORE vs BAD STR

}  // namespace
