#include <stdio.h>		//printf
#include <string.h>		//strcpy, strcmp, strncpy, strtok, strcat
#include <stdlib.h>		//exit, malloc
#include <netinet/in.h>	// ??
#include <arpa/inet.h>	//inet_ntop
#include <sys/types.h>	// ??
#include <sys/socket.h>	// ??
#include <unistd.h>		//write, read, close
#include <pthread.h>	//pthread_create, pthread_detach
#include <sys/stat.h>	//mkdir, rmdir
#include <dirent.h>		//DIR, opendir

//Static Define
#define NUMBER_OF_CLIENT	32
#define MAX_FILE_PER_TOPIC	5
#define CLIENT_PER_TOPIC	5
#define NAME_SIZE			32
#define DATA_SIZE			1024
#define MTU 				1200
#define PORT 				5000
#define	SA 					struct sockaddr

//Struct Declare and Definition
typedef struct Topic {
	char title[NAME_SIZE];
	int member[CLIENT_PER_TOPIC];
	int countMember;
	char file[MAX_FILE_PER_TOPIC][NAME_SIZE];
	int countFile;
	struct Topic *next;
} Topic;

typedef struct Client {
	int sockfd;
	char username[NAME_SIZE];
	char title[NAME_SIZE];
	struct Client *next;
} Client;

//Function Declare
static void *doit( void *connfd );
Topic *getTopicByTitle( char title[NAME_SIZE] );
Client *getClientBySocket( int sockfd );
Client *getClientByName( char username[NAME_SIZE] );
int joinRoom( int sockfd, char title[NAME_SIZE] );
int buildListFileInFolder( char *listFile[], const char *path );
void deleteFolder( const char *path );

void clientChat( int sockfd, char message[MTU] );					//a
void addClient( int sockfd, char username[NAME_SIZE] );				//-1
Client *deleteClient( int sockfd );
void createTopic( int sockfd, char title[NAME_SIZE] );				//0
Topic *deleteTopic( char title[NAME_SIZE] );
void inviteClient( int sockfd, char message[MTU] );					//1
void clientJoin( int sockfd, char title[NAME_SIZE] );				//2
void sendListOnline( int sockfd );									//3
void sendListUser( int sockfd );									//4
void sendListFile( int sockfd );									//5
void sendListTopic( int sockfd );									//6
void sendHelp( int sockfd, char message[MTU]);										//7
void clientOut( int sockfd );										//8
void clientExit( int sockfd );										//9
void getFileFromClient( int sockfd, char filename[NAME_SIZE] );		//b
void sendFileToClient( int sockfd, char filename[NAME_SIZE] );		//c

//Global Variable
Client *clients = NULL;
Topic *topics = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//Main Function
int main( int argc, char **argv ) {
	//Socket()
	int listenfd = socket(AF_INET, SOCK_STREAM, 0);
	if ( listenfd < 0 ) {
		perror( "Error when creating a listen socket!\n" );
		exit(1);
	} else {
		printf( "Socket created!\n" );
	}
	
	//Server IP and Port
	struct sockaddr_in servaddr;
	memset((char*)&servaddr, 0, sizeof(servaddr));     
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port 	 = htons(PORT);
	
	//Bind()
	if( bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0 ) {
		perror( "ERROR when binding!\n" );
		exit(1);
	} else {
		printf( "Bind success!\n" );
	}
	
	//Listen()
	if( listen(listenfd, NUMBER_OF_CLIENT) < 0 ) {
		perror( "ERROR when listening!\n" );
		exit(1);
	} else {
		printf( "Running...\n\n" );
	}
	
	//Accept()
	int *connfd;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	pthread_t tid;
	char message[MTU];
	for( ; ; ) {
		connfd = malloc(sizeof(int));
		*connfd = accept(listenfd, (SA*)&cliaddr, &clilen);
		printf( "IP %s , port %d \n", inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port );
		for( ; ; ) {
			memset(message, 0, sizeof(message));
			read(*connfd, message, sizeof(message));	//Nhan goi tin chua username tu client moi
			if( getClientByName(message) == NULL ) {
				write(*connfd, "OK", 2);
				break;
			} else {
				write(*connfd, "Duplicate", 9);
			}
		}
		pthread_mutex_lock(&mutex);
		addClient(*connfd, message);					//Goi den ham cho client moi vao danh sach luu tru
		pthread_mutex_unlock(&mutex);
		pthread_create(&tid, NULL, &doit, (void*)connfd);
	}

	return 0;
}

