#include <stdio.h>	//printf
#include <string.h>	//strcpy, strcmp, strncpy, strtok
#include <stdlib.h>	//exit, malloc
#include <netinet/in.h>	// ??
#include <arpa/inet.h>	//inet_ntop
#include <sys/types.h>	// ??
#include <sys/socket.h>	// ??
#include <unistd.h>	//write, read, close
#include <pthread.h>	//pthread_create, pthread_detach
#include <dirent.h>	//rmdir, mkdir

#define NUMBER_OF_CLIENT 32
#define NUMBER_OF_FILE 5
#define CLIENT_PER_TOPIC 5
#define NAME_SIZE 32
#define DATA_SIZE 1024
#define MTU 1200
#define PORT 5000

struct topic {
	char title[NAME_SIZE];
	int member[CLIENT_PER_TOPIC];
	int countMember;
	char file[NUMBER_OF_FILE][NAME_SIZE];
	int countFile;
	struct topic *next;
};

struct client {
	int socket;
	char username[NAME_SIZE];
	char title[NAME_SIZE];
	struct client *next;
};

//Ham them client moi vao danh sach client online
void clientCreate(int socket, char username[NAME_SIZE]);

//Ham them client dang online vao danh sach client trong room chat
void clientJoin(int socket, char title[NAME_SIZE]);

//
int joinRoom(int socket, char title[NAME_SIZE]);

//Ham xoa client, chi goi den khi client gui command @exit (lenh so 9)
struct client *clientDelete(int socket);

//Ham them topic moi vao danh sach topic
void topicCreate(int socket, char title[NAME_SIZE]);

//Ham xoa topic, chi goi den khi client cuoi cung thoat room chat
struct topic *topicDelete(char title[NAME_SIZE]);

//Ham tra ve topic theo ten topic
struct topic *getTopicByTitle(char title[NAME_SIZE]);

//Ham tra ve ten topic ma client tham gia
char *getClientTitleBySocket(int socket);

//Ham tra ve username cua client theo socket
char *getClientUsernameBySocket(int socket);

//
int getSocketByUsername(char username[NAME_SIZE]);

//
struct client *getClientByName(char username[NAME_SIZE]);

//
int isTopicExist(char username[NAME_SIZE]);

//Ham gui mess cho toan bo client trong room chat, ngoai tru nguoi gui
void clientChat(int socket, char message[MTU]);

//
void sendListOnline(int socket);

//
void sendListUser(int socket);

//
void sendListTopic(int socket);

//
void sendListFile(int socket);

//Ham lay list file co trong folder
int buildListFileInFolder(char *listFile[], const char *path);

//Ham xoa folder
void deleteFolder(const char *path);

//Ham ... deo biet mo ta the nao
void clientInvite(int socket, char message[MTU]);

//Ham xu ly yeu cau tu client
static void *doit(void *socket);

//Ham sendFile tu server cho client
// void *sendfile(void * );

//Ham nhan file tu client
// void *downfile(void *);

