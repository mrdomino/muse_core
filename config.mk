CC = gcc
LD = $(CC)
CXX = clang++
#CXX = g++
CXXLD = $(CXX)
CXXLDFLAGS += -fsanitize=address -fsanitize=undefined

PREFIX = /usr/local

OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror -pedantic
CFLAGS += $(OFLAGS) $(WFLAGS) -std=c11
CXXFLAGS += $(OFLAGS) $(WFLAGS) -std=c++1y

# Uncomment this to use bundled 3rdparty/hammer (not recommended)
#USE_BUNDLED_HAMMER = yes
