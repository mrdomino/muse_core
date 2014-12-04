CC = clang
LD = $(CC)
INCS = -Iinclude
LIBS =
CFLAGS = -O2 $(INCS) -Wall -Wextra -Werror -std=c99 -pedantic
LDFLAGS = -O2 $(LIBS)

OBJS = src/main.o \
       src/version.o

all: muse

$(OBJS): include/all.h \
         include/version.h

muse: $(OBJS)
	$(LD) $(LDFLAGS) -o muse $(OBJS)

clean:
	@rm -f $(OBJS) muse

.PHONY: all clean
