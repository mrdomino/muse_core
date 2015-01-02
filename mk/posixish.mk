CC = gcc
LD = $(CC)
CXX = clang++
CXXLD = $(CXX)

PREFIX = /usr/local

OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror -pedantic
CFLAGS += $(OFLAGS) $(WFLAGS) -std=c11 -fvisibility=hidden
CXXFLAGS += $(OFLAGS) $(WFLAGS) -std=c++11 -Wno-error=missing-field-initializers
CFLAGS_S += -fPIC

_L = -L
_I = -I
LIB = lib
