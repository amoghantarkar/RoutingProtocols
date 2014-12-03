#ifndef SERVER_H_
#define SERVER_H_


/*Declarations*/
#include <stdio.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>


#include "../include/global.h"
#include "../include/logger.h"


/*Declarations*/
void routing_init();
void DVT();
int setup_udp(const char* pno);
void command(char buff[200]);
void* createpacket();
void sendneighbors(void *packetptr);
void receivepacket();
void Bellmanford();
int* findmin(int array[10]);
#endif


