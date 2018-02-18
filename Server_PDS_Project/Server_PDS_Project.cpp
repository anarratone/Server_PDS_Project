#include "stdafx.h"
#include "Thread.h"



#define MAX_CLIENTS 10

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

/*Data structure containing all the running windows*/
std::map<std::wstring, Thread> threads;
std::unordered_set<Thread> new_threads;

std::unordered_set<SOCKET> clients;

std::list<Action> actions;
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

/*Condition variable 3: notify of incoming actions*/
std::condition_variable cv3;

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

	// Connection related variables
	SOCKET client, s;
	struct sockaddr_in sa_server;
	struct sockaddr_in sa_client;
	int client_len;
	WSADATA wsadata;

	memset(&sa_server, 0, sizeof(struct sockaddr_in));
	memset(&sa_client, 0, sizeof(struct sockaddr_in));
	sa_server.sin_family = AF_INET;
	sa_server.sin_addr.s_addr = INADDR_ANY;
	sa_server.sin_port = htons(4444);

	if (WSAStartup(MAKEWORD(2, 2), &wsadata) != 0) {
		std::cerr << "WSAStartup FAILED" << std::endl;
		exit(-1);
	}

	if ((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
		std::cout << "ERROR OPENING A SOCKET" << std::endl;
		exit(-1);
	}

	if (bind(s, reinterpret_cast<struct sockaddr *>(&sa_server), sizeof(struct sockaddr_in)) != 0) {
		err_fatal("BIND FAILED");
	}

	if (listen(s, MAX_CLIENTS) != 0) {
		err_fatal("LISTEN FAILED");
	}

	while (true) {
		client_len = sizeof(struct sockaddr_in);
		if ((client = accept(s, reinterpret_cast<struct sockaddr *>(&sa_client), &client_len)) > 0) {
			std::unique_lock<std::mutex> ul(m2);
			clients.insert(client);
			std::cout << "NEW CONNECTION FROM IP " << inet_ntoa(sa_client.sin_addr) << std::endl;
			thread_list_changed = true;
			cv1.notify_all();
		}
	}
	closesocket(s);
	WSACleanup();
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
	auto last_notify = beginning;
	int iteration = 0;

	while (true) {
		std::unordered_set<Thread> to_remove;
		bool changes = false;
		Sleep(50);
		//system("cls");
		std::unique_lock<std::mutex> l(m1);
		cv2.wait(l, []() { return !thread_list_changed; });
		auto now = std::chrono::system_clock::now();
		EnumWindows(EnumWindowsProc, reinterpret_cast<LPARAM>(&ret));
		for (auto pair = threads.begin(); pair != threads.end(); pair++) {
			Thread t = pair->second;

			auto it = new_threads.find(t);
			if (it != new_threads.end()) {
				Thread new_thread = *it;
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
		// We want to flush anyway if 1 sec is passed
		if ((now.time_since_epoch().count() - last_notify.time_since_epoch().count()) 
			> 10000000) {
			changes = true;
		}
		last = now;
		if (changes) {
			thread_list_changed = true;
			cv1.notify_all();
			last_notify = now;
		}
		for (auto pair = threads.begin(); pair != threads.end(); pair++) {
			Thread t = pair->second; 
			//std::wcout << t.name << L" ==> " << t.active_percentage << std::endl;
		}
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
		std::unordered_set<SOCKET> to_remove;
		std::unique_lock<std::mutex> l1(m1);
		cv1.wait(l1, []() { return thread_list_changed; });
		std::unique_lock<std::mutex> l2(m2);
		for (SOCKET s : clients) {
			for (auto pair : threads) {
				Thread t = pair.second;
				size_t string_size;
				std::string serialized_thread = t.serialize(&string_size);
				if (send(s, serialized_thread.c_str(), string_size, 0) == -1) {
					std::cout << "CLIENT DISCONNECTED" << std::endl;
					to_remove.insert(s);
					break;
				}
			}
		}
		for (SOCKET s : to_remove) {
			clients.erase(clients.find(s));
		}
		thread_list_changed = false;
	}
}

void poll_actions() {
	fd_set read_set;
	struct timeval tv;

	tv.tv_sec = 0;
	tv.tv_usec = 0;

	while (true) {
		Sleep(50);
		std::unordered_set<SOCKET> to_remove;
		std::unique_lock<std::mutex> l2(m2);
		SOCKET max = 0;

		FD_ZERO(&read_set);
		for (SOCKET s : clients) {
			FD_SET(s, &read_set);
			if (s > max) max = s;
		}
		if (select(max + 1, &read_set, NULL, NULL, &tv) > 0) {
			for (SOCKET s : clients) {
				if (FD_ISSET(s, &read_set)) {
					char buffer[256] = {0};
					char *c = buffer;
					while (recv(s, c, 1, 0) == 1) {
						if (*c == '\n') {
							*c = '\0';
							break;
						}
						c++;
					}
					if (c == buffer) {
						// No received bytes, socket is closed
						to_remove.insert(s);
					}
					else {
						JSONValue *value = JSON::Parse(buffer);

						if (value == nullptr)
							continue;

						JSONObject root = value->AsObject();

						if (root.find(L"key") != root.end() && root[L"key"]->IsString()) {
							Action action(root[L"key"]->AsString());
							if (root.find(L"inputs") != root.end() && root[L"inputs"]->IsArray()) {
								JSONArray inputs = root[L"inputs"]->AsArray();
								for (auto input : inputs) {
									if (input->IsNumber())
										action.add_input(input->AsNumber());
								}
							}
							std::unique_lock<std::mutex> l(m3);
							actions.push_front(action);
							cv3.notify_all();
						}
					}
				}
			}
		}
		for (SOCKET s : to_remove) {
			std::cout << "REMOVING CLIENT" << std::endl;
			clients.erase(clients.find(s));
		}
	}
}

void execute_actions() {
	while (true) {
		std::unique_lock<std::mutex> l(m3);
		cv3.wait(l, []() {return !actions.empty(); });
		while (!actions.empty()) {
			Action action = actions.back();
			actions.pop_back();

			if (threads.find(action.key) != threads.end() && threads[action.key].focus) {
				INPUT *inputs = action.get_inputs();
				SendInput(action.inputs.size() * 2, inputs, sizeof(INPUT));
				delete inputs;
			}
		}
	}
}

void err_fatal(char *mes) {
	std::cout << mes << " ERROR NUMBER = " << WSAGetLastError() << std::endl;
	perror("");
	getchar();
	exit(-1);
}