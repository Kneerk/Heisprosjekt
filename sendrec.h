#pragma once

#include<stdio.h> //printf
#include<string.h> //memset
#include<stdlib.h> //exit(0);
#include<arpa/inet.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<unistd.h>
#include<string>

#define BUFLEN 512  //Max length of buffer
#define BROADCASTPORT 8000   //The Broadcastport
#define MASTERPORT 20019 //The localport
//#define OURADDRESS "129.241.187.157"
//#define SADDRESS "129.241.187.155"

extern int bsocket, lsocket;

struct code_message{
	char * rip;
	int port;
	char * data;
	int length;
};

void die(char *);

void udp_init(int localport);
int udp_Broadcaster(std::string message);
int udp_Sender(std::string message, int localport, char* ip);
struct code_message udp_Reciever();
struct code_message udp_recieve_broadcast();
void udp_close();

int Bytes();
