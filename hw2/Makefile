CC=gcc
CFLAGS=-c -Wall -g

all: p2

p2: server.o p2.o
	$(CC) server.o p2.o -o p2

server.o: server.c
	$(CC) $(CFLAGS) server.c

p2.o: p2.c
	$(CC) $(CFLAGS) p2.c

clean:
	/bin/rm -f p2 *.o *.tar.gz

run:
	./p2 8888

tarball:
	tar -cvzf p2sol.tar.gz Makefile p2.c server.c server.h SimplePost.html
