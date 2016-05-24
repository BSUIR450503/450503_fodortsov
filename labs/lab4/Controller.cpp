#include "Controller.h"
#pragma warning(disable:4996)

#include <stdexcept>
#include <vector>
#include <fstream>
#include <iostream>
#include <QDebug>
#define BUFSIZE 512

using std::runtime_error;
using std::string;
using std::vector;
using std::wstring;


Controller::Controller()
{
}

void writeData()
{
	int t, op;
	std::cout << "Enter number: ";
	std::cin >> t;
	std::cout << "Enter operation code: ";
	std::cin >> op;
	std::ofstream file("text.txt", std::ios::out | std::ios::trunc);
	file << t << " " << op;
	file.close();
}

void readData()
{
	int t;
	std::ifstream load("text.txt");
	load >> t;
	std::cout << "Result is " << t << std::endl;
	load.close();
}

#if defined(_WIN32) || defined (_WIN64)

#include <windows.h>
#include <windows.h>

static const int SIZE_OF_BUFFER = 25;

HANDLE Controller::lastProcess = NULL;

wstring utf8toUtf16(const string & str)
{
    if (str.empty())
        return wstring();
    size_t charsNeeded = ::MultiByteToWideChar(CP_UTF8, 0,
        str.data(), (int)str.size(), NULL, 0);
    if (charsNeeded == 0)
        throw runtime_error("Failed converting UTF-8 string to UTF-16");

    vector<wchar_t> buffer(charsNeeded);
    int charsConverted = ::MultiByteToWideChar(CP_UTF8, 0,
        str.data(), (int)str.size(), &buffer[0], buffer.size());
    if (charsConverted == 0)
        throw runtime_error("Failed converting UTF-8 string to UTF-16");

    return wstring(&buffer[0], charsConverted);
}

void startSynchronization()
{

}

void  Controller::createProcess(char *proc_name)
{
            STARTUPINFO si;
            PROCESS_INFORMATION pi;
            ZeroMemory(&si, sizeof(si));
            si.cb = sizeof(si);
            ZeroMemory(&pi, sizeof(pi));
            std::wstring name = utf8toUtf16(proc_name);
            LPTSTR szCmdline = _tcsdup(name.c_str());
            writeData();
            if (!CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
            {
                printf("CreateProcess failed (%d).\n", GetLastError());
                return;
            }
            WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(pi.hProcess);
            CloseHandle(pi.hThread);
            readData();
}

HANDLE firstEvent;
HANDLE secondEvent;
HANDLE thirdEvent;
std::vector<PROCESS_INFORMATION> prInf;

void  Controller::createOutputProcess(int num)
{
    firstEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("StartEvent"));
    secondEvent = CreateEvent(NULL, TRUE, TRUE, TEXT("AddEvent"));
    thirdEvent = CreateEvent(NULL, TRUE, TRUE, TEXT("LockEvent"));
    WaitForSingleObject(thirdEvent, INFINITE);
    ResetEvent(thirdEvent);
    WaitForSingleObject(secondEvent, INFINITE);
    WaitForSingleObject(firstEvent, INFINITE);
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    char buff[100];
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    std::string str = ("Client.exe ");
    itoa(num,buff,10);
    str += buff;
    std::wstring name = utf8toUtf16(str);
    LPTSTR szCmdline = _tcsdup(name.c_str());
    if (!CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, NULL, NULL, NULL, &si, &pi))
    {
        printf("CreateProcess failed (%d).\n", GetLastError());
        return;
    }
    prInf.push_back(pi);
    SetEvent(firstEvent);
    SetEvent(thirdEvent);
}

void  Controller::endLastProcess()
{
    if (prInf.size() == 0) return;
    firstEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("StartEvent"));
    WaitForSingleObject(firstEvent, INFINITE);
    TerminateProcess(prInf[prInf.size() - 1].hProcess,0);
    CloseHandle(prInf[prInf.size() - 1].hProcess);
    CloseHandle(prInf[prInf.size() - 1].hThread);
    prInf.pop_back();
    SetEvent(firstEvent);
}

void Controller::endAll()
{
    while (prInf.size()!=0)
    {
        TerminateProcess(prInf[prInf.size() - 1].hProcess, 0);
        CloseHandle(prInf[prInf.size() - 1].hProcess);
        CloseHandle(prInf[prInf.size() - 1].hThread);
        prInf.pop_back();
    }
}

HANDLE channels[2], exitSemaphore[5];
PROCESS_INFORMATION pi;
int server;

