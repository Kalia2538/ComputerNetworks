CC=gcc
CFLAGS= -g -Wall

all: proxy

proxy: proxy.c
	$(CC) $(CFLAGS) -g -o proxy_parse.o -c proxy_parse.c
	$(CC) $(CFLAGS) -g -o proxy.o -c proxy.c
	$(CC) $(CFLAGS) -g -o proxy proxy_parse.o proxy.o

clean:
	rm -f proxy *.o
