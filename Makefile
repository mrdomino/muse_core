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

-include config.$(OS).mk

SRCDIR = src
BUILDDIR = build
BUILDLIBDIR = $(BUILDDIR)/lib
BUILDINCDIR = $(BUILDDIR)/include
BUILDDIR_S = $(BUILDDIR)/s
BUILDDIR_A = $(BUILDDIR)/a

DESTDIR = dist
LIBDIR = $(DESTDIR)$(PREFIX)/lib
INCDIR = $(DESTDIR)$(PREFIX)/include

INCS = -I$(INCDIR) -I$(BUILDINCDIR)
LIBS = -L$(LIBDIR) -L$(BUILDLIBDIR) -lhammer

CFLAGS += $(INCS)
CXXFLAGS += $(INCS)
LDFLAGS += $(LIBS)
CXXLDFLAGS += $(LIBS)

MUSE_CORE_MOD = connect packet r result util version

MUSE_CORE_INC = connect defs muse_core packet result serial util version

MUSE_CORE_INCDIR = $(INCDIR)/muse_core

MUSE_CORE_A_O = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_A)/src/$(mod).o)
MUSE_CORE_H = $(foreach inc,$(MUSE_CORE_INC),$(MUSE_CORE_INCDIR)/$(inc).h)
MUSE_CORE_S_O = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_S)/src/$(mod).o)

LIBMUSE_CORE_S = $(LIBDIR)/libmuse_core.$S
LIBMUSE_CORE_A = $(LIBDIR)/libmuse_core.$A

all: options dirs deps copy-headers lib

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

DIRS = $(BUILDLIBDIR) $(BUILDINCDIR) $(BUILDDIR_S)/src $(BUILDDIR_A)/src \
       $(BUILDDIR_A)/test $(LIBDIR) $(MUSE_CORE_INCDIR)

dirs: $(DIRS)

$(DIRS):
	@echo mkdir $@
	@mkdir -p $@

lib: $(LIBMUSE_CORE_S) $(LIBMUSE_CORE_A)

copy-headers: $(MUSE_CORE_H)

$(LIBMUSE_CORE_S): $(MUSE_CORE_S_O)
	@echo ld $(LIBMUSE_CORE_S)
	@$(LD) -shared -o $(LIBMUSE_CORE_S) $(CFLAGS_S) $(CFLAGS) $(MUSE_CORE_S_O) \
	  $(LDFLAGS)

$(LIBMUSE_CORE_A): $(MUSE_CORE_A_O)
	@echo ar $(LIBMUSE_CORE_A)
	@ar rcs $(LIBMUSE_CORE_A) $(MUSE_CORE_A_O)

$(MUSE_CORE_INCDIR)/%.h: $(SRCDIR)/%.h
	@echo copying $@
	@cp $< $@

$(BUILDDIR_S)/src/%.o: $(SRCDIR)/%.c
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS_S) $(CFLAGS) $<

$(BUILDDIR_A)/src/%.o: $(SRCDIR)/%.c
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS) $<

clean:
	rm -rf $(BUILDDIR_A) $(BUILDDIR_S)
	rm -f unittests

distclean: clean
	git clean -xdf
	git submodule foreach git clean -xdf

.PHONY: all clean copy-headers default deps dirs distclean lib options


################################################################################
######  Dependency (for unittests): gtest                                 ######
################################################################################
ifeq (,$(SKIP_TESTS))

GTEST_SRC = 3rdparty/gtest
CXXFLAGS += -I$(GTEST_SRC) -I$(GTEST_SRC)/include
GTEST_A = $(BUILDLIBDIR)/libgtest.$A
GTEST_MAIN_A = $(BUILDLIBDIR)/libgtest_main.$A

all: test

test: unittests
	@echo unittests
	@./unittests

UNITTEST_MOD = connect_test muse_core_test packet_test version_test
UNITTEST_A_O = $(foreach mod,$(UNITTEST_MOD),$(BUILDDIR_A)/test/$(mod).o)

$(UNITTEST_A_O): $(MUSE_CORE_H)

$(BUILDDIR_A)/test/%.o: test/%.cpp
	@echo c++ $@
	@$(CXX) -c -o $@ $(CXXFLAGS) $<

unittests: $(LIBMUSE_CORE_S) $(MUSE_CORE_H) $(UNITTEST_A_O) $(GTEST_A) \
           $(GTEST_MAIN_A) $(HAMMER_A)
	@echo c++ld $@
	@$(CXXLD) -o unittests \
	  -Wl,-rpath,$(LIBDIR):$(BUILDLIBDIR) $(CXXLDFLAGS) $(CXXFLAGS) \
	  $(UNITTEST_A_O) -lmuse_core -lhammer -lgtest -lgtest_main

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

HAMMER_A = $(BUILDLIBDIR)/libhammer.$A
HAMMER_H = $(BUILDINCDIR)/hammer/glue.h $(BUILDINCDIR)/hammer/hammer.h

deps: $(HAMMER_A) $(HAMMER_H)

$(HAMMER_A) $(HAMMER_H):
	@echo building hammer
	@scons -C 3rdparty/hammer >/dev/null 2>&1
	@echo installing hammer to $(BUILDDIR)
	@scons -C 3rdparty/hammer install prefix=../../$(BUILDDIR) >/dev/null 2>&1

endif
