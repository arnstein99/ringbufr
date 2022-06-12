CPPFLAGS += -std=c++20 -g -Wall -pthread
LDFLAGS += -lstdc++ -lpthread
LINK.o = g++ $(LDFLAGS)

all: testring ringcat tcpget

testring: testring.o
ringcat: ringcat.o copyfd.o
tcpget: tcpget.o copyfd.o

copyfd.o: ringbufr.h
ringcat.o: copyfd.h
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
tcpget.o: copyfd.h
