UNAME=$(shell uname)
ifeq ($(UNAME),Darwin)
  OS=OSX
else ifeq ($(UNAME),Linux)
  OS=LINUX
else ifeq ($(UNAME),Windows)
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
  SO_EXT=dll
endif

INCS = -Iinclude -I$(GTEST_SRC)/include
LIBS =
TESTLIBS = -L. -lgtest -lgtest_main -lmuse_core
OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror
CFLAGS = $(INCS) $(OFLAGS) $(WFLAGS) -fPIC -std=c99 -pedantic
CXXFLAGS = $(INCS) $(OFLAGS) $(WFLAGS) -std=c++1y
TESTFLAGS = $(CXXFLAGS) $(TESTOSFLAGS)
LDFLAGS = $(LDOSFLAGS) $(OFLAGS) $(LIBS)
LDTESTFLAGS = $(LDFLAGS) $(TESTLIBS)

SRCOBJS = src/util.o \
          src/version.o

TESTOBJS = test/version_test.o

ALLOBJS = $(SRCOBJS) $(TESTOBJS)

LIB = libmuse_core.$(SO_EXT)


all: $(LIB) test

test: unittests
	./unittests

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

unittests: $(LIB) $(TESTOBJS) libgtest.a libgtest_main.a
	$(CXXLD) -o unittests $(LDTESTFLAGS) $(TESTOBJS)

clean:
	rm -f $(ALLOBJS) $(LIB) unittests

distclean: clean
	rm -f libgtest.a libgtest_main.a

.PHONY: all clean distclean test
