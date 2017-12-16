#include <stdio.h>			//printf
#include <string.h>			//strcpy, strcmp, strncpy, strtok, strcat
#include <stdlib.h>			//exit, malloc
#include <netinet/in.h>		//??
#include <arpa/inet.h>		//inet_ntop
#include <sys/types.h>		//??
#include <sys/socket.h>		//??
#include <unistd.h>			//write, read, close
#include <pthread.h>		//pthread_create, pthread_detach

#include <sys/stat.h>		//mkdir, rmdir
#include <dirent.h>			//DIR, opendir

//Static Define
#define NUMBER_OF_CLIENT	32
#define MAX_FILE_PER_TOPIC	5
#define CLIENT_PER_TOPIC	5
#define NAME_SIZE			32
#define DATA_SIZE			1024
#define MTU 				1200
#define PORT 				50001
#define	SA 					struct sockaddr

//Struct Declare and Definition
typedef struct Topic {
	char topicName[NAME_SIZE];
	int member[CLIENT_PER_TOPIC];		// socket file description of member
	int countMember;
	char file[MAX_FILE_PER_TOPIC][NAME_SIZE];
	int countFile;
	struct Topic *next;
} Topic;

typedef struct Client {
	int sockfd;
	char username[NAME_SIZE];
	char topicName[NAME_SIZE];
	struct Client *next;
} Client;

//Function Declare
static void *doit( void *connfd );
Topic *getTopicByTopicName( char topicName[NAME_SIZE] );
Client *getClientBySocket( int sockfd );
Client *getClientByName( char username[NAME_SIZE] );
int joinRoom( int sockfd, char topicName[NAME_SIZE] );
int buildListFile( char *listFile[], const char *path );
void deleteFolder( const char *path );

void sendToAllClient( int sockfd, char message[MTU] );					//a
void addClient( int sockfd, char username[NAME_SIZE] );				//-1
Client *deleteClient( int sockfd );
void createTopic( int sockfd, char topicName[NAME_SIZE] );				//0
Topic *deleteTopic( char topicName[NAME_SIZE] );
void inviteClient( int sockfd, char message[MTU] );					//1
void clientJoin( int sockfd, char topicName[NAME_SIZE] );				//2
void sendListOnline( int sockfd );									//3
void sendListUser( int sockfd );									//4
void sendListFile( int sockfd );									//5
void sendListTopic( int sockfd );									//6
void clientOut( int sockfd );										//8
void clientExit( int sockfd );										//9
void upFile( int sockfd, char filename[NAME_SIZE] );		//b
void downFile( int sockfd, char filename[NAME_SIZE] );		//c

//Global Variable
Client *clients = NULL;
Topic *topics = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Main Function
int main( int argc, char **argv ) {
	//Socket()
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( listenfd < 0 ) {
		perror("Error when creating a listen socket!\n");
		exit(1);
	} else {
		puts("Socket created!");
	}
	
	//Server IP and Port
	struct sockaddr_in servaddr;
	memset((char*)&servaddr, 0, sizeof(servaddr));
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port 	 = htons(PORT);
	
	//Bind()
	if( bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0 ) {
		perror("ERROR when binding!\n");
		exit(1);
	} else {
		puts("Bind success!");
	}
	
	//Listen()
	if( listen(listenfd, NUMBER_OF_CLIENT) < 0 ) {
		perror("ERROR when listening!\n");
		exit(1);
	} else {
		puts( "Running...\n" );
	}
	
	//Accept()
	int *connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	pthread_t tid;
	for( ; ; ) {
		
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (SA*)&cliaddr, &clilen);
		printf("IPv4 Address: %s, Port: %d\n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		
		pthread_create(&tid, NULL, &doit, (void*)connfd);
	}

	return 0;
}

