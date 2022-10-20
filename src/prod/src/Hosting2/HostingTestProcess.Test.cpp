// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

#if !defined(PLATFORM_UNIX)
const wstring ProcessSyncEventName = L"HostingProcessSync";

void StringConsoleOut();
void SetEvent();
void SetCtrlHandler(bool returnVal);
BOOL WINAPI CtrlCHandlerRoutine(__in  DWORD dwCtrlType);
bool CtrlCAction;
RealConsole console;

int __cdecl wmain(int argc, __in_ecount(argc) wchar_t* argv[])
{
    wstring arg = L"";
    if (argc >= 2)
    {
        arg = argv[1];
    }
    else
    {
        exit(-1);
    }

    if (StringUtility::AreEqualCaseInsensitive(arg, L"consoleWrite"))
    {
        StringConsoleOut();
    }
    else if(StringUtility::AreEqualCaseInsensitive(arg, L"CtrlCExit"))
    {
        SetCtrlHandler(false);
    }
    else if(StringUtility::AreEqualCaseInsensitive(arg, L"CtrlCBlock"))
    {
        SetCtrlHandler(true);
    }
}

void StringConsoleOut()
{
    for (int i = 0; i < 1000; i++)
    {
        console.WriteLine("Writing test data to console");
    }
    SetEvent();
    exit(0);
}

void SetCtrlHandler(bool returnVal)
{
    CtrlCAction = returnVal;
    ::SetConsoleCtrlHandler(
        &CtrlCHandlerRoutine,
        true);

    SetEvent();

    // Wait to receive the ctrl signal
    Sleep(20000);
    exit(0);
}

BOOL WINAPI CtrlCHandlerRoutine(__in  DWORD)
{
    return CtrlCAction;
}

void SetEvent()
{
    HandleUPtr handle = make_unique<Handle>(
    ::CreateEventW(
        NULL,
        true,
        false,
        ProcessSyncEventName.c_str()));
    ::SetEvent(handle->Value);
}
#else

RealConsole console;
bool CtrlCAction = false;
int syncEventFD = -1;

void StringConsoleOut()
{
    for (int i = 0; i < 1; i++)
    {
        console.WriteLine("HostingTestProcess {0}: Writing test data to console", getpid());
        fprintf(stdout, "HostingTestProcess %d: Writing test data to stdout\n", getpid());
        fprintf(stderr, "HostingTestProcess %d: Writing test data to stderr\n", getpid());
    }

    fflush(stdout);
    fflush(stderr);
    exit(0);
}

void signal_parent()
{
    if (syncEventFD == -1)
    {
        return;
    }

    uint64 value = 1;
    int res = write(syncEventFD, &value, sizeof(value));
    if (res == -1)
    {
        fprintf(stderr, "HostingTestProcess %d: Error while signalling parent. Exiting.\n", getpid());
        exit(-1);
    }
}

void sigint_handler(int sig)
{
    if(CtrlCAction)
        exit(0);
}

void SetCtrlHandler(bool returnVal)
{
    CtrlCAction = returnVal;
    signal(SIGINT, sigint_handler);

    // sigint_handler is set.
    // Signal readiness to parent before looping.
    signal_parent();

    for (;;) pause();
}

int main(int argc, char* argv[])
{
    string arg = "";

    if (argc >= 2)
    {
        arg = argv[1];
        if (argc >= 3)
        {
            syncEventFD = atoi(argv[2]);
        }
    }
    else
    {
        fprintf(stderr, "HostingTestProcess %d: argc should be >=2. Exiting.\n", getpid());
        exit(-1);
    }

    wstring warg;
    StringUtility::Utf8ToUtf16(arg, warg);
    if (StringUtility::AreEqualCaseInsensitive(warg, L"consoleWrite"))
    {
        StringConsoleOut();
    }
    else if(StringUtility::AreEqualCaseInsensitive(warg, L"CtrlCExit"))
    {
        SetCtrlHandler(true);
    }
    else if(StringUtility::AreEqualCaseInsensitive(warg, L"CtrlCBlock"))
    {
        SetCtrlHandler(false);
    }
}
#endif