DWORD WINAPI inputMessage(LPVOID lpParam)
{
    int isServer = server;
    char buffer[SIZE_OF_BUFFER];
    std::string mes;
    DWORD NumberOfBytesWritten;
    while (true)
    {
        std::cout << "Enter message:";
        std::cin >> mes;
        ReleaseSemaphore(exitSemaphore[2 * isServer], 2, NULL);
        if (mes == "quit")
        {
            ReleaseSemaphore(exitSemaphore[4], 1, NULL);
            if(isServer) WaitForSingleObject(pi.hProcess, INFINITE);
            CloseHandle(exitSemaphore[0]);
            CloseHandle(exitSemaphore[1]);
            CloseHandle(exitSemaphore[2]);
            CloseHandle(exitSemaphore[3]);
            CloseHandle(exitSemaphore[4]);
            CloseHandle(channels[0]);
            CloseHandle(channels[1]);
            break;
        }
        int NumberOfBlocks = mes.size() / SIZE_OF_BUFFER + 1;
        if (!WriteFile(channels[1-isServer], &NumberOfBlocks, sizeof(NumberOfBlocks), &NumberOfBytesWritten, (LPOVERLAPPED)NULL)) std::cout << "Write Error\n";
        int size = mes.size();
        WriteFile(channels[1 - isServer], &size, sizeof(size), &NumberOfBytesWritten, (LPOVERLAPPED)NULL);
        for (int i = 0; i < NumberOfBlocks; i++)
        {
            mes.copy(buffer, SIZE_OF_BUFFER, i*SIZE_OF_BUFFER);
            if (!WriteFile(channels[1 - isServer], buffer, SIZE_OF_BUFFER, &NumberOfBytesWritten, (LPOVERLAPPED)NULL)) std::cout << "Write Error\n";
        }
        WaitForSingleObject(exitSemaphore[1+2*(1 - isServer)], INFINITE); //1 for Client, 3 for Server
    }
    return 0;
}

void Controller::exexuteApplicationLogic(int isServer)
{
    char buffer[SIZE_OF_BUFFER];
    std::string mes;
    server = isServer;

    DWORD   dwThreadIdArray[1];
    HANDLE  hThreadArray[1];
    if (isServer) {

        STARTUPINFO si;
        ZeroMemory(&si, sizeof(si));
        si.cb = sizeof(si);
        ZeroMemory(&pi, sizeof(pi));
        std::string parametres = "Laba3.exe ";
        parametres += "Random_stuff";
        std::wstring name = utf8toUtf16(parametres.c_str());
        LPTSTR szCmdline = _tcsdup(name.c_str());
        channels[0] = CreateNamedPipe(TEXT("\\\\.\\pipe\\Channel1"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, (LPSECURITY_ATTRIBUTES)NULL);
        exitSemaphore[0] = CreateSemaphore(NULL, 0, 2, TEXT("serverInputSemaphore"));
        exitSemaphore[1] = CreateSemaphore(NULL, 0, 2, TEXT("serverOutputSemaphore"));
        exitSemaphore[2] = CreateSemaphore(NULL, 0, 2, TEXT("clientInputSemaphore"));
        exitSemaphore[3] = CreateSemaphore(NULL, 0, 2, TEXT("clientOutputSemaphore"));
        exitSemaphore[4] = CreateSemaphore(NULL, 0, 2, TEXT("exitSemaphore"));
        std::cout << "This is server\n";
        CreateProcess(NULL, szCmdline, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
        if (!ConnectNamedPipe(channels[0], (LPOVERLAPPED)NULL))
            std::cout << "Connection failure\n";
        channels[1] = CreateFile(TEXT("\\\\.\\pipe\\Channel2"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    }
    else {
        std::cout << "This is client\n";
        channels[1] = CreateNamedPipe(TEXT("\\\\.\\pipe\\Channel2"), PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_WAIT, PIPE_UNLIMITED_INSTANCES, 0, 0, INFINITE, (LPSECURITY_ATTRIBUTES)NULL);
        exitSemaphore[0] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("serverInputSemaphore"));
        exitSemaphore[1] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("serverOutputSemaphore"));
        exitSemaphore[2] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("clientInputSemaphore"));
        exitSemaphore[3] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("clientOutputSemaphore"));
        exitSemaphore[4] = OpenSemaphore(SEMAPHORE_ALL_ACCESS, TRUE, TEXT("exitSemaphore"));
        channels[0] = CreateFile(TEXT("\\\\.\\pipe\\Channel1"), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    }
    hThreadArray[0] = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size
        inputMessage,       // thread function name
        (LPVOID)NULL,          // argument to thread function
        0,                      // use default creation flags
        &dwThreadIdArray[1]);   // returns the thread identifier

    if (hThreadArray[0] == NULL)
    {
        ExitProcess(3);
    }
    while (true) {
        DWORD NumberOfBytesRead;
        mes.clear();
        bool successFlag;
        int index = WaitForMultipleObjects(5, exitSemaphore, FALSE, INFINITE) - WAIT_OBJECT_0;
        if (index == 4) break;
        if (index == 2 * (1 - isServer))
        {
            std::cout << "\nOutput message:";
            int NumberOfBlocks;
            if (!ReadFile(channels[isServer], &NumberOfBlocks, sizeof(NumberOfBlocks), &NumberOfBytesRead, NULL)) break;

            int size;
            if (!ReadFile(channels[isServer], &size, sizeof(size), &NumberOfBytesRead, NULL)) break;

            for (int i = 0; i < NumberOfBlocks; i++)
            {
                successFlag = ReadFile(channels[isServer], buffer, sizeof(buffer), &NumberOfBytesRead, NULL);
                if (!successFlag) break;

                mes.append(buffer, sizeof(buffer));
            }
            if (!successFlag) break;

            mes.resize(size);
            for (int i = 0; i < size; i++)
            {
                std::cout << mes[i];
                Sleep(100);
            }
            std::cout << "\nEnter message:";
            ReleaseSemaphore(exitSemaphore[1+ 2 * isServer], 2, NULL); //1 for Client, 3 for Server
        }
    }
}

