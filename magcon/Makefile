# $Id: Makefile,v 1.1 2003/02/02 10:41:16 niki Exp $
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

SOURCES=main.c magellan.c ser.c
OBJS=main.o magellan.o ser.o
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

main.o: main.c resource.h ser.h magellan.h
magellan.o: magellan.c
ser.o: ser.c resource.h

