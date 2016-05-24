// Laba4.cpp: определяет точку входа для консольного приложения.
//

#include "stdafx.h"
#include <iostream>
#include "Controller.h"
#include <conio.h>


int _tmain(int argc, _TCHAR* argv[])
{
	char c;
	int out = 0;
	int eventCount = 0;
	while (!out)
	{
		c = _getch();
		switch (c)
		{
		case '+':
			Controller::createOutputThread(eventCount);
			eventCount++;
			break;
		case '-':
			Controller::endLastThread();
			if(eventCount) eventCount--;
			break;
		case 'q':
			Controller::endAllThreads();
			out = 1;
			break;
		default:
			break;
		}
	}
	return 0;
}

