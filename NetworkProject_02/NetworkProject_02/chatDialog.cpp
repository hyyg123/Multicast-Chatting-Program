//#pragma comment(lib, "ws2_32")

#include "MulticastHandler.h"
#include <windows.h>
#include <windowsx.h>
#include <tchar.h>
#include <time.h>
#include <chrono>
#include <atlstr.h>

using namespace std;

// ��Ʈ�ѵ��� ���̵�
#define ACC_BTN			10
#define SEND_BTN		11

#define IP_EDIT			100
#define PORT_EDIT		101
#define NICK_EDIT		102
#define MSG_EDIT		103
#define SHOW_TEXT		104

// ��Ʈ�ѵ��� ��ġ�� ���� ����
#define MARGINE			20
#define EDIT_W			100
#define EDIT_H			20
#define BTN_H			30
#define STEP_HEIGHT		40

// ������ ũ��
#define WIDTH		540
#define HEIGHT		600

// ��ȭ�� �ߺ� �÷��� ��
#define _OVER		"1"

// ��ȭ�� �ߺ� ��� ����
#define WARNING "!!! ��ȭ���� �ߺ��˴ϴ�. �������ּ���! !!!"

// // // // ���� ���� // // // //

HINSTANCE hInst;					// �ν��Ͻ� ����
MulticastHandler *mulHandler;		// ��Ƽĳ��Ʈ �ڵ鷯 ��ü

HANDLE rThread, sThread;			// ���ú�, ���� ������

char ip[20];						// �Է¹��� IP ����
char nickName[50];					// �Է¹��� ��ȭ�� ����
int port;							// �Է¹��� ��Ʈ ����

char nickCheckInfo[15] = "[nci]";	// ��ȭ�� �ߺ� �˻縦 ���� ��Ŷ ���
char nickAckInfo[15] = "[nai]";		// ��ȭ�� �ߺ� ���θ� ���� ��Ŷ ���
char idFlag[10] = "[id]";			// Ŭ���̾�Ʈ�� �����ϱ� ���� ID�� (�ð����� ������)

char recMsg[BUFSIZE + 1];		// 	���Ź��� �޽��� ���� ����
char sendMsg[BUFSIZE + 1];			// �۽��� �޽��� ����
char sendNmsg[BUFSIZE + 1];			// ��ȭ�� �ߺ� �˻� ��Ŷ�� ���� �۽� ����

int sendFlag = 0;					// ���� �÷���, 1�̸� ����
int sendFlagNmsg = 0;				// ��ȭ�� �ߺ� �˻� ���� �÷���, 1�̸� ����

int isAccess = 0;					// ���� ����
int exitFlag = 0;					// ���� ���� ����
int isExist = 0;					// ��ȭ�� �ߺ� ����

unsigned long sendTime;				// Ŭ���̾�Ʈ ������ ���� ID�� (�ð����� ����)

// IP, PORT, NICKNAME, MSG, SHOW ���� ��Ʈ�ѵ�
HWND ipText, portText, nickText, msgText;
HWND ipEdit, portEdit, nickEdit, msgEdit;
HWND accServerBtn, sendMsgBtn;
HWND showText;

// ������ Ŭ���� ����
static TCHAR szWindowClass[] = _T("win32app");
static TCHAR szTitle[] = _T("NetworkProject02");
// �޽����ڽ� ����
static TCHAR msgTitle[] = _T("���â");
// �޽����ڽ� �Ӽ�
int msgBoxA = MB_OK | MB_ICONERROR;

// ����ȭ
CRITICAL_SECTION cs;

// // // // �Լ� ���� // // // //
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

// ������ �̺�Ʈ �Լ� ����
void _WM_CREATE(HWND, UINT, WPARAM, LPARAM);
void _WM_COMMAND(HWND, UINT, WPARAM, LPARAM);

// ���� ���ӽ� ���̴� �Լ���
int checkIp(char *);			// ������ �˻�
int checkPort(char *);			// ��Ʈ �˻�
int checkAccess(HWND hWnd);		// ������, ��Ʈ �˻�

int accessServer(HWND hWnd);	// ���� ����
void setMsg(HWND hWnd);			// �۽� �޽��� ����

void showMessage(char *msg);	// �޽��� ���

DWORD WINAPI ReceiveProcess(LPVOID arg);	// ���� ������ ���μ��� �Լ�
DWORD WINAPI SendProcess(LPVOID arg);		// �۽� ������ ���μ��� �Լ�

