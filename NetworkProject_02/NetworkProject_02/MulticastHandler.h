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

	// ������ ���ſ� ����� ����
	SOCKADDR_IN peeraddr;
	int addrlen;
	char buf[BUFSIZE + 1];
	struct ip_mreq mreq;	// ��Ƽĳ��Ʈ �׷� 

	// ������ �۽ſ� ����� ����
	SOCKADDR_IN remoteaddr;
	int len;
	char buf2[BUFSIZE + 1];


public :
	// �۽� ����
	SOCKET sendSocket;
	// ���� ����
	SOCKET recSocket;
	
	MulticastHandler(char *ip, int port, char *nickName) {
		strcpy(this->ip, ip);
		this->port = port;
		strcpy(this->nickName, nickName);
	}

	// �ۼ��� ���� �ʱ�ȭ
	int initRecSocket();
	int initSendSocket();

	// �ۼ��� �Լ�
	int send(char *msg);
	int receive(char *msg);

	// �ۼ��� ���� ����
	int closeRecSocket();
	int closeSendSocket();

	// ���� ��� �Լ�
	void err_quit(char *msg);
	void err_display(char *msg);
};