default: all

UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  OS=OSX
else ifeq ($(UNAME),Linux)
  OS=LINUX
else ifneq (,$(filter CYGWIN% MINGW%,$(UNAME)))
  OS=WIN
else
  $(error unknown os)
endif

ifeq ($(OS),LINUX)
  EXPORT_COSFLAGS=-fPIC
  CXXTESTOSFLAGS=-pthread
  LDTESTOSFLAGS=-pthread -Wl,-rpath=.,--enable-new-dtags
  SO_EXT=so
  LIB_EXT=a
endif
ifeq ($(OS),OSX)
  EXPORT_COSFLAGS=-fPIC
  SO_EXT=dylib
  LIB_EXT=a
endif
ifeq ($(OS),WIN)
  EXPORT_COSFLAGS=-DBUILDING_DLL
  SO_EXT=dll
  LIB_EXT=lib
endif

CC = clang
LD = $(CC)
CXX = clang++
CXXLD = $(CXX)
GTEST_SRC = 3rdparty/gtest
HAMMER_SRC = 3rdparty/hammer
BUILD_DIR = $(PWD)/build

HAMMER_LIB = $(BUILD_DIR)/lib/libhammer.$(LIB_EXT)
GTEST_LIB = $(BUILD_DIR)/lib/libgtest.$(LIB_EXT)
GTEST_MAIN_LIB = $(BUILD_DIR)/lib/libgtest_main.$(LIB_EXT)

INCS = -Iinclude -I$(BUILD_DIR)/include -I$(GTEST_SRC)/include
# libs that should be linked against to get muse_core
LIBS = -L. -lmuse_core $(HAMMER_LIB)
TESTLIBS = $(GTEST_LIB) $(GTEST_MAIN_LIB)
EXPORT_CFLAGS = $(EXPORT_COSFLAGS) -fvisibility=hidden
OFLAGS = -O0 -g
#OFLAGS = -O2 -DNDEBUG
WFLAGS = -Wall -Wextra -Werror -pedantic
CSTDFLAGS = -std=c11
CXXSTDFLAGS = -std=c++1y
BASE_CFLAGS = $(INCS) $(OFLAGS) $(WFLAGS)
#
# XXX Sticking EXPORT_CFLAGS here is a hack
#
CFLAGS = $(BASE_CFLAGS) $(EXPORT_CFLAGS) $(COSFLAGS) $(CSTDFLAGS)
CXXFLAGS = $(BASE_CFLAGS) $(CXXSTDFLAGS)
CXXTESTFLAGS = $(CXXFLAGS) $(CXXTESTOSFLAGS)
LDFLAGS = $(LDOSFLAGS) $(OFLAGS)
LDTESTFLAGS = $(LDFLAGS) $(LDTESTOSFLAGS) -fsanitize=address -fsanitize=undefined

SRCOBJS = src/except.o \
          src/packet.o \
          src/util.o \
          src/version.o

TESTOBJS = test/packet_test.o \
           test/version_test.o

ALLOBJS = $(SRCOBJS) $(TESTOBJS)

LIB = libmuse_core.$(SO_EXT)


all: build stats test

build: $(LIB)

test: unittests
	./unittests

stats: $(LIB)
	size $(LIB)

$(ALLOBJS): include/defs.h \
            include/except.h \
            include/packet.h \
            include/util.h \
            include/version.h \
            build/include/hammer/hammer.h

$(LIB): $(SRCOBJS)
	$(LD) -shared -o $(LIB) $(CFLAGS) $(LDFLAGS) $(SRCOBJS)

$(GTEST_LIB):
	mkdir -p $$(dirname $(GTEST_LIB))
	$(CXX) -static -c -o $(GTEST_LIB) $(CXXTESTFLAGS) -I$(GTEST_SRC) \
	  -Wno-missing-field-initializers $(GTEST_SRC)/src/gtest-all.cc

$(GTEST_MAIN_LIB):
	mkdir -p $$(dirname $(GTEST_MAIN_LIB))
	$(CXX) -static -c -o $(GTEST_MAIN_LIB) $(CXXTESTFLAGS) -I$(GTEST_SRC) \
	  $(GTEST_SRC)/src/gtest_main.cc

unittests: $(LIB) $(TESTOBJS) $(GTEST_LIB) $(GTEST_MAIN_LIB)
	$(CXXLD) -o unittests $(LDTESTFLAGS) $(TESTOBJS) $(LIBS) $(TESTLIBS)

$(HAMMER_LIB) build/include/hammer/hammer.h:
	env CC=$(CC) scons -C $(HAMMER_SRC)
	env CC=$(CC) scons -C $(HAMMER_SRC) install prefix=$(BUILD_DIR)

clean:
	rm -f $(ALLOBJS) $(LIB) unittests

distclean: clean
	rm -rf $(BUILD_DIR)

.PHONY: all build clean default distclean test stats
