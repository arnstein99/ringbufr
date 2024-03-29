# Tuning
ifeq ($(strip $(OPT)),)
    CCFLAGS += -g
    LDFLAGS += -g
else
    CCFLAGS += $(OPT)
    LDFLAGS += $(OPT)
endif
ifneq ($(strip $(NDEBUG)),)
    CPPFLAGS += -DNDEBUG=$(NDEBUG)
endif
ifneq ($(strip $(VERBOSE)),)
    CPPFLAGS += -DVERBOSE=$(VERBOSE)
endif
ifeq ($(strip $(PUSH_PAD)),)
    CPPFLAGS += -DPUSH_PAD=16*1024
else
    CPPFLAGS += -DPUSH_PAD=$(PUSH_PAD)
endif
ifeq ($(strip $(POP_PAD)),)
    CPPFLAGS += -DPOP_PAD=16*1024
else
    CPPFLAGS += -DPOP_PAD=$(POP_PAD)
endif

CCFLAGS += -std=c++2a -Wall
LDLIBS += -lpthread
LINK.o = c++ $(LDFLAGS)
COMPILE.cc = c++ -c $(CPPFLAGS) $(CCFLAGS)

all: testring tcpcat tcppipe

testring: testring.o miscutils.o
tcpcat: tcpcat.o copyfd.o miscutils.o netutils.o
tcppipe: tcppipe.o copyfd.o miscutils.o netutils.o

copyfd.o: ringbufr.h ringbufr.tcc
testring.o: ringbufr.h ringbufr.tcc
tcpcat.o: copyfd.h miscutils.h netutils.h
tcppipe.o: copyfd.h miscutils.h netutils.h
netutils.o: netutils.h miscutils.h ringbufr.h
miscutils.o: miscutils.h
