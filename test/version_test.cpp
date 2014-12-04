extern "C" {
#include "all.h"
}

#include <gtest/gtest.h>
#include <string>

using std::string;


namespace {

class VersionTest : public ::testing::Test {
protected:
  ix_muse_version version;
};

TEST_F(VersionTest, FindNoneIfNone) {
  EXPECT_EQ(-1, ix_version_find_start("", 0));
  EXPECT_EQ(-1, ix_version_find_start("tsst 1234", strlen("tsst 1234")));
}

TEST_F(VersionTest, FindStartOfMinVer) {
  auto min_ver = string{MUSE_MINVER};
  EXPECT_EQ(0, ix_version_find_start(min_ver.c_str(), min_ver.size()));
  auto garbage = string{"blargh 1234 \x05\x01\x07"};
  auto gmv = garbage + min_ver;
  EXPECT_EQ(static_cast<ssize_t>(garbage.size()),
            ix_version_find_start(gmv.c_str(), gmv.size()));
}

TEST_F(VersionTest, VersionParseNeedMorePrefix) {
  EXPECT_EQ(IX_VP_NEED_MORE, ix_version_parse("MUSE ", 5, &version).err);
  EXPECT_EQ(IX_VP_NEED_MORE, ix_version_parse("MUSE A", 6, &version).err);
  EXPECT_EQ(IX_VP_NEED_MORE, ix_version_parse("MUSE AP", 7, &version).err);
  EXPECT_EQ(IX_VP_NEED_MORE, ix_version_parse("MUSE APP", 8, &version).err);
  EXPECT_EQ(IX_VP_NEED_MORE, ix_version_parse("MUSE APP ", 9, &version).err);
  EXPECT_EQ(IX_VP_NEED_MORE,
            ix_version_parse("MUSE APP HW-0", 13, &version).err);
}

TEST_F(VersionTest, VersionParseMinimal) {
  auto min_ver = string{MUSE_MINVER "\n"};
  ASSERT_LE(0, ix_version_parse(min_ver.c_str(), min_ver.size(), &version).end);
  EXPECT_EQ(0, version.hw_version.x);
  EXPECT_EQ(0, version.hw_version.y);
  EXPECT_EQ(0, version.fw_version.x);
  EXPECT_EQ(0, version.fw_version.y);
  EXPECT_EQ(0, version.fw_version.z);
  EXPECT_EQ(0, version.build_number);
  EXPECT_EQ(0, version.target_hw_version.x);
  EXPECT_EQ(0, version.target_hw_version.y);
  EXPECT_EQ(IX_FW_UNKNOWN, version.fw_type);
}

} // namespace