//Function Definition
static void *doit( void *connfd ) {
	int sockfd = *((int*)connfd);
	free(connfd);
	pthread_detach(pthread_self());
	char message[MTU] = "";
	while( read(sockfd, message, sizeof(message)) > 0 ) {  //Nhan goi tin chua username tu client moi
		if( getClientByName(message) == NULL ) {
			write(sockfd, "OK", strlen("OK"));
			break;
		} else {
			write(sockfd, "Duplicate", strlen("Duplicate"));
		}
		memset(message, 0, sizeof(message));
	}
	addClient(sockfd, message);					//Goi den ham cho client moi vao danh sach luu tru

	memset(message, 0, sizeof(message));

	while( read(sockfd, message, sizeof(message)) > 0 ) {
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if( command == 'a' ) {		//@chat
			char msgChat[MTU] = "0";
			strcat(msgChat, message);
			sendToAllClient(sockfd, msgChat);
		}
		else if( command == '0' ) {		//@create. Tao room chat.
			createTopic(sockfd, message);
		}
		else if( command == '1' ) {		//@invite.
			inviteClient(sockfd, message);
		}
		else if( command == '2' ) {		//@join.
			clientJoin(sockfd, message);
		}
		else if( command == '3' ) {		//@listonline. Gui danh sach nhung user dang online nhung chua vao room chat nao
			sendListOnline(sockfd);
		}
		else if( command == '4' ) {		//@listuser. Gui danh sach nhung user co mat trong cung room chat
			sendListUser(sockfd);
		}
		else if( command == '5' ) {		//@listfile. Gui danh sach nhung file da duoc upload len trong room chat
			sendListFile(sockfd);
		}
		else if( command == '6' ) {		//@listchatroom. Gui danh sach nhung chatroom hien co.
			sendListTopic(sockfd);
		}
		/*else if( command == '7' ) {		
			
		}*/
		else if( command == '8' ) {		//@out. Client thoat khoi room chat hien tai.
			clientOut(sockfd);
		}
		else if( command == '9' ) {		//@exit. Client 
			clientExit(sockfd);
		}
		else if(command == 'b' ) {		//@upfile
			upFile(sockfd, message);
		}
		else if (command == 'c' ) {		//@downfile
			downFile(sockfd, message);
		}
		memset(message, 0, sizeof(message));
	}

	close(sockfd);
	return NULL;
}

Topic *getTopicByTopicName( char topicName[NAME_SIZE] ) {
	if( topics == NULL ) {
		return NULL;
	}
	Topic *tmpTopic = topics;
	while( strcmp(tmpTopic->topicName, topicName) != 0 ) {
		if( tmpTopic->next == NULL ) {
			return NULL;
		} else {
			tmpTopic = tmpTopic->next;
		}
	}
	return tmpTopic;
}

Client *getClientBySocket( int sockfd ){
	Client *tmpClient;
	for( tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next ) {
		if( tmpClient->sockfd == sockfd ) {
			return tmpClient;
		}
	}
	return NULL;
}

Client *getClientByName( char username[NAME_SIZE] ) {
	Client *tmpClient;
	for( tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next ){
		if( strcmp(tmpClient->username, username) == 0 ) {
			return tmpClient;
		}
	}
	return NULL;
}

int joinRoom(int sockfd, char topicName[NAME_SIZE]) {
	Topic *tmpTopic = getTopicByTopicName(topicName);
	if( tmpTopic == NULL ) {					//return 1 nghia la khong tim thay chatroom
		return 1;
	} else if( tmpTopic->countMember == NUMBER_OF_CLIENT ) {
		return 2;								//return 2 nghia la chatroom da full nguoi
	} else {
		tmpTopic->member[tmpTopic->countMember] = sockfd;
		tmpTopic->countMember++;
		Client *tmpClient = getClientBySocket(sockfd);
		strcpy(tmpClient->topicName, topicName);
		return 0;								//return 0 nghia la join chatroom thanh cong
	}
}

