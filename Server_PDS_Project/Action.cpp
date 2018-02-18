#include "stdafx.h"
#include "Action.h"


void Action::add_input(int input) {
	this->inputs.push_back(input);
}

INPUT * Action::get_inputs()
{
	INPUT *inputs = new INPUT[this->inputs.size() * 2]; // We also want the key up
	for (int i = 0; i < this->inputs.size(); i++) {
		inputs[i].type = INPUT_KEYBOARD;
		inputs[i].ki.dwExtraInfo = NULL;
		inputs[i].ki.time = 0;
		inputs[i].ki.dwFlags = 0;
		inputs[i].ki.wVk = this->inputs[i];
	}
	for (int i = this->inputs.size(); i < this->inputs.size() * 2; i++) {
		inputs[i].type = INPUT_KEYBOARD;
		inputs[i].ki.dwExtraInfo = NULL;
		inputs[i].ki.time = 0;
		inputs[i].ki.dwFlags = KEYEVENTF_KEYUP;
		inputs[i].ki.wVk = this->inputs[i % this->inputs.size()];
	}
	return inputs;
}

void Action::print() {
	std::wcout << this->key << " ";
	for (auto i : this->inputs) std::wcout << i;
	std::wcout << std::endl;
}

Action::~Action()
{
}
