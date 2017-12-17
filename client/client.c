#include <stdio.h> 			//printf
#include <string.h>			//strcpy, strcmp, strncpy, strtok, strstr, strcat
#include <stdlib.h>			//exit, malloc
#include <netinet/in.h>		//??
#include <arpa/inet.h>		//inet_ntop
#include <sys/types.h>		//??types
#include <sys/socket.h>		//??socket
#include <unistd.h>			//write, read, close
#include <pthread.h>		//pthread_create, pthread_detach

#include <errno.h>
#include <netdb.h>
#include <sys/select.h>

//Static Define
#define NAME_SIZE	32
#define DATA_SIZE	1024
#define MTU 		1200
#define PORT 		50001
#define	SA 			struct sockaddr
#define PBSTR "||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||||"
#define PBWIDTH 70

//Function Declare
static void *sendHandler( void *connfd );
static void *receiveHandler( void *connfd );
void sendSingleVariable( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]);
void sendMultiVariables( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]);
void showMainCommand();
void showTopicCommand();
void sendFile( int sockfd, char buffer[DATA_SIZE] );
void downFile( int sockfd, char buffer[DATA_SIZE] );
void commandPrompt();
char *nameStandardize( char str[MTU] );
char *stringStandardize( char str[MTU] );
void printProgress ( double process );

//Global Variable
char username[NAME_SIZE], topicName[NAME_SIZE] = "";
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Main Function
int main( int argc, char *argv[] ) {
	if( argc != 2 ) {
		fprintf(stdin, "Enter IPv4 Address\n");
		exit(1);
	}

	//Create a socket
	int connfd = socket(AF_INET, SOCK_STREAM, 0);
	if( connfd < 0 ) {
		perror("Error when create!\n");
		exit(1);
	}else {
		puts("Socket created...");	
	}

	//Initialize sockaddr_in data structure
	struct sockaddr_in serv_addr;
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	serv_addr.sin_port = htons(PORT);

	puts("Connecting...");
	
	//Attempt a connection
	if( connect(connfd, (SA*)&serv_addr, sizeof(serv_addr)) < 0 ) {
		perror("Error: Connect Failed\n");
		exit(1);
	} else {
		puts("Connected\n");
	}
	
	char message[MTU];
	for( ; ; ) {
		printf("Import username: ");
		fgets(username, sizeof(username), stdin);
		fflush(stdin);
		strcpy(username, nameStandardize(username));
		if( strlen(username) > 0) {
			write(connfd, username, strlen(username));
			memset(message, 0, sizeof(message));
			read(connfd, message, sizeof(message));
			if( strcmp(message, "Duplicate") == 0 ) {
				puts("Notification from server: Username is duplicate!\n");
				memset(username, 0, sizeof(username));
			} else {
				puts("Username accepted! You're online now!\n");
				break;
			}
		}
	}

	pthread_t send_tid, recv_tid;
	pthread_create(&recv_tid, NULL, &receiveHandler,(void *) &connfd);
	pthread_create(&send_tid, NULL, &sendHandler,(void *) &connfd);
	pthread_join(recv_tid, NULL);
	pthread_join(send_tid, NULL);
	
	close(connfd);
	return 0;
}

//Send Mess (command)
//sendHandler pthread Funtion
static void *sendHandler( void *connfd ) {
	int sockfd = *((int*)connfd);
	char buffer[DATA_SIZE];
	for( ; ; ) {
		usleep(500);
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
			if( strcmp(topicName, "") == 0 ) {			//Client chua tao hoac tham gia chatroom
				if( strcmp(command, "@create" ) == 0 ) {			//Command create = 0
					sendSingleVariable(sockfd, "0", 8, buffer);
				}
				else if( strcmp(command, "@join") == 0 ) {			//Command join = 2
					sendSingleVariable(sockfd, "2", 6, buffer);
				}
				else if( strcmp(command, "@listonline") == 0 ) {	//Command listonline = 3
					write(sockfd, "3", 1);
				}
				else if( strcmp(command, "@listchatroom") == 0 ) {	//Command listchatroom = 6
					write(sockfd, "6", 1);
				}
				else if( strcmp(command, "@help") == 0 ) {			//Command help = 7
					showMainCommand();
				}
				else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
					write(sockfd, "9", 1);
					close(sockfd);
					exit(1);
				}
				else {
					puts("--Invalid Command! Type @help!\n");
				}
			}
			else {									//Client da tao hoac tham gia chatroom
				if( strcmp(command, "@invite") == 0 ) {				//Command invite = 1
					sendMultiVariables(sockfd, "1", 8, buffer);
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
					strcpy(topicName, "");
					printf("\n");
				}
				else if( strcmp(command, "@exit") == 0 ) {			//Command exit = 9
					write(sockfd, "9", 1);
					close(sockfd);
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
	}
	return 0;
}

//revc message thread function
static void *receiveHandler( void *connfd ) {
	int sockfd = *((int*)connfd);
	char message[MTU];
	while( read(sockfd, message, sizeof(message)) > 0 ) {
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if( command == '0' ) {				//0 -> nhan command thong bao
			puts(message);
		}
		else if( command == '1' ) {			//1 -> nhan command khi create chatroom thanh cong
			strcpy(topicName, message);
			printf("Chatroom %s created success!\n", topicName);
			printf("\nYou are now in chatroom %s!\n", topicName);
		}
		else if( command == '2' ) {			//2 -> nhan command khi bi thang ngu nao do keo minh vao chatroom
			strcpy(topicName, message);
			printf("You have been invited to chatroom %s\n", topicName);
			printf("You are now in chatroom %s!\n\n", topicName);
		}
		else if( command == '3' ) {			//3 -> tu minh join room thanh cong
			strcpy(topicName, message);
			printf("\nYou are now in chatroom %s!\n", topicName);
		}
		else if( command == '4' ) {			//4 ->FileName tuc la server chuan bi gui file cho minh
			char filename[NAME_SIZE];
			strcpy(filename, message);
			memset(message, 0, sizeof(message));
			read(sockfd, message, sizeof(message));	//Nhan kich thuoc file tu server
			int fileSize = atoi(message), receivedData = 0, n;
			FILE *file = fopen(filename, "w");
			while( receivedData < fileSize ) {		//downfile
				memset(message, 0, sizeof(message));
				n = read(sockfd, message, sizeof(message));
				fwrite(message, sizeof(char), n, file);
				receivedData += n;
				printProgress((double)receivedData/fileSize);
			}
			printf("\nDownload file %s completed!\n\n", filename);
			fclose(file);
		}
		memset(message, 0, sizeof(message));
	}
	return 0;
}

void sendSingleVariable( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]) {
	char message[MTU];
	strcpy(message, command);
	strncpy(buffer, buffer + skip, strlen(buffer));
	strcpy(buffer, nameStandardize(buffer));
	if( strlen(buffer) < 1 ) {
		puts("--Invalid Command!\n");
		return;
	}
	strcat(message, buffer);
	write(sockfd, message, strlen(message));
}