void sendMyNickInfo();				// �ڽ��� ��ȭ���� ���� (�ߺ� �˻縦 ����)
void getTime(char *time);			// ���� �ð�

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow) {
	// ������ Ŭ���� ���
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

	// ������ ����
	HWND hWnd = CreateWindow(szWindowClass, szTitle, WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, WIDTH, HEIGHT, NULL, NULL, hInstance, NULL);
	if (!hWnd) return 1;
	ShowWindow(hWnd, nCmdShow);
	UpdateWindow(hWnd);

	// �޽��� ����
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
		// ������ ũ�� ����?
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

// ������ ��Ʈ�� ����
void _WM_CREATE(HWND hWnd, UINT, WPARAM, LPARAM) {
	// ù ��
	int firstLineY = MARGINE;
	
	// IP�ּ� �ؽ�Ʈ
	int ipTextX = MARGINE, ipTextY = firstLineY, ipTextW = 70, ipTextH = EDIT_H;
	ipText = CreateWindow(L"static", L"IP �ּ� :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		ipTextX, ipTextY, ipTextW, ipTextH, hWnd, (HMENU)0, hInst, NULL);
	
	// IP�ּ� ����Ʈ
	int ipEditX = ipTextX + ipTextW, ipEditY = firstLineY;
	int ipEditW = EDIT_W * 2, ipEditH = EDIT_H;
	ipEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL |
		WS_TABSTOP, 
		ipEditX, ipEditY, ipEditW, ipEditH, hWnd, (HMENU)IP_EDIT,
		hInst, NULL);
	
	// ��Ʈ �ؽ�Ʈ
	int portTextX = ipEditX + ipEditW + MARGINE, portTextY = firstLineY;
	int portTextW = 90, portTextH = EDIT_H;
	portText = CreateWindow(L"static", L"��Ʈ ��ȣ :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		portTextX, portTextY, portTextW, portTextH, hWnd, (HMENU)0, hInst, NULL);
	
	// ��Ʈ ����Ʈ
	int portEditX = portTextX + portTextW, portEditY = firstLineY;
	int portEditW = EDIT_W, portEditH = EDIT_H;
	portEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		portEditX, portEditY, portEditW, portEditH, hWnd, (HMENU)PORT_EDIT,
		hInst, NULL);

	// �ι�° ��
	int secondLineY = firstLineY + STEP_HEIGHT;
	
	// �г��� �ؽ�Ʈ
	int nickTextX = MARGINE, nickTextY = secondLineY;
	int nickTextW = 70, nickTextH = EDIT_H;
	nickText = CreateWindow(L"static", L"��ȭ�� :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		nickTextX, nickTextY, nickTextW, nickTextH, hWnd, (HMENU)0, hInst, NULL);

	// �г��� ����Ʈ
	int nickEditX = nickTextX + nickTextW, nickEditY = secondLineY;
	int nickEditW = EDIT_W * 2, nickEditH = EDIT_H;
	nickEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		nickEditX, nickEditY, nickEditW, nickEditH, hWnd, (HMENU)NICK_EDIT, hInst, NULL);

	// ������ ��
	int thirdLineY = secondLineY + STEP_HEIGHT / 1.5;

	// ���� ���� ��ư
	int accServerBtnX = MARGINE, accServerBtnY = thirdLineY;
	int accServerBtnW = 480, accServerBtnH = BTN_H;
	accServerBtn = CreateWindow(L"button", L"����(ä�ù�)�� �����ϱ�",
		WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
		accServerBtnX, accServerBtnY, accServerBtnW, accServerBtnH, hWnd, (HMENU)ACC_BTN,
		hInst, NULL);

	// �׹�° ��
	int fourthLineY = thirdLineY + STEP_HEIGHT + MARGINE;

	// �޽��� �ؽ�Ʈ
	int msgTextX = MARGINE, msgTextY = fourthLineY;
	int msgTextW = 70, msgTextH = EDIT_H;
	msgText = CreateWindow(L"static", L"�޽��� :", WS_CHILD | WS_VISIBLE | SS_LEFT,
		msgTextX, msgTextY, msgTextW, msgTextH, hWnd, (HMENU)0, hInst, NULL);

	// �޽��� ����Ʈ
	int msgEditX = msgTextX + msgTextW, msgEditY = fourthLineY;
	int msgEditW = 410, msgEditH = EDIT_H;
	msgEdit = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL
		| WS_TABSTOP,
		msgEditX, msgEditY, msgEditW, msgEditH, hWnd, (HMENU)MSG_EDIT, hInst, NULL);

	// 5��° ��
	int fifthLineY = fourthLineY + STEP_HEIGHT / 1.5;

	// �޽��� ���� ��ư
	int sendMsgX = MARGINE, sendMsgY = fifthLineY;
	int sendMsgW = 480, sendMsgH = BTN_H;
	
	sendMsgBtn = CreateWindow(L"button", _T("�޽��� �����ϱ�"),
	WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | WS_TABSTOP,
	sendMsgX, sendMsgY, sendMsgW, sendMsgH, hWnd, (HMENU)SEND_BTN,
	hInst, NULL);

	// 6��° ��
	int sixthLineY = fifthLineY + STEP_HEIGHT;
	
	// �ްų� ������ �޽����� ����� �ؽ�Ʈ
	int showTextX = MARGINE, showTextY = sixthLineY;
	int showTextW = 480, showTextH = 330;
	showText = CreateWindow(L"edit", NULL, WS_CHILD | WS_VISIBLE | ES_MULTILINE |
		ES_READONLY | ES_AUTOVSCROLL | WS_BORDER | WS_VSCROLL,
		showTextX, showTextY, showTextW, showTextH, hWnd, (HMENU)SHOW_TEXT, hInst, NULL);

	SetFocus(ipEdit);
}

