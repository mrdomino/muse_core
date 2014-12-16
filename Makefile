default: all

A = a
S = so

SRCDIR = $(PWD)
BUILDDIR = $(PWD)/build
BUILDLIBDIR = $(BUILDDIR)/lib
BUILDINCDIR = $(BUILDDIR)/include
BUILDDIR_S = $(BUILDDIR)/s
BUILDDIR_A = $(BUILDDIR)/a

DESTDIR = $(PWD)/dist
LIBDIR = $(DESTDIR)$(PREFIX)/lib
INCDIR = $(DESTDIR)$(PREFIX)/include

INCS = -I$(INCDIR) -I$(BUILDINCDIR)
LIBS = -L$(LIBDIR) -L$(BUILDLIBDIR)

CFLAGS = $(INCS)

CFLAGS_S = -fPIC

MUSE_CORE_MOD = connect packet r result util version

MUSE_CORE_INC = connect defs muse_core packet result serial util version

MUSE_CORE_INCDIR = $(INCDIR)/muse_core

MUSE_CORE_A = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_A)/$(mod).o)
MUSE_CORE_H = $(foreach inc,$(MUSE_CORE_INC),$(MUSE_CORE_INCDIR)/$(inc).h)
MUSE_CORE_S = $(foreach mod,$(MUSE_CORE_MOD),$(BUILDDIR_S)/$(mod).o)

LIBMUSE_CORE_S = $(LIBDIR)/libmuse_core.$S
LIBMUSE_CORE_A = $(LIBDIR)/libmuse_core.$A

all: dirs deps copy-headers lib

DIRS = $(BUILDLIBDIR) $(BUILDINCDIR) $(BUILDDIR_S) $(BUILDDIR_A) $(LIBDIR) \
       $(MUSE_CORE_INCDIR)

dirs: $(DIRS)

$(DIRS):
	@echo mkdir $@
	@mkdir -p $@

lib: $(LIBMUSE_CORE_S) $(LIBMUSE_CORE_A)

copy-headers: $(MUSE_CORE_H)

$(LIBMUSE_CORE_S): $(MUSE_CORE_S)
	@echo $(LD) $(LIBMUSE_CORE_S)
	@$(LD) -shared -o $(LIBMUSE_CORE_S) $(CFLAGS_S) $(CFLAGS) $(MUSE_CORE_S)

$(LIBMUSE_CORE_A): $(MUSE_CORE_A)
	@echo ar $(LIBMUSE_CORE_A)
	@ar rcs $(LIBMUSE_CORE_A) $(MUSE_CORE_A)

$(MUSE_CORE_INCDIR)/%.h: src/%.h
	@echo copying $@
	@cp $< $@

$(BUILDDIR_S)/%.o: src/%.c
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS_S) $(CFLAGS) $<

$(BUILDDIR_A)/%.o: src/%.c
	@echo cc $@
	@$(CC) -c -o $@ $(CFLAGS) $<

HAMMER_A = $(BUILDLIBDIR)/libhammer.$A
HAMMER_H = $(BUILDINCDIR)/hammer/glue.h $(BUILDINCDIR)/hammer/hammer.h

deps: $(HAMMER_A) $(HAMMER_H)

$(HAMMER_A) $(HAMMER_H):
	@echo building hammer
	@scons -C 3rdparty/hammer
	@scons -C 3rdparty/hammer install prefix=$(BUILDDIR)

.PHONY: all copy-headers default deps dirs lib
