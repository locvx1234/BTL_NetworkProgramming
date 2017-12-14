#include <stdio.h> 			//printf
#include <string.h>			//strcpy, strcmp, strncpy, strtok, strstr, strcat
#include <stdlib.h>			//exit, malloc
#include <netinet/in.h>		//??
#include <arpa/inet.h>		//inet_ntop
#include <sys/types.h>		//??
#include <sys/socket.h>		//??
#include <unistd.h>			//write, read, close
#include <pthread.h>		//pthread_create, pthread_detach

#include <errno.h>
#include <netdb.h>
#include <sys/select.h>
#include <time.h>

//Static Define
#define NAME_SIZE	32
#define DATA_SIZE	1024
#define MTU 		1200
#define PORT 		50001
#define	SA 			struct sockaddr

//Function Declare
static void * send_handler( void *connfd );
static void * receive_handler( void *connfd );
void sendCommand( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]);
void sendCommands( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]);
void showMainMenuCommand();
void showTopicCommand();
void sendFile( int sockfd, char buffer[DATA_SIZE] );
void downFile( int sockfd, char buffer[DATA_SIZE] );
void commandPrompt();
char *nameStandardize( char str[MTU] );
char *stringStandardize( char str[MTU] );
void delay(unsigned int mseconds);

//Global Variable
char username[NAME_SIZE], title[NAME_SIZE] = "";
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
	
	char message[MTU];
	for( ; ; ) {
		printf( "Import username: " );
		fgets(username, sizeof(username), stdin);
		fflush(stdin);
		strcpy(username, nameStandardize(username));
		if( strlen(username) > 0) {
			write(connfd, username, strlen(username));
			memset(message, 0, sizeof(message));
			read(connfd, message, sizeof(message));
			if( strcmp(message, "Duplicate") == 0 ) {
				puts( "Notification from server: Username is duplicate!\n" );
				memset(username, 0, sizeof(username));
			} else {
				puts( "Username accepted! You're online now!\n" );
				break;
			}
		}
	}

	pthread_t send_tid, recv_tid;
  	pthread_create(&recv_tid, NULL, &receive_handler,(void *) &connfd);
	pthread_create(&send_tid, NULL, &send_handler,(void *) &connfd);
	pthread_join(recv_tid, NULL);
	pthread_join(send_tid, NULL);
	
	close(connfd);
	return 0;
}

static void *send_handler( void *connfd ) {
	int sockfd = *((int*)connfd);
	char buffer[DATA_SIZE];
	for( ; ; ) {
		commandPrompt();
		memset(buffer, 0, sizeof(buffer));
		fgets(buffer, sizeof(buffer), stdin);	//Client nhap lenh, hoac nhap cau chat
		fflush(stdin);
		if( buffer[0] != '\n') {
			if (buffer[strlen(buffer)-1] == '\n')
				buffer[strlen(buffer)-1] = '\0';
			char command[DATA_SIZE];
			strcpy(command, buffer);
			strtok(command, " ");
			if( strcmp(title, "") == 0 ) {			//Client chua tham gia chatroom
				if( strcmp(command, "@create" ) == 0 ) {			//Command create = 0
					sendCommand(sockfd, "0", 8, buffer);
				}
				else if( strcmp(command, "@join") == 0 ) {			//Command join = 2
					sendCommand(sockfd, "2", 6, buffer);
				}
				else if( strcmp(command, "@listonline") == 0 ) {	//Command listonline = 3
					write(sockfd, "3", 1);
				}
				else if( strcmp(command, "@listchatroom") == 0 ) {	//Command listchatroom = 6
					write(sockfd, "6", 1);
				}
				else if( strcmp(command, "@help") == 0 ) {			//Command help = 7
					showMainMenuCommand();
				}
				else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
					write(sockfd, "9", 1);
					exit(1);
				}
				else {
					printf( "--Invalid Command. Type @help.\n\n" );
				}
			}
			else {									//Client da tham gia chatroom
				if( strcmp(command, "@invite") == 0 ) {				//Command invite = 1
					sendCommands(sockfd, "1", 8, buffer);
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
				else if( strcmp(command, "@listchatroom") == 0 ) {	//Command listchatroom = 6
					write(sockfd, "6", 1);
				}
				else if( strcmp(command, "@help") == 0 ) {			//Command help = 7
					showTopicCommand();
				}
				else if( strcmp(command, "@out") == 0 ) {			//Command out = 8
					write(sockfd, "8", 1);
					strcpy(title, "");
					printf("\n");
				}
				else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
					write(sockfd, "9", 1);
					exit(1);
				}
				else if( strcmp(command, "@upfile") == 0 ) {		//Command upfile = b
					sendFile(sockfd, buffer);
				}
				else if( strcmp(command, "@downfile") == 0 ) {		//Command downfile = c
					downFile(sockfd, buffer);
				}
				else {												//Command chat = a
					char message[MTU] = "a";
					strcat(message, username);
					strcat(message, ": ");
					strcat(message, buffer);
					write(sockfd, message, strlen(message));
				}   
			}
		}
		delay(50000);
	}
	return 0;
}

