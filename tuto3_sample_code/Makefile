CC = gcc
CFLAG = -lpthread

all: server client MTserver MTclient

server: server.c
	$(CC) -o $@ $< -std=c99

client: client.c
	$(CC) -o $@ $< -std=c99

MTserver: MTserver.c
	$(CC) -o $@ $< $(CFLAG) -std=c99

MTclient: MTclient.c
	$(CC) -o $@ $< $(CFLAG) -std=c99

clean:
	rm server client MTserver MTclient
