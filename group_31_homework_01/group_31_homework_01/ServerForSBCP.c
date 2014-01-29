#include <stdio.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "StructForSBCP.h"

int clientCount = 0;
struct ClientInformation *clients;

//checks if username does not exist already
int checkUsername(char a[]){
    int i = 0;
    int ret = 0;
    for(i = 0; i < clientCount ; i++){
        if(!strcmp(a,clients[i].username)){
            ret = 1;
            break;
        }
    }
    return ret;
}

//send ACK message to client, with the client count and list of usernames
void sendACK(int connfd){

    struct MessageSBCP clientStatusMessage;
    struct HeaderSBCP clientStatusHeader;
    struct AttributeSBCP clientStatusAttribute;
    int i = 0;
    char temp[180];

    clientStatusHeader.vrsn=3;
    clientStatusHeader.type=7;
    //clientStatusHeader.length = 

    clientStatusAttribute.type = 4;

    // Only works till '9'
    temp[0] = (char)(((int)'0')+ clientCount);
    temp[1] = ' ';
    temp[2] = '\0';
    for(i =0; i<clientCount-1; i++)
    {
        strcat(temp,clients[i].username);
        if(i != clientCount-2)
        strcat(temp, ",");
    }
    clientStatusAttribute.length = strlen(temp)+1;
    strcpy(clientStatusAttribute.payload, temp);
    clientStatusMessage.header = clientStatusHeader;
    clientStatusMessage.attribute[0] = clientStatusAttribute;

    write(connfd,(void *) &clientStatusMessage,sizeof(clientStatusMessage));
}

//send NAK message to client, to refuse connection (for e.g. when username already exists)
void sendNAK(int connfd,int code){

    struct MessageSBCP clientFailMessage;
    struct HeaderSBCP clientFailHeader;
    struct AttributeSBCP clientFailAttribute;
    char temp[32];

    clientFailHeader.vrsn =3;
    clientFailHeader.type =5;

    clientFailAttribute.type = 1;

    //1 - Inv username
    if(code == 1)
    {
        strcpy(temp,"Invalid username");
    }
    clientFailAttribute.length = strlen(temp);
    strcpy(clientFailAttribute.payload, temp);

    clientFailMessage.header = clientFailHeader;
    clientFailMessage.attribute[0] = clientFailAttribute;

    write(connfd,(void *) &clientFailMessage,sizeof(clientFailMessage));

    close(connfd);

}

//checks if client does not already exist, and if not so, sends ACK message to client
int checkClient(int connfd){

    struct MessageSBCP joinMessage;
    struct AttributeSBCP joinMessageAttribute;
    char temp[16];
    int status = 0;
    read(connfd,(struct MessageSBCP *) &joinMessage,sizeof(joinMessage));
    //joinMessageAttribute.payload = (char *)malloc(sizeof(char *) * joinMessage.joinMessageAttribute.length);
    joinMessageAttribute = joinMessage.attribute[0];
    //clients[clientCount].username = (char *)malloc(sizeof(char *) * joinMessage.joinMessageAttribute.length);
    strcpy(temp, joinMessageAttribute.payload);
    status = checkUsername(temp);
    if(status == 1)
    {
        printf("\nClient already exists.");
        sendNAK(connfd, 1); // 1 for client already exists 
    }
    else
    {
        strcpy(clients[clientCount].username, temp);
        clients[clientCount].fd = connfd;
        clients[clientCount].clientCount = clientCount;
        clientCount = clientCount + 1;
        sendACK(connfd);
    }
    return status;
}


