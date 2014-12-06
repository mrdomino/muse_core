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

CC = gcc
LD = $(CC)
CXX = g++
CXXLD = $(CXX)
GTEST_SRC = 3rdparty/gtest

ifeq ($(OS),LINUX)
  EXPORT_COSFLAGS=-fPIC
  CXXTESTOSFLAGS=-pthread
  LDTESTOSFLAGS=-pthread -Wl,-rpath=.,--enable-new-dtags
  SO_EXT=so
endif
ifeq ($(OS),OSX)
  EXPORT_COSFLAGS=-fPIC
  SO_EXT=dylib
endif
ifeq ($(OS),WIN)
  EXPORT_COSFLAGS=-DBUILDING_DLL
  SO_EXT=dll
endif

INCS = -Iinclude -I$(GTEST_SRC)/include
LIBS =
TESTLIBS = -L. -lgtest -lgtest_main -lmuse_core
EXPORT_CFLAGS = $(EXPORT_COSFLAGS) -fvisibility=hidden
OFLAGS = -O0 -g
#OFLAGS = -O2 -DNDEBUG
WFLAGS = -Wall -Wextra -Werror -pedantic
CSTDFLAGS = -std=c99
CXXSTDFLAGS = -std=c++1y
BASE_CFLAGS = $(INCS) $(OFLAGS) $(WFLAGS)
# XXX Sticking EXPORT_CFLAGS here is a hack
CFLAGS = $(BASE_CFLAGS) $(EXPORT_CFLAGS) $(COSFLAGS) $(CSTDFLAGS)
CXXFLAGS = $(BASE_CFLAGS) $(CXXSTDFLAGS)
CXXTESTFLAGS = $(CXXFLAGS) $(CXXTESTOSFLAGS)
LDFLAGS = $(LDOSFLAGS) $(OFLAGS) $(LIBS)
LDTESTFLAGS = $(LDFLAGS) $(LDTESTOSFLAGS) $(TESTLIBS)

SRCOBJS = src/except.o \
          src/util.o \
          src/version.o

TESTOBJS = test/version_test.o

ALLOBJS = $(SRCOBJS) $(TESTOBJS)

LIB = libmuse_core.$(SO_EXT)


all: build stats test

build: $(LIB)

test: unittests
	./unittests

stats: $(LIB)
	size $(LIB)

$(ALLOBJS): include/all.h \
            include/version.h

$(LIB): $(SRCOBJS)
	$(LD) -shared -o $(LIB) $(LDFLAGS) $(SRCOBJS)

libgtest.a:
	$(CXX) -static -c -o libgtest.a $(CXXTESTFLAGS) -I$(GTEST_SRC) \
	  -Wno-missing-field-initializers $(GTEST_SRC)/src/gtest-all.cc

libgtest_main.a:
	$(CXX) -static -c -o libgtest_main.a $(CXXTESTFLAGS) -I$(GTEST_SRC) \
	  $(GTEST_SRC)/src/gtest_main.cc

unittests: $(LIB) $(TESTOBJS) libgtest.a libgtest_main.a $(LIB)
	$(CXXLD) -o unittests $(TESTOBJS) $(LDTESTFLAGS)

clean:
	rm -f $(ALLOBJS) $(LIB) unittests

distclean: clean
	rm -f libgtest.a libgtest_main.a

.PHONY: all build clean default distclean test stats
