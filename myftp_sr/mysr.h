/*
 * mysr.h
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#ifndef __mysr_h__
#define __mysr_h__

#define MAX_PAYLOAD_SIZE 512

struct MYSR_Packet {
  unsigned char protocol[2];             // protocol string (2 bytes) "sr"
  unsigned char type;                    // type (1 byte)
  unsigned int seqNum;                   // sequence number (4 bytes)
  unsigned int length;                   // length(header+payload) (4 bytes)
  unsigned char payload[MAX_PAYLOAD_SIZE];    // payload data
} __attribute__((packed));

struct mysr_sender {
  int sd; // SR sender socket
  // ... other member variables
  char* serverIP;
  int serverPort;
  int windowSize;
  int timeout;
  struct sockaddr_in serverAddr;
};

void mysr_init_sender(struct mysr_sender* mysr_sender, char* ip, int port, int N, int timeout);
int mysr_send(struct mysr_sender* mysr_sender, unsigned char* buf, int len);
void mysr_close_sender(struct mysr_sender* mysr_sender);

struct mysr_receiver {
  int sd; // SR receiver socket
  // ... other member variables
  int selfPort;
  struct sockaddr_in address;
};

void mysr_init_receiver(struct mysr_receiver* mysr_receiver, int port);
int mysr_recv(struct mysr_receiver* mysr_receiver, unsigned char* buf, int len);
void mysr_close_receiver(struct mysr_receiver* mysr_receiver);

#endif
