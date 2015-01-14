default: all
-include config.mk

UNAME = $(shell uname)
ifeq (Linux,$(UNAME))
  OS=linux
else ifeq (Darwin,$(UNAME))
  OS=osx
else
  ifneq (,$(findstring Windows,$(PATH)))
    OS=win
  else
    $(error unknown os)
  endif
endif

-include mk/config.$(OS).mk

SRCDIR = src
BUILDDIR = build
BUILDLIBDIR = $(BUILDDIR)/lib
BUILDINCDIR = $(BUILDDIR)/include
BUILDDIR_S = $(BUILDDIR)/s
BUILDDIR_A = $(BUILDDIR)/a

DESTDIR = dist
LIBDIR = $(DESTDIR)$(PREFIX)/lib
INCDIR = $(DESTDIR)$(PREFIX)/include

INCS = $(_I)$(BUILDINCDIR)
LIBS = $(_L)$(BUILDLIBDIR) -lhammer

CFLAGS += $(INCS)
CXXFLAGS += $(INCS)
LDFLAGS += $(LIBS)
CXXLDFLAGS += $(LIBS)

MUSE_CORE_MOD = packet

MUSE_CORE_INC = defs muse_core packet

MUSE_CORE_A_O = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_A)/src/$(mod).o)
MUSE_CORE_H = $(foreach inc,$(MUSE_CORE_INC),$(BUILDINCDIR)/muse_core/$(inc).h)
MUSE_CORE_S_O = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_S)/src/$(mod).o)

MUSE_CORE_A = $(BUILDLIBDIR)/$(LIB)muse_core.$A
MUSE_CORE_S = $(BUILDLIBDIR)/$(LIB)muse_core.$S

all: options deps copy-headers lib

options:
	@echo build options:
	@echo "CC                 = $(CC)"
	@echo "LD                 = $(LD)"
	@echo "CFLAGS             = $(CFLAGS)"
	@echo "CFLAGS_S           = $(CFLAGS_S)"
	@echo "LDFLAGS            = $(LDFLAGS)"
	@echo "CXX                = $(CXX)"
	@echo "CXXFLAGS           = $(CXXFLAGS)"
	@echo "CXXLD              = $(CXXLD)"
	@echo "CXXLDFLAGS         = $(CXXLDFLAGS)"
	@echo "USE_BUNDLED_HAMMER = $(USE_BUNDLED_HAMMER)"
	@echo "SKIP_TESTS         = $(SKIP_TESTS)"
	@echo

DISTDIRS = $(INCDIR)/muse_core $(LIBDIR)

$(DISTDIRS):
	@echo mkdir $@
	@mkdir -p $@

lib: $(MUSE_CORE_S) $(MUSE_CORE_A)

copy-headers: $(MUSE_CORE_H)

$(MUSE_CORE_S): $(MUSE_CORE_S_O)
	@echo ld $(MUSE_CORE_S)
	@$(LD) -shared -o $(MUSE_CORE_S) $(CFLAGS_S) $(CFLAGS) $(MUSE_CORE_S_O) \
	  $(LDFLAGS)

$(MUSE_CORE_A): $(MUSE_CORE_A_O)
	@echo ar $(MUSE_CORE_A)
	@ar rcs $(MUSE_CORE_A) $(MUSE_CORE_A_O)

$(BUILDINCDIR)/muse_core/%.h: $(SRCDIR)/%.h $(BUILDINCDIR)/muse_core
	@echo copying $@
	@cp $< $@

# TODO(soon): autogenned header dependencies

$(BUILDDIR_S)/src/%.o: $(SRCDIR)/%.c $(MUSE_CORE_H)
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS_S) $(CFLAGS) $<

$(BUILDDIR_A)/src/%.o: $(SRCDIR)/%.c $(MUSE_CORE_H)
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS) $<

clean:
	@echo cleaning
	@rm -f benchmark unittests $(MUSE_CORE_A) $(MUSE_CORE_S) \
	@  $(MUSE_CORE_A_O) $(MUSE_CORE_S_O) $(MUSE_CORE_H)

