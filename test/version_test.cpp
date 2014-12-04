extern "C" {
#include "all.h"
}

#include <gtest/gtest.h>

#include <exception>
#include <string>

namespace {

class ParseVersion {
public:
  struct Failure : ::std::exception {};

  struct NeedMore : Failure {};
  struct BadStr : Failure {};
  struct BadVer : Failure {};

  static ix_muse_version parse(std::string const& s) {
    ix_muse_version out;
    auto ret = ix_version_parse(s.c_str(), s.size(), &out);
    if (ret.end >= 0) {
      return out;
    }
    else switch (ret.err) {
    default: assert(false);
    case IX_VP_FAIL:
      throw Failure();
    case IX_VP_NEED_MORE:
      throw NeedMore();
    case IX_VP_BAD_STR:
      throw BadStr();
    case IX_VP_BAD_VER:
      throw BadVer();
    }
  }
};

class VersionTest : public ::testing::Test {
protected:
  ix_muse_version version;
};

TEST_F(VersionTest, FindNoneIfNone) {
  EXPECT_EQ(-1, ix_version_find_start("", 0));
  EXPECT_EQ(-1, ix_version_find_start("tsst 1234", strlen("tsst 1234")));
}

TEST_F(VersionTest, FindStartOfMinVer) {
  auto min_ver = std::string{MUSE_MINVER};
  EXPECT_EQ(0, ix_version_find_start(min_ver.c_str(), min_ver.size()));
  auto garbage = std::string{"blargh 1234 \x05\x01\x07"};
  auto gmv = garbage + min_ver;
  EXPECT_EQ(static_cast<ssize_t>(garbage.size()),
            ix_version_find_start(gmv.c_str(), gmv.size()));
  // TODO test strings containing '\0'
}

TEST_F(VersionTest, VersionParseNeedMorePrefix) {
  EXPECT_THROW(ParseVersion::parse("MUSE"), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE "), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE A"), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE AP"), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE APP"), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE APP "), ParseVersion::NeedMore);
  EXPECT_THROW(ParseVersion::parse("MUSE APP HW-0"), ParseVersion::NeedMore);
}

// TODO malicious version strings, near misses, invalid types
TEST_F(VersionTest, DISABLED_VersionParseBadStrings) {}

TEST_F(VersionTest, VersionParseMinimal) {
  auto ver = ParseVersion::parse(MUSE_MINVER "\n");
  EXPECT_EQ(IX_IMG_APP, ver.img_type);
  EXPECT_EQ(0, ver.hw_version.x);
  EXPECT_EQ(0, ver.hw_version.y);
  EXPECT_EQ(0, ver.fw_version.x);
  EXPECT_EQ(0, ver.fw_version.y);
  EXPECT_EQ(0, ver.fw_version.z);
  EXPECT_EQ(0, ver.build_number);
  EXPECT_EQ(0, ver.target_hw_version.x);
  EXPECT_EQ(0, ver.target_hw_version.y);
  EXPECT_EQ(IX_FW_UNKNOWN, ver.fw_type);
}

// TODO at least consumer, research + app, boot, test + a few numbers
TEST_F(VersionTest, DISABLED_VersionParseFields) {}

TEST_F(VersionTest, DISABLED_RejectsOtherProtoVersions) {
  // expect IX_PV_BAD_VER
}

} // namespace
