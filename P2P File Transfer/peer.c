#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <dirent.h>
#include <pthread.h>
#include <ifaddrs.h>

typedef struct {
   int portno;
   char hostname[256];
   char filename[256];
   int serversock;
   char peerserverport[256];
} Info;

typedef struct {
   char name[256];
   char address[256];
   char port[256];
} File;

void serverThread(void * arg);
void sendFileThread(void * arg);
void recvFileThread(void * arg);
void dogetserver(int newsockfd, char *filename);
void dogetclient(int sockfd, char *filename);
void sendfilenames(int sockfd, char*);
void recvlist(int sockfd, File* files, int*, int*);
int getindex(char* buffer);
int dorecv(int sock, char* buffer, int buffersize, int bytestoread);
int newrecv(int sock, char* buffer, int buffersize, int bytestoread);

pthread_t peerserver, sendFile, recvfile;

int main(int argc, char* argv[])
{
   int sockfd, portno, portno2, ret, n;
   struct hostent* server;
   struct sockaddr_in serv_addr;
   char buffer[256];
   char filename[256];
	if(argc == 2){
		portno = 5000;
		portno2 = 6000;
		printf("%d %d\n", portno, portno2);
	}else if(argc == 3){
		portno = atoi(argv[2]);
		portno2 = 6000;
	}else if(argc == 4){
		portno = atoi(argv[2]);
		portno2 = atoi(argv[3]);
		printf("%d %d\n", portno, portno2);
	}else{
		fprintf(stderr,"Usage: %s <server-hostname> [server-port-number] [peer-port-number]\n", argv[0]);
    return 1;
	}

   /*Create  the socket */
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   /* Socket address struct */   
   server = gethostbyname(argv[1]);  
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
   serv_addr.sin_port = htons(portno);

   /* Try to connect */ 
   connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
	printf("Connected to %s, successful\n", server->h_name);

   //send filenames to server
	char portno2string[10];
	sprintf(portno2string,"%d",portno2);
   sendfilenames(sockfd, portno2string);

   //thread for peer server to accept connections from peers
   ret = pthread_create(&peerserver,NULL,(void*)(&serverThread),(void*)(long)portno2);
 
   File* files = (File*)malloc(200*sizeof(File));
   int filesmaxsize = 200;
   int filessize = 0;

   while(1){

      fgets(buffer, 255, stdin);
      n = strlen(buffer);
      if(n>0 && buffer[n-1] == '\n') buffer[n-1] = '\0';
      if(strcmp(buffer,"list") == 0){
         n = send(sockfd, buffer, sizeof(buffer), 0);
         recvlist(sockfd, files, &filesmaxsize, &filessize);

      }else if(strncmp(buffer,"download ", 9) == 0){

         int index = getindex(buffer);
         if(index == -1){
            printf("error: Usage 'download [fileindex]'\n");
            continue;
         }
         if(index > filessize-1){
            //error user specified index that doesn't exist
            printf("error: [fileindex] not found, use command list to refresh your list and pass an existing [fileindex]\n");
            continue;
         }
         int check = 0;
         struct ifaddrs *addrs;
         getifaddrs(&addrs);
         struct ifaddrs *tmp = addrs;
         while (tmp) 
         {
             if (tmp->ifa_addr && tmp->ifa_addr->sa_family == AF_INET)
             {
                 struct sockaddr_in *pAddr = (struct sockaddr_in *)tmp->ifa_addr;
                 if(strcmp(inet_ntoa(pAddr->sin_addr), files[index].address) == 0 && strcmp(portno2string, files[index].port) == 0)
                    check = 1;
            }
             tmp = tmp->ifa_next;
         }
         freeifaddrs(addrs);

         Info *info = malloc(sizeof(Info));
         info->portno = atoi(files[index].port);
         strcpy(info->hostname, files[index].address);
         strcpy(info->filename, files[index].name);

         info->serversock = sockfd;
         strcpy(info->peerserverport, portno2string);
         //create thread to connect to peer and recv file
         ret = pthread_create(&recvfile,NULL,(void*)(&recvFileThread),(void*)info);
         if(ret != 0)
            printf("Peer download create thread failed!\n");
         } else if(strcmp(buffer,"exit") == 0){

			free(files);

         n = send(sockfd, buffer, strlen(buffer), 0);
         break;
      } else{
         printf("Unrecognized command %s\n\n", buffer);

      }

   }  //end while

  close(sockfd);
  return 0;
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

void recvlist(int sockfd, File* files, int *filesmaxsize, int *filessize){

   *filessize = 0;
   int n;
   char buffer[256];
	char* p = buffer;
	memset(buffer, '\0', sizeof(buffer));
   int i = 0;
	int z = 0;
	int count;
	p = buffer;
	n = newrecv(sockfd, p, 256, 20);
	if(n < 0) return;
	count = atoi(buffer);
	p = buffer;
	n = newrecv(sockfd, p, 256, 20);
	if(n < 0) return;
	int peerssize = atoi(buffer);
   while(i < peerssize){
		int j = 0;
		p = buffer;
		n = newrecv(sockfd, p, 256, 20);
		if(n < 0) break;
		int filessizesent = atoi(buffer);
		while(j < filessizesent){
			p = buffer;
			n = newrecv(sockfd, p, 256, 255);
			if(n < 0) break;
			sprintf(files[z].name, "%s", buffer);
			p = buffer;
			n = newrecv(sockfd, p, 256, 255);
			if(n < 0) break;
			sprintf(files[z].address, "%s", buffer);
			p = buffer;
			n = newrecv(sockfd, p, 256, 255);
			if(n < 0) break;
			sprintf(files[z].port, "%s", buffer);
			(*filessize)++;
			p = buffer;
			n = newrecv(sockfd, p, 256, 255);
			if(n < 0) break;

			   printf("%s\n", buffer);
			j++; z++;
		}
		i++;
   }
	printf("\n");

}

void sendfilenames(int sockfd, char* portno){

   char buffer[256];
   int n;
	memset(buffer, 0, 256);
   //send peer server port number
	sprintf(buffer, "%s", portno);
   n = send(sockfd, buffer, 20, 0);
   DIR *d;
   struct dirent *dir;
	//send number of files to server
	int count = 0;
   d = opendir(".");
   if (d)
   {
      while ((dir = readdir(d)) != NULL){
         if (dir->d_type == DT_REG){
		   count++;
         }
      }
      closedir(d);
   }
	sprintf(buffer, "%d", count);
	n = send(sockfd, buffer, 20, 0);
	d = opendir(".");
   if (d)
   {
      while ((dir = readdir(d)) != NULL){
         if (dir->d_type == DT_REG){
		      sprintf(buffer, "%s", dir->d_name);
            n = send(sockfd, buffer, 255, 0);      
         }
      }
      closedir(d);
   }

   buffer[0] = EOF;
   n = send(sockfd, buffer, 255, 0);
   if(n < 0){ perror("can't send to server"); return; }
}

//thread for peer to send filename to other peer and recv file
void recvFileThread(void * arg){

   Info *info = (Info*) arg;
   int sockfd, portno, ret, n;
   struct hostent* server;
   struct sockaddr_in serv_addr;
   char buffer[256];   
   /*Create  the socket */
   sockfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

   server = gethostbyname(info->hostname);
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr = *((struct in_addr*)server->h_addr);
   serv_addr.sin_port = htons(info->portno);

   connect(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
   //sendfilename
   n = send(sockfd, info->filename, sizeof(info->filename), 0);
      dogetclient(sockfd, info->filename);

   //send confirmation of file downloaded for tracker to update list
   //tell server we are updating list
   strcpy(buffer, "download ");
   n = send(info->serversock, buffer, sizeof(buffer), 0);
   if(n < 0){ perror("can't send to server"); return; }

   //printf("sending filenames with server port: %s\n", info->peerserverport);
   sendfilenames(info->serversock, info->peerserverport);

   close(sockfd);

}

//thread for peer server to accept connections from peers
void serverThread(void * arg){

   int sockfd, newsockfd, portno;
   struct sockaddr_in serv_addr, clt_addr;
   socklen_t addrlen;
   int ret;

   portno = (int)(long) arg;
   sockfd = socket(AF_INET, SOCK_STREAM, 0); 
   
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_addr.s_addr = INADDR_ANY;
   serv_addr.sin_port = htons(portno);

   bind(sockfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
  
   listen(sockfd, 5);

   while(1) {
     printf("Servicing and waiting for download requests on port %d.\n", portno);
     addrlen = sizeof(clt_addr);
     newsockfd = accept(sockfd, (struct sockaddr*)&clt_addr, &addrlen);
     if(newsockfd < 0){ perror("can't accept"); continue; }
      ret = pthread_create(&sendFile,NULL,(void*)(&sendFileThread),(void*)(long) newsockfd);
   }

}

void sendFileThread(void * sock){

   char buffer[256];
   char filename[256];
   int n;
   int newsockfd = (int)(long) sock;

   n = recv(newsockfd, filename, 256, 0);
   if(n < 0){ perror("can't receive from server"); exit(-1); }
   if(n == 0){ perror("server terminated"); exit(-1); }

   dogetserver(newsockfd, filename);
   close(newsockfd);

}

void getfilename(char *buffer, char *filename){
   int x = 9;
   int y = 0;
   while(x < strlen(buffer)){
      filename[y] = buffer[x];
      x++; y++;
   }
   filename[y] = '\0';
}

void dogetserver(int newsockfd, char *filename){

   FILE *fp;
   char buffer[256];
   int n;

	if ((fp = fopen(filename,"r")) == NULL){
      sprintf(buffer, "Can't open file %s", filename);
      n = send(newsockfd, buffer, sizeof(buffer), 0);
      return;
	}

   //send confirmation of file existence
   sprintf(buffer, "file %s open", filename);
   n = send(newsockfd, buffer, sizeof(buffer), 0);

   int fd, count_r, count_w;
   char buf[256];
   char* bufptr;

   fseek(fp,0,SEEK_END);
   uint32_t len = ftell(fp);
   uint32_t len_n = htonl(len);
   fclose(fp);
   n = send(newsockfd, (char*) &len_n, 4, 0);
   if(n < 0){ perror("can't send to client"); return;}
   fd = open(filename, O_RDONLY);
   if(fd == -1){ printf("file open error"); return;}

   while((count_r = read(fd, buf, 256))>0){
      count_w = 0;
      bufptr = buf;
      while (count_w < count_r){
	      count_r -= count_w;
	      bufptr += count_w;
	      count_w = send(newsockfd, bufptr, count_r, 0);
	      if (count_w == -1) { perror("Socket write error"); return;}
      }
   }
}

void dogetclient(int sockfd, char *filename){

   FILE *fp;
   char buffer2[256];
   memset(buffer2, '\0', 256);
   int n;

   //recieve confirmation of file existence
   n = recv(sockfd, buffer2, 256, 0);
   if(n < 0){ perror("can't receive from server"); return; }
   else buffer2[n-1] = '\0';

   if(buffer2[0] != 'f'){
      //file not found
      printf("%s\n\n", buffer2);
      return;
   }

   if((fp = fopen(filename,"wb")) == NULL){ perror("File create error"); return; }
   fclose(fp);

   int fd, count, count_w, count_r;
   char buf[256];
   char* bufptr;

   uint32_t len;
   recv(sockfd, &len,4,0);
   if(n < 0){ perror("can't receive from server"); return;}
   len = ntohl(len);

   fd = open(filename, O_WRONLY);
   if(fd == -1){ printf("file open error\n\n"); return;}

   while (len != 0 && (count = recv(sockfd, buf, 256, 0))>0){
      count_r = count;
      count_w = 0;
      bufptr = buf;
      while (count_w < count_r){
         count_r -= count_w;
         bufptr += count_w;
         count_w = write(fd, bufptr, count_r);
         if (count_w == -1) { perror("write error"); return; }
      }// end while2
      len -= count;
   }// end while1
   if (count == -1){ perror("recieve error"); return; }
   printf("'%s' downloaded successfully\n\n", filename);

}

int dorecv(int sock, char* buffer, int buffersize, int bytestoread){

	char buffer2[buffersize];
	int n;
	int bytesread = 0;

	n = recv(sock, buffer, bytestoread, 0);
	if(n < 0){ perror("can't receive from server"); return -1; }
	if(n == 0){ perror("server terminated"); return -1; }
	if(buffer[strlen(buffer)] == '\0'){ printf("true\n"); }
	bytesread += n;

	while(n < bytestoread){
		bytestoread -= n;
		n = recv(sock, buffer2, bytestoread, 0);
		if(n < 0){ perror("can't receive from server"); return -1; }
		if(n == 0){ perror("server terminated"); return -1; }
		bytesread += n;
		sprintf(buffer, "%s%s", buffer, buffer2);
	}

	buffer[bytesread-1] = '\0';
	return bytesread;

}

int newrecv(int sock, char* buffer, int buffersize, int bytestoread){

	char buffer2[buffersize];
	int n;
	int bytesread = 0;

	n = recv(sock, buffer, bytestoread, 0);
	if(n < 0){ perror("can't receive from server"); return -1; }
	if(n == 0){ perror("server terminated"); return -1; }
	bytesread += n;
	buffer += n;

	while(n < bytestoread){
		bytestoread -= n;
		n = recv(sock, buffer2, bytestoread, 0);
		if(n < 0){ perror("can't receive from server"); return -1; }
		if(n == 0){ perror("server terminated"); return -1; }
		bytesread += n;
		buffer += n;
	}

	return bytesread;

}






















