# $Id: Makefile,v 1.2 2003/02/08 18:43:24 niki Exp $
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

SOURCES=main.c magellan.c ser.c deldb.c
OBJS=main.o magellan.o ser.o deldb.o
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
	rm -f ../magcon.zip
	zip -9 ../magcon *.[ch] *.rcp *.bmp Makefile README LICENSE

# Dependency targets
main.o: main.c resource.h magellan.h ser.h deldb.h
magellan.o: magellan.c resource.h ser.h
ser.o: ser.c magellan.h resource.h
deldb.o: deldb.c resource.h
