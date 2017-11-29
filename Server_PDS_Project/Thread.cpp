#include "stdafx.h"
#include "Thread.h"
//#include "saveicon.h"
//#include "base64.h"

std::string Thread::serialize(size_t *size)
{
	JSONObject json;

	json[L"id"] = new JSONValue(this->id);
	json[L"titolo"] = new JSONValue(this->name);
	json[L"pid"] = new JSONValue((int) this->tid);
	json[L"isFocused"] = new JSONValue(this->focus);
	json[L"notification"] = new JSONValue(this->notification);

	//SaveIcon3(L"C:\\Users\\filip\\Desktop\\icone\\icona.ico", &this->hIcon, 1);
	//std::ifstream file("C:\\Users\\filip\\Desktop\\icone\\icona.ico", std::ios::binary | std::ios::ate);
	/*std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);
	std::vector<char> bytes(size);
	file.read(bytes.data(), size);
	json[l"iconsize"] = new jsonvalue((int)size);
	json[l"icon"] = new jsonvalue(base64_encode(bytes.data(), size).c_str());
	*/
	JSONValue *value = new JSONValue(json);
	std::wstring str = value->Stringify();
	std::string s(str.begin(), str.end());
	s.append("\r\n");
	*size = s.length();
	delete value;
	return s;
}

void Thread::setFocusFlag()
{
	this->focus = true;
}

HWND Thread::getHandle() {
	return this->hWnd;
}

Thread::~Thread()
{
}
