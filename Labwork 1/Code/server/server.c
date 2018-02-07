#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <signal.h>
#include <ctype.h>          
#include <arpa/inet.h>
#include <netdb.h>

#define LENGTH 512 


int main ()
{
	int sockfd; 
	int nsockfd; 
	int num;
	int sin_size; 
	struct sockaddr_in ad; /* client addr */
	struct sockaddr_in ad_serv; /* server addr */
	char revbuf[LENGTH]; // Receiver buffer
	socklen_t ad_length = sizeof(ad);
	
	/* Create the socket */
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
	 	
	/* Bind a special Port */
	memset(&ad, 0, sizeof(ad));
        ad.sin_family = AF_INET;
        ad.sin_addr.s_addr = INADDR_ANY;
        ad.sin_port = htons(12345);
        bind(sockfd, (struct sockaddr *)&ad, ad_length);
	
        listen(sockfd, 0);
	
	while(1)
	{
		sin_size = sizeof(struct sockaddr_in);

		/* Connect */
		 nsockfd = accept(sockfd, (struct sockaddr *)&ad, &ad_length); 

		/*Receive File from client */
		char* fr_name = "received from client.txt";
		FILE *fr = fopen(fr_name, "a");
		if(fr == NULL)
			printf("File %s cannot be opened on server.\n", fr_name);
		else
		{
			bzero(revbuf, LENGTH); 
			int fr_block_sz = 0;
			while((fr_block_sz = recv(nsockfd, revbuf, LENGTH, 0)) > 0) 
			{
			    int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
			    bzero(revbuf, LENGTH);
		            if (fr_block_sz == 0 || fr_block_sz != 512) 
			     {
					break;
			      }
			}
			printf("Received from client!\n");
			fclose(fr); 
		}

		    /* Send file to client */
                    char* fs_name = "letter.txt";
		    char sdbuf[LENGTH]; // Send buffer
		    printf("Sending %s to the client.\n", fs_name);
		    FILE *fs = fopen(fs_name, "r");
		    if(fs == NULL)
		    {
		        fprintf(stderr, "File %s not found on server.\n", fs_name);
			exit(1);
		    }

		    bzero(sdbuf, LENGTH); 
		    /* Code was found at : https://goo.gl/ssENRv */
		    int fs_block_sz; 
		    while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs))>0)
		    {
		        if(send(nsockfd, sdbuf, fs_block_sz, 0) < 0)
		        {
		            fprintf(stderr, "Failed to send file %s.\n", fs_name);
		            exit(1);
		        }
		        bzero(sdbuf, LENGTH);
		    }
		    printf("Sent the file to client!\n");  

		    close(nsockfd);	
			
	}
	return 0;
}