int buildListFile( char *listFile[], const char *path ) {
	DIR *d = opendir(path);
	int i = 0;
	if( d ) {
		struct dirent *dir;
		while( (dir = readdir(d)) != NULL ) {
			if( !strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..") ) continue;
			listFile[i] = malloc(NAME_SIZE);
			strcpy(listFile[i], dir->d_name);
			i++;
		}
		closedir(d);
	}
	return i;
}

void deleteFolder( const char *path ) {
	struct stat st = {0};
	if( stat(path, &st) != 0 ) {		//stat = 0 nghia la folder co ton tai
		return;
	}
	char *listFile[NAME_SIZE];
	int countFile = buildListFile(listFile, path);
	int i = 0;
	for( i = 0; i < countFile; i++ ) {
		char *link;
		int length = strlen(path)+strlen(listFile[i]) + 2;
		link = malloc(length);
		snprintf(link, length, "%s/%s", path, listFile[i]);
		unlink(link);
		free(link);
	}
	rmdir(path);
}

void sendToAllClient( int sockfd, char message[MTU] ) {
	Topic *tmpTopic = getTopicByTopicName(getClientBySocket(sockfd)->topicName);
	int i;
	for( i = 0; i < tmpTopic->countMember; i++ ) {
		if( sockfd != tmpTopic->member[i] ) {
			write(tmpTopic->member[i], message, strlen(message));
		}
	}
}

void addClient( int sockfd, char username[NAME_SIZE] ) {
	Client *tmpClient = (Client*)malloc(sizeof(Client));
	tmpClient->sockfd = sockfd;
	strcpy(tmpClient->username, username);
	strcpy(tmpClient->topicName, "");
	tmpClient->next = clients;
	clients = tmpClient;
	printf("Client %s created!\n", username);
}

Client *deleteClient( int sockfd ) {
	Client *tmpClient = getClientBySocket(sockfd);
	printf("Client %s deleted!\n", tmpClient->username);
	if (clients == NULL) {
		return NULL;
	}
	Client *current = clients;
	Client *previous = NULL;
	while (current->sockfd != sockfd) {
		if (current->next == NULL) {
 			return NULL;
		} else {
 			previous = current;
 			current = current->next;
		}
	}
	if (current == clients) {
		clients = clients->next;
	} else {
		previous->next = current->next;
	}
	return current;
}

void createTopic( int sockfd, char topicName[NAME_SIZE] ) {
	Topic *tmpTopic = getTopicByTopicName(topicName);
	char message[MTU];
	if( tmpTopic != NULL ) {
		strcpy(message, "0Your chatroom name is already existed!");
		
	} else {
		tmpTopic = (Topic *)malloc(sizeof(Topic));
		strcpy(tmpTopic->topicName, topicName);
		tmpTopic->member[0] = sockfd;
		tmpTopic->countMember = 1;
		tmpTopic->countFile = 0;
		tmpTopic->next = topics;
		topics = tmpTopic;
		Client *tmpClient = getClientBySocket(sockfd);
		strcpy(tmpClient->topicName, topicName);
		char path[NAME_SIZE+4] = "./";
		mkdir(strcat(path, topicName), 0777);
		printf("Chatroom %s created!\n", topicName);
		sprintf(message, "1%s", topicName);
	}
	write(sockfd, message, strlen(message));
}

Topic *deleteTopic( char topicName[NAME_SIZE] ) {
	char path[NAME_SIZE+4] = "./";
	strcat(path, topicName);
	deleteFolder(path);
	Topic *current = topics;
	Topic *previous = NULL;
	printf("Chatroom %s is deleted!\n", topicName);
	while (!strcmp(current->topicName, topicName)) {
		if (current->next == NULL) {
 			return NULL;
		} else {
 			previous = current;
 			current = current->next;
		}
	}
	if (current == topics) {
		topics = topics->next;
	} else {
		previous->next = current->next;
	}
	return current;
}

void inviteClient( int sockfd, char message[MTU] ) {
	Topic *tmpTopic = getTopicByTopicName(getClientBySocket(sockfd)->topicName);
	char buffer[MTU];
	char *targetName ;
    targetName = strtok(message, " ");
    while( targetName != NULL) {
    	memset(buffer, 0, sizeof(buffer));
		Client *targetClient = getClientByName(targetName);
		if( targetClient != NULL ) {
			if( strcmp(targetClient->topicName, "") == 0 ) {
				int check = joinRoom(targetClient->sockfd, tmpTopic->topicName);
				if( check == 0 ) {			// invite success
					sprintf(buffer, "2%s", tmpTopic->topicName);
					write(targetClient->sockfd, buffer, strlen(buffer));				
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "0Invite user %s success!\n", targetName);
				} else if (check == 2) {	//Neu chatroom full nguoi
					sprintf(buffer, "0Invite user %s fail because this chatroom is full!\n", targetName);
				}
			} else {
				if( strcmp(targetClient->topicName, tmpTopic->topicName) == 0 ){
					sprintf(buffer, "0User %s is already in this chatroom!\n", targetName);
				} else {
					sprintf(buffer, "0User %s is in chatroom %s!\n", targetName, targetClient->topicName);
				}
			}
		} else {
			sprintf(buffer, "0User %s doesn't exist!\n", targetName);
		}
		write(sockfd, buffer, strlen(buffer));
        targetName = strtok(NULL, " ");
        usleep(500);
    }
}