//revcmessage thread function
static void *receive_handler( void *connfd ) {
	int sockfd = *((int*)connfd);
	char message[MTU];
	while( read(sockfd, message, sizeof(message)) > 0 ) {
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if( command == '0' ) {				//0 -> nhan command thong bao
			puts(message);
		}
		else if( command == '1' ) {			//1 -> nhan command khi create chatroom thanh cong
			strcpy(title, message);
			printf( "Chatroom %s created success!\n\n", title);
		}
		else if( command == '2' ) {			//2 -> nhan command khi bi thang ngu nao do keo minh vao chatroom
			strcpy(title, message);
			printf( "You have been invited to chatroom %s\n\n", title);
		}
		else if( command == '3' ) {			//3 -> tu minh join room thanh cong
			strcpy(title, message);
			printf( "You are now in chatroom %s.\n\n", title);
		}
		else if( command == '4' ) {		//4 tuc la server chuan bi gui file cho minh
			char fileName[NAME_SIZE];			
			strncpy(fileName, message - 5, 5);
			strcpy(fileName, message);
			puts(fileName);
			int bytesReceived = 0;
		    	char recvBuff[256];
		    	memset(recvBuff, '0', sizeof(recvBuff));
			FILE *fp = fopen(fileName, "wb"); 
		   	do {
				  memset(recvBuff, 0, sizeof(recvBuff));
			  	  bytesReceived = read(sockfd, recvBuff, sizeof(recvBuff));
				  //printf("%d", bytesReceived);
				  if( strcmp(recvBuff,"error") == 0 ){
			      		memset(recvBuff, 0, sizeof(recvBuff));
			      		printf("\nFile name doesn't exist in your server or invalid. \n");
			      	  	continue;
			      	  } else {
				  	fwrite(recvBuff, 1,bytesReceived,fp);
		      		  }
		   	} while( bytesReceived >= 256 );	   
			fclose(fp);
			if( bytesReceived < 0 ){
			    	printf("Read Error \n");
			}
		}
		
		memset(message, 0, sizeof(message));
	}
	return 0;
}

void sendCommand( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]) {
	char message[MTU];
	strcpy(message, command);
	strncpy(buffer, buffer + skip, strlen(buffer));
	strcat(message, nameStandardize(buffer));
	write(sockfd, message, strlen(message));
}

void sendCommands( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]) {
	char message[MTU];
	strcpy(message, command);
	strncpy(buffer, buffer + skip, strlen(buffer));
	strcpy(buffer, stringStandardize(buffer));
	strcat(message, buffer);
	write(sockfd, message, strlen(message));
}

void showMainMenuCommand() {
	puts("\t@create <chatroom_name>			create a new chatroom");
	puts("\t@join <chatroom_name>			join an existed chatroom");
	puts("\t@listonline			list all users online");
	puts("\t@listchatroom			list all chatroom");
	puts("\t@help				list command");
	puts("\t@out				leave chatroom");
	puts("\t@exit				exit program\n");
}

void showTopicCommand() {
	puts("\t@invite <user_name>			invite 1 or many user");
	puts("\t@listonline			list all users online");
	puts("\t@listuser			list all users in chatroom");
	puts("\t@listfile			list all uploaded files in chatroom");
	puts("\t@listchatroom			list all chatroom");
	puts("\t@upfile <file_name>			upload a file");
	puts("\t@downfile <file_name>		download a file");
	puts("\t@help				list command");
	puts("\t@out				leave chatroom");
	puts("\t@exit				exit program\n");
}

void sendFile( int sockfd, char buffer[NAME_SIZE] ) {
	char fileName[DATA_SIZE];
	strncpy(fileName, buffer + 8, strlen(buffer));
	strcpy(fileName, nameStandardize(fileName));
	char message[MTU] = "b";
	strcat(message, fileName);
	write(sockfd, message, strlen(message));

    	FILE *fp = fopen(fileName, "rb");
    	if( fp == NULL ) {
	        printf("File open error or not exist file.\n");
	        write(sockfd, "error", sizeof("error"));
    	} else {
		int nread;
		char contentFile[255] = {0};
		do {
	    		nread = fread(contentFile, 1, 256, fp);
	    		write(sockfd, contentFile, nread);
		} while( nread >= 256 );

	        if( nread < 256 ){
	            if( feof(fp) )
	                printf("Send file successfull.\n");
	            if( ferror(fp) )
	                printf("Error reading file.\n");
	        }
    	fclose(fp);
    }
}

//downloadfile from server
void downFile( int sockfd, char buffer[NAME_SIZE] ) {
	char fileName[DATA_SIZE];
	strncpy(fileName, buffer + 10, strlen(buffer));
	strcpy(fileName, nameStandardize(fileName));
	char message[MTU] = "c";
	strcat(message, fileName);
	write(sockfd, message, strlen(message));	
}

void commandPrompt() {
	if( strcmp(title, "") == 0 ) {
		printf("Main>");
	} else {
		printf("%s>", title);
	}
}

char *nameStandardize(char str[MTU]) {
  	char *temp = malloc(NAME_SIZE);
  	int i, j = 0;
  	for( i = 0 ; i < strlen(str) ; i++ ) {
    		if( str[i] != '\n' && str[i] != ' ' && str[i] != '\r' && str[i] != '\t' ) {
	      		*(temp+j) = str[i];
	      		j++;
    		}
    		if( j == 32 ) break;
  	}
  	return temp;
}

char *stringStandardize(char str[MTU]) {
  	char *token, *src = malloc(MTU), *des = malloc(MTU);
  	strcpy(src, str);
  	token = strtok(src, " ");
  	while( token != NULL ) {
	    char temp[32];
	    strcpy(temp, token);
	    if( strcmp(nameStandardize(temp), "") != 0 ) {
	      	strcat(des, nameStandardize(temp));
	      	strcat(des, " ");
	    }
	    token = strtok(NULL, " ");
  	}
  	*(des + strlen(des) - 1) = '\0';
  	return des;
}

void delay(unsigned int mseconds) {
    	clock_t goal = mseconds + clock();
    	while( goal > clock() );
}
