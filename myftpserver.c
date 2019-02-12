#include "myftp.h"
#include <pthread.h>
#include <dirent.h>

pthread_mutex_t mutex;
int avaliable[10];

void bind_socket(int sd, int port){
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=htonl(INADDR_ANY);
	server_addr.sin_port=htons(port);
	if(bind(sd,(struct sockaddr *) &server_addr,sizeof(server_addr))<0){
		printf("bind error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
}

void listen_socket(int sd){
	if(listen(sd,10)<0){
		printf("listen error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
}

int accept_socket(int sd){
	int client_sd;
	struct sockaddr_in client_addr;
	int addr_len=sizeof(client_addr);
	if((client_sd=accept(sd,(struct sockaddr *) &client_addr,&addr_len))<0){
		printf("accept erro: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	return client_sd;
}

struct message_s forgereply(unsigned char type, int payloadlen){
	struct message_s message;
	message.protocol[0] = 109; //m
	message.protocol[1] = 121; //y
	message.protocol[2] = 102; //f
	message.protocol[3] = 116; //t
	message.protocol[4] = 112; //p
	message.type = type;
	message.length = 10 + payloadlen;
	return message;
}

void sendfilelist(int client_sd){
	//prepare files to send
	char* dirholder = malloc(5000*sizeof(char));
	*dirholder = '\0';
	DIR *d;
	struct dirent *dir;
	d = opendir(".");
	if(d){
		while ((dir = readdir(d)) != NULL){
			if(dir->d_type == 8){ //DT_REG == 8
				strcat(dirholder,dir->d_name);
				strcat(dirholder,"\n");
			}
		}
		strcat(dirholder,"\0");
		closedir(d);
	}else{
		printf("Cannot open directory.\n");
	}
	//printf("Send length: %d\n contents:\n",strlen(dirholder)+1);
	//puts(dirholder);

	//send file header
	struct message_s message = forgereply(0xA2, strlen(dirholder)+1);
	int len;
	if((len=send(client_sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
	}

	//send file payload
	send_socket(client_sd, dirholder, strlen(dirholder)+1);

	//wrap up
	free(dirholder);
}

void downloadfile(int client_sd, char *filename){
	FILE *fp;
	int len;
	long fsize;
	char fullfilename[202];
	fullfilename[0] = '\0';
	struct message_s reply;
	struct message_s filereply;

	//open files
	strcat(fullfilename, "./");
	strcat(fullfilename, filename);
	printf("file to download: %s\n", filename);
	fp = fopen(fullfilename, "rb");
	if (fp == NULL){
		printf("file not exist. Send error header\n");
		//send error message header
		reply = forgereply(0xB3, 0);
		if((len=send(client_sd, &reply , sizeof(reply),0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			return;
		}
		return;
	}else{
		printf("file exist. Send OK header\n");
		//send exist message header
		reply = forgereply(0xB2, 0);
		if((len=send(client_sd, &reply , sizeof(reply),0))<0){
			printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
			return;
		}
	}

	//get filesize
	fseek(fp, 0, SEEK_END);
	fsize = ftell(fp);
	fseek(fp, 0, SEEK_SET);
	printf("file size prepare: %d\n", fsize);

	//read whole data into buffer
	char *data = malloc(fsize + 1);
	*data = '\0';
	fread(data, fsize, 1, fp);
	data[fsize] = '\0';
	//wrap up
	fclose(fp);

	//send file (header)
	filereply = forgereply(0xFF, strlen(data));
	if((len=send(client_sd, &filereply , sizeof(filereply),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		return;
	}
	//send file (file)
	send_socket(client_sd, data, strlen(data));
	free(data);
}

void uploadfile(int client_sd, char *filename){
	struct message_s reply;
	int len;
	//send OK reply
	reply = forgereply(0xC2, 0);
	if((len=send(client_sd, &reply , sizeof(reply),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		return;
	}
	//open File
	char *buff[100];
	FILE *fp;
	long fsize;
	fp = fopen(filename, "w");
	//get file Header
	if((len=recv(client_sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *fileheader = malloc(sizeof(struct message_s));
	memcpy(fileheader,buff,10);
	//printf("header: %s, %d, %d\n", fileheader->protocol, fileheader->type, fileheader->length);

	//get file payload
	int state;
	int remainlen = fileheader->length - 10;
	while((state = recv_socket(client_sd, buff, remainlen>100?100:remainlen)) > 0){
		//printf("Buff length: %d, Content: \n", remainlen>100?100:remainlen );
		//printf("%s\n\n", buff);
		fwrite (buff , sizeof(char), remainlen>100?100:remainlen, fp);
		remainlen -= 100;
		if(remainlen <= 0) break;
	}

	//wrap up
	fclose(fp);
	free(fileheader);

}

void *worker(void *args){
	int flag;
	char* buff[100];
	char filename[200];
	filename[0] = '\0';
	int state, remainlen;
	int *argument = (int *)args;
	int threadnum = *argument;

	pthread_detach(pthread_self());
//get header
	int len;
	if((len=recv(*(argument+1), buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		pthread_exit(NULL);
	}
	struct message_s *header = malloc(sizeof(struct message_s));
	memcpy(header,buff,10);
	//printf("header: %s, %d, %d\n", header->protocol, header->type, header->length);
	if(header->type == 0xA1){
		sendfilelist(*(argument+1));
	}else if(header->type == 0XB1){
		//get payload
		remainlen = header->length - 10;
		recv_socket(*(argument+1), filename, remainlen);
//		printf("File to download: %s\n", filename);
		downloadfile(*(argument+1), filename);
	}else if(header->type == 0XC1){
		//get payload
		remainlen = header->length - 10;
		recv_socket(*(argument+1), filename, remainlen);
		printf("File to upload: %s\n", filename);
		uploadfile(*(argument+1), filename);
	}else{
		printf("Header error: Wrong header type\n");
		free(header);
		pthread_exit(NULL);
	}


	pthread_mutex_lock(&mutex);
	for(int j = 0 ; j < 10 ; j++){
		if(j == -1){
			avaliable[j] = threadnum;
		}
	}
	pthread_mutex_unlock(&mutex);
	free(header);
	pthread_exit(NULL);
}

int main(int argc, char **argv){
  pthread_t thread[10];
	int sd, client_sd, i = 0, avaliable_flag, avaliable_temp;
  pthread_mutex_init(&mutex, NULL);
	pthread_mutex_lock(&mutex);
	for(int j = 0 ; j < 10 ; j++){
		avaliable[j] = j;
	}
	pthread_mutex_unlock(&mutex);
//**************CONNECTION****************
	if(argc == 1){printf("Usage: >./myftpserver PORT_NUMBER\n"); return 0;}
	else {
		sd = socket_create();
		bind_socket(sd, (int)strtol(argv[1], NULL, 10));
	}
//**************END CONNECTION****************

//****************MAIN THREAD*****************
	while(1){
		listen_socket(sd);
		client_sd = accept_socket(sd);
		pthread_mutex_lock(&mutex);
		avaliable_flag = 0;
		for(int j = 0 ; j < 10 ; j++){
			if(avaliable_flag == 1){
				avaliable_temp = avaliable[j];
				avaliable[j] = avaliable[j-1];
				avaliable[j-1] = avaliable_temp;
			}else if(i == j){
				avaliable[j] = -1;
				avaliable_flag = 1;
			}
		}
		pthread_mutex_unlock(&mutex);
		int arg[2] = {i,client_sd};
		printf("%s%d\n", "connected to ", client_sd);
		int ret_val = pthread_create(&thread[i], NULL, worker, arg);
		i++;
	}
//****************END MAIN********************

//****************WRAP UP**************************
	close(sd);
	return 0;
}
