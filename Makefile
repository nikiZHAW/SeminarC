CFLAGS=-Wall -g -O2 -std=gnu99# -I./include -L./lib
LIBS=-lpthread
LRT=-lrt
all: bin

clean:
	rm -f run/server.o test/client.o


bin: src/server.c src/client.c src/mylib.h
	gcc $(CFLAGS) src/server.c $(LIBS) -o run/server.o $(LRT)
	gcc $(CFLAGS) src/client.c $(LIBS) -o test/client.o $(LRT)