void sendMultiVariables( int sockfd, char command[2], int skip, char buffer[DATA_SIZE]) {
	char message[MTU];
	strcpy(message, command);
	strncpy(buffer, buffer + skip, strlen(buffer));
	strcpy(buffer, stringStandardize(buffer));
	if( strlen(buffer) < 1 ) {
		puts("--Invalid Command!\n");
		return;
	}
	strcat(message, buffer);
	write(sockfd, message, strlen(message));
}

//Show list menu (Client in Main)
void showMainCommand() {
	puts("\t@create <chatroom_name>		create a new chatroom");
	puts("\t@join <chatroom_name>		join an existed chatroom");
	puts("\t@listonline			list all users online");
	puts("\t@listchatroom			list all chatroom");
	puts("\t@help				list command");
	puts("\t@exit				exit program\n");
}

//Show list Menu (Client in Chatroom)
void showTopicCommand() {
	puts("\t@invite <username1> <username2>...		invite 1 or many user");
	puts("\t@listonline				list all users online");
	puts("\t@listuser				list all users in chatroom");
	puts("\t@listfile				list all uploaded files in chatroom");
	puts("\t@listchatroom				list all chatroom");
	puts("\t@upfile <file_name>			upload a file");
	puts("\t@downfile <file_name>			download a file");
	puts("\t@help					list command");
	puts("\t@out					leave chatroom");
	puts("\t@exit					exit program\n");
}

//Send file to Server
void sendFile( int sockfd, char buffer[NAME_SIZE] ) {
	char fileName[DATA_SIZE];
	strncpy(fileName, buffer + 8, strlen(buffer));	//cat chuoi de lay filename
	strcpy(fileName, nameStandardize(fileName));

	FILE *file = fopen(fileName, "r+");
	if( file == NULL ) {
		printf("File %s doens't exist!\n\n", fileName);
	} else {
		fseek(file, 0, SEEK_END);
		int fileSize = ftell(file);
		if( fileSize > 1000000000) {
			puts("You can only send file with size less than 1Gb!\n");
			return;
		}
		char message[MTU] = "b";
		strcat(message, fileName);
		write(sockfd, message, strlen(message));	// Gui thong bao bFileName
		usleep(100);
		memset(message, 0, sizeof(message));
		sprintf(message, "%d", fileSize);
		write(sockfd, message, sizeof(message));	// Gui kich thuoc file
		usleep(100);
		rewind(file);
		puts("\nUploading file... Please don't do anything until done!\n");
		int sendedData = 0, n;
		while (sendedData < fileSize) {					//Gui file
			memset(message, 0, sizeof(message));
			n = fread(message, sizeof(char), MTU, file);
			sendedData += n;
			write(sockfd, message, n);
			usleep(100);
			printProgress((double)sendedData/fileSize);
		}
		if( sendedData == fileSize ) {
			puts("\nUpload completed!\n");
		}
		fclose(file);
	}
}

//Message 'downloadfile from server'
void downFile( int sockfd, char buffer[NAME_SIZE] ) {
	char fileName[DATA_SIZE];
	strncpy(fileName, buffer + 10, strlen(buffer)); //cat chuoi lay ten filename
	strcpy(fileName, nameStandardize(fileName));
	char message[MTU] = "c";
	strcat(message, fileName);
	write(sockfd, message, strlen(message));		// Gui thong bao cFileName
}

//command prompt
void commandPrompt() {
	if( strcmp(topicName, "") == 0 ) {
		printf("Main>");			//dau nhac Main chinh
	} else {
		printf("%s>", topicName);	//dau nhac topic
	}
}

char *nameStandardize( char str[MTU] ) {
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

char *stringStandardize( char str[MTU] ) {
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

//Print "%" Progress 
void printProgress( double process ) {
	int val = (int) (process * 100);
	int lpad = (int) (process * PBWIDTH);
	int rpad = PBWIDTH - lpad;
	printf("\r%3d%% [%.*s%*s]", val, lpad, PBSTR, rpad, "");
	fflush(stdout);
}
