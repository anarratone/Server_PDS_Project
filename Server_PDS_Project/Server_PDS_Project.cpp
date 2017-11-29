// Server_PDS_Project.cpp : definisce il punto di ingresso dell'applicazione console.
//

#include "stdafx.h"

#pragma once
//#include <WinSock2.h>
//#include <Windows.h>
//#include <WS2tcpip.h>

namespace std {
#if defined _UNICODE || defined UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}

/* Global variables declaration */
int current_win_id = 0; // Contains the id of the thread that is used to index it
std::vector<Thread> threads; // Contains the list of the threads that have an active window

/* Function prototypes */
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);
void CALLBACK WinEventProcCallback(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
);
void hookEvents();

void err_fatal(char *mes) {
	printf("%s, errno=%d\n", mes, WSAGetLastError());
	perror("");
	getchar();
	exit(1);
}

int main()
{
	struct sockaddr_in saServer;
	struct sockaddr_in client;
	int clientLen = sizeof(struct sockaddr_in);
	short port = 4444;

	saServer.sin_family = AF_INET;
	saServer.sin_addr.s_addr = INADDR_ANY;
	saServer.sin_port = htons(port);
	WSADATA wsadata;
	// Main loop 
	// 1) Try to open a socket and accept on it
	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		std::cerr << "WSAStartup() failed" << std::endl;
		exit(1);
	}
	SOCKET s = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
	bind(s, (struct sockaddr *)&saServer, sizeof(saServer));
	listen(s, 10);

	for (;;) {
		SOCKET cs = accept(s, (struct sockaddr *)&client, &clientLen);
		//if (cs == -1) err_fatal("NOMARIA");
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&threads)); //WinAPI per ottenere la lista delle finestre nel sistema
		std::cout << "Mando la lista" <<std::endl;
		for each  (Thread t in threads)
		{
			size_t size;
			std::string buffer = t.serialize(&size);
			std::cout << buffer.c_str();
			send(cs, reinterpret_cast<const char *>(buffer.c_str()), size, 0);
		}
//		while (1) {
//			boost::asio::read_until(sock, buffer, '}');
//			std::getline(is, str);
//			std::cout << str << std::endl;
//			std::stringstream ss;
//			ss << str;
//			boost::property_tree::ptree pt;
//			boost::property_tree::read_json(ss, pt);
//			int numberOfInputs = pt.get<int>("numberOfKeys");
//			std::cout << pt.get<int>("numberOfKeys") << std::endl;
//			std::vector<int> keys = as_vector<int>(pt, "keys");
//			INPUT *inputs = new INPUT[numberOfInputs * 2]; //Vogliamo il keyUp
//			for (int i = 0; i < numberOfInputs; i++) {
//				inputs[i].type = INPUT_KEYBOARD;
//				inputs[i].ki.dwExtraInfo = NULL;
//				inputs[i].ki.time = 0;
//				inputs[i].ki.dwFlags = 0;
//				inputs[i].ki.wVk = keys[i];
//			}
//			for (int i = numberOfInputs; i < numberOfInputs * 2; i++) {
//				inputs[i].type = INPUT_KEYBOARD;
//				inputs[i].ki.dwExtraInfo = NULL;
//				inputs[i].ki.time = 0;
//				inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
//				inputs[i].ki.wVk = keys[i % numberOfInputs];
//			}
//			SendInput(numberOfInputs * 2, inputs, sizeof(INPUT));
//			delete inputs;
//		}
//		system("pause");
//		system("cls");
//		system("pause");
//	}
//}
system("pause");
return 0;
}
    return 0;
}


BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam)
{

	if (!IsWindowVisible(hWnd))
		return TRUE;
	int length = GetWindowTextLength(hWnd);
	if (0 == length)
		return TRUE;

	HICON hIcon;
	TCHAR* buffer;
	DWORD threadId;
	LPWSTR className = new WCHAR[20 + 1];

	buffer = new TCHAR[length + 1];
	memset(buffer, 0, (length + 1) * sizeof(TCHAR));
	memset(className, 0, (20) * sizeof(WCHAR));

	GetWindowText(hWnd, buffer, length + 1);
	threadId = GetWindowThreadProcessId(hWnd, NULL);
	GetClassName(hWnd, className, 20);

	hIcon = (HICON)(::SendMessageW(hWnd, WM_GETICON, ICON_SMALL, 0));
	if (hIcon == 0) {
		// Alternative method. Get from the window class
		hIcon = reinterpret_cast<HICON>(::GetClassLongPtrW(hWnd, GCLP_HICONSM));
	}
	// Alternative: get the first icon from the main module 
	if (hIcon == 0) {
		hIcon = ::LoadIcon(GetModuleHandleW(0), MAKEINTRESOURCE(0));
	}
	// Alternative method. Use OS default icon
	if (hIcon == 0) {
		hIcon = ::LoadIcon(0, IDI_APPLICATION);
	}

	Thread p(current_win_id++, hWnd, buffer, threadId, hIcon);
	p.notification = 1;

	if (GetForegroundWindow() == hWnd) {
		p.setFocusFlag();
	}
	reinterpret_cast<std::vector<Thread>*>(lParam)->push_back(p);
	//threads.push_back(p);
	delete[] className;
	delete[] buffer;
	return TRUE;
}


void CALLBACK WinEventProcCallback(
	HWINEVENTHOOK hWinEventHook,
	DWORD         event,
	HWND          hwnd,
	LONG          idObject,
	LONG          idChild,
	DWORD         dwEventThread,
	DWORD         dwmsEventTime
) {
	if (event == EVENT_SYSTEM_FOREGROUND) {
		current_win_id = 0;
		std::vector<Thread> newThreads;
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&newThreads));
		bool found = false;
		/*
		for each (Thread t in threads)
		{
		for each(Thread t1 in newThreads) {
		if (t1.name == t.name && t1.tid == t.tid) {
		found = true;
		t1.notification = 0; //Era vecchio
		break;
		}
		}
		if (found) {
		t.notification = 0; //C'è ancora
		}
		else {
		t.notification = 2; //Cancellato
		boost::asio::write(sock, boost::asio::buffer(t.toStringa()), ignored_error); //Lo mando
		}
		found = false;
		}
		*/
		for (auto t = threads.begin(); t != threads.end(); t++) {
			for (auto t1 = newThreads.begin(); t1 != newThreads.end(); t1++) {
				if (t1->name == t->name && t1->tid == t->tid) {
					found = true;
					t1->notification = 0; //Era vecchio
					t->notification = 0; //C'è ancora
					break;
				}
			}
			if (!found) {
				//t->notification = 2; //Cancellato
				//std::cout << "Mando cose cancellate" << std::endl;
				//boost::asio::write(sock, boost::asio::buffer(t->toStringa()), ignored_error); //Lo mando
			}
			found = false;
		}

		for each(Thread t in newThreads) {
			//if (t.notification == 1 || t.focus) { //E' nuovo
			//	std::cout << "Mando cose nuove" << std::endl;
			//	boost::asio::write(sock, boost::asio::buffer(t.toStringa()), ignored_error); //Lo mando
			//}
		}
		threads = newThreads;
	}
	return;
}

void hookEvents() {
	SetWinEventHook(EVENT_SYSTEM_FOREGROUND, EVENT_SYSTEM_FOREGROUND, NULL, WinEventProcCallback, 0, 0, WINEVENT_OUTOFCONTEXT | WINEVENT_SKIPOWNPROCESS);

	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}