void clientJoin( int sockfd, char topicName[NAME_SIZE] ) {
	int check = joinRoom(sockfd, topicName);
	char message[MTU];
	if (check == 1) {			//1 nghia la ko tim thay chatroom nao co ten nhu the de join
		sprintf(message, "0Chatroom %s doesn't exist!\n", topicName);
		write(sockfd, message, strlen(message));
	} else if (check == 2) {	//2 nghia la chatroom muon join full nguoi cmnr
		sprintf(message, "0Chatroom %s is full!\n", topicName);
		write(sockfd, message, strlen(message));
	} else if (check == 0) {	//0 nghia la join room thanh cong roi
		sprintf(message, "3%s", topicName);
		write(sockfd, message, strlen(message));
		Client *tmpClient = getClientBySocket(sockfd);
		memset(message, 0, sizeof(message));
		sprintf(message, "0\n%s has join this chatroom!\n", tmpClient->username);
		sendToAllClient(sockfd, message);
	}
}

void sendListOnline( int sockfd ){
	Client *tmpClient;
	char message[MTU] = "0List user online:";
	for( tmpClient = clients ; tmpClient != NULL; tmpClient = tmpClient->next ) {
		strcat(message, "\n---");
		strcat(message, tmpClient->username);
	}
	strcat(message, "\n");
	write(sockfd, message, strlen(message));
}

void sendListUser( int sockfd ){
	Topic *tmpTopic = getTopicByTopicName(getClientBySocket(sockfd)->topicName);
	char message[MTU] = "0List user in this chatroom:";
	int i;
	for( i = 0 ; i < tmpTopic->countMember ; i++ ){
		strcat(message, "\n---");
		strcat(message, getClientBySocket(tmpTopic->member[i])->username);
	}
	strcat(message, "\n");
	write(sockfd, message, strlen(message));
}

void sendListFile( int sockfd ) {
	char *listFile[NAME_SIZE];
	char path[NAME_SIZE + 4] = "./";
	strcat(path, getClientBySocket(sockfd)->topicName);
	int countFile = buildListFile(listFile, path);
	char message[MTU];
	if( countFile == 0 ) {
		strcpy(message, "0No files yet!");
	} else {
		strcpy(message, "0List file:");
		int i = 0;
		for( i = 0; i < countFile; i++ ) {
			strcat(message,"\n---");
			strcat(message, listFile[i]);
		}
	}
	write(sockfd, message, strlen(message));
}

