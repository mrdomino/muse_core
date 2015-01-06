#include <cstddef>
#include <cstdint>

extern "C" {
#include <muse_core/defs.h>
#include <muse_core/result.h>
#include <muse_core/packet.h>
}

#include <exception>
#include <gtest/gtest.h>
#include <type_traits>
#include <utility>
#include <vector>

#include "packet_builders.h"

using std::exception;
using std::make_pair;
using std::pair;
using std::remove_reference;
using std::vector;

namespace {

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
  if (r > 0) {
    return make_pair(r, pacs);
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

class EstLenTest : public ::testing::Test {
protected:
  size_t est_len_buf() { return ix_packet_est_len(buf.data(), buf.size()); }
  parse_input buf;
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

TEST_F(EstLenTest, EmptyString) {
  EXPECT_EQ(4u, ix_packet_est_len(buf.data(), 0));
}

TEST_F(EstLenTest, Null) {
  EXPECT_EQ(4u, ix_packet_est_len(NULL, 0));
  //EXPECT_DEATH(ix_packet_est_len(NULL, 1), "");
}

TEST_F(EstLenTest, InvalidTypeIsZero) {
  buf.push_back(0);
  EXPECT_EQ(0u, est_len_buf());
}

TEST_F(EstLenTest, ValidFullPackets) {
  auto inputs = vector<parse_input>();
  inputs.push_back(eeg_packet(1u, 2u, 3u, 4u));
  inputs.push_back(eeg_packet(1u, 2u, 3u, 4u, 5u));
  inputs.push_back(acc_packet(1u, 2u, 3u));
  inputs.push_back(acc_packet(1u, 2u, 3u, 4u));
  inputs.push_back(drlref_packet(1u, 2u));
  inputs.push_back(error_packet(123));
  inputs.push_back(sync_packet());
  inputs.push_back(battery_packet(1, 2, 3, 4));
  for (auto input : inputs) {
    EXPECT_EQ(input.size(), ix_packet_est_len(input.data(), input.size()));
  }
}

TEST_F(EstLenTest, EegPrefix) {
  buf.push_back(0xe << 4);
  EXPECT_EQ(6u, est_len_buf());
  buf.push_back(0);
  EXPECT_EQ(6u, est_len_buf());

  buf.clear();
  buf.push_back(0xe << 4 | 1 << 3);
  EXPECT_EQ(8u, est_len_buf());
  buf.push_back(1);
  buf.push_back(2);
  EXPECT_EQ(8u, est_len_buf());
}

TEST_F(EstLenTest, AccPrefix) {
  buf.push_back(0xa << 4);
  EXPECT_EQ(5u, est_len_buf());
  buf.push_back(1);
  EXPECT_EQ(5u, est_len_buf());

  buf.clear();
  buf.push_back(0xa << 4 | 1 << 3);
  EXPECT_EQ(7u, est_len_buf());
}

TEST_F(EstLenTest, OtherPrefixes) {
  buf.push_back(0xb << 4);
  EXPECT_EQ(9u, est_len_buf());

  buf.clear();
  buf.push_back(0x9 << 4);
  EXPECT_EQ(4u, est_len_buf());

  buf.clear();
  buf.push_back(0xd << 4);
  EXPECT_EQ(5u, est_len_buf());

  buf.clear();
  buf.push_back(0xff);
  EXPECT_EQ(4u, est_len_buf());
}

// TODO(soon): est_len compressed EEG + length + IX_PAC_MAXSIZE

}  // namespace
