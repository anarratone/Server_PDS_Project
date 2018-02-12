#include "stdafx.h"

namespace std {
#if defined _UNICODE || defined UNICODE
	typedef wstring tstring;
#else
	typedef string tstring;
#endif
}

namespace std
{
	template<>
	struct hash<Thread>
	{
		size_t operator()(Thread const& s) const
		{
			return std::hash<std::wstring>()(s.name);
		}
	};
}

/*********************Global variables declaration*********************/
/*Store a unique identifier for the window*/
int WINDOWS_ID = 0;

/*Data structure containing all the running windows*/
std::map<std::wstring, Thread> threads;
std::unordered_set<Thread> new_threads;
std::vector<SOCKET> clients;

/*Mutex 1: control access to the threads map*/
std::mutex m1;
/*Mutex 2: control access to the clients list*/
std::mutex m2;
/*Mutex 3: control access to the actions list*/
std::mutex m3;
/*Mutex m4: control access to the thread_list_changed variable*/
std::mutex m4;

/*Condition variable 1: notify of changes in the windows map*/
std::condition_variable cv1;
bool thread_list_changed = false;

/*Condition variable 2: set while t2 is notifying the clients*/
std::condition_variable cv2;
bool notifying_clients = false;


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
	//std::thread t3(poll_actions);
	/*Thread 4: send the input to the specified window*/
	//std::thread t4(execute_actions);


system("pause");
return 0;
}

void poll_windows() {
	/*This thread is in charge of polling the status of the
	windows fomr the OS, updating the map of windows and
	notifying thread t2 of any changes through cv1*/
	bool ret;
	auto beginning = std::chrono::system_clock::now();
	auto last = beginning;

	while (true) {
		std::unordered_set<Thread> to_remove;
		bool changes = false;
		Sleep(50);
		system("cls");
		std::unique_lock<std::mutex> l(m1);
		cv2.wait(l, []() { return !thread_list_changed; });
		auto now = std::chrono::system_clock::now();
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&ret));
		for (auto pair = threads.begin(); pair != threads.end(); pair++) {
			Thread t = pair->second;

			auto it = new_threads.find(t);
			if (it != new_threads.end()) {
				Thread new_thread = *it;
				//std::cout << now.time_since_epoch().count() - last.time_since_epoch().count() << std::endl;
				if (new_thread.focus) {
					new_thread.active_time = t.active_time + now.time_since_epoch().count(
					) - last.time_since_epoch().count();
				}
				else {
					new_thread.active_time = t.active_time;
				}
				new_thread.active_percentage = new_thread.active_time / (
					now.time_since_epoch().count() - beginning.time_since_epoch().count());
				threads[new_thread.get_key()] = new_thread;
				new_threads.erase(it);
			}
			else {
				to_remove.insert(pair->second);
				changes = true;
			}
		}
		// Now we have in to_remove all the deleted threads
		for (Thread t : to_remove) {
			threads.erase(threads.find(t.get_key()));
		}
		for (Thread t : new_threads) {
			threads[t.get_key()] = t;
			changes = true;
		}
		new_threads.clear();
		last = now;
		if (changes) {
			std::cout << threads.size();
			thread_list_changed = true;
			cv1.notify_all();
		}
		for (auto pair = threads.begin(); pair != threads.end(); pair++) {
			Thread t = pair->second; 
			std::wcout << t.name << L" ==> " << t.active_percentage << std::endl;
		}
		//std::cout << "NOW: " << now.time_since_epoch().count() << std::endl;
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


	/* Thanks Stack Overflow */
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
	/*END Thanks Stack Overflow */

	Thread t(hWnd, buffer, threadId, hIcon);
	if (GetForegroundWindow() == hWnd) {
		t.focus = true;
	}
	new_threads.insert(t);
	delete[] className;
	delete[] buffer;
	return TRUE;
}

void notify_clients() {
	/*Iterate through the clients sockets and notify of changes in the list*/
	while (true) {
		std::unique_lock<std::mutex> l(m1);
		cv1.wait(l, []() { return thread_list_changed; });
		thread_list_changed = false;
		std::cout << "DIO" << std::endl;
	}
}

void err_fatal(char *mes) {
	printf("%s, errno=%d\n", mes, WSAGetLastError());
	perror("");
	getchar();
	exit(1);
}