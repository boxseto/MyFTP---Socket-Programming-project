#include "myftp.h"

int connect_socket(int sd, char *ipaddr, int port){
	struct sockaddr_in server_addr;
	memset(&server_addr,0,sizeof(server_addr));
	server_addr.sin_family=AF_INET;
	server_addr.sin_addr.s_addr=inet_addr(ipaddr);
	server_addr.sin_port=htons(port);
	if(connect(sd,(struct sockaddr *)&server_addr,sizeof(server_addr))<0){
		printf("connection error: %s (Errno:%d)\n",strerror(errno),errno);
		return 0;
	}
	return 1;
}

void listfile(int sd){
	//send header
	struct message_s message;
	message.protocol[0] = 109; //m
	message.protocol[1] = 121; //y
	message.protocol[2] = 102; //f
	message.protocol[3] = 116; //t
	message.protocol[4] = 112; //p
	message.type = 0xA1;
	message.length = 10;
	int len;
	if((len=send(sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}

	//get file Header
	char *buff[100];
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *header = malloc(sizeof(struct message_s));
	memcpy(header,buff,10);
	//printf("header: %s, %d, %d", header->protocol, header->type, header->length);

	//get file payload
	int state;
	int remainlen = header->length - 10;
	while((state = recv_socket(sd, buff, remainlen>100?100:remainlen)) > 0){
		printf("%s", buff);
		remainlen -= 100;
		if(remainlen <= 0) break;
	}
	free(header);
}

void downloadfile(int sd, char *filename){
	//send header
	struct message_s message;
	message.protocol[0] = 109; //m
	message.protocol[1] = 121; //y
	message.protocol[2] = 102; //f
	message.protocol[3] = 116; //t
	message.protocol[4] = 112; //p
	message.type = 0xB1;
	message.length = 10 + strlen(filename) + 1;
	int len;
	if((len=send(sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	//send payload
	send_socket(client_sd, filename, strlen(filename)+1);

	//get Header
	char *buff[100];
	FILE *fp;
	long fsize;
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *header = malloc(sizeof(struct message_s));
	memcpy(header,buff,10);
	if(header->type == 0xB3){
		//Error
		printf("File not exist.\n", );
		return;
	}else{
		fp = fopen(filename, "w");
	}

	//get file Header
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *fileheader = malloc(sizeof(struct message_s));
	memcpy(fileheader,buff,10);
	//printf("header: %s, %d, %d", header->protocol, header->type, header->length);

	//get file payload
	int state;
	int remainlen = fileheader->length - 10;
	while((state = recv_socket(sd, buff, remainlen>100?100:remainlen)) > 0){
		fwrite (buff , sizeof(char), sizeof(buff), fp);
		remainlen -= 100;
		if(remainlen <= 0) break;
	}
	fclose(fp);
}

void uploadfile(int sd, char *filename){
	//send header
	struct message_s message;
	message.protocol[0] = 109; //m
	message.protocol[1] = 121; //y
	message.protocol[2] = 102; //f
	message.protocol[3] = 116; //t
	message.protocol[4] = 112; //p
	message.type = 0xC1;
	message.length = 10;
	int len;
	if((len=send(sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}


	char *buff[100];
	int state;
	while((state = recv_socket(sd, buff,100)) > 0){
		printf("%s\n", buff);
	}

}


int main(int argc, char **argv) {
	int sd;
	//**************CONNECTION****************
	int connection = 0;
	if(argc < 4){printf("Usage: >./myftpclient SERVER_IP SERVER_PORT COMMAND FILE\n"); return 0;}
	else {
		sd = socket_create();
		connection = connect_socket(sd, argv[1], (int)strtol(argv[2], NULL, 10));
	}
	if(connection == 0){return 0;}
	//**************CONNECTION END****************

	//**************PARSE COMMAND*****************
	if(strcmp(argv[3], "list") == 0){
		listfile(sd);
	}else if(strcmp(argv[3],"get") == 0){
		if(argc == 5){downloadfile(sd, argv[5]);}
		else{printf("no file specified.\n");}
	}else if(strcmp(argv[3],"put") == 0){
		if(argc == 5){uploadfile(sd, argv[5]);}
		else{printf("no file specified.\n");}
	}

	//*****************WRAP UP***********************
	close(sd);
	return 0;
}