int main(int argc, char const *argv[])
{
    /* code */
    struct MessageSBCP clientMessage,forwardedMessage,forwardOnlineMessage,forwardOfflineMessage;
    struct AttributeSBCP clientAttribute;

    int serverSockFd, connfd;
    unsigned int len;
    int ret;
    int clientStatus = 0;
    struct sockaddr_in servAddr,*cli;
    struct hostent* hret;
    
    fd_set master;
    fd_set read_fds;
    int fdmax;
    int temp;
    int i=0,y;
    int j=0;
    int x=0;
    int nbytes;
    int maxClients=0;

    serverSockFd = socket(AF_INET,SOCK_STREAM,0);
    if(serverSockFd == -1)
    {
        printf("Server Socket creation failed.\n");
        ret = -1;
    }
    else{
        printf("Server Socket successfully created.\n");
    }
    
    bzero(&servAddr,sizeof(servAddr));

    servAddr.sin_family = AF_INET;
    hret = gethostbyname(argv[1]);
    memcpy(&servAddr.sin_addr.s_addr, hret->h_addr,hret->h_length);
    servAddr.sin_port = htons(atoi(argv[2]));

    maxClients=atoi(argv[3]);

    clients= (struct ClientInformation *)malloc(maxClients*sizeof(struct ClientInformation));
    cli=(struct sockaddr_in *)malloc(maxClients*sizeof(struct sockaddr_in));
			
    if((bind(serverSockFd, (struct sockaddr*)&servAddr, sizeof(servAddr)))!=0)
    {
        printf("\nServer Socket bind failed.");
        ret = -1;
    }
    else
    {
        printf("\nServer Socket successfully bound.");
    }
    
    if((listen(serverSockFd, maxClients))!=0)
    {
        printf("\nServer Listen failed.");
        ret = -1;
    }
    else
    {
        printf("\nServer listening.");
    }

    FD_SET(serverSockFd, &master);
    fdmax = serverSockFd;

    for(;;)
    {
        read_fds = master;
        if(select(fdmax+1, &read_fds, NULL, NULL, NULL) == -1)
	{
            perror("select");
            exit(4);
        }

        for(i=0 ; i<=fdmax ; i++)
	{
            if(FD_ISSET(i, &read_fds))
	    {
                if(i == serverSockFd)
		{
                    //New Connections
                    //Add them to the client address array cli
                    len = sizeof(cli[clientCount]);
                    connfd = accept(serverSockFd,(struct sockaddr *)&cli[clientCount],&len);
                    if(connfd < 0)
                    {
                        printf("\nServer accept failed.");
                        ret = -1;
                    }
                    else
                    {
                        temp = fdmax;
                        FD_SET(connfd, &master);
                        if(connfd > fdmax){
                            fdmax = connfd;
                        }   
                        clientStatus = checkClient(connfd);
                        if(clientStatus != 1)
			{   
                            //Send an ONLINE Message to all the clients except this
                            printf("\nServer accepted the client : %s",clients[clientCount-1].username);
           		    forwardOnlineMessage.header.vrsn=3;
                            forwardOnlineMessage.header.type=8;
        		    forwardOnlineMessage.attribute[0].type=2;
		            strcpy(forwardOnlineMessage.attribute[0].payload,clients[clientCount-1].username);	
			    
                            for(j = 0; j <= fdmax; j++) 
   		    	    {
	                    	    // send to everyone!
        	            	    if (FD_ISSET(j, &master)) 
        	            	    {
        	            	        // except the listener and ourselves
        	            	        if (j != serverSockFd && j != connfd)
			                {
        	                    	    if ((write(j,(void *) &forwardOnlineMessage,sizeof(forwardOnlineMessage))) == -1)
					    {
        	                            	perror("send");
        	                            }
        	                        }
        	                    }       
        	             }
                            ret = connfd;
                        }
		        else
			{
                            printf("\nClient username already exists");
                            fdmax = temp;
                            FD_CLR(connfd, &master);//clear connfd if username does not exist
                        }   
                    }
                }
                else
		{
		    //existent connections
		    if ((nbytes=read(i,(struct MessageSBCP *) &clientMessage,sizeof(clientMessage))) <= 0) 
                    {
	        	// got error or connection closed by client
			if (nbytes == 0) 
			{
				// connection closed
				printf("\nSocket %d hung up\n", i);

                                //Send an OFFLINE Message to all the clients except this
		                for(y=0;y<clientCount;y++)
				{
				    	if(clients[y].fd==i)
					{
					        forwardOfflineMessage.attribute[0].type=2;
						strcpy(forwardOfflineMessage.attribute[0].payload,clients[y].username);	
					}
				}
                                
                                forwardOfflineMessage.header.vrsn=3;
                                forwardOfflineMessage.header.type=6;
                                
                                for(j = 0; j <= fdmax; j++) 
   		    	        {
	                    	    // send to everyone!
        	            	    if (FD_ISSET(j, &master)) 
        	            	    {
        	            	        // except the listener and ourselves
        	            	        if (j != serverSockFd && j != i)
			                {

        	                    	    if ((write(j,(void *) &forwardOfflineMessage,sizeof(forwardOfflineMessage))) == -1)
					    {
        	                            	perror("send");
        	                            }
        	                        }
        	                    }       
        	                }
			}
			else
			{
				perror("recv");
			}
			close(i); // bye!
			FD_CLR(i, &master); // remove from master set
			
			for(x=i;x<clientCount;x++)
		        {
				clients[x]=clients[x+1];
			}
                        clientCount--;
		    }
                    else
		    {
                        //Received an nbyte message from the client
                    	clientAttribute = clientMessage.attribute[0];//message
			forwardedMessage=clientMessage;
 			forwardedMessage.header.type=3;
		        forwardedMessage.attribute[1].type=2;//username

 			int payloadLength=0;
                        char temp[16];
                        payloadLength=strlen(clientAttribute.payload);
                        strcpy(temp,clientAttribute.payload);
                        temp[payloadLength]='\0';

                    	printf("Payload is %s",temp);

                        //Forward the message to all the clients except this
                    	//new data
 		        for(y=0;y<clientCount;y++)
			{
			    	if(clients[y].fd==i)
				{
					strcpy(forwardedMessage.attribute[1].payload,clients[y].username);	

				}
			}
                    	for(j = 0; j <= fdmax; j++) 
		    	{
                    	    // send to everyone!
                    	    if (FD_ISSET(j, &master)) 
                    	    {
                    	        // except the listener and ourselves
                    	        if (j != serverSockFd && j != i)
		                {
                            	    if ((write(j,(void *) &forwardedMessage,nbytes)) == -1)
				    {
                                    	perror("send");
                                    }
                                }
                            }       
                       }
                    }
                }
             }
        }
    }
    
    close(serverSockFd);

    return 0;
}
