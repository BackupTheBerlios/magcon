GCC=gcc -Wall -g  -Icoldsync -static
COLDSYNC=coldsync/util.o coldsync/pdb.o
OBJS=2mag.o $(COLDSYNC)

.c.o:
	$(GCC) -c $< -o $@

all : 2mag

2mag : $(OBJS)
	$(GCC) $(OBJS) -o 2mag 
clean :
	rm -f $(OBJS) 2mag

dist : clean
	rm -rf 2mag-src
	ln -s . 2mag-src
	tar -czf ../2mag-src.tar.gz 2mag-src/*.c 2mag-src/*.h 2mag-src/Makefile 2mag-src/LICENSE 2mag-src/README 2mag-src/coldsync/*
	rm -f 2mag-src

