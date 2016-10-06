CC = gcc
CFLAGS = -Wall -std=gnu99 
all = client server
all: client server

client: client.c
	$(CC) $(CFLAGS) client.c -o  client.o

server: server.c
	$(CC) $(CFLAGS) server.c -o server.o

clean:
	-rm -f *.o $(all) core
