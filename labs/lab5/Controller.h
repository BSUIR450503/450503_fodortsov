#pragma once
#include <windows.h>
#include <stdio.h>
#include <tchar.h>
#include <stdlib.h>
#include <cstring>
#include <string>

class Controller
{
	static HANDLE lastProcess;
public:
	Controller();
	static void createProcess(char *proc_name);
	static void createOutputProcess(int num);
	static void endLastProcess();
	static void endAll();
	static void exexuteApplicationLogic(int isServer);
	static void executeClient();
	static void createOutputThread(int num);
	static void endLastThread();
	static void endAllThreads();
	static void startAoi();
	~Controller();
};

