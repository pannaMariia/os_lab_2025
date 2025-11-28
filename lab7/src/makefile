CC = gcc
CFLAGS = -Wall -Wextra -std=c99

all: tcpclient_orig tcpserver_orig udpclient_orig udpserver_orig

tcpclient_orig: tcpclient.c
	$(CC) $(CFLAGS) -o tcpclient_orig tcpclient.c

tcpserver_orig: tcpserver.c
	$(CC) $(CFLAGS) -o tcpserver_orig tcpserver.c

udpclient_orig: udpclient.c
	$(CC) $(CFLAGS) -o udpclient_orig udpclient.c

udpserver_orig: udpserver.c
	$(CC) $(CFLAGS) -o udpserver_orig udpserver.c

clean:
	rm -f tcpclient_orig tcpserver_orig udpclient_orig udpserver_orig

.PHONY: all clean