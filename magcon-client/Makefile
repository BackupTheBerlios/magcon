GCC=gcc -Wall -ansi -g -pedantic

all : 2mag

2mag.o : 2mag.c
	$(GCC) -c 2mag.c

2mag : 2mag.o
	$(GCC) -o 2mag 2mag.o
clean :
	rm -f 2mag 2mag.o

dist : clean
	rm -rf 2mag-src
	ln -s . 2mag-src
	tar -czf ../2mag-src.tar.gz 2mag-src/*.c 2mag-src/*.h 2mag-src/Makefile 2mag-src/LICENSE 2mag-src/README
	rm -f 2mag-src

