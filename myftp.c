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

int send_socket(int sd, void * buff, int buf_len){
	int n_left = buf_len;
	int n;
	while (n_left > 0){
		if((n = send(sd,buff+(buf_len-n_left),n_left, 0)) < 0){
			if(errno == EINTR)
				n = 0;
			else
				return -1;
		}else if(n == 0){
			return 0;
		}
		n_left -= n;
	}
	return buf_len;
}

int recv_socket(int sd, void * buff, int buf_len){
	int n_left = buf_len;
	int n;
	printf("FIRST READ: %d, remaining: %d \n", n, n_left);
	while (n_left > 0){
	printf("1READ: %d, remaining: %d \n", n, n_left);
		if((n = recv(sd,buff+(buf_len-n_left),n_left, 0)) < 0){
		printf("2READ: %d, remaining: %d \n", n, n_left);
			if(errno == EINTR)
				n = 0;
			else
				return -1;
		}else if(n == 0){
		printf("3READ: %d, remaining: %d \n", n, n_left);
			return 0;
		}
		n_left -= n;
	}
	printf("FINAL READ: %d, remaining: %d \n", n, n_left);
	return buf_len;
}