DIST = \
  $(foreach inc,$(MUSE_CORE_INC),$(INCDIR)/muse_core/$(inc).h) \
  $(LIBDIR)/$(LIB)muse_core.$A \
  $(LIBDIR)/$(LIB)muse_core.$S

dist: $(DISTDIRS) $(DIST)

$(INCDIR)/muse_core/%.h: $(BUILDINCDIR)/muse_core/%.h
	@echo copying $@
	@cp $< $@

$(LIBDIR)/%: $(BUILDLIBDIR)/%
	@echo copying $@
	@cp $< $@

distclean:
	@echo rm $(DIST)
	@rm -f $(DIST)
	@echo rmdir $(DISTDIRS)
	@-rmdir -p $(DISTDIRS) >/dev/null 2>&1

install: copy-headers lib
	make dist DESTDIR=

uninstall:
	make distclean DESTDIR=

.PHONY: all clean copy-headers default deps dist distclean install lib \
        mark options uninstall


BENCHMARK_A_O = $(BUILDDIR_A)/test/packet_benchmark.o

mark: benchmark
	./benchmark

benchmark: $(BENCHMARK_A_O) $(MUSE_CORE_S) $(HAMMER_A)
	@echo c++ld benchmark
	@$(CXXLD) -o benchmark \
	  -Wl,-rpath,$(LIBDIR):$(BUILDLIBDIR) $(CXXLDFLAGS) $(CXXFLAGS) \
	  $(BENCHMARK_A_O) -lmuse_core -lhammer

# Not included in all -- too slow.
#all: mark


################################################################################
######  Dependency (for unittests): gtest                                 ######
################################################################################
ifeq (,$(SKIP_TESTS))

GTEST_SRC = 3rdparty/gtest
CXXFLAGS += -I$(GTEST_SRC) -I$(GTEST_SRC)/include
GTEST_A = $(BUILDLIBDIR)/$(LIB)gtest.$A
GTEST_MAIN_A = $(BUILDLIBDIR)/$(LIB)gtest_main.$A

all: test

.PHONY: test

test: unittests
	@echo unittests
	@./unittests

UNITTEST_MOD = muse_core_test packet_test
UNITTEST_A_O = $(foreach mod,$(UNITTEST_MOD),$(BUILDDIR_A)/test/$(mod).o)

$(UNITTEST_A_O): $(MUSE_CORE_H)

$(BUILDDIR_A)/test/%.o: test/%.cpp
	@echo c++ $@
	@$(CXX) -c -o $@ $(CXXFLAGS) $<

unittests: $(MUSE_CORE_S) $(MUSE_CORE_H) $(UNITTEST_A_O) $(GTEST_A) \
           $(GTEST_MAIN_A) $(HAMMER_A)
	@echo c++ld $@
	@$(CXXLD) -o unittests \
	  -Wl,-rpath,$(LIBDIR):$(BUILDLIBDIR) $(CXXLDFLAGS) $(CXXFLAGS) \
	  $(UNITTEST_A_O) -lmuse_core -lgtest -lgtest_main

$(GTEST_A): $(GTEST_SRC)/src/gtest-all.cc
	@echo c++ $@
	@$(CXX) -static -c -o $(GTEST_A) $(CXXFLAGS) $(GTEST_SRC)/src/gtest-all.cc

$(GTEST_MAIN_A): $(GTEST_SRC)/src/gtest_main.cc
	@echo c++ $@
	@$(CXX) -static -c -o $(GTEST_MAIN_A) $(CXXFLAGS) \
	  $(GTEST_SRC)/src/gtest_main.cc

endif

################################################################################
###### Dependency: hammer                                                 ######
################################################################################
ifneq (,$(USE_BUNDLED_HAMMER))

HAMMER_A = $(BUILDLIBDIR)/$(LIB)hammer.$A
HAMMER_H = $(BUILDINCDIR)/hammer/glue.h $(BUILDINCDIR)/hammer/hammer.h

deps: $(HAMMER_A) $(HAMMER_H)

$(HAMMER_A) $(HAMMER_H):
	@echo building hammer
	@scons -C 3rdparty/hammer >/dev/null 2>&1
	@echo installing hammer to $(BUILDDIR)
	@scons -C 3rdparty/hammer install prefix=../../$(BUILDDIR) >/dev/null 2>&1

endif
