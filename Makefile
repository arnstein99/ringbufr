CPPFLAGS += -std=c++20 -g -Wall -pthread
LDFLAGS += -lstdc++ -lpthread
testring: testring.o
testring.o: posix_ringbufr.h ringbufr.h ringbufr.tcc
