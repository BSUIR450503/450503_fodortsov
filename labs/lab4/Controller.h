#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string>

class Controller
{

public:
	Controller();
	static void createProcess(char *proc_name);
    static void createOutputThread(int num);
	static void  endLastProcess();
	static void endAll();
	~Controller();
};

