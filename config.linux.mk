A = a
S = so
CFLAGS += -fvisibility=hidden
CFLAGS_S += -fPIC
CXXFLAGS += -pthread -Wno-error=missing-field-initializers
CXXLDFLAGS += -Wl,--enable-new-dtags
