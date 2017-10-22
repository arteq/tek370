CXX = g++
CFLAGS =-Wall
LFLAGS =
DRIVER = ../linux-gpib-3.2.10

all: tek370.o
	libtool --tag=CXX --mode=link g++ -o tek370 tek370.o ${DRIVER}/lib/libgpib.la -lpthread

tek370.o:
	${CXX} ${CFLAGS} -c tek370.cpp

clean:
	rm -f tek370.o
	rm -f tek370
