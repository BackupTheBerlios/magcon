GCC=diet gcc -Wall -g  -Icoldsync -static
COLDSYNC=coldsync/util.o coldsync/pdb.o
CONTRIB=contrib/mag2pdb.o contrib/pdb2txt.o
OBJS=2mag.o $(COLDSYNC)
OBJS2=2gpx.o $(COLDSYNC)

.c.o:
	$(GCC) -c $< -o $@

all : 2mag 2gpx mag2pdb pdb2txt

2mag : $(OBJS)
	$(GCC) $(OBJS) -o 2mag 
	
2gpx :  $(OBJS2)
	$(GCC) $(OBJS2) -o 2gpx 

mag2pdb : $(CONTRIB)
	$(GCC) contrib/mag2pdb.o -o contrib/mag2pdb

pdb2txt : $(CONTRIB)
	$(GCC) contrib/pdb2txt.o -o contrib/pdb2txt

clean :
	rm -f $(OBJS) $(CONTRIB) 2gpx.o 2mag 2gpx contrib/pdb2txt contrib/mag2pdb

dist : clean
	rm -rf 2mag-src
	ln -s . 2mag-src
	tar -czf ../2mag-src.tar.gz 2mag-src/*.c 2mag-src/*.h 2mag-src/Makefile 2mag-src/LICENSE 2mag-src/README 2mag-src/coldsync/*
	rm -f 2mag-src

