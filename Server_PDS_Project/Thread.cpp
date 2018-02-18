#include "stdafx.h"
#include "Thread.h"
//#include "saveicon.h"
//#include "base64.h"

bool Thread::operator==(const Thread &other) const
{
	return this->tid == other.tid && this->name == other.name; //&& this->hWnd == other.hWnd;
}

Thread::Thread(const Thread & t)
{
	this->hWnd = t.hWnd;
	this->name = t.name;
	this->tid = t.tid;
	this->hIcon = t.hIcon;
	this->active_time = t.active_time;
	this->active_percentage = t.active_percentage;
	this->focus = t.focus;
	this->notification = t.notification;
}

Thread & Thread::operator=(const Thread & t)
{
	if (this != &t) {
		this->hWnd = t.hWnd;
		this->name = t.name;
		this->tid = t.tid;
		this->hIcon = t.hIcon;
		this->active_time = t.active_time;
		this->active_percentage = t.active_percentage;
		this->focus = t.focus;
		this->notification = t.notification;
	}
	return *this;
}

std::wstring Thread::get_key()
{
	std::wstring name = this->name;
	return std::wstring(name.append(std::wstring(std::to_wstring(this->tid))));
}

std::string Thread::serialize(size_t *size)
{
	JSONObject json;

	json[L"title"] = new JSONValue(this->name);
	json[L"pid"] = new JSONValue((int) this->tid);
	json[L"isFocused"] = new JSONValue(this->focus);
	json[L"notification"] = new JSONValue(this->notification);

	/*SaveIcon3(L"C:\\Users\\filip\\Desktop\\icone\\icona.ico", &this->hIcon, 1);
	std::ifstream file("C:\\Users\\filip\\Desktop\\icone\\icona.ico", std::ios::binary | std::ios::ate);
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> bytes(size);
	file.read(bytes.data(), size);
	json[l"iconsize"] = new jsonvalue((int)size);
	json[l"icon"] = new jsonvalue(base64_encode(bytes.data(), size).c_str());*/
	JSONValue *value = new JSONValue(json);
	std::wstring str = value->Stringify();
	std::string s(str.begin(), str.end());
	s.append("\r\n");
	*size = s.length();
	delete value;
	return s;
}


HWND Thread::get_handle() {
	return this->hWnd;
}

Thread::~Thread()
{
}
