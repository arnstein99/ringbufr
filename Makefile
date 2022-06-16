CCFLAGS += -std=c++20 -O -Wall
LINK.o = c++ $(LDFLAGS)
COMPILE.cc = c++ -c $(CCFLAGS)

all: testring ringcat tcpget tcpput tcprelay tcppull tcppush

testring: testring.o
ringcat: ringcat.o copyfd.o miscutils.o
tcpget: tcpget.o copyfd.o miscutils.o netutils.o
tcpput: tcpput.o copyfd.o miscutils.o netutils.o
tcppull: tcppull.o copyfd.o miscutils.o netutils.o
tcppush: tcppush.o copyfd.o miscutils.o netutils.o
tcprelay: tcprelay.o copyfd.o miscutils.o netutils.o

copyfd.o: ringbufr.h
ringcat.o: copyfd.h
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
tcpget.o: copyfd.h miscutils.h netutils.h
tcpput.o: copyfd.h miscutils.h netutils.h
tcppull.o: copyfd.h miscutils.h netutils.h
tcppush.o: copyfd.h miscutils.h netutils.h
tcprelay.o: copyfd.h miscutils.h netutils.h
netutils.o: netutils.h miscutils.h
miscutils.o: miscutils.h
