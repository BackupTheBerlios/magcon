GCC=gcc -Wall -g  -Icoldsync -static
COLDSYNC=coldsync/util.o coldsync/pdb.o
OBJS=2mag.o $(COLDSYNC)
OBJS2=2gpx.o $(COLDSYNC)

.c.o:
	$(GCC) -c $< -o $@

all : 2mag 2gpx

2mag : $(OBJS)
	$(GCC) $(OBJS) -o 2mag 
	
2gpx :  $(OBJS2)
	$(GCC) $(OBJS2) -o 2gpx 

clean :
	rm -f $(OBJS) 2gpx.o 2mag 2gpx

dist : clean
	rm -rf 2mag-src
	ln -s . 2mag-src
	tar -czf ../2mag-src.tar.gz 2mag-src/*.c 2mag-src/*.h 2mag-src/Makefile 2mag-src/LICENSE 2mag-src/README 2mag-src/coldsync/*
	rm -f 2mag-src

