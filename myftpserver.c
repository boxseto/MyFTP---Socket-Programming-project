#include "myftp.h"

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
}

int main(int argc, char **argv){
	//**************CONNECTION****************
	int sd, client_sd;
	if(argc == 1){printf("Usage: >./myftpserver PORT_NUMBER"); return 0;}
	else {
		sd = socket_create();
		bind_socket(sd, (int)strtol(argv[1], NULL, 10));
	}
	//**************END CONNECTION****************

	//****************MAIN THREAD*****************
	while(1){
		client_sd = accept_socket(sd);
		int ret_val = pthread_create(&thread[i], NULL, pthread_prog, &arg[i]);
	}
	//****************END MAIN********************

	//****************WRAP UP**************************
	close(sd);
	return 0;
}
