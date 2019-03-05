#include "mysr.h"

//base_num: oldest unack packet
//seq_num: sequence number of data packet
//mutex: mutex lock
int base_num = 1, seq_num = 1;
pthread_mutex_t base_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t seq_mutex = PTHREAD_MUTEX_INITIALIZER;

void send_ack(struct mysr_receiver* mysr_receiver, int seq_num){
  struct MYSR_Packet ack_packet;
  int n;

  //prepare ack data packet
  ack_packet.protocol[0] = 115; //s
  ack_packet.protocol[1] = 114; //r
  ack_packet.type = 0xA1;
  ack_packet.seqNum = htonl(seq_num);
  ack_packet.length = htonl(11);

  //send packet
  if((n = sendto(mysr_receiver->sd,
                  &ack_packet ,
                  11 ,
                  0,
                  (struct sockaddr *)&(mysr_receiver->address),
                  sizeof(mysr_receiver->address)
                )
      ) < 0){
    if(errno == EINTR){
      printf("ack INTERRUPT\n");
      n = 0;
    }else{
      printf("ack Error: %s (Errno:%d)\n",strerror(errno),errno);
    }
  }else if(n == 0){
    printf("ack is 0, not send\n");
  }

  printf("sent ACK with seqNum %d.\n", seq_num);
}

void send_end(struct mysr_sender* mysr_sender, int seq_num){
  struct MYSR_Packet end_packet;
  int n;

  //prepare ack data packet
  end_packet.protocol[0] = 115; //s
  end_packet.protocol[1] = 114; //r
  end_packet.type = 0xA2;
  end_packet.seqNum = htonl(seq_num);
  end_packet.length = htonl(11);

  //send packet
  if((n = sendto(mysr_sender->sd,
                  &end_packet ,
                  11 ,
                  0,
                  (struct sockaddr *)&(mysr_sender->serverAddr),
                  sizeof(mysr_sender->serverAddr)
                )
      ) < 0){
    if(errno == EINTR){
      printf("end INTERRUPT\n");
      n = 0;
    }else{
      printf("end Error: %s (Errno:%d)\n",strerror(errno),errno);
    }
  }else if(n == 0){
    printf("end is 0, not send\n");
  }

  printf("sent end with seqNum %d.\n", seq_num);
}

void *receieve_ack(void *arg){
  //initialize
  //mysr_sender: sender struct
  //n: size receieved
  //header_buff: buffer for recording header
  struct mysr_sender mysr_sender = (struct mysr_sender*) arg;
  int n;
  unsigned char *header_buff[11];
  socklen_t recv_addr_len = sizeof(mysr_sender->serverAddr);

  while (1) {
    //receieve header
    if((n = recvfrom(mysr_sender->sd,
                      header_buff ,
                      sizeof(header_buff) ,
                      0,
                      (struct sockaddr *)&(mysr_sender->address) ,
                      &recv_addr_len
                    )
    ) < 0){
      if(errno == EINTR){
        printf("recv-ack INTERRUPT\n");
        n = 0;
      }else{
        printf("recv-ack Error: %s (Errno:%d)\n",strerror(errno),errno);
        return -1;
      }
    }else if(n == 0){
      printf("recv-ack n is 0\n");
      return 0;
    }

    //analysis header
    struct MYSR_Packet *header = malloc(sizeof(struct MYSR_Packet));
    memcpy(header,header_buff,11);
    if( header->type == 0xA1 ){    //it is a ack packet
      printf("Receieved ack packet for seqNum: %d.\n", header->seqNum);
      pthread_mutex_lock(&base_mutex);
      base_num++;
      pthread_mutex_unlock(&base_mutex);
    }
  }
}

void mysr_init_sender(struct mysr_sender* mysr_sender, char* ip, int port, int N, int timeout){
  mysr_sender->sd = socket(AF_INET,SOCK_DGRAM,0);
  mysr_sender->serverIP = ip;
  mysr_sender->serverPort = port;
  mysr_sender->windowSize = N;
  mysr_sender->timeout = timeout;

  // initialize the address of server
  struct sockaddr_in destination;
  memset(&destination, 0, sizeof(struct sockaddr_in));
  destination.sin_family = AF_INET;
  inet_pton(AF_INET, mysr_sender->serverIP, &(destination.sin_addr));
  destination.sin_port = htons(mysr_sender->serverPort);
  mysr_sender->serverAddr = destination;
}

