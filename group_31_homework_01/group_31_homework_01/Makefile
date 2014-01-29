# Enable gcc compiler 
CC = gcc
# Set -Wall as per requirement and -g for debug.
CFLAGS=-Wall -g
all: sbcpserver sbcpclient
 sbcpserver: ServerForSBCP.c StructForSBCP.h
		$(CC) $(CFLAGS) ServerForSBCP.c -o Server

 sbcpclient: ClientForSBCP.c StructForSBCP.h
		$(CC) $(CFLAGS)  ClientForSBCP.c -o Client 

 clean:
	rm -rf *o Server Client
