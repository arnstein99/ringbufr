CCFLAGS += -std=c++20 -g -Wall -pthread
LDFLAGS += -lpthread
LINK.o = c++ $(LDFLAGS)
COMPILE.cc = c++ -c $(CCFLAGS)

all: testring ringcat tcpget tcpput

testring: testring.o
ringcat: ringcat.o copyfd.o
tcpget: tcpget.o copyfd.o
tcpput: tcpput.o copyfd.o

copyfd.o: ringbufr.h
ringcat.o: copyfd.h
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
tcpget.o: copyfd.h
tcpput.o: copyfd.h
