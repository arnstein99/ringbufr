ifneq ($(strip $(VERBOSE)),)
    CPPFLAGS += -DVERBOSE=$(VERBOSE)
endif
CCFLAGS += -std=c++2a -g -Wall
LDLIBS += -lpthread
LINK.o = c++ $(LDFLAGS)
COMPILE.cc = c++ -c $(CCFLAGS) $(CPPFLAGS)

normal: testring tcpcat tcppipe
obsolete: ringcat tcpget tcpput tcprelay tcppull tcppush
all: normal obsolete

testring: testring.o
ringcat: ringcat.o copyfd.o miscutils.o
tcpget: tcpget.o copyfd.o miscutils.o netutils.o
tcpput: tcpput.o copyfd.o miscutils.o netutils.o
tcppull: tcppull.o copyfd.o miscutils.o netutils.o
tcppush: tcppush.o copyfd.o miscutils.o netutils.o
tcpcat: tcpcat.o copyfd.o miscutils.o netutils.o
tcppipe: tcppipe.o copyfd.o miscutils.o netutils.o
tcprelay: tcprelay.o copyfd.o miscutils.o netutils.o

copyfd.o: ringbufr.h ringbufr.tcc
ringcat.o: copyfd.h
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
tcpget.o: copyfd.h miscutils.h netutils.h
tcpput.o: copyfd.h miscutils.h netutils.h
tcppull.o: copyfd.h miscutils.h netutils.h
tcppush.o: copyfd.h miscutils.h netutils.h
tcpcat.o: copyfd.h miscutils.h netutils.h
tcppipe.o: copyfd.h miscutils.h netutils.h
tcprelay.o: copyfd.h miscutils.h netutils.h
netutils.o: netutils.h miscutils.h ringbufr.h
miscutils.o: miscutils.h
