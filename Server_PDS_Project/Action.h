#pragma once
#include "stdafx.h"
#include <vector>

class Action
{
public:
	// The id of the window to send the keystrokes to
	std::wstring key;
	std::vector<int> inputs;

	Action(std::wstring key) :
		key(key) {};
	void add_input(int input);
	INPUT *get_inputs();
	void print();
	~Action();
};

