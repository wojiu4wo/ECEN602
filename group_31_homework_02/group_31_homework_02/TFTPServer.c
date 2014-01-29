/* 
 * udpserver.c - A simple UDP echo server 
 * usage: udpserver <port>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "udpstruct.h"

#define BUFSIZE 1024
#define MAX_DATA_SIZE 512
#define MAX_FILE_SIZE 52428800
#define MAX_CLIENTS 4
#define ERR1 "FILE NOT FOUND"
#define ERR2 "FILE SIZE EXCEEDED"

struct RRQ_Packet RRQPacket;
struct client clientList[MAX_CLIENTS];
int clientcount=0;
FILE *fp = NULL;

/*
 * error - wrapper for perror
 */
void error(char *msg) 
{
  perror(msg);
  exit(1);
}

/*Sends the data packet to the client in the form 512 bytes payload*/
void senddatapacket(FILE * *fp, int blocknum, char sendbuf[516], int sockfd, struct sockaddr_in clientaddr, int clientlen ) 
{
   int result=0;
   int opcode= 3;
   int n;
   memset(sendbuf,0,516);
   *(int *)sendbuf = htons(opcode);
   *(int *)(sendbuf+2)=htons(blocknum);

   char *buffer;

  // allocate memory to contain the whole file:
  buffer = (char*) malloc (MAX_DATA_SIZE);
  if (buffer == NULL) {fputs ("Memory error",stderr); exit (2);}

  // copy the file into the buffer:
  result = fread (buffer,1,MAX_DATA_SIZE,*fp);
  if(result>=0)
  {
		  buffer[result]='\0';
                  //printf("Buffer is : %s",buffer);
		  printf("Result is %d",result);
  }


  memcpy(sendbuf+4,buffer,result);
   if(feof(*fp))
   {
         fclose(*fp);
        *fp= NULL ;
        printf("Inside send data packet File transmitted successfully \n");
	fflush(stdout);
       // exit(0);
   }
   else 
   {           
//   	fseek(*fp, -1L, SEEK_CUR);
   }  
   n = sendto(sockfd, sendbuf, result+4, 0, (struct sockaddr *) &clientaddr, clientlen ) ;
   if(n<0)
   error(" ERROR in sendto \n");
}

/*Sends error message to the client when 1. File is not found or 2. File exceeds maximum permissible size*/
int senderrorpacket(int errorcode, char sendbuf[516], int sockfd, struct sockaddr_in clientaddr, int clientlen )
{
   int lenerr;
   int len;
   int erroropcode = 5;
   
   *(int *)(sendbuf)=htons(erroropcode);
   *(int *)(sendbuf+2)= htons(errorcode);
   int n;
   char errorMessage1[]="FILE NOT FOUND\0";
   char errorMessage2[]="FILE EXCEEDED MAXIMUM SIZE\0";
   switch(errorcode)
   {
     case 1:lenerr = strlen(errorMessage1);
            strncpy(sendbuf+4, errorMessage1, lenerr);
            len = 4 + lenerr;
            break;
     case 2:lenerr = strlen(errorMessage2);
            strncpy(sendbuf+4, errorMessage2, lenerr);
            len = 4 + lenerr;
            break; 
     default: 
            return 0;
   }
   sendbuf[len]='\0'; 
   n = sendto(sockfd, sendbuf, len, 0, (struct sockaddr *) &clientaddr, clientlen ) ;
   if(n<0)
   error(" ERROR in sendto \n");
   return 0;
}  

