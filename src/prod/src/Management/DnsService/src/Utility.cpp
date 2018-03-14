// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "Utility.h"

void DNS::ExecuteCommandLine(
    __in std::string const & cmdline,
    __out std::string& output,
    __in ULONG timeoutInSec,
    __in IDnsTracer& tracer)
{
#if !defined(PLATFORM_UNIX)
    SECURITY_ATTRIBUTES saAttr;
    saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
    saAttr.bInheritHandle = TRUE;
    saAttr.lpSecurityDescriptor = NULL;

    HANDLE hOutRead = NULL;
    HANDLE hOutWrite = NULL;
    if (!CreatePipe(&hOutRead, &hOutWrite, &saAttr, 0))
    {
        tracer.Trace(DnsTraceLevel_Warning, "ExecuteCommandLine, Failed to create pipe, error {0}", GetLastError());
        return;
    }

    SetHandleInformation(hOutRead, HANDLE_FLAG_INHERIT, 0);
    DWORD dwMode = PIPE_NOWAIT;
    SetNamedPipeHandleState(hOutRead, &dwMode, NULL, NULL);

    // Auto cleanup for handles
    HandleSPtr spHandleOutRead(hOutRead, NULL);
    HandleSPtr spHandleOutWrite(hOutWrite, NULL);

    STARTUPINFOA startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);
    startupInfo.hStdOutput = hOutWrite;
    startupInfo.hStdError = hOutWrite;
    startupInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    if (!CreateProcessA(NULL, const_cast<LPSTR>(cmdline.c_str()), NULL, NULL, TRUE, 0, NULL, NULL, &startupInfo, &processInfo))
    {
        tracer.Trace(DnsTraceLevel_Warning, "ExecuteCommandLine, Failed to create process, error {0}", GetLastError());
        return;
    }

    // Auto cleanup for handles
    HandleSPtr spHandleProcess(processInfo.hProcess, NULL);
    {
        HandleSPtr spHandleThread(processInfo.hThread, NULL);
        spHandleOutWrite.Cleanup();
    }

    // Read the output
    CHAR szOutput[2048];
    Common::DateTime startTime = Common::DateTime::Now();
    for (;;)
    {
        Common::TimeSpan duration = Common::DateTime::Now() - startTime;
        if (duration > Common::TimeSpan::FromSeconds(timeoutInSec))
        {
            tracer.Trace(DnsTraceLevel_Warning, "ExecuteCommandLine, timeout while executing child process");
            break;
        }

        DWORD dwRead = 0;
        ReadFile(hOutRead, szOutput, ARRAYSIZE(szOutput) - 1, &dwRead, NULL);

        if (dwRead > 0)
        {
            szOutput[dwRead] = '\0';
            std::string tmp(szOutput);
            output += tmp;
        }
        else
        {
            DWORD error = WaitForSingleObject(processInfo.hProcess, 0);
            if (error == WAIT_OBJECT_0)
            {
                // process has terminated
                break;
            }
        }
    }

#else
    Common::ProcessUtility::GetStatusOutput(cmdline, output);
#endif
}
