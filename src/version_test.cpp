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
  EXPECT_EQ(garbage.size(), ix_version_find_start(gmv.c_str(), gmv.size()));
}

} // namespace
