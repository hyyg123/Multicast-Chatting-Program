//#pragma comment(lib, "ws2_32")

#include "MulticastHandler.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <time.h>
#include <chrono>
#include <atlstr.h>

using namespace std;

// 컨트롤들의 아이디값
#define ACC_BTN			10
#define SEND_BTN		11

#define IP_EDIT			100
#define PORT_EDIT		101
#define NICK_EDIT		102
#define MSG_EDIT		103
#define SHOW_TEXT		104

// 컨트롤들의 배치를 위한 정의
#define MARGINE			20
#define EDIT_W			100
#define EDIT_H			20
#define BTN_H			30
#define STEP_HEIGHT		40

// 위도우 크기
#define WIDTH		540
#define HEIGHT		600

// 대화명 중복 플래그 값
#define _OVER		"1"

// 대화명 중복 경고 문구
#define WARNING "!!! 대화명이 중복됩니다. 변경해주세요! !!!"

// // // // 변수 선언 // // // //

HINSTANCE hInst;					// 인스턴스 공유
MulticastHandler *mulHandler;		// 멀티캐스트 핸들러 객체

HANDLE rThread, sThread;			// 리시브, 샌드 쓰레드

char ip[20];						// 입력받은 IP 저장
char nickName[50];					// 입력받은 대화명 저장
int port;							// 입력받은 포트 저장

char nickCheckInfo[15] = "[nci]";	// 대화명 중복 검사를 위한 패킷 헤더
char nickAckInfo[15] = "[nai]";		// 대화명 중복 여부를 위한 패킷 헤더
char idFlag[10] = "[id]";			// 클라이언트를 구별하기 위한 ID값 (시간으로 구별함)

char recMsg[BUFSIZE + 1];		// 	수신받은 메시지 버퍼 저장
char sendMsg[BUFSIZE + 1];			// 송신할 메시지 버퍼
char sendNmsg[BUFSIZE + 1];			// 대화명 중복 검사 패킷을 위한 송신 버퍼

int sendFlag = 0;					// 전송 플래그, 1이면 전송
int sendFlagNmsg = 0;				// 대화명 중복 검사 전송 플래그, 1이면 전송

int isAccess = 0;					// 접속 여부
int exitFlag = 0;					// 연결 종료 여부
int isExist = 0;					// 대화명 중복 여부

unsigned long sendTime;				// 클라이언트 구별을 위한 ID값 (시간으로 구별)

// IP, PORT, NICKNAME, MSG, SHOW 등의 컨트롤들
HWND ipText, portText, nickText, msgText;
HWND ipEdit, portEdit, nickEdit, msgEdit;
HWND accServerBtn, sendMsgBtn;
HWND showText;

// 윈도우 클래스 제목
static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("NetworkProject02");
// 메시지박스 제목
static TCHAR msgTitle[] = _T("경고창");
// 메시지박스 속성
int msgBoxA = MB_OK | MB_ICONERROR;

// 동기화
CRITICAL_SECTION cs;

// // // // 함수 선언 // // // //
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// 윈도우 이벤트 함수 정의
void _WM_CREATE(HWND, UINT, WPARAM, LPARAM);
void _WM_COMMAND(HWND, UINT, WPARAM, LPARAM);

// 서버 접속시 쓰이는 함수들
int checkIp(char *);			// 아이피 검사
int checkPort(char *);			// 포트 검사
int checkAccess(HWND hWnd);		// 아이피, 포트 검사

int accessServer(HWND hWnd);	// 서버 접속
void setMsg(HWND hWnd);			// 송신 메시지 설정

void showMessage(char *msg);	// 메시지 출력

DWORD WINAPI ReceiveProcess(LPVOID arg);	// 수신 스레드 프로세스 함수
DWORD WINAPI SendProcess(LPVOID arg);		// 송신 스레드 프로세스 함수

