// Copyright 2014 Interaxon, Inc.

extern "C" {
#include "all.h"
}

#include <gtest/gtest.h>
#include <exception>
#include <string>

namespace {

// TODO(someday): move me
namespace version {
  struct Failure : ::std::exception {};

  struct NeedMore : Failure {};
  struct BadStr : Failure {};
  struct BadVer : Failure {};

  ::std::ptrdiff_t findStart(::std::string const& s) {
    return ix_version_find_start(s.c_str(), s.size());
  }

  ix_muse_version parse(::std::string const& s) {
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
}  // namespace version

TEST(VersionTest, FindNoneIfNone) {
  EXPECT_EQ(-1, version::findStart(""));
  EXPECT_EQ(-1, version::findStart("tsst 1234"));
}

TEST(VersionTest, FindStartOfMinVer) {
  auto min_ver = std::string{MUSE_MINVER};
  EXPECT_EQ(0, version::findStart(min_ver));
  auto garbage = std::string{"blargh 1234 \x05\x01\x07"};
  auto gmv = garbage + min_ver;
  EXPECT_EQ(static_cast<std::ptrdiff_t>(garbage.size()),
            version::findStart(gmv));
  EXPECT_EQ(2, version::findStart("  MU"));
  auto buf = std::string(50, '\0');
  buf.append(min_ver);
  EXPECT_EQ(50, version::findStart(buf));
}

TEST(VersionTest, ParseNeedMorePrefix) {
  auto s = std::string{MUSE_MINVER};
  // MUSE_MINVER ends in a number, so we'd need to see one more char to know we
  // had the whole thing
  EXPECT_THROW(version::parse(s), version::NeedMore);
  while (s.size()) {
    s.pop_back();
    EXPECT_THROW(version::parse(s), version::NeedMore) << '"' << s << '"';
  }
}

TEST(VersionTest, ParseBadStrings) {
  EXPECT_THROW(version::parse("MUBAD"), version::BadStr);
  // TODO(soon): malicious version strings, near misses, invalid types
}

TEST(VersionTest, ParseMinimal) {
  auto ver = version::parse(MUSE_MINVER "\n");
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

// TODO(soon): at least consumer, research + app, boot, test + a few numbers
TEST(VersionTest, DISABLED_ParseFields) {}

// TODO(soon): expect IX_PV_BAD_VER
TEST(VersionTest, DISABLED_ParseRejectsOtherProtoVersions) {}

}  // namespace