std::string getNumLessThenTen(int num)
{
    switch (num)
    {
    case 1:
        return "FIRST";
        break;
    case 2:
        return "SECOND";
        break;
    case 3:
        return "THIRD";
        break;
    case 4:
        return "FOURTH";
        break;
    case 5:
        return "FIFTH";
        break;
    case 6:
        return "SIXTH";
        break;
    case 7:
        return "SEVENTH";
        break;
    case 8:
        return "FIRST";
        break;
    case 9:
        return "FIRST";
        break;
    default:
        return "";
        break;
    }
}

typedef struct MyData {
    int threadNumber;
} MYDATA, *PMYDATA;
std::vector<HANDLE> threadHandles;

DWORD WINAPI threadFunction(LPVOID lpParam)
{
    PMYDATA pData;
    pData = (PMYDATA)lpParam;
    std::string processName = getNumLessThenTen(pData->threadNumber + 1);
    processName += " THREAD\n";
    firstEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("StartEvent"));
    secondEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("AddEvent"));
    ResetEvent(secondEvent);
    while (true)
    {
        WaitForSingleObject(firstEvent, INFINITE);
        int i = 0;
        while (i < processName.size())
        {
            ResetEvent(secondEvent);
            Sleep((DWORD)50);
            std::cout << processName[i];
            i++;
        }
        if (pData->threadNumber == threadHandles.size() - 1) SetEvent(secondEvent);
        SetEvent(firstEvent);
    }
    return 0;
}

void Controller::createOutputThread(int num)
{
    PMYDATA pData;
    DWORD   dwThreadIdArray;
    HANDLE  hThreadArray;
    Sleep(100);
    firstEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("StartEvent"));
    secondEvent = CreateEvent(NULL, TRUE, TRUE, TEXT("AddEvent"));
    thirdEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("LockEvent"));
    if (num != threadHandles.size())WaitForSingleObject(thirdEvent, INFINITE);
    //ResetEvent(thirdEvent);
    WaitForSingleObject(secondEvent, INFINITE);
    //WaitForSingleObject(firstEvent, INFINITE);
    pData = (PMYDATA)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY,sizeof(MYDATA));
    pData->threadNumber = num;
    hThreadArray = CreateThread(
        NULL,                   // default security attributes
        0,                      // use default stack size
        threadFunction,       // thread function name
        pData,          // argument to thread function
        0,                      // use default creation flags
        &dwThreadIdArray);
    threadHandles.push_back(hThreadArray);
    if(num == 0) SetEvent(firstEvent);
    SetEvent(thirdEvent);
}

void Controller::endLastThread()
{
    if (threadHandles.size() == 0) return;
    firstEvent = CreateEvent(NULL, FALSE, TRUE, TEXT("StartEvent"));
    WaitForSingleObject(firstEvent, INFINITE);
    TerminateThread(threadHandles[threadHandles.size() - 1], 0);
    CloseHandle(threadHandles[threadHandles.size() - 1]);
    threadHandles.pop_back();
    SetEvent(firstEvent);
}

