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
	message.length = htonl(10);
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
	int remainlen = ntohl(header->length) - 10;
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
	message.length = htonl(10 + strlen(filename) + 1);
	int len;
	if((len=send(sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
	//send payload
//	printf("Send request Payload: %s\n", filename);
	send_socket(sd, filename, strlen(filename)+1);

	//get Header
	unsigned char *buff[100];
	FILE *fp;
	long fsize;
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *header = malloc(sizeof(struct message_s));
	memcpy(header,buff,10);
	//printf("header: %s, %d, %d\n", header->protocol, header->type, header->length);
	if(header->type == 0xB3){
		//Error
		printf("File not exist.\n");
		return;
	}else{
		fp = fopen(filename, "wb");
	}

	//get file Header
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
	struct message_s *fileheader = malloc(sizeof(struct message_s));
	memcpy(fileheader,buff,10);
//	printf("header: %d, %d\n", fileheader->type, fileheader->length);

	//get file payload
	int state;
	int remainlen = ntohl(fileheader->length) - 10;
	//printf("total length: %d\n", remainlen);
	while((state = recv_socket(sd, buff, remainlen>100?100:remainlen)) > 0){
		//printf("Buff length: %d\n", remainlen>100?100:remainlen );
		//printf("%u\n\n", buff);
		fwrite (buff , sizeof(char), remainlen>100?100:remainlen, fp);
		remainlen -= 100;
		if(remainlen <= 0) break;
	}
		//printf("State: %d\n", state );

	//wrap up
	fclose(fp);
	free(header);
	free(fileheader);
}

void uploadfile(int sd, char *filename){
	FILE *fp;
	int len;
	char *buff[100];
	long fsize;
	//check file exist
	fp = fopen(filename, "rb");
	if (fp == NULL){
		printf("file not exist.\n");
		exit(0);
	}
	//send header
	struct message_s message;
	message.protocol[0] = 109; //m
	message.protocol[1] = 121; //y
	message.protocol[2] = 102; //f
	message.protocol[3] = 116; //t
	message.protocol[4] = 112; //p
	message.type = 0xC1;
	message.length = htonl(10 + strlen(filename) + 1);
	if((len=send(sd, &message , sizeof(message),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		fclose(fp);
		exit(0);
	}
	//send payload
	send_socket(sd, filename, strlen(filename)+1);

	//get Header
	if((len=recv(sd, buff,sizeof(struct message_s),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		fclose(fp);
		exit(0);
	}
	struct message_s *header = malloc(sizeof(struct message_s));
	memcpy(header,buff,10);
	if(header->type != 0xC2){
		printf("Server error Cannot upload\n");
		fclose(fp);
		free(header);
		exit(0);
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

	//send file (header)
	struct message_s fileheader;
	fileheader.protocol[0] = 109; //m
	fileheader.protocol[1] = 121; //y
	fileheader.protocol[2] = 102; //f
	fileheader.protocol[3] = 116; //t
	fileheader.protocol[4] = 112; //p
	fileheader.type = 0xFF;
	fileheader.length = htonl(10 + fsize);
	if((len=send(sd, &fileheader , sizeof(fileheader),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		fclose(fp);
		free(header);
		free(data);
		exit(0);
	}
	//send file (file)
	send_socket(sd, data, fsize);
	free(data);

	//wrap up
	free(header);
	fclose(fp);
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
		if(argc == 5){downloadfile(sd, argv[4]);}
		else{printf("no file specified.\n");}
	}else if(strcmp(argv[3],"put") == 0){
		if(argc == 5){uploadfile(sd, argv[4]);}
		else{printf("no file specified.\n");}
	}

	//*****************WRAP UP***********************
	close(sd);
	return 0;
}
