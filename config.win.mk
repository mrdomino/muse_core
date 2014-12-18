CC = cl
LD = link
CXX = $(CC)
CXXLD = $(LD)

A = lib
S = dll
CFLAGS_S += /DBUILDING_MUSE_CORE_DLL

_L='/LIBPATH:'
_I='/I'
