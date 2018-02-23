#include "stdafx.h"
#include "Thread.h"
#include "SaveIcon.h"
#include "Base64.h"

bool Thread::operator==(const Thread &other) const {
	return this->tid == other.tid && this->name == other.name;
}

Thread::Thread(const Thread & t) {
	this->hWnd = t.hWnd;
	this->name = t.name;
	this->tid = t.tid;
	this->hIcon = t.hIcon;
	this->active_time = t.active_time;
	this->active_percentage = t.active_percentage;
	this->focus = t.focus;
}

Thread & Thread::operator=(const Thread & t) {
	if (this != &t) {
		this->hWnd = t.hWnd;
		this->name = t.name;
		this->tid = t.tid;
		this->hIcon = t.hIcon;
		this->active_time = t.active_time;
		this->active_percentage = t.active_percentage;
		this->focus = t.focus;
	}
	return *this;
}

std::wstring Thread::get_key() {
	std::wstring name = this->name;
	return std::wstring(name.append(std::wstring(std::to_wstring(this->tid))));
}

std::string Thread::serialize(size_t *size) {
	JSONObject json;

	json[L"percentage"] = new JSONValue((float) this->active_percentage * 100);
	json[L"title"] = new JSONValue(this->name);
	json[L"pid"] = new JSONValue((int) this->tid);
	json[L"isFocused"] = new JSONValue(this->focus);

	// Ugly as hell: we save the Icon data on a file and we encode the file base64
	// It is out of the scope of this project to handle Bitmaps and hIcon data
	SaveIcon3(L"icon.ico", &this->hIcon, 1);
	std::ifstream file(L"icon.ico", std::ios::binary | std::ios::ate);
	std::streamsize icon_size = file.tellg();
	file.seekg(0, std::ios::beg);
    std::vector<char> bytes(icon_size);
	file.read(bytes.data(), icon_size);

	json[L"iconSize"] = new JSONValue((int) icon_size);
	std::string icon_encoded = base64_encode(bytes.data(), icon_size);
	std::wstring wicon_encoded(icon_encoded.length(), L' ');
	std::copy(icon_encoded.begin(), icon_encoded.end(), wicon_encoded.begin());
	json[L"icon"] = new JSONValue(wicon_encoded);
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

Thread::~Thread() {
}