// ������ ��¿� ���� ����
void _WM_COMMAND(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (LOWORD(wParam)) {
	case ACC_BTN :	// ���� ��ư ����
		if (isAccess) {
			// ���� ���� ��ƾ
			strcpy(sendMsg, "[");
			strcat(sendMsg, nickName);
			strcat(sendMsg, "] ���� ������ ������ ����Ǿ����ϴ�.");
			sendFlag = 1;
			Button_SetText(accServerBtn, L"����(ä�ù�)�� �����ϱ�");
			isAccess = 0;
			exitFlag = 1;
			WSACleanup();
			break;
		}

		if (accessServer(hWnd)) {
			Button_SetText(accServerBtn, L"���� ���� �����ϱ�");
			isAccess = 1;

			// ���� �˸�
			char tempChar[BUFSIZE];
			strcpy(tempChar, "[");
			strcat(tempChar, nickName);
			strcat(tempChar, "] ���� �����ϼ̽��ϴ�.");
			strcpy(sendMsg, tempChar);
			sendFlag = 1;

			// �ߺ� �˻縦 ���� �ڽ��� ��ȭ�� ����
			sendMyNickInfo();
		}
		break;
	case SEND_BTN :		// �޽��� ���� ��ư ����
		if(isAccess) setMsg(hWnd);
		else MessageBox(hWnd, L"���� ���� �� �̿����ּ���.", msgTitle, msgBoxA);
		break;
	case MSG_EDIT :
		break;
	}
}

// ���� ���� �Լ�
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

// �޽��� ���� �Լ�
void setMsg(HWND hWnd) {
	char tempStr1[BUFSIZE + 1], tempStr2[BUFSIZE + 1];
	char timeStr[100], nickStr[BUFSIZE + 1];

	// �޽��� ���� ���
	GetWindowTextA(msgEdit, (LPSTR)tempStr1, BUFSIZE);
	
	// ��ȭ�� ���
	GetWindowTextA(nickEdit, (LPSTR)tempStr2, BUFSIZE);

	// ��ȭ�� ��
	strcpy(nickStr, tempStr2);
	if (strcmp(nickName, nickStr) != 0) {
		isExist = 0;
		char temp[BUFSIZE];
		char timeTemp[BUFSIZE];
		getTime(timeTemp);	// ���� �ð�
		strcpy(temp, nickName);
		strcat(temp, " -> ");
		strcat(temp, nickStr);
		strcat(temp, " (��)�� ��ȭ���� �����ϼ̽��ϴ�. [");
		strcat(temp, timeTemp);
		strcat(temp, "] \r\n");
		strcpy(sendMsg, temp);
		nickName[0] = '\0';
		strcpy(nickName, nickStr);

		// �ߺ� �˻縦 ����
		sendMyNickInfo();
	}

	// ���� �ð� ���ϱ�
	getTime(timeStr);

	// ���ڿ� �߰�
	strcat(sendMsg, tempStr2);
	strcat(sendMsg, " (");
	strcat(sendMsg, timeStr);
	strcat(sendMsg, ") : ");
	strcat(sendMsg, tempStr1);

	// �޽��� ����Ʈ ����
	Edit_SetText(msgEdit, L"");

	sendFlag = 1;

	// ��ȭ�� �ߺ� ���
	if (isExist) {
		showMessage(WARNING);
	}
}

// ���� ������ ���μ��� �Լ�
DWORD WINAPI ReceiveProcess(LPVOID arg) {
	SOCKET socket = (SOCKET)arg;
	while (1) {
		if (exitFlag) break;
		int r = mulHandler->receive(recMsg);
		
		if (r == 1) {
			char *p = strstr(recMsg, nickCheckInfo);	// �ٸ� ��ȭ���� ������ �޽���
			char *q = strstr(recMsg, nickAckInfo);		// ��ȭ�� ���� �ߺ� ���� �޽���
			char *pp = strstr(recMsg, idFlag);		// Ŭ���̾�Ʈ�� ������ �޽���

			// ���� �����ϰų� ������ Ŭ���� ������
			// ��ȭ���� �ߺ� �˻�
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

			// �ߺ� �˻縦 �ٸ� Ŭ���� ���� �� �ߺ� ���θ� ����
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
			// �Ϲ� ä�� �޽���
			if(q == NULL && p == NULL){
				showMessage(recMsg);
			}
		// ���� ����
		}else if (r == -1) {
			showMessage("SOCKET ERROR");
		}
	}

	mulHandler->closeRecSocket();
	return 0;
}

