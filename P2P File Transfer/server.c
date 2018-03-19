#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <arpa/inet.h>

typedef struct {
   int sock;
   struct sockaddr_in addr;
} Info;

typedef struct {
   char name[256];
} Filename;

struct PeerNode {
   char port[256];
   char hostname[256];
   Filename* files;
   int filessize;
   int filesmaxsize;
   struct PeerNode* next;
};

void peerThread(void * newsockfd);
int savefiles(int newsockfd, char* address, char* returnarray);
void sendlist(int);
void removepeer(char* address, char* port);
int dorecv(int sock, char* buffer, int buffersize, int bytestoread);

pthread_t peer;
struct PeerNode* peers;
int peerssize;
int numfiles;

int main(int argc, char *argv[])
{

   int sockfd, newsockfd, portno;
   struct sockaddr_in serv_addr, clt_addr;
   socklen_t len;
   int pid;
   int ret;
	peerssize = 0;
	numfiles = 0;

   if( argc == 2) {
      portno = atoi(argv[1]);
   }

   /* Create the socket */
   sockfd = socket(AF_INET, SOCK_STREAM, 0); 
   /* Bind a special Port */
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

   bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
   printf("bind socket to port %d...\n", portno);

   listen(sockfd, 0);

   while(1) {
     len = sizeof(clt_addr);
     newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &len);
     
     struct sockaddr_in x = clt_addr;
     Info *info = malloc(sizeof(Info));
     info->addr = clt_addr;
     info->sock = newsockfd;

     ret = pthread_create(&peer,NULL,(void*)(&peerThread),(void*) info);
   }
   return 0;
}

int savefiles(int newsockfd, char* address, char* returnarray){

   char buffer[256];
   char file[256];
   char file2[256];
   char port[256];
   int n;
	memset(buffer, 0, 256);
	n = dorecv(newsockfd, buffer, 256, 20);
	if(n < 0) return -1;
   	printf("SERVER GOT MESSAGE: %s n: %d\n", buffer, n);
	sprintf(port, "%s", buffer);
	sprintf(returnarray, "%s", buffer);
   	int i = 0;
	int count;
	//recv number of files in directory, malloc for that much	
	n = dorecv(newsockfd, buffer, 256, 20);
	if(n < 0) return -1;
	count = atoi(buffer);
	numfiles += count;
    //   //if list empty, malloc for root node
      if(peers == NULL){
      peers = (struct PeerNode*)malloc(sizeof(struct PeerNode));
	  sprintf(peers->port, "%s", port);
	  sprintf(peers->hostname, "%s", address);
      peers->files = (Filename*)malloc(count*sizeof(Filename));
      peers->filesmaxsize = count;
      peers->filessize = 0;

      //receive filenames
      while(1){
	   n = dorecv(newsockfd, buffer, 256, 255);
	   if(n < 0) break;
      if(buffer[0] == EOF) break;
		sprintf(peers->files[i].name, "%s", buffer);
      peers->filessize++;
      i++;
      }

      peers->next = NULL;
	   peerssize++;
      return 0;
   }

   //go to last index in list
   struct PeerNode* curr = peers;
   while(curr->next != NULL){
      curr= curr->next;
   }
   curr->next = (struct PeerNode*)malloc(sizeof(struct PeerNode));
	sprintf(curr->next->port, "%s", port);
	sprintf(curr->next->hostname, "%s", address);
   curr->next->files = (Filename*)malloc(count*sizeof(Filename));
   curr->next->filesmaxsize = count;
   curr->next->filessize = 0;

   while(1){
	n = dorecv(newsockfd, buffer, 256, 255);
	if(n < 0) break;
      printf("SERVER GOT MESSAGE: %s n: %d\n", buffer, n);
      if(buffer[0] == EOF) break;
	   sprintf(curr->next->files[i].name, "%s", buffer);
      curr->next->filessize++;
      i++;
   }

   curr->next->next = NULL;
	peerssize++;
	printf("%d\n", peerssize);
	printf("out\n");
   return 0;
}

