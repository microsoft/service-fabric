// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const Trace_ProcessActivationContext("ProcessActivationContext");

ProcessActivationContext::ProcessActivationContext(Common::ProcessWaitSPtr const & processMonitor,
Common::ProcessHandleUPtr && processHandle,
Common::JobHandleUPtr && jobHandle,
Common::HandleUPtr && threadHandle,
DWORD processId,
ProcessConsoleRedirectorSPtr processStdoutRedirector,
ProcessConsoleRedirectorSPtr processStderrRedirector,
bool hasConsole,
DWORD debuggerProcId,
Common::ProcessHandleUPtr && debuggerprocessHandle,
Common::HandleUPtr && debuggerThreadHandle,
Common::ProcessWaitSPtr const & debugprocessMonitor)
: processMonitor_(processMonitor),
processHandle_(move(processHandle)),
jobHandle_(move(jobHandle)),
threadHandle_(move(threadHandle)),
workstationHandle_(),
desktopHandle_(),
processId_(processId),
processStdoutRedirector_(processStdoutRedirector),
processStderrRedirector_(processStderrRedirector),
hasConsole_(hasConsole),
debuggerProcId_(debuggerProcId),
debuggerProcessHandle_(move(debuggerprocessHandle)),
debuggerThreadHandle_(move(debuggerThreadHandle)),
debugprocessMonitor_(debugprocessMonitor)
{
}

ProcessActivationContext::ProcessActivationContext(
    Common::ProcessWaitSPtr const & processMonitor,
    Common::ProcessHandleUPtr && processHandle,
    Common::JobHandleUPtr && jobHandle,
    Common::HandleUPtr && threadHandle,
    Common::WorkstationHandleUPtr && workstationHandle,
    Common::DesktopHandleUPtr && desktopHandle,
    wstring const & containerId,
    DWORD processId,
    ProcessConsoleRedirectorSPtr processStdoutRedirector,
    ProcessConsoleRedirectorSPtr processStderrRedirector,
    bool hasConsole,
    DWORD debuggerProcessId,
    Common::ProcessHandleUPtr && debuggerprocessHandle,
    Common::HandleUPtr && debuggerThreadHandle,
    Common::ProcessWaitSPtr const & debugprocessMonitor)
    : processMonitor_(processMonitor),
    processHandle_(move(processHandle)),
    jobHandle_(move(jobHandle)),
    threadHandle_(move(threadHandle)),
    workstationHandle_(move(workstationHandle)),
    desktopHandle_(move(desktopHandle)),
    containerId_(containerId),
    processId_(processId),
    processStdoutRedirector_(processStdoutRedirector),
    processStderrRedirector_(processStderrRedirector),
    hasConsole_(hasConsole),
    debuggerProcId_(debuggerProcessId),
    debuggerProcessHandle_(move(debuggerprocessHandle)),
    debuggerThreadHandle_(move(debuggerThreadHandle)),
    debugprocessMonitor_(debugprocessMonitor)
{
}

void ProcessActivationContext::CreateProcessActivationContext(
    Common::ProcessWaitSPtr const & processMonitor,
    Common::ProcessHandleUPtr && processHandle,
    Common::JobHandleUPtr && jobHandle,
    Common::HandleUPtr && threadHandle,
    Common::WorkstationHandleUPtr && workstationHandle,
    Common::DesktopHandleUPtr && desktopHandle,
    wstring containerId,
    DWORD processId,
    ProcessConsoleRedirectorSPtr processStdoutRedirector,
    ProcessConsoleRedirectorSPtr processStderrRedirector,
    bool hasConsole,
    DWORD debuggerProcessId,
    Common::ProcessHandleUPtr && debuggerProcessHandle,
    Common::HandleUPtr && debuggerThreadHandle,
    Common::ProcessWaitSPtr const & debugprocessMonitor,
    __out ProcessActivationContextSPtr & activationContext)
{
    activationContext = ProcessActivationContextSPtr(new ProcessActivationContext(
        processMonitor,
        move(processHandle),
        move(jobHandle),
        move(threadHandle),
        move(workstationHandle),
        move(desktopHandle),
        containerId,
        processId,
        processStdoutRedirector,
        processStderrRedirector,
        hasConsole,
        debuggerProcessId,
        move(debuggerProcessHandle),
        move(debuggerThreadHandle),
        debugprocessMonitor));
}

ProcessActivationContext::~ProcessActivationContext()
{
}

ErrorCode ProcessActivationContext::TerminateAndCleanup(bool processGracefulExited, UINT uExitCode)
{
    ErrorCode error(ErrorCodeValue::Success);

    // try to terminate the process if needed
    if (!processGracefulExited)
    {
        if (!::TerminateProcess(ProcessHandle->Value, uExitCode))
        {
            // log and do not return, close the job object as well
            error = Common::ErrorCode::FromWin32Error();
            WriteWarning(
                Trace_ProcessActivationContext,
                "Terminate process failed for process {0}, error {1}",
               this->ProcessId,
                error);
        }
    }

    if (JobHandle)
    {
        // in interactive sandboxed mode, JobObject is not valid

        // terminate and close job object
        if (!::TerminateJobObject(JobHandle->Value, uExitCode))
        {
            // if close the job object fails than simply log and return success
            error = Common::ErrorCode::FromWin32Error();
            WriteWarning(
                Trace_ProcessActivationContext,
                "Terminate job failed for process {0}, error {1}",
                this->ProcessId,
                error);
        }
    }
    return error;
}

ErrorCode ProcessActivationContext::ResumeProcess()
{
    DWORD val = ::ResumeThread(ThreadHandle->Value);
    if (val != 1)
    {
        auto error = Common::ErrorCode::FromWin32Error();
        WriteWarning(
            Trace_ProcessActivationContext,
            "Resume process failed for process {0}, error {1}",
            this->ProcessId,
            error);
    }
    return Common::ErrorCodeValue::Success;
}
