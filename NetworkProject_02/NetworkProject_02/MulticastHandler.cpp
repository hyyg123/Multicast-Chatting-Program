#include "MulticastHandler.h"

int MulticastHandler::initRecSocket() {
	int retval;

	// 원속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;

	// socket()
	recSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (recSocket == INVALID_SOCKET) {
		//err_quit("socket()");
		return 0;
	}
	// SO_RESUEADDR 옵션 설정
	BOOL optval = TRUE;
	retval = setsockopt(recSocket, SOL_SOCKET, SO_REUSEADDR, 
		(char*)&optval, sizeof(optval));
	if (retval == SOCKET_ERROR) {
		//err_quit("setsockopt()");
		return 0;
	}

	// bind()
	SOCKADDR_IN localaddr;
	ZeroMemory(&localaddr, sizeof(localaddr));
	localaddr.sin_family = AF_INET;
	localaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	localaddr.sin_port = htons(port);
	retval = bind(recSocket, (SOCKADDR*)&localaddr, sizeof(localaddr));

	// 멀티캐스트 그룹 가입
	mreq.imr_multiaddr.s_addr = inet_addr(ip);
	mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	retval = setsockopt(recSocket, IPPROTO_IP, IP_ADD_MEMBERSHIP,
		(char*)&mreq, sizeof(mreq));
	if (retval == SOCKET_ERROR) {
		//err_quit("setsockopt()");
		return 0;
	}
	return 1;
}

int MulticastHandler::receive(char *msg) {
	int retval;

	// 데이터 받기
	addrlen = sizeof(peeraddr);
	retval = recvfrom(recSocket, buf, BUFSIZE, 0,
		(SOCKADDR*)&peeraddr, &addrlen);
	if (retval == SOCKET_ERROR) {
		//err_display(msg);
		WSASetLastError(retval);
		return -1;
	}

	// 받은 데이터 출력
	buf[retval] = '\0';
	strcpy(msg, buf);
	return 1;
}

int MulticastHandler::closeRecSocket() {
	int retval;
	retval = setsockopt(recSocket, IPPROTO_IP, IP_DROP_MEMBERSHIP,
		(char*)&mreq, sizeof(mreq));
	if (retval == SOCKET_ERROR) return 0;

	closesocket(recSocket);
	//WSACleanup();
	return 1;
}

int MulticastHandler::initSendSocket() {
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 0;

	// socket()
	sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if (sendSocket == INVALID_SOCKET) return 0;

	// 멀티캐스트 TTL 설정
	int ttl = 2;
	retval = setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_TTL,
		(char*)&ttl, sizeof(ttl));
	if (retval == SOCKET_ERROR) return 0;

	// 소켓 주소 구조체 초기화
	ZeroMemory(&remoteaddr, sizeof(remoteaddr));
	remoteaddr.sin_family = AF_INET;
	remoteaddr.sin_addr.s_addr = inet_addr(ip);
	remoteaddr.sin_port = htons(port);

	return 1;
}

int MulticastHandler::send(char *msg) {
	int retval;

	strcpy(buf2, msg);
	/*if (fgets(buf2, BUFSIZE + 1, stdin) == NULL)
		return 0;*/

	// '\n' 문자 제거
	len = strlen(buf2);
	if (buf2[len - 1] == '\n')
		buf[len - 1] = '\0';
	if (strlen(buf2) == 0)
		return 0;

	// 데이터 보내기
	retval = sendto(sendSocket, buf2, strlen(buf2), 0,
		(SOCKADDR*)&remoteaddr, sizeof(remoteaddr));
	if (retval == SOCKET_ERROR) return -1;

	return 1;
}

int MulticastHandler::closeSendSocket() {
	closesocket(sendSocket);
	//WSACleanup();
	return 1;
}

// 소켓 함수 오류 출력 후 종료
void MulticastHandler::err_quit(char *msg) {
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	MessageBox(NULL, (LPCTSTR)lpMsgBuf, (LPCWSTR)msg, MB_ICONERROR);
	LocalFree(lpMsgBuf);
	exit(1);
}

// 소켓 함수 오류 출력
void MulticastHandler::err_display(char *msg)
{
	LPVOID lpMsgBuf;
	FormatMessage(
		FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
		NULL, WSAGetLastError(),
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf, 0, NULL);
	printf("[%s] %s", msg, (char *)lpMsgBuf);
	LocalFree(lpMsgBuf);
}