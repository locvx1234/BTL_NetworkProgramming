#include <stdio.h>	//printf
#include <string.h>	//strcpy, strcmp, strncpy, strtok
#include <stdlib.h>	//exit, malloc
#include <netinet/in.h>	// ??
#include <arpa/inet.h>	//inet_ntop
#include <sys/types.h>	// ??
#include <sys/socket.h>	// ??
#include <unistd.h>	//write, read, close
#include <pthread.h>	//pthread_create, pthread_detach
//
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

#define NAME_SIZE 32
#define DATA_SIZE 1024
#define MTU 1200
#define PORT 5000

void *send_handler(void *sock);
void *receive_handler(void *sock);
void * sendfile(void * sock);
void downfile(int sock);
void *get_in_addr(struct sockaddr *sa);

//Global Variable
unsigned char status = 0;	//0 la o ngoai room chat, 1 la da vao trong room chat

int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stdin, "Enter IPv4 Address\n");
		exit(1);
	}
    
	//Create a socket
	int sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("Error when create!\n");
		exit(1);
	}

	//Initialize sockaddr_in data structure
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	puts("Connecting...\n");
	

	//Attempt a connection
	if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr))<0) {
		perror("Error: Connect Failed \n");
		exit(1);
	} else {
		puts("Connected\n");
	}
	
	char username[NAME_SIZE];
	puts("Import username:");
	fgets(username, sizeof(username), stdin);
	fflush(stdin);
	write(sockfd, username, sizeof(username));
	
	pthread_t send, recv;
	if (pthread_create(&recv, NULL, receive_handler, &sockfd) < 0) {   
		perror("Could not create thread receive");
		exit(1);
	} else {
		pthread_join(recv, NULL);
	}
	if (pthread_create(&send, NULL, send_handler, &sockfd) < 0) {   
		perror("Could not create thread send");
		exit(1);
	} else {
		pthread_join(send, NULL);
	}
	
	close(sockfd);
	return 0;
}

void *send_handler(void *socket) {
	//Tao giao dien
	puts("Connected\n");
	puts("List Main Menu");
	puts("-------------------------------------------------------------------------------------------");
	puts("|'@exit' to quit							'@create' to create new topic		|");
	puts("|'@s' to send File						'@join' to join a existed topic		|");
	puts("|'@f' to download File				'@listonline' to list all users online	|");
	puts("|'@listtopic' to list all topic'			'@listuser' to list...				|");

	puts("===========================================================================================");
	puts("Connected Chat");
	printf("Me				Myfriend\n");
	printf("=======================================================================================\n");
	
	//Nhap va xu ly message
	char buffer[DATA_SIZE];
	for (;;) {
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);		//Client nhap lenh, hoac nhap cau chat
		if (status == 0) {				//Client chua tham gia room chat
			if (strcmp(buffer, "@create")) {			//0
				puts("Import Topic's name:");

			} else if (strcmp(buffer, "@join")) {		//2

			} else if (strcmp(buffer, "@listonline")) {	//3

			} else if (strcmp(buffer, "@listtopic")) {	//6

			} else if (strcmp(buffer, "@exit")) {		//9

			}
		} else {					//Client dang o trong room chat
			if (strcmp(buffer, "@invite")) {		//1

			} else if (strcmp(buffer, "@listonline")) {	//3

			} else if (strcmp(buffer, "@listuser")) {	//4

			} else if (strcmp(buffer, "@listfile")) {	//5

			} else if (strcmp(buffer, "@listtopic")) {	//6
			
			} else if (strcmp(buffer, "@out")) {		//8
	
			} else if (strcmp(buffer, "@exit")) {		//9
			
			} else if (strcmp(buffer, "@upfile")) {		//b

			} else if (strcmp(buffer, "@downfile")) {	//c

			} else {					//a chat
				int count = 0;
				while(count < strlen(username))
				{
				    message[count] = username[count];
				    count++;
				}
				count--;
				message[count] = ':';
				count++;
				for(int i = 0; i < strlen(buff); i++)
				{
				    message[count] = buff[i];
				    count++;
				}
				message[count] = '\0';
				if(send(socket, message, strlen(message), 0) < 0)
				{
				    puts("Send failed");
				    exit(1);
				}
				    memset(&buff, sizeof(buff), 0);
				}   
			}
		
	
    
	return 0;
}

//revcmessage thread function
void *receive_handler(void *connfd) {
	int socket = *((int*)connfd), nbytes;
	char message[MTU];
	
	for (;;) {
		memset(message, 0, sizeof(message));
		read(socket, message, sizeof(message));
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if (command == '0') {		//nhan message
			puts(message);
		} else if (command == '1') {	//nhan file
			downfile(socket, message);
		}
	}
	return 0;
}
//sendfile from client to server
//editting...
void * sendfile(void * sock){
	int connfd = *(int*)sock;
	char fileName[256];
	scanf("%s", fileName);
	bzero(fileName,256);
	while(1){
			write(connfd, fileName, 256);
            printf("\nClient want to sendfile : %s. \n", fileName);
            
            FILE *fp;
        	fp = fopen(fileName,"rb");
            if(fp==NULL){
		        printf("File open error or not exist file.\n");
		        write(connfd, "error", sizeof("error"));
                continue;
            }else{
 				int nread;
    // send content file
        		char contentFile[255] = {0};
        		do{
    /* Read file in chunks of 256 bytes */
		    		nread=fread(contentFile, 1, 256, fp);
		    		write(connfd, contentFile, nread);
        		}while(nread >= sizeof(contentFile));

		        if (nread < 256){
		            if (feof(fp))
		                printf("Send file successfull.\n");
		            if (ferror(fp))
		                printf("Error reading file.\n");
		        }
            }
            	fclose(fp);
        }
}
//downloadfile from server funtion
void downfile(int socket) {
    int bytesReceived = 0;
    char recvBuff[256], fileName[256];
    memset(recvBuff, '0', sizeof(recvBuff));
	while(1){
        	memset(recvBuff, 0, sizeof(recvBuff));
		printf("\nEnter file name to download: ");
		scanf("%s", fileName);
        	fflush(stdin);
		printf("Request file : %s to server.\n", fileName);
		write(socket, fileName, sizeof(fileName));
	        if(strcmp(fileName,"@") == 0){
	        	printf("outting downloadfile!\n");
		    	break;
	    	}
        
    		FILE *fp;	    
    		fp = fopen(fileName, "wb"); 
	   	do {
			  memset(recvBuff, 0, sizeof(recvBuff));
		  	  bytesReceived=read(socket, recvBuff, sizeof(recvBuff));
		  	  //printf("%d", bytesReceived);
			  if(strcmp(recvBuff,"error") == 0){
		      		memset(recvBuff, 0, sizeof(recvBuff));
		      		printf("File name doesn't exist in your server or invalid. \n");
		      	  	continue;
		          }
			  else{
		          //printf("Bytes received %d\n",bytesReceived);
		          	fwrite(recvBuff, 1,bytesReceived,fp);
               		  }
	   	}while(bytesReceived >= 256);
	   
	   	fclose(fp);
     
	        if(bytesReceived < 0){
		    printf("Read Error \n");
	        }   	
	}
	
	close(socket);
}
