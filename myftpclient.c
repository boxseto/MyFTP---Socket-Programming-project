#include "myftp.h"

int sd;

int connect_socket(char *ipaddr, int port){
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

void listfile(){
	message_s message;
	message->protocol = "myftp";
	message->type = 0xA1;
	message->length = 10;
	if((len=send(sd,message,message->length,0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
}

void downloadfile(char *filename){

}

void uploadfile(char *filename){

}


int main(int argc, char **argv) {
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
		listfile();
	}else if(strcmp(argv[3],"get") == 0){
		if(argc == 5){downloadfile(argv[5]);}
		else{printf("no file specified.\n");}
	}else if(strcmp(argv[3],"put") == 0){
		if(argc == 5){uploadfile(argv[5]);}
		else{printf("no file specified.\n");}
	}

	//*****************WRAP UP***********************
	close(sd);
	return 0;
}
