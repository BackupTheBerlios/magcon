# $Id: Makefile,v 1.5 2003/10/05 10:17:52 niki Exp $
# Macros

PROGRAM=mag1
CC=m68k-palmos-gcc
PILRC=pilrc
OBJRES=m68k-palmos-obj-res
ICONTEXT='MagCon'
BUILDPRC=build-prc
APID=Mag1
CFLAGS=-Wall -g
LFLAGS=-g

SOURCES=main.c magellan.c ser.c dbdia.c
OBJS=main.o magellan.o ser.o dbdia.o
PRC=$(PROGRAM).prc
RESOURCES=$(PROGRAM).rcp


# Major targets

all: $(PRC)

$(PRC): $(PROGRAM) bin.res
	$(BUILDPRC) $(PRC) $(ICONTEXT) $(APID) *.bin *.grc

$(PROGRAM): $(OBJS)
	$(CC) -o $(PROGRAM) $(OBJS) $(LFLAGS)
	$(OBJRES) $(PROGRAM)

bin.res: $(RESOURCES) resource.h icon.bmp
	$(PILRC) $(RESOURCES)
	touch bin.res

ctags:
	ctags $(SOURCES)

clean:
	rm -f *.bin *.grc *.o bin.res tags $(PRC) $(PROGRAM)

dist: clean
	rm -f ../magcon-src.zip
	zip -9 ../magcon-src *.[ch] *.rcp *.bmp Makefile README LICENSE

install: all
	 pilot-xfer -i mag1.prc

# Dependency targets
main.o: main.c resource.h magellan.h ser.h dbdia.h
magellan.o: magellan.c resource.h ser.h dbdia.h
ser.o: ser.c magellan.h resource.h
dbdia.o: dbdia.c resource.h magellan.h 
