#include <cmath>
#include <cstdint>

#include <type_traits>
#include <utility>
#include <vector>

using std::enable_if;
using std::forward;
using std::vector;

using parse_input = vector<uint8_t>;

parse_input operator+(parse_input const& lhs, parse_input const& rhs) {
  parse_input ret = lhs;
  ret.insert(ret.end(), rhs.begin(), rhs.end());
  return ret;
}

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