/*Creates the socket for client to identify it*/
int create_socket()
{
	  printf("\nEntering create socket");
	  fflush(stdout);
          int clientfd;
	  int portno=0;
	  struct sockaddr_in clientaddr; /* client addr */
	  /* 
	   * socket: create the parent socket 
	   */
	  clientfd = socket(AF_INET, SOCK_DGRAM, 0);
	  if (clientfd < 0) 
	    error("ERROR opening socket of client");

	  /* setsockopt: Handy debugging trick that lets 
	   * us rerun the server immediately after we kill it; 
	   * otherwise we have to wait about 20 secs. 
	   * Eliminates "ERROR on binding: Address already in use" error. 
	   */
	  int optval = 1;
	  setsockopt(clientfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

	  /*
	   * build the server's Internet address
	   */
	  bzero((char *) &clientaddr, sizeof(clientaddr));
	  clientaddr.sin_family = AF_INET;
	  clientaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	  clientaddr.sin_port = htons((unsigned short)portno);

	  /* 
	   * bind: associate the parent socket with a port 
	   */
	  if (bind(clientfd, (struct sockaddr *) &clientaddr,sizeof(clientaddr)) < 0) 
	  	error("ERROR on binding for client");
          else
           printf(" Bind successfully completed of client \n");
          return clientfd;
}

/*Add a new client to the clientlist*/
void addClient(int clientfd,FILE *fp)
{
	clientList[clientcount].clientfd=clientfd;
        clientList[clientcount].fp=fp;
        printf("In addClient");
        fflush(stdout);
        if(clientList[clientcount].fp!=NULL)
	{
		printf("fp is not null");
	}
        clientList[clientcount].block_no=1;
        clientcount++;
}

/*Searches an existent client in the clientlist*/
struct client* searchClient(int clientfd)
{
      int i=0;
      for(i=0;i<clientcount;i++)
      {
            if(clientfd==clientList[i].clientfd)
            {
            	return &clientList[i];   
            }
      }
      return NULL;
}


/*Handles and decodes client RRQ or DATA Packets*/
int handleClientRequest(int sockfd)  
{
	struct sockaddr_in clientaddr; /* client addr */
	struct hostent *hostp; /* client host info */
	char *hostaddrp; /* dotted decimal host addr string */
        char sendbuf[516] = {'\0'}; 
        char *buff;
        int clientfd=0;
        int n;
        socklen_t clientlen;

        buff=(char *)malloc(sizeof(struct RRQ_Packet));
        clientlen = sizeof(clientaddr);
	/*
	* recvfrom: receive a UDP datagram from a client
	  */
	memset(buff,0, 516);
	n = recvfrom(sockfd, buff, sizeof(struct RRQ_Packet), 0,(struct sockaddr *)&clientaddr,&clientlen);
        printf("\n Got a client request");
	int len;
	uint16_t opcode;
	char filename[512];
	uint16_t acknum; 
	memset(&opcode,0,sizeof(uint16_t));
	memcpy(&opcode,buff,sizeof(uint16_t));
		    		    
	opcode=ntohs(opcode);
	if (opcode == 1)
	{
                if((clientfd=create_socket())>0)
                {
			printf("Client got socket descriptor : %d",clientfd);
                }
                         
		memset(&filename,0,512);
		memcpy(&filename,buff + sizeof(uint16_t),512);
		len=strlen(filename);
		filename[len]='\0';

		if (n < 0)
			error("ERROR in recvfrom");

		/* 
		* gethostbyaddr: determine who sent the datagram
		*/
		hostp = gethostbyaddr((const char *)&clientaddr.sin_addr.s_addr,
		sizeof(clientaddr.sin_addr.s_addr), AF_INET);
		if (hostp == NULL)
			error("ERROR on gethostbyaddr");
		hostaddrp = inet_ntoa(clientaddr.sin_addr);
		if (hostaddrp == NULL)
		error("ERROR on inet_ntoa\n");
		printf("\nOpcode from client %s: %d",hostaddrp,opcode);	
		fflush(stdout);
		printf("\nFilename from client %s: %s",hostaddrp,filename);	
		fflush(stdout);

		fp= fopen(filename, "rb");
		addClient(clientfd,fp);
                struct client *client1;
                client1=searchClient(clientfd);
               
                if(client1!=NULL)
                {
                   printf("\nclient fd :%d",client1->clientfd);
                }
			     
		// obtain file size
                FILE *fp1;
		fp1=fopen(filename, "rb");
                if(fp1!=NULL)
		{
			fseek (fp1 , 0 , SEEK_END);
			int lSize = ftell (fp1);
			fclose(fp1);
					     
			if(lSize>MAX_FILE_SIZE)
			{
				n=senderrorpacket(2, sendbuf, sockfd, clientaddr, clientlen);
			        printf("Exceeded maximum file size \n");   
		  	}
                }

		if(!(client1->fp))
		{
			n= senderrorpacket(1, sendbuf, sockfd, clientaddr, clientlen );
			printf("File not found \n");      
		}
		else
		{
			client1->block_no = 1;

			senddatapacket(&(client1->fp), client1->block_no, sendbuf, client1->clientfd, clientaddr, clientlen ); 
			if((client1->fp)==NULL)
			{
				printf("File pointer now set to NULL.");
				fflush(stdout); 
			}
		}
	}//if opcode==1   
	else if( opcode ==4)
        {
    	        memset(&acknum,0,sizeof(uint16_t));
		memcpy(&acknum,buff + sizeof(uint16_t),sizeof(uint16_t));
		acknum = htons(acknum);
		printf("\nOpcode from client : %d",opcode);	
		fflush(stdout);
		printf("\nAck num from client : %d",acknum);	
		fflush(stdout);
                struct client *client2;
                client2=searchClient(sockfd);
  		if(client2!=NULL)
		{
		if (acknum == client2->block_no)
		{
			printf("Inside if acknum=blocknum");
		   	fflush(stdout);
			client2->block_no = (client2->block_no+1)%65536;
			printf("blocknum : %d",client2->block_no);
			fflush(stdout);
					       
			if(client2->fp==NULL)
			{
				printf("End of file transfer");
			}
			else if(client2->fp!=NULL)
			{
				if(feof(client2->fp))
			        {  
					fclose(client2->fp);
				        fp= NULL ;
					printf("In opcode==4 File transmitted successfully \n");
					fflush(stdout);
			        } 
				else
				{
					printf("In else ACK block");
					fflush(stdout);
				     	senddatapacket(&client2->fp, client2->block_no, sendbuf, sockfd, clientaddr, clientlen);  
				}
			}
		} //acknum==blocknum
		}//client2!=NULL
	}//if opcode==4
	
        return clientfd;
}

int main(int argc, char **argv) 
{

	  int sockfd; /* socket */
	  int clientfd; /* client */
	  int portno; /* port to listen on */
	  //int clientlen; /* byte size of client's address */
	  struct sockaddr_in serveraddr; /* server's addr */
	  
	  int optval; /* flag value for setsockopt */
	  fd_set master;
	  fd_set read_fds;
	  int i;
	  int fdmax;
          struct hostent* hret;	  

	  /* 
	   * check command line arguments 
	   */
	  if (argc != 3) {
	    fprintf(stderr, "usage: %s <IP> <port>\n", argv[0]);
	    exit(1);
	  }
	  portno = atoi(argv[2]);

	  /* 
	   * socket: create the parent socket 
	   */
	  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	  if (sockfd < 0) 
	    error("ERROR opening socket");
          printf("Socket opened successfully");
          fflush(stdout);

	  /* setsockopt: Handy debugging trick that lets 
	   * us rerun the server immediately after we kill it; 
	   * otherwise we have to wait about 20 secs. 
	   * Eliminates "ERROR on binding: Address already in use" error. 
	   */
	  optval = 1;
	  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR,(const void *)&optval , sizeof(int));

	  /*
	   * build the server's Internet address
	   */
	  bzero((char *) &serveraddr, sizeof(serveraddr));
	  serveraddr.sin_family = AF_INET;
          hret = gethostbyname(argv[1]);
          memcpy(&serveraddr.sin_addr.s_addr, hret->h_addr,hret->h_length);
	  serveraddr.sin_port = htons((unsigned short)portno);

	  /* 
	   * bind: associate the parent socket with a port 
	   */
	  if (bind(sockfd, (struct sockaddr *) &serveraddr,sizeof(serveraddr)) < 0) 
	  	error("ERROR on binding");
          else
           printf(" Bind successfully completed \n");
	  
          FD_SET(sockfd, &master);//add the server socker address in the master file descriptor set
	  fdmax =sockfd;// set the maximum file descriptor initially to server sockfd
	  
	  /* 
	   * main loop: wait for a datagram, then echo it
	   */
	 

          /* Wait for 2 seconds for ACK to arrive*/
          int selectVal;

	  while (1)
	  {

	       read_fds = master;//set the local file descriptor read_fds to master file descriptor file
	       
               //select is used for Multiple I/O
	       if((selectVal=select(fdmax+1, &read_fds, NULL, NULL, NULL)) == -1)
	       {
		    perror("select");
		    exit(4);
	       }
     	      /* else if(selectVal==0)
               {
		    printf("Timeout occurred");
                    blocknum=blocknum-1;
                    fseek(fp,-1*result,SEEK_CUR);
		    senddatapacket(&fp, blocknum, sendbuf, sockfd, clientaddr, clientlen ); 
	       }*/
	       
	       
                   for(i=0 ; i<=fdmax ; i++)
	       	   {
		 	if(FD_ISSET(i, &read_fds))
		 	{
		     		if(i == sockfd)
				{//new client connections
		                	clientfd=handleClientRequest(i);
					FD_SET(clientfd,&master);
                                        if(clientfd>fdmax)
                                        {
						fdmax=clientfd;
		                        }
 	
		    		}//i==sockfd
				else
				{
					clientfd=handleClientRequest(i);
				}
                                
		}//if FD_ISSET
            }//for loop
          
	}//while 1
}
