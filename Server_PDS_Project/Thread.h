#pragma once
#include <string>
#include "stdafx.h"
#include <Windows.h>

class Thread
{
private:
	HICON hIcon;

public:
	DWORD tid;
	bool focus = false;
	std::wstring name;
	HWND hWnd;
	int notification;
	double active_time = 0;
	double active_percentage = 0;

	std::string serialize(size_t *size);
	HWND get_handle();
	Thread(HWND hWnd, std::wstring name, DWORD tid, HICON hIcon) :
		notification(0), hWnd(hWnd), name(name), tid(tid), hIcon(hIcon) {};
	Thread() :
		notification(0), hWnd(NULL), name(L""), tid(0), hIcon(NULL) {};
	bool operator== (const Thread &other) const;
	Thread(const Thread &t);
	Thread &operator= (const Thread &t);
	std::wstring get_key();
	~Thread();
};