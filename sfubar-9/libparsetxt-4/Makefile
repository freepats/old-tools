PREFIX?=.
CFLAGS?=	-O0 -g -Wall -I$(PREFIX)/include

AR=$(TOOLDIR)ar
CC=$(TOOLDIR)gcc
RANLIB=$(TOOLDIR)ranlib

PT=parsetxt-4

OBJS=		parsetxt.o

lib$(PT).a: install-headers $(OBJS)
	${AR} -r lib${PT}.a ${OBJS}
	${RANLIB} lib${PT}.a

clean:
	rm -f *.o lib${PT}.a

install: lib$(PT).a
	cp lib${PT}.a ${PREFIX}/lib/

install-headers: parsetxt.h
	cp parsetxt.h ${PREFIX}/include/

parsetxt.o:	parsetxt.c
