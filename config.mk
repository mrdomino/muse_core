CC = gcc
LD = $(CC)
CXX = g++
CXXLD = $(CXX)

PREFIX = /usr/local

OFLAGS = -O0 -g
WFLAGS = -Wall -Wextra -Werror -pedantic
CFLAGS += $(OFLAGS) $(WFLAGS) -std=c11
CXXFLAGS += $(OFLAGS) $(WFLAGS) -std=c++1y
