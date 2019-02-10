#include "myftp.h"
#include <pthread.h>

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

void *worker(void *args){
	int flag;
	char* buff[100];
	int *argument = (int *)args;
	int threadnum = *argument;

	pthread_detach(pthread_self());

	recv_socket(*(argument+1), buff, 100);
	printf("%s\n", buff);

	pthread_mutex_lock(&mutex);
	for(int j = 0 ; j < 10 ; j++){
		if(j == -1){
			avaliable[j] = threadnum;
		}
	}
	pthread_mutex_unlock(&mutex);
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
