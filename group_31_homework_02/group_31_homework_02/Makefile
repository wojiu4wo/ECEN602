# Enable gcc compiler 
CC = gcc
# Set -Wall as per requirement and -g for debug.
CFLAGS=-Wall -g
all: tftpserver
 tftpserver: TFTPServer.c udpstruct.h
		$(CC) $(CFLAGS) TFTPServer.c -o Server

 clean:
	rm -rf *o Server