//Function Definition
static void *doit( void *connfd ) {
	int sockfd = *((int*)connfd);
	free(connfd);
	pthread_detach(pthread_self());
	
	char message[MTU];
	for( ; ; ) {
		memset(message, 0, sizeof(message));
		read(sockfd, message, sizeof(message));
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if( command == 'a' ) {		//@chat
			clientChat(sockfd, message);
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
		else if( command == '6' ) {		//@listtopic. Gui danh sach nhung topic hien co.
			sendListTopic(sockfd);
		}
		else if( command == '7' ) {		//@help
			sendHelp(sockfd, message);
		}
		else if( command == '8' ) {		//@out. Client thoat khoi room chat hien tai.
			clientOut(sockfd);
		}
		else if( command == '9' ) {		//@exit. Client 
			clientExit(sockfd);
		}
		else if(command == 'b' ) {		//@upfile
			getFileFromClient(sockfd, message);
		}
		else if (command == 'c' ) {		//@getFileFromClient
			sendFileToClient(sockfd, message);
		}
	}

	close(sockfd);
	return NULL;
}

Topic *getTopicByTitle( char title[NAME_SIZE] ) {
	if( topics == NULL ) {
		return NULL;
	}
	Topic *tmpTopic = topics;
	while( strcmp(tmpTopic->title, title) != 0 ) {
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

int joinRoom(int sockfd, char title[NAME_SIZE]) {
	Topic *tmpTopic = getTopicByTitle(title);
	if( tmpTopic == NULL ) {					//return 1 nghia la khong tim thay topic
		return 1;
	} else if( tmpTopic->countMember == NUMBER_OF_CLIENT ) {
		return 2;								//return 2 nghia la topic da full nguoi
	} else {
		tmpTopic->member[tmpTopic->countMember] = sockfd;
		tmpTopic->countMember++;
		return 0;								//return 0 nghia la join thanh cong roi
	}
}

int buildListFileInFolder( char *listFile[], const char *path ) {
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
	if( stat("/some/directory", &st) != 0 ) {		//stat = 0 nghia la folder co ton tai
		return;
	}
	char *listFile[NAME_SIZE];
	int countFile = buildListFileInFolder(listFile, path);
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

void clientChat( int sockfd, char message[MTU] ) {
	Topic *tmpTopic = getTopicByTitle(getClientBySocket(sockfd)->title);
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
	strcpy(tmpClient->title, "");
	tmpClient->next = clients;
	clients = tmpClient;
	printf("Client %s created!\n", username);
}

Client *deleteClient( int sockfd ) {
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
	printf("Client deleted!\n");
	return current;
}

void createTopic( int sockfd, char title[NAME_SIZE] ) {
	Topic *tmpTopic = getTopicByTitle(title);
	if( tmpTopic != NULL ) {
		char message[MTU] = "0Topic name is already existed!";
		write(sockfd, message, strlen(message));
	} else {
		char message[MTU] = "";
		tmpTopic = (Topic *)malloc(sizeof(Topic));
		strcpy(tmpTopic->title, title);
		char path[NAME_SIZE+4] = "./";
		mkdir(strcat(path,title), 0777);		
		tmpTopic->member[0] = sockfd;
		tmpTopic->countMember = 1;
		tmpTopic->countFile = 0;
		tmpTopic->next = topics;
		topics = tmpTopic;
		printf("Topic %s created!\n", title);
		sprintf(message, "2%s", title);
		write(sockfd, message, strlen(message));
	}
}

Topic *deleteTopic( char title[NAME_SIZE] ) {
	char path[NAME_SIZE+4] = "./";
	strcat(path, title);
	deleteFolder(path);
	Topic *current = topics;
	Topic *previous = NULL;
	while (!strcmp(current->title, title)) {
		if (current->next == NULL) {
 			return NULL;
		} else {
 			previous = current;
 			current = current->next;
		}
	}
	if (current == topics)
		topics = topics->next;
	else
		previous->next = current->next;
	printf("Topic deleted!\n");
	return current;
}

void inviteClient( int sockfd, char message[MTU] ) {
	Topic *tmpTopic = getTopicByTitle(getClientBySocket(sockfd)->title);
	char buffer[MTU];
	char *targetName ;
    targetName = strtok(message, " ");
    while( targetName != NULL) {
    	memset(buffer, 0, sizeof(buffer));
		Client *targetClient = getClientByName(targetName);
		if( targetClient != NULL ){
			if( strcmp(targetClient->title, "") == 0 ) {
				int check = joinRoom(targetClient->sockfd, tmpTopic->title);
				if( check == 0 ) {			// invite success
					sprintf(buffer, "1%s", tmpTopic->title);
					write(targetClient->sockfd, buffer, strlen(buffer));				
					memset(buffer, 0, sizeof(buffer));
					sprintf(buffer, "0Invite %s success!", targetName);
				} else if (check == 2) {	//Neu room full nguoi
					sprintf(buffer, "0Invite %s fail because this topic is full!", targetName);
				}
			} else {
				if( strcmp(targetClient->title, tmpTopic->title) == 0 ){
					sprintf(buffer, "0%s is already in this topic!", targetName);
				} else {
					sprintf(buffer, "0%s is in topic %s!", targetName, targetClient->title);
				}
			}
		} else {
			sprintf(buffer, "0%s doesn't exist!", targetName);
		}
		write(sockfd, buffer, strlen(buffer));			// loc : co the gay ra bug  
        targetName = strtok(NULL, " ");
    }
}

void clientJoin( int sockfd, char title[NAME_SIZE] ) {
	int check = joinRoom(sockfd, title);
	char message[MTU];
	if (check == 1) {			//1 nghia la ko tim thay topic nao co ten nhu the de join
		sprintf(message, "0Topic %s doesn't exist!", title);
		write(sockfd, message, strlen(message));
	} else if (check == 2) {	//2 nghia la topic muon join full nguoi cmnr
		sprintf(message, "0Topic %s is full!", title);
		write(sockfd, message, strlen(message));
	} else if (check == 0) {	//0 nghia la join room thanh cong roi
		sprintf(message, "2%s", title);
		write(sockfd, message, strlen(message));
	}
}

void sendListOnline( int sockfd ){
	Client *tmpClient;
	char message[MTU] = "0List user online:";
	for( tmpClient = clients ; tmpClient != NULL; tmpClient = tmpClient->next ) {
		strcat(message, "\n---");
		strcat(message, tmpClient->username);
	}
	write(sockfd, message, strlen(message));
}

void sendListUser( int sockfd ){
	Topic *tmpTopic = getTopicByTitle(getClientBySocket(sockfd)->title);
	char message[MTU] = "0List user in this topic:";
	int i;
	for( i = 0 ; i < tmpTopic->countMember ; i++ ){
		strcat(message, "\n---");
		strcat(message, getClientBySocket(tmpTopic->member[i])->username);
	}
	write(sockfd, message, strlen(message));
}

void sendListFile( int sockfd ) {
	char *listFile[NAME_SIZE];
	char path[NAME_SIZE + 4] = "./";
	strcat(path, getClientBySocket(sockfd)->title);
	int countFile = buildListFileInFolder(listFile, path);
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
		strcpy(message, "0No topics yet!");
	} else {
		Topic *tmpTopic;
		strcpy(message, "0List topic:");
		for( tmpTopic = topics ; tmpTopic != NULL ; tmpTopic = tmpTopic->next ) {
			strcat(message, "\n---");
			strcat(message, tmpTopic->title);
		}
	}
	write(sockfd, message, strlen(message));
}

void sendHelp( int sockfd, char message[MTU] ) {
	char buffer[MTU];
	if( message[0] == '0' ) {
		strcpy(buffer, "Main menu command:\n---@create <topic name>\n---@join <topic name>");
		strcpy(buffer, "\n---@listonline\n---@listtopic\n---@help\n---@exit\n");
	}
	else if( message[0] == '1' ) {
		strcpy(buffer, "Topic command:\n---@invite <client name 1> <client name 2> ...");
		strcpy(buffer, "\n---@listonline\n---@listuser\n---@listfile\n---@listtopic");
		strcpy(buffer, "\n---@help\n---@out\n---@exit\n");
	}
	write(sockfd, buffer, strlen(buffer));
}

void clientOut( int sockfd ) {
	Client *tmpClient = getClientBySocket(sockfd);
	Topic *tmpTopic = getTopicByTitle(tmpClient->title);
	strcpy(tmpClient->title, "");

	if( tmpTopic->countMember == 1 ) {
		deleteTopic(tmpTopic->title);
	} else {
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
}

void clientExit( int sockfd ) {
	if( strcmp(getClientTitleBySocket(socket), "") != 0 ){
			clientOut(sockfd);
	}
	deleteClient(sockfd);
	close(sockfd);
}

void getFileFromClient( int sockfd, char fileName[NAME_SIZE] ) {
// 	int bytesReceived = 0;
//     	char recvBuff[256], fileName[256];
// 	while(1){
// 		read(connfd, filename, 256);	
// 		printf("\nClient want to send of file : %s. \n", filename);
		
// 		FILE *fp;	    
//     		fp = fopen(fileName, "wb"); 
// 	   	do {
// 			  memset(recvBuff, 0, sizeof(recvBuff));
// 		  	  bytesReceived=read(sockfd, recvBuff, sizeof(recvBuff));
// 			  if(strcmp(recvBuff,"error") == 0){
// 		      		memset(recvBuff, 0, sizeof(recvBuff));
// 		      		printf("File name doesn't exist in your server or invalid. \n");
// 		      	  	continue;
// 		          }
// 			  else{
// 		          	fwrite(recvBuff, 1,bytesReceived,fp);
//                		  }
// 	   	}while(bytesReceived >= 256);	   
// 	   	fclose(fp);     
// 	        if(bytesReceived < 0){
// 		    printf("Read Error \n");
// 	        }   	
// 	}
}

void sendFileToClient( int sockfd, char fileName[NAME_SIZE] ) {
//     int connfd = *(int*)sockfd;
//     char filename[256];
//     bzero(filename,256);
//     while(1)
//     {
//             read(connfd, filename, 256);
//             if(strcmp(filename,"@") == 0){
//                 printf("Connection ended!");
//                 break;
//             }
//             printf("\nClient want to download file : %s. \n", filename);
            
//             FILE *fp;
//             fp = fopen(filename,"rb");
//             if(fp==NULL)
//             {
//                 	printf("File open error or not exist file.\n");
//                 	write(connfd, "error", sizeof("error"));
//                  continue;
//             }else{
 
//             int nread;
//             // send content file
//             char contentfile[255] = {0};
//             do{
//             /* Read file in chunks of 256 bytes */
// 		    		nread=fread(contentfile, 1, 256, fp);
// 		    		write(connfd, contentfile, nread);
//             }while(nread >= sizeof(contentfile));

//             	if (nread < 256){
//                     if (feof(fp))
//                         printf("Send file successfull.\n");
//                     if (ferror(fp))
//                         printf("Error reading file.\n");
//                 }
//             }
//             fclose(fp);
//      }
}