//Bien toan cuc
struct client *clients = NULL;
struct topic *topics = NULL;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//HAM MAIN CHINH
int main(int argc, char **argv) {
	//Socket()
	int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
	if (listenSocket < 0) {
		perror("Error when creating a listen socket!\n");
		exit(1);
	} else {
		printf("Socket created!\n");
	}
	
	//Server IP and Port
	struct sockaddr_in servaddr;
	memset((char *)&servaddr, 0, sizeof(servaddr));     
	servaddr.sin_family 	 = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port 	 = htons(PORT);
	
	//Bind()
	if (bind(listenSocket, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
		perror("ERROR when binding!\n");
		exit(1);
	} else {
		printf("Bind success!\n");
	}
	
	//Listen()
	if (listen(listenSocket, NUMBER_OF_CLIENT) < 0) {
		perror("ERROR when listening!\n");
		exit(1);
	} else {
		printf("Running...\n\n");
	}
	
	//Accept()
	int *connSocket;
	struct sockaddr_in cliaddr;
	socklen_t clilen = sizeof(cliaddr);
	pthread_t tid;
	char message[MTU];
	for (;;) {
		connSocket = malloc(sizeof(int));
		*connSocket = accept(listenSocket, (struct sockaddr*)&cliaddr, &clilen);
		pthread_mutex_lock(&mutex);
		printf("IP %s , port %d \n",inet_ntoa(cliaddr.sin_addr), cliaddr.sin_port);
		memset(message, 0, sizeof(message));
		read(*connSocket, message, sizeof(message));		//Nhan goi tin chua username tu client moi
		clientCreate(*connSocket, message);			//Goi den ham cho client moi vao danh sach luu tru
		pthread_create(&tid, NULL, &doit, (void *) connSocket);
		pthread_mutex_unlock(&mutex);
	}
	// send file
	// int i, *connfd;
 //    	for (i = 0; i < 5; i++){ /*Gioi han gui 5 file - neu co thay doi thi se sua sau*/
 //        	connfd = malloc(sizeof(int));
 //       		*connfd = accept(listenSocket, (struct sockaddr *)NULL ,NULL);
 //        	pthread_create(&tid, NULL, &sendfile, (void *)connfd);
	// 		pthread_create(&tid, NULL, &downfile, (void *)connfd);
 //        	printf("Connect client %d\n", i); 
 //    	}
	return 0;
}

// void * downfile(void * sockfd){
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
// }

// void * sendfile(void * sockfd){
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
//                 printf("File open error or not exist file.\n");
//                 write(connfd, "error", sizeof("error"));
//                     continue;
//             }else{
 
//             int nread;
//             // send content file
//             char contentfile[255] = {0};
//             do{
//             /* Read file in chunks of 256 bytes */
// 		    nread=fread(contentfile, 1, 256, fp);
// 		    write(connfd, contentfile, nread);
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
// }

static void *doit(void *socket) {
	int connfd = *((int*)socket);
	free(socket);
	pthread_detach(pthread_self());
	
	char message[MTU];
	for (;;) {
		memset(message, 0, sizeof(message));
		read(connfd, message, sizeof(message));
		char command = message[0];
		strncpy(message, message+1, strlen(message));
		if (command == '0') {		//@create. Tao room chat.
			topicCreate(connfd, message);
		} else if (command == '1') {		//@invite.
			clientInvite(connfd, message);
		} else if (command == '2') {		//@join.
			clientJoin(connfd, message);
		} else if (command == '3') {		//@listonline. Gui danh sach nhung user dang online nhung chua vao room chat nao
			sendListOnline(connfd);
		} else if (command == '4') {		//@listuser. Gui danh sach nhung user co mat trong cung room chat
			sendListUser(connfd);
		} else if (command == '5') {		//@listfile. Gui danh sach nhung file da duoc upload len trong room chat
			sendListFile(connfd);
		} else if (command == '6') {		//@listtopic. Gui danh sach nhung topic hien co.
			sendListTopic(connfd);
		} else if (command == '7') {
			// module unused
		} else if (command == '8') {		//@out. Client thoat khoi room chat hien tai.
			//
		} else if (command == '9') {		//@exit. Client 
			//
		} else if (command == 'a') {		//@chat
			clientChat(connfd, message);
		} else if (command == 'b') {		//@upfile
			//
		} else if (command == 'c') {		//@downfile
			//
		}
	}
	close(connfd);
	return NULL;
}

void clientCreate(int socket, char username[NAME_SIZE]) {
	struct client *tmpClient = (struct client *)malloc(sizeof(struct client));
	tmpClient->socket = socket;
	strcpy(tmpClient->username, username);
	strcpy(tmpClient->title, "");
	tmpClient->next = clients;
	clients = tmpClient;
}

void clientJoin(int socket, char title[NAME_SIZE]) {
	int check = joinRoom(socket, title);
	if (check == 1) {
		write(socket, "1", 1);	//2 nghia la ko tim thay topic nao co ten nhu the de join
	} else if (check == 2) {
		write(socket, "2", 1);	//3 nghia la topic muon join full nguoi cmnr
	} else if (check == 0) {
		write(socket, "0", 1);	//0 nghia la join thanh cong roi
	}
}

int joinRoom(int socket, char title[NAME_SIZE]) {
	struct topic *tmpTopic = getTopicByTitle(title);
	if (tmpTopic == NULL) {	//1 nghia la khong tim thay topic
		return 1;
	} else if (tmpTopic->countMember == NUMBER_OF_CLIENT) {
		return 2;	//2 nghia la topic da full nguoi
	} else {
		tmpTopic->member[tmpTopic->countMember] = socket;
		tmpTopic->countMember++;
		return 0;	//0 nghia la join thanh cong roi
	}
}

struct client *clientDelete(int socket) {
	if (clients == NULL) {
		return NULL;
	}
	struct client *current = clients;
	struct client *previous = NULL;
	while (current->socket != socket) {
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

void topicCreate(int socket, char title[NAME_SIZE]) {
	struct topic *tmpTopic = (struct topic *)malloc(sizeof(struct topic));
	strcpy(tmpTopic->title, title);
	tmpTopic->member[0] = socket;
	tmpTopic->countMember = 1;
	tmpTopic->countFile = 0;
	tmpTopic->next = topics;
	topics = tmpTopic;
}

struct topic *topicDelete(char title[NAME_SIZE]) {
	if (topics == NULL) {
		return NULL;
	}
	struct topic *current = topics;
	struct topic *previous = NULL;
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
	return current;
}

char *getClientTitleBySocket(int socket) {
	if (clients == NULL) {
		return "";
	} else {
		struct client *tmpClient;
		for (tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next) {
			if (tmpClient->socket == socket) {
				return tmpClient->title;
				break;
			}
		}
		return "";
	}
}

char *getClientUsernameBySocket(int socket) {
	if (clients == NULL) {
		return "";
	} else {
		struct client *tmpClient;
		for (tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next) {
			if (tmpClient->socket == socket) {
				return tmpClient->username;
				break;
			}
		}
		return "";
	}
}

int getSocketByUsername(char username[NAME_SIZE]){
	int socket;
	struct client *tmpClient;
	for (tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next) {
		if (tmpClient->username == username) {
			socket = tmpClient->socket;
			break;
		}
	}
	return socket;
}

struct client *getClientByName(char username[NAME_SIZE]){
	struct client *tmpClient;
	for (tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next) {
		if (tmpClient->username == username) {
			return tmpClient;
		}
	}
	return NULL;
}

int isTopicExist(char title[NAME_SIZE]){
	struct topic *tmpTopic;
	for (tmpTopic = topics; tmpTopic != NULL; tmpTopic = tmpTopic->next) {
		if(tmpTopic->title == title){
			return 1;
		}
	}
	return 0;
}

void clientChat(int socket, char message[MTU]) {
	struct topic *tmpTopic;
	for (tmpTopic = topics; tmpTopic != NULL; tmpTopic = tmpTopic->next) {
		if (strcmp(tmpTopic->title, getClientTitleBySocket(socket)) == 0) {
			int i;
			for (i = 0; i < tmpTopic->countMember; i++) {
				if (socket != tmpTopic->member[i]) {
					write(tmpTopic->member[i], message, strlen(message));
				}
			}
			break;
        	}
	}
}

struct topic *getTopicByTitle(char title[NAME_SIZE]) {
	if (topics == NULL) {
		return NULL;
	}
	struct topic *tmpTopic = topics;
	while (strcmp(tmpTopic->title, title) != 0) {
		if (tmpTopic->next == NULL) {
			return NULL;
		} else {
			tmpTopic = tmpTopic->next;
		}
	}
	return tmpTopic;
}


void clientInvite(int socket, char message[MTU]) {
	struct topic *tmpTopic = getTopicByTitle(getClientTitleBySocket(socket));
	char buffer[DATA_SIZE] = "";
	//tach user 
	char *name ;
    name = strtok(message, " ");

    while( name != NULL){
    	memset(buffer, 0, sizeof(buffer));
        printf("%s\n", name);
        strcat(buffer, name);
        // check trang thai user va gui thong bao
		
		struct client *tmpClient = getClientByName(name);

		if (tmpClient != NULL){
			if ( strcmp(tmpClient->title, "") == 0 ) {
				int check = joinRoom(tmpClient->socket, tmpTopic->title);
				if (check == 0) {
					strcat(buffer, " invited successfull");
					char invitation[DATA_SIZE] = "0You has been invited to topic ";
					strcat(invitation, tmpTopic->title);
					write(tmpClient->socket, buffer, strlen(buffer));
				} else if (check == 2) {	//Neu room full nguoi
					strcat(buffer, " moi ko duoc do phong day");
				}
			} else {
				if (strcmp(tmpClient->title, tmpTopic->title) == 0 ){
					strcat(buffer, " already in this topic");
				} else {
					strcat(buffer, " already in topic ");
					strcat(buffer, tmpClient->title);
				}
			}
		} else {
			strcat(buffer, " doesn't exist");
		}

		write(socket, buffer, strlen(buffer));
        name = strtok(NULL, " ");
    }
}

void sendListOnline(int socket){
	struct client *tmpClient;
	char buffer[DATA_SIZE] = "0List user online:";

	for (tmpClient = clients; tmpClient != NULL; tmpClient = tmpClient->next) {
		strcat(buffer, "\n");
		strcat(buffer, tmpClient->username);
	}
	write(socket, buffer, strlen(buffer));
}

void sendListUser(int socket){
	struct topic *tmpTopic = getTopicByTitle(getClientTitleBySocket(socket));
	char buffer[DATA_SIZE] = "0List user in this topic:";
	int i;
	for (i = 0 ; i < tmpTopic->countMember ; i++){
		strcat(buffer, "\n");
		strcat(buffer, getClientUsernameBySocket(tmpTopic->member[i]));
	}
	write(socket, buffer, strlen(buffer));
}

void sendListTopic(int socket){
	struct topic *tmpTopic;
	char buffer[DATA_SIZE] = "0List topic:";
	for (tmpTopic = topics; tmpTopic != NULL; tmpTopic = tmpTopic->next) {
		strcat(buffer, "\n");
		strcat(buffer, tmpTopic->title);
	}
	write(socket, buffer, strlen(buffer));
}

void sendListFile(int socket){
	struct topic *tmpTopic = getTopicByTitle(getClientTitleBySocket(socket));
	char buffer[DATA_SIZE] = "0List file in this topic:";
	char *path = tmpTopic->title;
	// strcat(path, tmpTopic->title);
	DIR *d = opendir(path);
	
	if (d) {
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL) {
			if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
			strcat(buffer, "\n");
			strcat(buffer, dir->d_name);
		}
		closedir(d);
	}
	write(socket, buffer, strlen(buffer));
}

int buildListFileInFolder(char *listFile[], const char *path) {
	DIR *d = opendir(path);
	int i = 0;
	if (d) {
		struct dirent *dir;
		while ((dir = readdir(d)) != NULL) {
			if (!strcmp(dir->d_name, ".") || !strcmp(dir->d_name, "..")) continue;
			listFile[i] = malloc(NAME_SIZE);
			strcpy(listFile[i], dir->d_name);
			i++;
		}
		closedir(d);
	}
	return i;
}

void deleteFolder(const char *path) {
	char *listFile[NAME_SIZE];
	int countFile = buildListFileInFolder(listFile, path);
	int i = 0;
	for (i = 0; i < countFile; i++) {
		char *link;
		int length = strlen(path)+strlen(listFile[i]) + 2;
		link = malloc(length);
		snprintf(link, length, "%s/%s", path, listFile[i]);
		printf("%s", link);
		unlink(link);
		free(link);
	}
	printf("%s",path);
	rmdir(path);
}