void Controller::endAllThreads()
{
    while (threadHandles.size() != 0)
    {
        TerminateProcess(threadHandles[threadHandles.size() - 1], 0);
        CloseHandle(threadHandles[threadHandles.size() - 1]);
        threadHandles.pop_back();
    }
}
#endif

#if defined(linux) || defined(__linux)
#include <sys/types.h>
#include <wait.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>

void  Controller::createProcess(char *proc_name)
{
    pid_t pid = fork();
    if (pid == 0)
    {
        fflush(NULL);
        execlp("./Math","Math",(char*)NULL);
        perror("Launch process fail");
        exit(0);
    }
    else if (pid > 0)
    {
        writeData();
        kill(pid,SIGUSR1);
        int returnStatus;
        waitpid(pid, &returnStatus, 0);
        readData();
    }
    else
    {
        printf("fork() failed!\n");
        return;
    }

}

std::vector<pthread_t> threads;
pthread_t mainThread = 0;
int now = 0;
bool start = true;
pid_t ppid;
int number=1;
int endLast=0;

std::string getNumLessThenTen(int num)
{
    switch (num)
    {
        case 1:
        return "FIRST";
        break;
        case 2:
        return "SECOND";
        break;
        case 3:
        return "THIRD";
        break;
        case 4:
        return "FOURTH";
        break;
        case 5:
        return "FIFTH";
        break;
        case 6:
        return "SIXTH";
        break;
        case 7:
        return "SEVENTH";
        break;
        case 8:
        return "FIRST";
        break;
        case 9:
        return "FIRST";
        break;
        default:
        return "";
        break;
    }
}

void sig_handler_thread(int signo)
{
  if (signo == SIGUSR1){
     if(now==threads.size()-1&&endLast!=0)
     {
         endLast --;
         now = 0;
         threads.pop_back();
         if(threads.size()!=0) pthread_kill(threads[now],SIGUSR1);
         pthread_exit(0);
         return;
     }
     std::string processName = getNumLessThenTen(now+1);
     processName += " THREAD\n";
     int i = 0;
     while (i < processName.size())
     {
         usleep(100000);
         std::cout << processName[i];
         i++;
         fflush(stdout);
     }
     if(threads.size()==0) return;
     now++;


     if(now>=threads.size()) now=0;
     pthread_kill(threads[now],SIGUSR1);
  }
}

void * thread_func(void *arg)
{
    number = * (int*)arg;
    if (signal(SIGUSR1, sig_handler_thread) == SIG_ERR)
          printf("\ncan't catch SIGINT\n");
    while(true)
    {
        sleep(1);
    }
    return NULL;
}

//void sig_handler_term(int signo)
//{
//    if (signo == SIGUSR1){
//       if(now==pids.size()-1) now = 0;
//       zombie = pids[pids.size()-1];
//       kill(zombie,SIGKILL);
//       pids.pop_back();
//       if (signal(SIGUSR1, sig_handler_out) == SIG_ERR)
//             printf("\ncan't catch SIGINT\n");
//       kill(pids[now],SIGUSR1);
//     }
//}

void sig_handler_out(int signo)
{
    if (signo == SIGUSR1){
        std::cout << "asdasdasd";

     }
}

//void kill_emm_all(int signo)
//{
//    if (signo == SIGUSR1){
//       while(pids.size()!=0)
//       {
//           zombie = pids[pids.size()-1];
//           kill(zombie,SIGKILL);
//           pids.pop_back();
//           usleep(1000);
//       }
//       if (signal(SIGUSR1, sig_handler_out) == SIG_ERR)
//             printf("\ncan't catch SIGINT\n");
//     }
//}

void  Controller::createOutputThread(int num)
{
    pthread_t thread;
    if(mainThread==0) mainThread = pthread_self();
    ppid = getpid();
    int arg = threads.size() +1;
    int result = pthread_create(&thread, NULL, thread_func,&arg);
    if (result != 0) {
    perror("Creating the first thread");
    }
    threads.push_back(thread);
    if (signal(SIGUSR1, sig_handler_out) == SIG_ERR)
          printf("\ncan't catch SIGINT\n");
    if(threads.size()==1) {
        usleep(50000);
        pthread_kill(threads[0],SIGUSR1);
    }
}

void  Controller::endLastProcess()
{
    endLast++;
}

//void Controller::endAll()
//{
//    if (signal(SIGUSR1, kill_emm_all) == SIG_ERR)
//          printf("\ncan't catch SIGINT\n");
//}

#endif

Controller::~Controller()
{
}