void peerThread(void * arg){

   Info *info = (Info*) arg;
   int newsockfd = info->sock;
   char port[256];
   int x = savefiles(newsockfd, inet_ntoa(((struct sockaddr_in)info->addr).sin_addr), port);
   char buffer[256];
   int n;
   while(1){       
      n = dorecv(newsockfd, buffer, 256, 256);
      if(n == -2) {
         removepeer(inet_ntoa(((struct sockaddr_in)info->addr).sin_addr), port);
         printf("Peer Connection Terminated\n");
         return;
      }
      printf("SERVER GOT MESSAGE: %s n: %d\n", buffer, n);
      if(strcmp(buffer,"list") == 0){
         sendlist(newsockfd);
      }else if(strncmp(buffer,"download ", 9) == 0){
		//peer downloaded a file so update the list
         printf("removing files\n");
         removepeer(inet_ntoa(((struct sockaddr_in)info->addr).sin_addr), port);
         printf("done removing files\n");
         int x = savefiles(newsockfd, inet_ntoa(((struct sockaddr_in)info->addr).sin_addr), port);
         // if(x == -1){ printf("error\n"); return; }
         printf("done saving files\n");

      }else if(strcmp(buffer,"exit") == 0){
         removepeer(inet_ntoa(((struct sockaddr_in)info->addr).sin_addr), port);
         printf("connection terminated\n");
         break;

      }else{

         sprintf(buffer, "unrecognized command %s", buffer);
      }
   }
}

int getindex(char* buffer){
   char temp[256];
   int i = 9;
   int j = 0;
   while(buffer[i] != '\0'){
      temp[j] = buffer[i];
      j++; i++;
   }
   temp[j] = '\0';
   int z = atoi(temp);
   if(z == 0 && buffer[9] != '0')
      return -1;
   return z;
}

void sendlist(int newsockfd){

   char buffer[256];
   int n;
   struct PeerNode* curr = peers;
   int i = 0;
	//send number of total files so peer can malloc
	sprintf(buffer, "%d", numfiles);
         n = send(newsockfd, buffer, 20, 0);
         if(n < 0){ perror("can't send to server"); return; }
	printf("sent numfiles: %s\n", buffer);

	sprintf(buffer, "%d", peerssize);
         n = send(newsockfd, buffer, 20, 0);
         if(n < 0){ perror("can't send to server"); return; }
   while(curr != NULL){
      int j = 0;
	sprintf(buffer, "%d", curr->filessize);
         n = send(newsockfd, buffer, 20, 0);
         if(n < 0){ perror("can't send to server"); return; }
      while(j < curr->filessize){

	sprintf(buffer, "%s", curr->files[j].name);
         n = send(newsockfd, buffer, 255, 0);
         if(n < 0){ perror("can't send to server"); return; }

	sprintf(buffer, "%s", curr->hostname);
         n = send(newsockfd, buffer, 255, 0);
         if(n < 0){ perror("can't send to server"); return; }

	  sprintf(buffer, "%s", curr->port);
         n = send(newsockfd, buffer, 255, 0);
         if(n < 0){ perror("can't send to server"); return; }


         sprintf(buffer, "[%d] %s %s %s", i, curr->files[j].name, curr->hostname, curr->port);
         printf("%s\n",buffer);
         n = send(newsockfd, buffer, 255, 0);
         if(n < 0){ perror("can't send to server"); return; }
         j++; i++;

      }
      curr= curr->next;
   }

}

void removepeer(char* address, char* port){
   
    printf("in removepeers with address: %s and port: %s\n", address, port);
    
    struct PeerNode* curr = peers;
   if(peers == NULL){
      return;
   }
   //removing root
   if(strcmp(curr->hostname, address) == 0 && strcmp(curr->port, port) == 0){
	numfiles -= peers->filesmaxsize;
      peers = curr->next;
	peerssize--;
      return;
   }
   while(curr->next != NULL){
      if(strcmp(curr->next->hostname, address) == 0 && strcmp(curr->next->port, port) == 0){
	numfiles -= curr->filesmaxsize;
         curr->next = curr->next->next;
	  peerssize--;
         return;
      }
      curr = curr->next;
   }
}

int dorecv(int sock, char* buffer, int buffersize, int bytestoread){

	char buffer2[buffersize];
	int n;
	int bytesread = 0;
	n = recv(sock, buffer, bytestoread, 0);
	bytesread += n;
	while(n < bytestoread){
		bytestoread -= n;
		n = recv(sock, buffer2, bytestoread, 0);
		if(n < 0){ perror("can't receive from server"); return -1; }
		if(n == 0){ return -2; }
		bytesread += n;
		sprintf(buffer, "%s%s", buffer, buffer2);
	}

	return bytesread;

}



















