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


int main(int argc, char *argv[])
{
	int sockfd; 
	int nsockfd;
	char revbuf[LENGTH]; 
	struct sockaddr_in ad;
	socklen_t ad_length = sizeof(ad);
        struct hostent *hep;

        /*Create  the socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Socket address struct */	
	hep = gethostbyname(argv[1]);
        memset(&ad, 0, sizeof(ad));
        ad.sin_family = AF_INET;
        ad.sin_addr = *(struct in_addr *)hep->h_addr_list[0];
        ad.sin_port = htons(12345);
	
	/* Try to connect */	
        connect(sockfd, (struct sockaddr *)&ad, ad_length);

	/* Send File to Server */	
	char* fs_name = "message.txt";
	char sdbuf[LENGTH]; 
	printf("Sending file %s to the server. \n ", fs_name);
	FILE *fs = fopen(fs_name, "r");
	if(fs == NULL)
	{
        	printf("File %s not found.\n", fs_name);
		exit(1);
	}

	bzero(sdbuf, LENGTH); 
	int fs_block_sz; 
	while((fs_block_sz = fread(sdbuf, sizeof(char), LENGTH, fs)) > 0)
	{
		if(send(sockfd, sdbuf, fs_block_sz, 0) < 0)
		{
		   fprintf(stderr, "Failed to send the file %s. \n", fs_name);
		   break;
		}
	        bzero(sdbuf, LENGTH);
	}
	printf("File %s from the client was sent\n", fs_name);
	

	/* Receive File from Server */
	char* fr_name = "received from server.txt";
	FILE *fr = fopen(fr_name, "a");
	if(fr == NULL)
	{
            printf("File %s cannot be opened.\n", fr_name);
	}
	else
	{
	    bzero(revbuf, LENGTH); 
	    int fr_block_sz = 0;
	    while((fr_block_sz = recv(sockfd, revbuf, LENGTH, 0)) > 0)
	    {
		int write_sz = fwrite(revbuf, sizeof(char), fr_block_sz, fr);
	    }
	
	    printf("Received the file from server.\n");
	    fclose(fr);
	}
	close (sockfd);
	
	return 0;
}
