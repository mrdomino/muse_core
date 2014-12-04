CC = gcc
LD = $(CC)
CXX = g++
CXXLD = $(CXX)
GTEST_SRC = /usr/src/gtest

INCS = -Iinclude -I$(GTEST_SRC)/include
LIBS =
OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror
CFLAGS = $(INCS) $(OFLAGS) $(WFLAGS) -fPIC -std=c99 -pedantic
CXXFLAGS = $(INCS) $(OFLAGS) $(WFLAGS) -std=c++1y
LDFLAGS = $(OFLAGS) $(LIBS)

SRCOBJS = src/version.o

TESTOBJS = src/version_test.o

ALLOBJS = $(SRCOBJS) $(TESTOBJS)

LIB = libmusecore.so

all: $(LIB) test

test: unittests
	LD_LIBRARY_PATH=. ./unittests

$(ALLOBJS): include/all.h \
            include/version.h

$(LIB): $(SRCOBJS)
	$(CXX) -shared -o $(LIB) $(SRCOBJS)

libgtest.a:
	$(CXX) -static -c -o libgtest.a $(CXXFLAGS) -I$(GTEST_SRC) -pthread \
	  -Wno-missing-field-initializers $(GTEST_SRC)/src/gtest-all.cc

libgtest_main.a:
	$(CXX) -static -c -o libgtest_main.a $(CXXFLAGS) -I$(GTEST_SRC) -pthread \
	  $(GTEST_SRC)/src/gtest_main.cc

unittests: $(LIB) $(TESTOBJS) libgtest.a libgtest_main.a
	$(CXX) -o unittests $(TESTFLAGS) -L. -lmusecore -lgtest -lgtest_main $(TESTOBJS)

clean:
	rm -f $(ALLOBJS) $(LIB) unittests

.PHONY: all clean test
