default: all test stats

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
  TESTOSFLAGS=-pthread
  LDOSFLAGS=-pthread -Wl,-rpath=.
  SO_EXT=so
endif
ifeq ($(OS),OSX)
  SO_EXT=dylib
endif
ifeq ($(OS),WIN)
  EXPORT_COSFLAGS=-DBUILDING_DLL
  SO_EXT=dll
endif

INCS = -Iinclude -I$(GTEST_SRC)/include
LIBS =
TESTLIBS = -L. -lgtest -lgtest_main -lmuse_core
EXPORT_CFLAGS = $(EXPORT_COSFLAGS) -fvisibility=hidden -fPIC
OFLAGS = -O0 -g
#OFLAGS = -O2 -DNDEBUG
WFLAGS = -Wall -Wextra -Werror -pedantic
CSTDFLAGS = -std=c99
CXXSTDFLAGS = -std=c++1y
BASE_CFLAGS = $(INCS) $(OFLAGS) $(WFLAGS)
# XXX Sticking EXPORT_CFLAGS here is a hack
CFLAGS = $(BASE_CFLAGS) $(EXPORT_CFLAGS) $(COSFLAGS) $(CSTDFLAGS)
CXXFLAGS = $(BASE_CFLAGS) $(CXXSTDFLAGS)
TESTFLAGS = $(CXXFLAGS) $(TESTOSFLAGS)
LDFLAGS = $(LDOSFLAGS) $(OFLAGS) $(LIBS)
LDTESTFLAGS = $(LDFLAGS) $(TESTLIBS)

SRCOBJS = src/util.o \
          src/version.o

TESTOBJS = test/version_test.o

ALLOBJS = $(SRCOBJS) $(TESTOBJS)

LIB = libmuse_core.$(SO_EXT)


all: $(LIB)

test: unittests
	./unittests

stats: $(LIB)
	@echo -n "$(LIB): "
	@stat --format=%s $(LIB)

$(ALLOBJS): include/all.h \
            include/version.h

$(LIB): $(SRCOBJS)
	$(CC) -shared -o $(LIB) $(SRCOBJS)

libgtest.a:
	$(CXX) -static -c -o libgtest.a $(TESTFLAGS) -I$(GTEST_SRC) \
	  -Wno-missing-field-initializers $(GTEST_SRC)/src/gtest-all.cc

libgtest_main.a:
	$(CXX) -static -c -o libgtest_main.a $(TESTFLAGS) -I$(GTEST_SRC) \
	  $(GTEST_SRC)/src/gtest_main.cc

unittests: $(LIB) $(TESTOBJS) libgtest.a libgtest_main.a $(LIB)
	$(CXXLD) -o unittests $(TESTOBJS) $(LDTESTFLAGS)

clean:
	rm -f $(ALLOBJS) $(LIB) unittests

distclean: clean
	rm -f libgtest.a libgtest_main.a

.PHONY: all clean default distclean test stats
