CC = gcc
CFLAGS = -Wall -std=gnu99 
all = client server
all: client server

client: client.c
	$(CC) $(CFLAGS) client.c utility.c -o chat379

server: server.c
	$(CC) $(CFLAGS) server.c utility.c -o server379

clean:
	-rm -f *.o $(all) core
