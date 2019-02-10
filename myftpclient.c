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
	struct message_s message;
	message.protocol[0] = 109;
	message.protocol[1] = 121;
	message.protocol[2] = 102;
	message.protocol[3] = 116;
	message.protocol[4] = 112;
	message.type = 0xA1;
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

void downloadfile(int sd, char *filename){

}

void uploadfile(int sd, char *filename){

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
