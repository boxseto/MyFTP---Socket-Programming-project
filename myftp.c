#include "myftp.h"

int socket_create(){
	int sd=socket(AF_INET,SOCK_STREAM,0);
	return sd;
}

void getHostByName(char * hostname){
	struct hostent * he;
	struct in_addr ** addrList;
	he = gethostbyname(hostname);
	if(he == NULL){
		perror("gethostbyname()");
		exit(2);
	}
	printf("Host Name: %s\n", he->h_name);
	printf("IP Address(es):\n");
	addrList=(struct in_addr**)he->h_addr_list;
	int i;
	for( i = 0 ; addrList[i] != NULL ; i++){
		printf("%s\n", inet_ntoa(*addrList[i]));
	}
}

void send_socket(int sd, char * buff){
	int len;
	if((len=send(sd,buff,strlen(buff),0))<0){
		printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
		exit(0);
	}
}

void recv_socket(int client_sd, char * buff){
	int len;
	if((len=recv(client_sd,buff,sizeof(buff),0))<0){
		printf("receive error: %s (Errno:%d)\n", strerror(errno),errno);
		exit(0);
	}
}
