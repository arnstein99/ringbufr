CCFLAGS += -std=c++20 -O -Wall -pthread
LINK.o = c++ $(LDFLAGS)
COMPILE.cc = c++ -c $(CCFLAGS)

all: testring ringcat tcpget tcpput tcprelay tcppull

testring: testring.o
ringcat: ringcat.o copyfd.o miscutils.o
tcpget: tcpget.o copyfd.o miscutils.o
tcpput: tcpput.o copyfd.o miscutils.o
tcppull: tcppull.o copyfd.o miscutils.o
tcprelay: tcprelay.o copyfd.o miscutils.o

copyfd.o: ringbufr.h
ringcat.o: copyfd.h
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
tcpget.o: copyfd.h
tcpput.o: copyfd.h
tcppull.o: copyfd.h
tcprelay.o: copyfd.h
miscutils.o: miscutils.h
