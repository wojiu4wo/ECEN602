#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include "StructForSBCP.h"

//Read the Server Message (could be of type FWD, ONLINE, OFFLINE, ACK,NAK)
int getServerMessage(int sockfd){

    struct MessageSBCP serverMessage;
    int status = 0;
    int nbytes=0;

    //printf("GET SERVER MESSAGE \n");
    nbytes=read(sockfd,(struct MessageSBCP *) &serverMessage,sizeof(serverMessage));
    


    if(serverMessage.header.type==3)
    {
    	if((serverMessage.attribute[0].payload!=NULL || serverMessage.attribute[0].payload!='\0') && (serverMessage.attribute[1].payload!=NULL || serverMessage.attribute[1].payload!='\0') && serverMessage.attribute[0].type==4 && serverMessage.attribute[1].type==2)
	{     	
		printf("Forwarded Message from %s is %s", serverMessage.attribute[1].payload,serverMessage.attribute[0].payload);
	}
        status=0;
    }

    if(serverMessage.header.type==5)
    {
    	if((serverMessage.attribute[0].payload!=NULL || serverMessage.attribute[0].payload!='\0') && serverMessage.attribute[0].type==1)
       {
                printf("NAK Message from Server is %s",serverMessage.attribute[0].payload);
       }
       status=1;
    }

    if(serverMessage.header.type==6)
    {
	if((serverMessage.attribute[0].payload!=NULL || serverMessage.attribute[0].payload!='\0') && serverMessage.attribute[0].type==2)
       {
                printf("OFFLINE Message %s is now OFFLINE",serverMessage.attribute[0].payload);
       }
       status=0;
    }

    if(serverMessage.header.type==7)
    {    	
	if((serverMessage.attribute[0].payload!=NULL || serverMessage.attribute[0].payload!='\0') && serverMessage.attribute[0].type==4)
       {
                printf("ACK Message from Server is %s",serverMessage.attribute[0].payload);
       }
       status=0;
    }

    if(serverMessage.header.type==8)
    {
	if((serverMessage.attribute[0].payload!=NULL || serverMessage.attribute[0].payload!='\0') && serverMessage.attribute[0].type==2)
       {
                printf("ONLINE Message %s is now ONLINE",serverMessage.attribute[0].payload);
       }
       status=0;
    }

    if(serverMessage.header.type==9)
    {
    }

    return status;
}

//Send a JOIN Message to server when connected to server
void sendJoin(int sockfd,const char *arg[]){

    struct HeaderSBCP header;
    struct AttributeSBCP attrib;
    struct MessageSBCP msg;
    int status = 0;

    header.vrsn = '3';
    header.type = '2';//JOIN MSG type=2
    //header.length = 
    attrib.type = 2;//Username
    attrib.length = strlen(arg[1]) + 1;
    strcpy(attrib.payload,arg[1]);
    msg.header = header;
    msg.attribute[0] = attrib;

    write(sockfd,(void *) &msg,sizeof(msg));

    // Sleep to allow Server to reply
    sleep(1);
    status = getServerMessage(sockfd); 
    if(status == 1){
            close(sockfd);
    }
}


//Accept user input, and send it to server for broadcasting
void chat(int connectionDesc){

    struct MessageSBCP msg;
    struct AttributeSBCP clientAttribute;

    int nread = 0;
    char temp[512];
    //printf("\n>");
    //scanf("%[^\n]%*c",temp);
    //gets(temp);
    struct timeval tv;
    fd_set readfds;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    FD_ZERO(&readfds);
    FD_SET(STDIN_FILENO, &readfds);
    // don't care about writefds and exceptfds:
    select(STDIN_FILENO+1, &readfds, NULL, NULL, &tv);
    if (FD_ISSET(STDIN_FILENO, &readfds))
    {
	nread = read(STDIN_FILENO, temp, sizeof(temp));
        if(nread > 0)
	{
	        temp[nread] = '\0';
        }
    
    	clientAttribute.type = 4;
    	strcpy(clientAttribute.payload,temp);
    	msg.attribute[0] = clientAttribute;
    	write(connectionDesc,(void *) &msg,sizeof(msg));
    }
    else
    {
	printf("Timed out.\n");
    }
}

int main(int argc, char const *argv[])
{
    /* code */
    if(argc == 4)
    {
	    int sockfd;
	    struct sockaddr_in servaddr;
	    struct hostent* hret;
	    fd_set master;
	    fd_set read_fds;
	    FD_ZERO(&read_fds);
	    FD_ZERO(&master);
	    sockfd = socket(AF_INET,SOCK_STREAM,0);

	    if(sockfd==-1)
	    {
        	printf("socket creation failed...\n");
	        exit(0);
	    }

	    else
	    {
	        printf("Socket successfully created..\n");
	    }
    
	    bzero(&servaddr,sizeof(servaddr));
	    servaddr.sin_family=AF_INET;
	    hret = gethostbyname(argv[2]);
	    memcpy(&servaddr.sin_addr.s_addr, hret->h_addr,hret->h_length);
	    servaddr.sin_port = htons(atoi(argv[3]));

	    if(connect(sockfd,(struct sockaddr *)&servaddr,sizeof(servaddr))!=0)
	    {
	        printf("connection with the server failed...\n");
	        exit(0);
	    }
	    else
	    {
	        printf("connected to the server..\n"); 
	        sendJoin(sockfd, argv);
	        FD_SET(sockfd, &master);
	        FD_SET(STDIN_FILENO, &master);
	        for(;;)
		{
	            read_fds = master;
	            printf("\n");
	            //printf("%d",sockfd);
	            
		    if(select(sockfd+1, &read_fds, NULL, NULL, NULL) == -1)
		    {
	                perror("select");
                	exit(4);
            	    }

	            if(FD_ISSET(sockfd, &read_fds))
		    {
                    	getServerMessage(sockfd);
                    }
 
	            if(FD_ISSET(STDIN_FILENO, &read_fds))
		    {
                	//printf("STDIN\n");
                        chat(sockfd);
                    }
            	    //printf("Hello SELECT send \n");
                }

                printf("\n initConnection Ends \n");
            }
     }
     printf("\n Client close \n");
     return 0;
}