int mysr_send(struct mysr_sender* mysr_sender, unsigned char* buf, int len){
  //initialize variables
  //n_left: unsent data size
  //n: accurate number sent by function
  //n_sent: sent data size
  //n_start: index of place to start sending
  //n_to_be_send: data size to be sent
  //ack_thread: thread to receieve ack
  int n_left = len, n, n_sent = 0, n_start, n_to_be_send;
  struct MYSR_Packet data_message;
  pthread_t ack_thread[1];

  //set thread for receieve ack packet
  int ret_val = pthread_create(&thread[0], NULL, receieve_ack, mysr_sender);

  while (n_left > 0){
    //calculate data to send
    n_start = n_sent + (len - n_left);
    n_to_be_send = (n_left > MAX_PAYLOAD_SIZE) ? MAX_PAYLOAD_SIZE : n_left;

    //prepare send data packet
    data_message.protocol[0] = 115; //s
    data_message.protocol[1] = 114; //r
    data_message.type = 0xA0;
    data_message.seqNum = htonl(seq_num);
    data_message.length = htonl(11 + n_to_be_send);
    memcpy(data_message.payload,buf + n_start, n_to_be_send);

    //send
    if((n = sendto(mysr_sender->sd,
                    &data_message ,
                    sizeof(data_message) ,
                    0,
                    (struct sockaddr *)&(mysr_sender->serverAddr),
                    sizeof(mysr_sender->serverAddr)
                )
        ) < 0){
      if(errno == EINTR){
        printf("send INTERRUPT\n");
        n = 0;
      }else{
        printf("Send Error: %s (Errno:%d)\n",strerror(errno),errno);
        return -1;
      }
    }else if(n == 0){
      printf("n is 0\n");
      return 0;
    }
    printf("sent data packet with seqNum %d.\n", seq_num);


    //finish send, wrap up
    n_sent  = n_start + n_to_be_send;
    n_left -= n_to_be_send;
    pthread_mutex_lock(&seq_mutex);
    seq_num++;
    pthread_mutex_unlock(&seq_mutex);
  }

  //end sent all data packet, send end packet
  send_end(mysr_sender, seq_num);

  return len;
}

void mysr_close_sender(struct mysr_sender* mysr_sender){
  close(mysr_sender->sd);
}

void mysr_init_receiver(struct mysr_receiver* mysr_receiver, int port){
  mysr_receiver->sd = socket(AF_INET,SOCK_DGRAM,0);
  long val = 1;
  if(setsockopt(mysr_receiver->sd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(long)) == -1){
    perror("setsockopt");
    exit(-1);
  }
  mysr_receiver->selfPort = port;
  struct sockaddr_in address;
  mysr_receiver->address = address;
  memset(&(mysr_receiver->address), 0, sizeof(mysr_receiver->address));
  mysr_receiver->address.sin_family = AF_INET;
  mysr_receiver->address.sin_port = htons(mysr_receiver->selfPort);
  mysr_receiver->address.sin_addr.s_addr = htonl(INADDR_ANY);
  bind(mysr_receiver->sd, (struct sockaddr *)&(mysr_receiver->address), sizeof(struct sockaddr));
}

int mysr_recv(struct mysr_receiver* mysr_receiver, unsigned char* buf, int len){
  //initialize
  //header_buff: buffer for recording header
  //payload_buff: buffer for recording payload
  //end_flag: check if data sent end
  //n: data size receieved
  //payload_len: receieved payload size
  //recv_addr_len: length of mysr_receiever->address
  unsigned char *header_buff[11];
  unsigned char *payload_buff[MAX_PAYLOAD_SIZE];
  int end_flag = 0;
  int n;
  int payload_len;
  socklen_t recv_addr_len = sizeof(mysr_receiver->address);

  while (end_flag == 0) {
    //receieve header
    if((n = recvfrom(mysr_receiver->sd,
                      header_buff ,
                      sizeof(header_buff) ,
                      0,
                      (struct sockaddr *)&(mysr_receiver->address) ,
                      &recv_addr_len
                    )
        ) < 0){
      if(errno == EINTR){
        printf("recv INTERRUPT\n");
        n = 0;
      }else{
        printf("recv Error: %s (Errno:%d)\n",strerror(errno),errno);
        return -1;
      }
    }else if(n == 0){
      printf("recv n is 0\n");
      return 0;
    }

    //analysis header
    struct MYSR_Packet *header = malloc(sizeof(struct MYSR_Packet));
    memcpy(header,header_buff,11);
    if( header->type == 0xA0 ){    //it is a data packet
      //analysis data size to receieve
      payload_len = ntohl(header->length) - 11;

      //receieve payload
      if((n = recvfrom(mysr_receiver->sd,
                        payload_buff ,
                        payload_len ,
                        0,
                        (struct sockaddr *)&(mysr_receiver->address),
                        &recv_addr_len
                      )
          ) < 0){
        if(errno == EINTR){
          printf("recv INTERRUPT\n");
          n = 0;
        }else{
          printf("recv Error: %s (Errno:%d)\n",strerror(errno),errno);
          return -1;
        }
      }else if(n == 0){
        printf("recv n is 0\n");
        return 0;
      }

      //concat buffer
      unsigned char allstring[sizeof(buf)+sizeof(payload_buff)];
      memcpy(allstring, buf, sizeof(buf));
      memcpy(allstring+sizeof(buf), payload_buff, sizeof(payload_buff));
      *buf = *allstring;

      //finished send ack packet
      send_ack(mysr_receiver, ntohl(header->seqNum));

    }else if( header->type == 0xA2){//it is a end packet
      printf("receieved an end packet\n" );
      send_ack(mysr_receiver, ntohl(header->seqNum));
      end_flag = 1; // break loop
    }

  }

  return len;
}

void mysr_close_receiver(struct mysr_receiver* mysr_receiver) {
  close(mysr_receiver->sd);
}
