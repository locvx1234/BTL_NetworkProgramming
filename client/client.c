#include <stdio.h>	//printf
#include <string.h>	//strcpy, strcmp, strncpy, strtok, strstr, strcat
#include <stdlib.h>	//exit, malloc
#include <netinet/in.h>	// ??
#include <arpa/inet.h>	//inet_ntop
#include <sys/types.h>	// ??
#include <sys/socket.h>	// ??
#include <unistd.h>	//write, read, close
#include <pthread.h>	//pthread_create, pthread_detach
#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>

//Static Define
#define NAME_SIZE	32
#define DATA_SIZE	1024
#define MTU 		1200
#define PORT 		5000
#define	SA 			struct sockaddr

//Function Declare
static void *send_handler( void *connfd );
static void *receive_handler( void *connfd );
void sendCommand( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]);
void sendFile( int sockfd, char fileName[NAME_SIZE] );
void downFile( int sockfd, char fileName[NAME_SIZE] );
void showMainMenu();
void showTopicMenu();
//Global Variable
char username[NAME_SIZE], title[NAME_SIZE] = "", message[MTU];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Main Function
int main( int argc, char *argv[] ) {
	if( argc != 2 ) {
		fprintf( stdin, "Enter IPv4 Address\n" );
		exit(1);
	}
    
	//Create a socket
	int connfd = socket(AF_INET, SOCK_STREAM, 0);
	if( connfd < 0 ) {
		perror( "Error when create!\n" );
		exit(1);
	}else {
		printf( "Socket created...\n" );	
	}

	//Initialize sockaddr_in data structure
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	puts( "Connecting..." );
	
	//Attempt a connection
	if( connect(connfd, (SA*)&serv_addr, sizeof(serv_addr))<0 ) {
		perror( "Error: Connect Failed \n" );
		exit(1);
	} else {
		puts( "Connected\n" );
	}
	
	for( ; ; ) {
		printf( "Import username: " );
		fgets(username, sizeof(username), stdin);
		fflush(stdin);
		if (username[strlen(username)-1] == '\n' || username[strlen(username)-1] == '\r')
			username[strlen(username)-1] = '\0';
		write(connfd, username, strlen(username));
		memset(message, 0, sizeof(message));
		read(connfd, message, sizeof(message));
		if( strcmp(message, "Duplicate") == 0 ) {
			puts( "Notification from server: Username is duplicate!\n" );
		} else {
			puts( "Username accepted! You're online now!" );
			break;
		}
	}

	pthread_t send_tid, recv_tid;
  	pthread_create(&recv_tid, NULL, &receive_handler,(void *) &connfd);
	pthread_create(&send_tid, NULL, &send_handler,(void *) &connfd);
	pthread_join(recv_tid, NULL);
	pthread_join(send_tid, NULL);
	
	close(connfd);
	return 0;

	/*pthread_t send, recv;
	if( pthread_create(&recv, NULL, receive_handler, &connfd) < 0 ) {   
		perror( "Could not create thread receive" );
		exit(1);
	} else {
		pthread_join(recv, NULL);
	}
	if( pthread_create(&send, NULL, send_handler, &connfd) < 0 ) {   
		perror( "Could not create thread send" );
		exit(1);
	} else {
		pthread_join(send, NULL);
	}
	
	close(connfd);
	return 0;*/
}
void showMainMenu(){
		//Interface
	puts("List Main Menu");
	puts("-----------------------------------------------------------");
	puts("| @create <topic name>	: create new topic 				|");
	puts("| @join <topic name>		: join an existed topic 		|");
	puts("| @listonline				: show all users online			|");
	puts("| @listtopic				: show all existed topics		|");
	puts("| @help					: show all commands available	|");
	puts("| @exist 					: exit program					|");
	puts("-----------------------------------------------------------");
}

void showTopicMenu(){
	puts("List Topic Menu");
	puts("---------------------------------------------------------------");
	puts("| @invite <username>			: invite an user 				|");
	puts("| @listonline					: show all users online			|");
	puts("| @listuser					: show all users in topic		|");
	puts("| @listfile					: show all files in topic		|");
	puts("| @listtopic					: show all existed topics		|");	
	puts("| @help						: show all commands available	|");
	puts("| @out						: leave the topic				|");
	puts("| @exit						: exit program					|");
	puts("| @upfile	<filename>			: upload a file to topic		|");
	puts("| @downfile <filename>		: download a file from topic	|");
	puts("---------------------------------------------------------------");
}

