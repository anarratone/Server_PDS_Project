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
	Thread(HWND hWnd, std::wstring name, DWORD tid, HICON hIcon) :
		notification(0), hWnd(hWnd), name(name), tid(tid), hIcon(hIcon) {};
	std::string serialize(size_t *size);
	void setFocusFlag();
	HWND getHandle();
	~Thread();
};