void sendMyNickInfo();				// 자신의 대화명을 전송 (중복 검사를 위해)
void getTime(char *time);			// 현재 시간

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {
	// 윈도우 클래스 등록
	WNDCLASSEX wndclass;
	wndclass.cbSize = sizeof(WNDCLASSEX);
	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_APPLICATION));
	wndclass.hCursor = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_WINDOW);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szWindowClass;
	wndclass.hIconSm = LoadIcon(wndclass.hInstance, MAKEINTRESOURCE(IDI_APPLICATION));

	if (!RegisterClassEx(&wndclass)) {
		return 1;
	}

	hInst = hInstance;

	// 윈도우 생성
	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
	if (!hWnd) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// 메시지 루프
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return (int) msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	PAINTSTRUCT ps;
	HDC hdc;

	switch (uMsg) {
	case WM_GETMINMAXINFO :
		// 윈도우 크기 고정?
		((MINMAXINFO*)lParam)->ptMaxTrackSize.x = WIDTH;
		((MINMAXINFO*)lParam)->ptMaxTrackSize.y = HEIGHT;
		return 0;
	case WM_CREATE : 
		_WM_CREATE(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_COMMAND :
		_WM_COMMAND(hWnd, uMsg, wParam, lParam);
		return 0;
	case WM_KEYDOWN :
		return 0;
	case WM_SIZE :
		return 0;
	case WM_CLOSE :
		DestroyWindow(hWnd);
		return 0;
	case WM_DESTROY :
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

// 윈도우 컨트롤 구성
void _WM_CREATE(HWND hWnd, UINT, WPARAM, LPARAM) {
	// 첫 줄
	int firstLineY = MARGINE;
	
	// IP주소 텍스트
	int ipTextX = MARGINE, ipTextY = firstLineY, ipTextW = 70, ipTextH = EDIT_H;
	ipText = CreateWindow(L"static", L"IP 주소 :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		ipTextX, ipTextY, ipTextW, ipTextH, hWnd, (HMENU)0, hInst, NULL);
	
	// IP주소 에디트
	int ipEditX = ipTextX + ipTextW, ipEditY = firstLineY;
	int ipEditW = EDIT_W * 2, ipEditH = EDIT_H;
	ipEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL |
		WS_TABSTOP, 
		ipEditX, ipEditY, ipEditW, ipEditH, hWnd, (HMENU)IP_EDIT,
		hInst, NULL);
	
	// 포트 텍스트
	int portTextX = ipEditX + ipEditW + MARGINE, portTextY = firstLineY;
	int portTextW = 90, portTextH = EDIT_H;
	portText = CreateWindow(L"static", L"포트 번호 :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		portTextX, portTextY, portTextW, portTextH, hWnd, (HMENU)0, hInst, NULL);
	
	// 포트 에디트
	int portEditX = portTextX + portTextW, portEditY = firstLineY;
	int portEditW = EDIT_W, portEditH = EDIT_H;
	portEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		portEditX, portEditY, portEditW, portEditH, hWnd, (HMENU)PORT_EDIT,
		hInst, NULL);

	// 두번째 줄
	int secondLineY = firstLineY + STEP_HEIGHT;
	
	// 닉네임 텍스트
	int nickTextX = MARGINE, nickTextY = secondLineY;
	int nickTextW = 70, nickTextH = EDIT_H;
	nickText = CreateWindow(L"static", L"대화명 :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		nickTextX, nickTextY, nickTextW, nickTextH, hWnd, (HMENU)0, hInst, NULL);

	// 닉네임 에디트
	int nickEditX = nickTextX + nickTextW, nickEditY = secondLineY;
	int nickEditW = EDIT_W * 2, nickEditH = EDIT_H;
	nickEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		nickEditX, nickEditY, nickEditW, nickEditH, hWnd, (HMENU)NICK_EDIT, hInst, NULL);

	// 세번쨰 줄
	int thirdLineY = secondLineY + STEP_HEIGHT / 1.5;

	// 서버 접속 버튼
	int accServerBtnX = MARGINE, accServerBtnY = thirdLineY;
	int accServerBtnW = 480, accServerBtnH = BTN_H;
	accServerBtn = CreateWindow(L"button", L"서버(채팅방)에 접속하기",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		accServerBtnX, accServerBtnY, accServerBtnW, accServerBtnH, hWnd, (HMENU)ACC_BTN,
		hInst, NULL);

	// 네번째 줄
	int fourthLineY = thirdLineY + STEP_HEIGHT + MARGINE;

	// 메시지 텍스트
	int msgTextX = MARGINE, msgTextY = fourthLineY;
	int msgTextW = 70, msgTextH = EDIT_H;
	msgText = CreateWindow(L"static", L"메시지 :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		msgTextX, msgTextY, msgTextW, msgTextH, hWnd, (HMENU)0, hInst, NULL);

	// 메시지 에디트
	int msgEditX = msgTextX + msgTextW, msgEditY = fourthLineY;
	int msgEditW = 410, msgEditH = EDIT_H;
	msgEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		msgEditX, msgEditY, msgEditW, msgEditH, hWnd, (HMENU)MSG_EDIT, hInst, NULL);

	// 5번째 줄
	int fifthLineY = fourthLineY + STEP_HEIGHT / 1.5;

	// 메시지 전송 버튼
	int sendMsgX = MARGINE, sendMsgY = fifthLineY;
	int sendMsgW = 480, sendMsgH = BTN_H;
	
	sendMsgBtn = CreateWindow(L"button", _T("메시지 전송하기"),
	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
	sendMsgX, sendMsgY, sendMsgW, sendMsgH, hWnd, (HMENU)SEND_BTN,
	hInst, NULL);

	// 6번째 줄
	int sixthLineY = fifthLineY + STEP_HEIGHT;
	
	// 받거나 전송한 메시지를 출력할 텍스트
	int showTextX = MARGINE, showTextY = sixthLineY;
	int showTextW = 480, showTextH = 330;
	showText = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | ES_MULTILINE |
		ES_READONLY | ES_AUTOVSCROLL | WS_BORDER | WS_VSCROLL,
		showTextX, showTextY, showTextW, showTextH, hWnd, (HMENU)SHOW_TEXT, hInst, NULL);

	SetFocus(ipEdit);
}

// 윈도우 명력에 대한 동작
void _WM_COMMAND(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {
	case ACC_BTN :	// 접속 버튼 동작
		if (isAccess) {
			// 서버 종료 루틴
			strcpy(sendMsg, "[");
			strcat(sendMsg, nickName);
			strcat(sendMsg, "] 님이 서버와 연결이 종료되었습니다.");
			sendFlag = 1;
			Button_SetText(accServerBtn, L"서버(채팅방)에 접속하기");
			isAccess = 0;
			exitFlag = 1;
			WSACleanup();
			break;
		}

		if (accessServer(hWnd)) {
			Button_SetText(accServerBtn, L"서버 연결 종료하기");
			isAccess = 1;

			// 접속 알림
			char tempChar[BUFSIZE];
			strcpy(tempChar, "[");
			strcat(tempChar, nickName);
			strcat(tempChar, "] 님이 접속하셨습니다.");
			strcpy(sendMsg, tempChar);
			sendFlag = 1;

			// 중복 검사를 위해 자신의 대화명 전송
			sendMyNickInfo();
		}
		break;
	case SEND_BTN :		// 메시지 전송 버튼 동작
		if(isAccess) setMsg(hWnd);
		else MessageBox(hWnd, L"서버 접속 후 이용해주세요.", msgTitle, msgBoxA);
		break;
	case MSG_EDIT :
		break;
	}
}

// 서버 접속 함수
int accessServer(HWND hWnd) {
	exitFlag = 0;
	if (!checkAccess(hWnd)) return 0;

	InitializeCriticalSection(&cs);
	
	mulHandler = new MulticastHandler(ip, port, nickName);

	if (mulHandler->initRecSocket()) {
		rThread = CreateThread(NULL, 0, ReceiveProcess, 
			(LPVOID)mulHandler->recSocket, 0, NULL);
	}else {
		return 0;
	}
	
	if (mulHandler->initSendSocket()) {
		sThread = CreateThread(NULL, 0, SendProcess, (LPVOID)mulHandler->sendSocket, 0, NULL);
	}else {
		return 0;
	}

	return 1;
}

// 메시지 설정 함수
void setMsg(HWND hWnd) {
	char tempStr1[BUFSIZE + 1], tempStr2[BUFSIZE + 1];
	char timeStr[100], nickStr[BUFSIZE + 1];

	// 메시지 내용 얻기
	GetWindowTextA(msgEdit, (LPSTR)tempStr1, BUFSIZE);
	
	// 대화명 얻기
	GetWindowTextA(nickEdit, (LPSTR)tempStr2, BUFSIZE);

	// 대화명 비교
	strcpy(nickStr, tempStr2);
	if (strcmp(nickName, nickStr) != 0) {
		isExist = 0;
		char temp[BUFSIZE];
		char timeTemp[BUFSIZE];
		getTime(timeTemp);	// 현재 시간
		strcpy(temp, nickName);
		strcat(temp, " -> ");
		strcat(temp, nickStr);
		strcat(temp, " (으)로 대화명을 변경하셨습니다. [");
		strcat(temp, timeTemp);
		strcat(temp, "] \r\n");
		strcpy(sendMsg, temp);
		nickName[0] = '\0';
		strcpy(nickName, nickStr);

		// 중복 검사를 위해
		sendMyNickInfo();
	}

	// 현재 시간 구하기
	getTime(timeStr);

	// 문자열 추가
	strcat(sendMsg, tempStr2);
	strcat(sendMsg, " (");
	strcat(sendMsg, timeStr);
	strcat(sendMsg, ") : ");
	strcat(sendMsg, tempStr1);

	// 메시지 에디트 비우기
	Edit_SetText(msgEdit, L"");

	sendFlag = 1;

	// 대화명 중복 경고
	if (isExist) {
		showMessage(WARNING);
	}
}

// 수신 스레드 프로세스 함수
DWORD WINAPI ReceiveProcess(LPVOID arg) {
	SOCKET socket = (SOCKET)arg;
	while (1) {
		if (exitFlag) break;
		int r = mulHandler->receive(recMsg);
		
		if (r == 1) {
			char *p = strstr(recMsg, nickCheckInfo);	// 다른 대화명을 받을떄 메시지
			char *q = strstr(recMsg, nickAckInfo);		// 대화명에 대한 중복 여부 메시지
			char *pp = strstr(recMsg, idFlag);		// 클라이어트를 구별할 메시지

			// 새로 접속하거나 변경한 클에서 보내온
			// 대화명을 중복 검사
			if (p != NULL) {
				p = p + strlen(nickCheckInfo);
				strtok(p, idFlag);
				pp = pp + strlen(idFlag);

				char reSendTime[50];
				sprintf(reSendTime, "%ld", sendTime);
				int recPort = atoi(pp);
				if (strcmp(nickName, p) == 0 && strcmp(reSendTime, pp) != 0) {
					strcpy(sendNmsg, nickAckInfo);
					strcat(sendNmsg, "1");
					strcat(sendNmsg, idFlag);
					strcat(sendNmsg, pp);
					sendFlagNmsg = 1;
				}
			}

			// 중복 검사를 다른 클에게 보낸 후 중복 여부를 받음
			if (q != NULL) {
				q = q + strlen(q);
				strtok(q, idFlag);
				pp = pp + strlen(idFlag);

				char reSendTime[50];
				sprintf(reSendTime, "%ld", sendTime);
				if (strcmp(q, _OVER) && strcmp(reSendTime, pp) == 0) {
					isExist = 1;
					showMessage(WARNING);
				}else {
					isExist = 0;
				}
			}
			// 일반 채팅 메시지
			if(q == NULL && p == NULL){
				showMessage(recMsg);
			}
		// 소켓 에러
		}else if (r == -1) {
			showMessage("SOCKET ERROR");
		}
	}

	mulHandler->closeRecSocket();
	return 0;
}

// 송신 스레드 프로세스 함수
DWORD WINAPI SendProcess(LPVOID arg) {
	SOCKET socket = (SOCKET)arg;
	int i = 0;
	while (1) {
		// 소켓 종료 플래그
		if (exitFlag) break;
		// 일반 메시지 블래그와 메시지
		if (sendFlag) {
			mulHandler->send(sendMsg);
			sendMsg[0] = '\0';
			sendFlag = 0;
		}
		// 대화명 중복 검사를 위한 플래그와 메시지
		if (sendFlagNmsg) {
			mulHandler->send(sendNmsg);
			sendNmsg[0] = '\0';
			sendFlagNmsg = 0;
		}
	}

	mulHandler->closeSendSocket();
	return 0;
}

// 메시지를 GUI로 출력하는 함수
void showMessage(char *msg) {
	char temp[BUFSIZE + 1];
	char *ptext, *rtext;
	int slen = strlen(msg);
	strcpy(temp, msg);
	
	int len = GetWindowTextLengthA(showText);
	ptext = (char*)malloc(sizeof(char) * (len + 1));
	rtext = (char*)malloc(sizeof(char) * (len + BUFSIZE));
	GetWindowTextA(showText, ptext, len + 1);

	strcpy(rtext, ptext);
	strcat(rtext, temp);
	strcat(rtext, "\r\n");
	
	SetWindowTextA(showText, rtext);

	// 스크롤을 아래로 유지
	SendMessage(showText, EM_SETSEL, 0, -1);
	SendMessage(showText, EM_SETSEL, -1, -1);
	SendMessage(showText, EM_SCROLLCARET, 0, 0);
}

// IP 포트를 검사하는 함수
int checkAccess(HWND hWnd) {
	char ipStr[50], portStr[50], nickStr[50];

	GetWindowTextA(ipEdit, ipStr, 50);
	GetWindowTextA(portEdit, portStr, 50);
	GetWindowTextA(nickEdit, nickStr, 50);

	if (strlen(ipStr) == 0 || strlen(portStr) == 0 || strlen(nickStr) == 0) {
		MessageBox(hWnd, L"빈칸이 존재합니다.", msgTitle, msgBoxA);
		return 0;
	}

	int ibool = checkIp(ipStr);
	if (ibool == 0) {
		MessageBox(hWnd, L"잘못된 IP 주소입니다.", msgTitle, msgBoxA);
		return 0;
	}
	else if (ibool == -1) {
		MessageBox(hWnd, L"Class D IP 주소를 이용해주세요.", msgTitle, msgBoxA);
		return 0;
	}

	int pbool = checkPort(portStr);
	if (pbool == -1) {
		MessageBox(hWnd, L"포트 번호는 숫자만 입력해주세요.", msgTitle, msgBoxA);
		return 0;
	}
	else if (pbool == 0) {
		MessageBox(hWnd, L"포트 번호 범위를 넘었습니다.", msgTitle, msgBoxA);
		return 0;
	}
	char tempStr[20];
	
	// 전역 변수로 공유하기 위해
	strcpy(ip, ipStr);
	strcpy(tempStr, portStr);
	port = atoi(tempStr);
	strcpy(nickName, nickStr);

	return 1;
}

// 아이피 검사
int checkIp(char *ip) {
	char tempStr[50];
	char *dclass;
	int len;

	strcpy(tempStr, ip);

	len = strlen(tempStr);
	// 마지막이 점일때
	if (tempStr[len - 1] == '.') return 0;
	// 숫자와 점 검사
	for (int i = 0; i < len; i++) {
		char ch = tempStr[i];
		if (!(('0' <= ch && ch <= '9') || ch == '.')) {
			return 0;
		}
	}

	// IP 주소 체크 (0 ~ 255) 검사
	dclass = strtok(tempStr, ".");
	for (int i = 224; i < 240; i++) {
		char tmpbuf[5];
		sprintf(tmpbuf, "%d", i);
		if ((strcmp(dclass, tmpbuf) == 0)) {
			dclass = strtok(NULL, ".");
			int cnt = 0;
			while (dclass != NULL) {
				cnt++;
				int num = atoi(dclass);
				if (num < 0 || num > 255) {
						return 0;
				}
				dclass = strtok(NULL, ".");
			}
			// 길이 검사
			if (cnt > 3) return 0;
			return 1;
		}
	}

	return -1;
}

// 포트 검사 함수
int checkPort(char *port) {
	int num;
	char tempStr[50];

	strcpy(tempStr, port);

	// 문자 여부 검사
	for (int i = 0; i < strlen(tempStr); i++) {
		char ch = tempStr[i];
		if (!('0' <= ch && ch <= '9')) return -1;
	}

	num = atoi(tempStr);
	// 범위 검사
	if (1024 > num || num > 65535) {
		return 0;
	}

	return 1;
}

// 중복 검사를 위하여 자신의 대화명을 알리는 함수
void sendMyNickInfo() {
	sendTime = GetTickCount64();
	char tempPort[50];
	//ultoa(sendTime, tempPort, 50);
	sprintf(tempPort, "%ld", sendTime);
	strcpy(sendNmsg, nickCheckInfo);
	strcat(sendNmsg, nickName);
	strcat(sendNmsg, idFlag);
	strcat(sendNmsg, tempPort);
	sendFlagNmsg = 1;
}

// 현재 시간을 구하는 함수
void getTime(char *timeStr) {
	time_t now;
	struct tm t;
	char buf[100];

	time(&now);

	t = *localtime(&now);

	sprintf(buf, "%d/%02d/%02d %02d:%02d:%02d",
		t.tm_year + 1990, t.tm_mon + 1, t.tm_mday,
		t.tm_hour, t.tm_min, t.tm_sec);

	strcpy(timeStr, buf);
}