CC = gcc
LD = $(CC)
CXX = clang++
CXXLD = $(CXX)
CXXLDFLAGS += -fsanitize=address -fsanitize=undefined

PREFIX = /usr/local

OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror -pedantic
CFLAGS += $(OFLAGS) $(WFLAGS) -std=c11 -fvisibility=hidden
CXXFLAGS += $(OFLAGS) $(WFLAGS) -std=c++1y -Wno-error=missing-field-initializers
CFLAGS_S += -fPIC

# TODO(soon): use these
_L='-L'
_I='-I'
