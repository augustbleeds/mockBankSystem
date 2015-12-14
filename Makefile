CC = gcc
CFLAGS = -Wall -g -lpthread

all: server_n_client

server_n_client: server.o client.o banksys.o
	$(CC) $(CFLAGS)  -o server server.o banksys.o
	$(CC) $(CFLAGS)  -o client client.o banksys.o

server.o: server.c
	$(CC) $(CFLAGS) -c server.c

client.o: client.c
	$(CC) $(CFLAGS) -c client.c

banksys.o: banksys.c banksys.h
	$(CC) $(CFLAGS) -c banksys.c

clean:
	rm -rf *.o client server

