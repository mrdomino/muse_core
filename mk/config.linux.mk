-include mk/posixish.mk
A = a
S = so

CXXFLAGS += -pthread
CXXLDFLAGS += -Wl,--enable-new-dtags
SANITIZEFLAGS += -fsanitize=address -fsanitize=undefined
