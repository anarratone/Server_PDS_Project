#include "stdafx.h"

namespace std {
#if defined _UNICODE || defined UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}

/*********************Global variables declaration*********************/
/*Store a unique identifier for the window*/
int WINDOWS_ID = 0;

/*Data structure containing all the running windows*/
std::map<int, Thread> threads;

/*Mutex 1: control access to the threads map*/
std::mutex m1;
/*Mutex 2: control access to the clients list*/
std::mutex m2;
/*Mutex 3: control access to the actions list*/
std::mutex m3;

/*Condition variable 1: notify of changes in the windows map*/
std::condition_variable cv1;

/*********************Function prototypes*********************/
/*Windows APIs to obtain the list of running windows*/
BOOL CALLBACK EnumWindowsProc(HWND hWnd, LPARAM lParam);

/*Utility function to print error messages for the sockets*/
void err_fatal(char *mes);

/*Callable for Thread1*/
void poll_windows();

/*Callable for Thread2*/
void notify_clients();

/*Callable for Thread3*/
void poll_actions();

/*Callable for Thread4*/
void execute_actions();

int main()
{
	/*********************Main variables*********************/
	/*Thread 1: poll the OS to get the windows*/
	std::thread t1(poll_windows);
	/*Thread 2: broadcast to clients the changes in the list*/
	std::thread t2(notify_clients);
	/*Thread 3: listen to clients sockets for incoming actions*/
	std::thread t3(poll_actions);
	/*Thread 4: send the input to the specified window*/
	std::thread t4(execute_actions);


system("pause");
return 0;
}

void poll_windows() {
	/*This thread is in charge of polling the status of the
	windows fomr the OS, updating the map of windows and
	notifying thread t2 of any changes through cv1*/
	bool ret;
	while (true) {
		Sleep(50);
		std::unique_lock<std::mutex> l(m1);
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&ret));
	}
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

	Thread p(hWnd, buffer, threadId, hIcon);

	if(threads.)
	p.notification = 1;

	if (GetForegroundWindow() == hWnd) {
		p.setFocusFlag();
	}
	reinterpret_cast<std::vector<Thread>*>(lParam)->push_back(p);
	delete[] className;
	delete[] buffer;
	return TRUE;
}

void err_fatal(char *mes) {
	printf("%s, errno=%d\n", mes, WSAGetLastError());
	perror("");
	getchar();
	exit(1);
}