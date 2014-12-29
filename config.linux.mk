-include mk/posixish.mk
A = a
S = so

CXXFLAGS += -pthread
CXXLDFLAGS += -Wl,--enable-new-dtags
CXXLDFLAGS += -fsanitize=address -fsanitize=undefined
