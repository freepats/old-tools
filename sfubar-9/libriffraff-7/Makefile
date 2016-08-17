SRCDIR?=..
PREFIX?=.
CFLAGS?=	-O0 -g -Wall -I$(PREFIX)/include

AR=$(TOOLDIR)ar
CC=$(TOOLDIR)gcc
RANLIB=$(TOOLDIR)ranlib

PT=		parsetxt-4
RR=		riffraff-7

HEADERS=	aif.h bio.h bits.h riff.h rifftypes.h wav.h ieee80.h
OBJS=		aif.o bio.o riff.o rifftypes.o wav.o ieee80.o

lib$(RR).a: $(PREFIX)/lib/lib$(PT).a ieee80-test $(OBJS)
	${AR} -r lib${RR}.a ${OBJS}
	${RANLIB} lib${RR}.a

clean:
	rm -f *.o lib${RR}.a ieee80-test
	cd ${SRCDIR}/lib${PT} && ${MAKE} clean

$(PREFIX)/lib/lib$(PT).a:
	cd ${SRCDIR}/lib${PT} && \
	PREFIX=${PREFIX} CFLAGS="${CFLAGS}" TOOLDIR=${TOOLDIR} ${MAKE} install

ieee80-test: ieee80.c
	${CC} ${CFLAGS} -D_DO_TEST -o $@ ieee80.c -lm

install: lib$(RR).a
	mkdir -p ${PREFIX}/include/riffraff/
	cp ${HEADERS} ${PREFIX}/include/riffraff/
	cp lib${RR}.a ${PREFIX}/lib/
	cp ieee80-test ${PREFIX}/bin/

aif.o:	aif.c
bio.o:	bio.c
ieee80.o:	ieee80.c
riff.o:	riff.c
rifftypes.o:	rifftypes.c
wav.o:	wav.c