// �۽� ������ ���μ��� �Լ�
DWORD WINAPI SendProcess(LPVOID arg) {
	SOCKET socket = (SOCKET)arg;
	int i = 0;
	while (1) {
		// ���� ���� �÷���
		if (exitFlag) break;
		// �Ϲ� �޽��� ���׿� �޽���
		if (sendFlag) {
			mulHandler->send(sendMsg);
			sendMsg[0] = '\0';
			sendFlag = 0;
		}
		// ��ȭ�� �ߺ� �˻縦 ���� �÷��׿� �޽���
		if (sendFlagNmsg) {
			mulHandler->send(sendNmsg);
			sendNmsg[0] = '\0';
			sendFlagNmsg = 0;
		}
	}

	mulHandler->closeSendSocket();
	return 0;
}

// �޽����� GUI�� ����ϴ� �Լ�
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

	// ��ũ���� �Ʒ��� ����
	SendMessage(showText, EM_SETSEL, 0, -1);
	SendMessage(showText, EM_SETSEL, -1, -1);
	SendMessage(showText, EM_SCROLLCARET, 0, 0);
}

// IP ��Ʈ�� �˻��ϴ� �Լ�
int checkAccess(HWND hWnd) {
	char ipStr[50], portStr[50], nickStr[50];

	GetWindowTextA(ipEdit, ipStr, 50);
	GetWindowTextA(portEdit, portStr, 50);
	GetWindowTextA(nickEdit, nickStr, 50);

	if (strlen(ipStr) == 0 || strlen(portStr) == 0 || strlen(nickStr) == 0) {
		MessageBox(hWnd, L"��ĭ�� �����մϴ�.", msgTitle, msgBoxA);
		return 0;
	}

	int ibool = checkIp(ipStr);
	if (ibool == 0) {
		MessageBox(hWnd, L"�߸��� IP �ּ��Դϴ�.", msgTitle, msgBoxA);
		return 0;
	}
	else if (ibool == -1) {
		MessageBox(hWnd, L"Class D IP �ּҸ� �̿����ּ���.", msgTitle, msgBoxA);
		return 0;
	}

	int pbool = checkPort(portStr);
	if (pbool == -1) {
		MessageBox(hWnd, L"��Ʈ ��ȣ�� ���ڸ� �Է����ּ���.", msgTitle, msgBoxA);
		return 0;
	}
	else if (pbool == 0) {
		MessageBox(hWnd, L"��Ʈ ��ȣ ������ �Ѿ����ϴ�.", msgTitle, msgBoxA);
		return 0;
	}
	char tempStr[20];
	
	// ���� ������ �����ϱ� ����
	strcpy(ip, ipStr);
	strcpy(tempStr, portStr);
	port = atoi(tempStr);
	strcpy(nickName, nickStr);

	return 1;
}

// ������ �˻�
int checkIp(char *ip) {
	char tempStr[50];
	char *dclass;
	int len;

	strcpy(tempStr, ip);

	len = strlen(tempStr);
	// �������� ���϶�
	if (tempStr[len - 1] == '.') return 0;
	// ���ڿ� �� �˻�
	for (int i = 0; i < len; i++) {
		char ch = tempStr[i];
		if (!(('0' <= ch && ch <= '9') || ch == '.')) {
			return 0;
		}
	}

	// IP �ּ� üũ (0 ~ 255) �˻�
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
			// ���� �˻�
			if (cnt > 3) return 0;
			return 1;
		}
	}

	return -1;
}

// ��Ʈ �˻� �Լ�
int checkPort(char *port) {
	int num;
	char tempStr[50];

	strcpy(tempStr, port);

	// ���� ���� �˻�
	for (int i = 0; i < strlen(tempStr); i++) {
		char ch = tempStr[i];
		if (!('0' <= ch && ch <= '9')) return -1;
	}

	num = atoi(tempStr);
	// ���� �˻�
	if (1024 > num || num > 65535) {
		return 0;
	}

	return 1;
}

// �ߺ� �˻縦 ���Ͽ� �ڽ��� ��ȭ���� �˸��� �Լ�
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

// ���� �ð��� ���ϴ� �Լ�
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