static void *send_handler( void *connfd ) {
	int sockfd = *((int*)connfd);

	showMainMenu();
	printf("\nMain>");

	//Nhap va xu ly message
	char buffer[DATA_SIZE];
	for( ; ; ) {
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);	//Client nhap lenh, hoac nhap cau chat
		fflush(stdin);
		if (buffer[strlen(buffer)-1] == '\n' || buffer[strlen(buffer)-1] == '\r')
			buffer[strlen(buffer)-1] = '\0';
		char command[DATA_SIZE];
		strcpy(command, buffer);
		strtok(command, " ");
		if( strcmp(title, "") == 0 ) {			//Client chua tham gia room chat
			if( strcmp(command, "@create" ) == 0 ) {				//Command create = 0
				sendCommand(sockfd, "0", 8, buffer);
			}
			else if( strcmp(command, "@join") == 0 ) {			//Command join = 2
				sendCommand(sockfd, "2", 6, buffer);
			}
			else if( strcmp(command, "@listonline") == 0 ) {		//Command listonline = 3
				write(sockfd, "3", 1);
			}
			else if( strcmp(command, "@listtopic") == 0 ) {		//Command listtopic = 6
				write(sockfd, "6", 1);
			}
			else if( strcmp(command, "@help") == 0 ) {			//Command help = 7
				// write(sockfd, "70", 2);
				showMainMenu();
			}
			else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
				write(sockfd, "9", 1);
				exit(1);
			}
			else {
				printf( "--Invalid Command. Please type @help for more information.\nMain>" );
			}
		}
		else {								//Client dang o trong room chat
			if( strcmp(command, "@invite") == 0 ) {				//Command invite = 1
				sendCommand(sockfd, "0", 8, buffer);
			}
			else if( strcmp(command, "@listonline") == 0 ) {	//Command listonline = 3
				write(sockfd, "3", 1);
			}
			else if( strcmp(command, "@listuser") == 0 ) {		//Command listuser = 4
				write(sockfd, "4", 1);
			}
			else if( strcmp(command, "@listfile") == 0 ) {		//Command listfile = 5
				write(sockfd, "5", 1);
			}
			else if( strcmp(command, "@listtopic") == 0 ) {		//Command listtopic = 6
				write(sockfd, "6", 1);
			}
			else if( strcmp(command, "@help") == 0 ) {			//Command help = 7
				showTopicMenu();
			}
			else if( strcmp(command, "@out") == 0 ) {			//Command out = 8
				write(sockfd, "8", 1);
				strcpy(title, "");
			}
			else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
				write(sockfd, "9", 1);
				exit(1);
			}
			else if( strcmp(command, "@upfile") == 0 ) {		//Command upfile = b
				printf("Chuc nang chua hoan thien!\n");
			}
			else if( strcmp(command, "@downfile") == 0 ) {		//Command downfile = c
				printf("Chuc nang chua hoan thien!\n");
			}
			else {												//Command chat = a
				memset(message, 0, sizeof(message));
				strcat(message, "a");
				strcat(message, username);
				strcat(message, ":");
				strcat(message, buffer);
				write(sockfd, message, strlen(message));
			}   
		}
	}
	return 0;
}

//revcmessage thread function
static void *receive_handler( void *connfd ) {
	int sockfd = *((int*)connfd);
	
	while( read(sockfd, message, sizeof(message)) > 0 ) {
		// printf("Messafe %s xx\n", message );
		// printf("Size %d\n", sizeof(message));
		// printf("Strlen %d\n", strlen(message) );
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if( command == '0' ) {			//0 tuc la nhan message thong thuong
			puts(message);
			if( strcmp(title, "") == 0 ) {
				printf("Main>");
			} else {
				printf("%s>", title);
			}
		} else if (command == '1') {	//1 tuc la nhan command khi bi thang ngu nao do keo vao room
			strcpy(title, message);
			printf( "\nYou have been invited to topic %s\n%s>", title, title );
		}
		else if (command == '2') {		//2 tuc la tu minh join room thanh cong
			// strcpy(title, message);
			printf("co in nra ko thi bao\n ");
			printf( "You are now in topic %s \n", title);
			printf("%s >", title);
		}
		else if (command == '3') {		//3 tuc la server chuan bi gui file cho minh
			pthread_mutex_lock(&mutex);
			downFile(sockfd, message);
			pthread_mutex_unlock(&mutex);
		}
		memset(message, 0, sizeof(message));
	}
	return 0;
}

void sendCommand( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]) {
	memset(message, 0, sizeof(message));
	strcat(message, command);
	strncat(message, buffer + skip, strlen(buffer));
	write(sockfd, message, strlen(message));
}

void sendFile( int sockfd, char fileName[NAME_SIZE] ) {
	/*char fileName[256];
	scanf("%s", fileName);
	bzero(fileName,256);
	while(1){
			write(sockfd, fileName, 256);
            printf("\nClient want to sendfile : %s. \n", fileName);
            
            FILE *fp;
        	fp = fopen(fileName,"rb");
            if(fp==NULL){
		        printf("File open error or not exist file.\n");
		        write(sockfd, "error", sizeof("error"));
                exit(1);
            }else{
 				int nread;
    // send content file
        		char contentFile[255] = {0};
        		do{
    // Read file in chunks of 256 bytes
		    		nread=fread(contentFile, 1, 256, fp);
		    		write(sockfd, contentFile, nread);
        		}while(nread >= sizeof(contentFile));

		        if (nread < 256){
		            if (feof(fp))
		                printf("Send file successfull.\n");
		            if (ferror(fp))
		                printf("Error reading file.\n");
		        }
            }
            	fclose(fp);
        }*/
}

void downFile( int sockfd, char fileName[NAME_SIZE] ) {
    /*int bytesReceived = 0;
    char recvBuff[256], fileName[256];
    memset(recvBuff, '0', sizeof(recvBuff));
	while(1){
        	memset(recvBuff, 0, sizeof(recvBuff));
			printf("\nImport NameFile to download: ");
			scanf("%s", fileName);
        	fflush(stdin);
			printf("Request file : %s to server.\n", fileName);
			write(sockfd, fileName, sizeof(fileName));
			
	        if(strcmp(fileName,"@") == 0){
	        	printf("outting downloadfile!\n");
		    	break;
	    	}
        
    		FILE *fp;	    
    		fp = fopen(fileName, "wb"); 
		   	do {
				  memset(recvBuff, 0, sizeof(recvBuff));
			  	  bytesReceived=read(sockfd, recvBuff, sizeof(recvBuff));
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
	}*/
}