void sendListTopic( int sockfd ) {
	char message[MTU];
	if( topics == NULL ) {
		strcpy(message, "0No chatrooms yet!\n");
	} else {
		Topic *tmpTopic;
		strcpy(message, "0List chatrooms:");
		for( tmpTopic = topics ; tmpTopic != NULL ; tmpTopic = tmpTopic->next ) {
			strcat(message, "\n---");
			strcat(message, tmpTopic->topicName);
		}
		strcat(message, "\n");
	}
	write(sockfd, message, strlen(message));
}

void clientOut( int sockfd ) {
	Client *tmpClient = getClientBySocket(sockfd);
	Topic *tmpTopic = getTopicByTopicName(tmpClient->topicName);
	if( tmpTopic->countMember == 1 ) {
		topics = deleteTopic(tmpTopic->topicName);
	} else {
		char buffer[DATA_SIZE];
		sprintf(buffer, "0\n%s has left this chatroom!\n", tmpClient->username);
		sendToAllClient(sockfd, buffer);
		int i, n = tmpTopic->countMember;
		for( i = 0; i < n; i++ ) {
			if( tmpTopic->member[i] == sockfd ) {
				while( i < n-1) {
					tmpTopic->member[i] = tmpTopic->member[i+1];
					i++;
				}
				tmpTopic->member[i] = 0;
				break;
			}
		}
		tmpTopic->countMember--;
	}
	strcpy(tmpClient->topicName, "");
}

void clientExit( int sockfd ) {
	if( strcmp(getClientBySocket(sockfd)->topicName, "") != 0 )  {
			clientOut(sockfd);
	}
	deleteClient(sockfd);
	close(sockfd);
}

void upFile( int sockfd, char filename[NAME_SIZE] ) {
	char path[NAME_SIZE + 4] = "./";
	strcat(path, getClientBySocket(sockfd)->topicName);
	strcat(path, "/");
	strcat(path, filename);

	char message[MTU];
	memset(message, 0, sizeof(message));
	read(sockfd, message, sizeof(message));
	int fileSize = atoi(message), receivedData = 0, n;

	FILE *file = fopen(path, "w");
	while( receivedData < fileSize ) {
		memset(message, 0, sizeof(message));
		n = read(sockfd, message, sizeof(message));
		fwrite(message, sizeof(char), n, file);
		receivedData += n;
	}
	printf("Upload file %s completed!\n", filename);
	fclose(file);
}

void downFile( int sockfd, char filename[NAME_SIZE] ) {
	char path[NAME_SIZE + 4] = "./", message[MTU];
	strcat(path, getClientBySocket(sockfd)->topicName);
	strcat(path, "/");
	strcat(path, filename);

	FILE *file = fopen(path, "r+");
	if( file == NULL ) {			// Neu file khong ton tai thi gui kich thuoc file la -1 (kieu string)
		memset(message, 0, sizeof(message));
		sprintf(message, "0File %s doesn't existed!\n", filename);
		write(sockfd, message, sizeof(message));
	} else {						// Neu file co ton tai
		strcpy(message, "4");
		strcat(message, filename);
		write(sockfd, message, strlen(message));

		usleep(1000);
		fseek(file, 0, SEEK_END);
		int fileSize = ftell(file);

		memset(message, 0, sizeof(message));
		sprintf(message, "%d", fileSize);
		write(sockfd, message, sizeof(message));	// Gui kich thuoc file

		rewind(file);
		int sendedData = 0, n;
		while (sendedData < fileSize) {					//Gui file
			memset(message, 0, sizeof(message));
			n = fread(message, sizeof(char), sizeof(message), file);
			sendedData += n;
			write(sockfd, message, n);
		}
		if( sendedData == fileSize ) {
			puts("Send file %s completed!");
		}
		fclose(file);
	}
}