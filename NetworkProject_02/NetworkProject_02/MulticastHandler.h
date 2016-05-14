#pragma comment(lib, "ws2_32")

#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include <iostream>
#include <string.h>

#define BUFSIZE 512
using namespace std;

class MulticastHandler {
private :
	int port;
	char ip[20];
	char nickName[20];

	// 데이터 수신에 사용할 변수
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	struct ip_mreq mreq;	// 멀티캐스트 그룹 

	// 데이터 송신에 사용할 변수
	SOCKADDR_IN remoteaddr;
	int len;
	char buf2[BUFSIZE + 1];


public :
	// 송신 소켓
	SOCKET sendSocket;
	// 수신 소켓
	SOCKET recSocket;
	
	MulticastHandler(char *ip, int port, char *nickName) {
		strcpy(this->ip, ip);
		this->port = port;
		strcpy(this->nickName, nickName);
	}

	// 송수신 소켓 초기화
	int initRecSocket();
	int initSendSocket();

	// 송수신 함수
	int send(char *msg);
	int receive(char *msg);

	// 송수신 소켓 종료
	int closeRecSocket();
	int closeSendSocket();

	// 에러 출력 함수
	void err_quit(char *msg);
	void err_display(char *msg);
};