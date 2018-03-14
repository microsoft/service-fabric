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

ProcessActivationContext::ProcessActivationContext(DWORD processId, wstring cgroupName):
    processMonitor_(nullptr),
    debugprocessMonitor_(nullptr),
    jobHandle_(nullptr),
    threadHandle_(nullptr),
    workstationHandle_(nullptr),
    desktopHandle_(nullptr),
    processId_(processId),
    debuggerProcId_(-1),
    debuggerProcessHandle_(nullptr),
    debuggerThreadHandle_(nullptr),
    processStdoutRedirector_(nullptr),
    processStderrRedirector_(nullptr),
    hasConsole_(false),
    cgroupName_(cgroupName)
{

}

void ProcessActivationContext::CreateProcessActivationContext(DWORD processId, wstring cgroupName, ProcessActivationContextSPtr& activationContext)
{
    activationContext = ProcessActivationContextSPtr(new ProcessActivationContext(processId, cgroupName));
}

ProcessActivationContext::~ProcessActivationContext()
{
    if (!cgroupName_.empty())
    {
        string cgroupName;
        StringUtility::Utf16ToUtf8(cgroupName_, cgroupName);
        int error = ProcessActivator::RemoveCgroup(cgroupName);
        WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                  Trace_ProcessActivationContext,
                  "Cgroup cleanup during activation context destruction of {0} completed with error code {1}, description of error if available {2}",
                  cgroupName_, error, cgroup_get_last_errno());
    }

}

ErrorCode ProcessActivationContext::TerminateAndCleanup(bool processGracefulExited, UINT uExitCode)
{
    if (processStdoutRedirector_ != nullptr)
    {
        processStdoutRedirector_->Close();
    }

    if (processStderrRedirector_ != nullptr)
    {
        processStderrRedirector_->Close();
    }

    if (!cgroupName_.empty())
    {
        string cgroupName;
        StringUtility::Utf16ToUtf8(cgroupName_, cgroupName);
        int error = ProcessActivator::RemoveCgroup(cgroupName);
        WriteTrace(error ? Common::LogLevel::Warning : Common::LogLevel::Info,
                  Trace_ProcessActivationContext,
                  "Cgroup cleanup of {0} completed with error code {1}, description of error if available {2}",
                  cgroupName_, error, cgroup_get_last_errno());
    }
    return ErrorCodeValue::Success;
}

ErrorCode ProcessActivationContext::ResumeProcess()
{
    return Common::ErrorCodeValue::Success;
}
