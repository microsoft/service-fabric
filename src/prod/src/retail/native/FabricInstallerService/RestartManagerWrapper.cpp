// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace FabricInstallerService;

const StringLiteral TraceType("RestartManagerWrapper");

const wstring SvcHostImageName = L"svchost.exe";

RestartManagerWrapper::RestartManagerWrapper()
    : traceId_()
    , sessionHandle_(INVALID_RM_SESSION_HANDLE)
    , isRestartable_(false)
{
}

RestartManagerWrapper::~RestartManagerWrapper()
{
#if !DotNetCoreClrIOT
    if (this->IsSessionValid)
    {
        DWORD dwError = RmEndSession(sessionHandle_);
        if (dwError != ERROR_SUCCESS)
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "Error {0} while ending the restart manager session",
                dwError);
        }

        sessionHandle_ = INVALID_RM_SESSION_HANDLE;
    }
#endif
}

void RestartManagerWrapper::EnumerateAndRegisterFiles(vector<wstring> const & dirs)
{
    ASSERT_IF(this->IsSessionValid, "Cannot register files to a opened restart manager session");
    for (wstring dir : dirs)
    {
        vector<wstring> files = Directory::GetFiles(dir, L"*", true, false);
        registeredFiles_.insert(registeredFiles_.begin(), files.begin(), files.end());
    }
}

ErrorCode RestartManagerWrapper::ShutdownProcesses()
{
#if !DotNetCoreClrIOT
    if (!this->IsSessionValid)
    {
        auto error = CreateNewSessionAndRegisterFiles();
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "CreateNewSessionAndRegister failed with error {0}",
                error);
            return error;
        }
    }
    
    ULONG rebootReason = 0;
    vector<RM_PROCESS_INFO> procInfos;
    bool succeeded = false;
    DWORD dwError = 0;
    // Getting the size just once then getting the data is insufficient since the number of procInfos could change between calls.  
    for (int attempt = 0; attempt < 5; ++attempt)
    {
        unsigned int procInfosNeeded = 0;
        unsigned int procInfoCount = static_cast<unsigned int>(procInfos.size());
        dwError = ::RmGetList(sessionHandle_, &procInfosNeeded, &procInfoCount, procInfos.data(), &rebootReason);
        procInfos.resize(procInfosNeeded);
        if (dwError == ERROR_SUCCESS)
        {
            succeeded = true;
            break;
        }

        if (dwError != ERROR_MORE_DATA)
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "RmGetList failed with error {0}",
                dwError);
            return ErrorCode::FromWin32Error(dwError);
        }
    }

    if (!succeeded)
    {
        WriteWarning(
            TraceType,
            this->traceId_,
            "Retry count exhausted to get the process list from restart manager");
        return ErrorCodeValue::InvalidOperation;
    }

    for (auto& procInfo : procInfos)
    {
        WriteInfo(
            TraceType,
            this->traceId_,
            "Registered process for shutdown pID:'{0}', Name:'{1}'", procInfo.Process.dwProcessId, procInfo.strAppName);
    }
    
    if (procInfos.size() > 0)
    {
        dwError = ::RmShutdown(sessionHandle_, RM_SHUTDOWN_TYPE::RmForceShutdown, ProcessShutdownProgressCallback);
        if (dwError != ERROR_SUCCESS)
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "Shutting down processes using restart manager returned error {0}", dwError);
            for(auto& procInfo : procInfos)
            {
                DWORD pid = procInfo.Process.dwProcessId;
                HANDLE hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
                if (hHandle == INVALID_HANDLE_VALUE)
                {
                    continue;
                }

                WriteInfo(
                    TraceType,
                    this->traceId_,
                    "Process still up ID: '{0}', Name:'{1}'", pid, procInfo.strAppName);
                resilientProcesses_.push_back(move(procInfo));

                if (!::CloseHandle(hHandle))
                {
                    WriteWarning(
                        TraceType,
                        this->traceId_,
                        "Process up CloseHandle failed for pid '{0}'", pid);
                }
            }
        }
        else
        {
            isRestartable_ = true;
        }
    }
    
    return ErrorCode::FromWin32Error(dwError);
#else
    return ErrorCodeValue::Success;
#endif
}

ErrorCode RestartManagerWrapper::RestartProcesses()
{
#if !DotNetCoreClrIOT
    if (isRestartable_)
    {
        ASSERT_IFNOT(this->IsSessionValid, "RestartProcesses cannot be called on a session that is not opened");
        WriteInfo(
            TraceType,
            this->traceId_,
            "Restarting processes that were closed by Restart Manager session {0} key {1}.",
            sessionHandle_,
            szSessionKey);
        DWORD dwError = ::RmRestart(sessionHandle_, 0, ProcessRestartProgressCallback);
        if (dwError != ERROR_SUCCESS)
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "Restarting processes failed with error {0}",
                dwError);
            return ErrorCode::FromWin32Error(dwError);
        }
    }
#endif
    return ErrorCodeValue::Success;
}

ErrorCode RestartManagerWrapper::ForceCloseNonCriticalProcesses()
{
    ErrorCode errorCode(ErrorCodeValue::Success);
    DWORD dwExitCode = 0;
    HANDLE hHandle;
    DWORD pid;

    const size_t svchostImageNameLen = SvcHostImageName.size();

    // Find svchost processes
    std::set<DWORD> pidSet;
    std::set<DWORD> svcHostPidSet;
    for (auto& procInfo : resilientProcesses_)
    {
        pid = procInfo.Process.dwProcessId;
        if (pidSet.find(pid) == pidSet.end()
            && procInfo.ApplicationType == RM_APP_TYPE::RmService)
        {
            pidSet.insert(pid);

            HANDLE hProcess = ::OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
            if (hProcess == INVALID_HANDLE_VALUE)
            {
                WriteInfo(
                    TraceType,
                    this->traceId_,
                    "Trying to get process handle failed for pid: '{0}'", pid);
                pidSet.erase(pid);
                continue;
            }

            wstring imageFilename;
            imageFilename.resize(MAX_PATH);
            size_t ifLength = (size_t)GetProcessImageFileName(hProcess, &imageFilename[0], (DWORD)imageFilename.size());
            if (ifLength == 0)
            {
                DWORD err = GetLastError();
                WriteWarning(
                    TraceType,
                    this->traceId_,
                    "GetProcessImageFileName failed for pid '{0}' with error {1}. Assuming process is not svchost.", pid, err);
            }
            else
            {
                if (ifLength >= svchostImageNameLen)
                {
                    std::transform(imageFilename.begin(), imageFilename.end(), imageFilename.begin(), ::towlower);

                    if (imageFilename.compare(ifLength - svchostImageNameLen, svchostImageNameLen, SvcHostImageName) == 0)
                    {
                        svcHostPidSet.insert(pid); // Process is svchost
                    }
                }
            }

            if (!::CloseHandle(hProcess))
            {
                DWORD err = GetLastError();
                WriteWarning(
                    TraceType,
                    this->traceId_,
                    "Process up CloseHandle failed for pid '{0}' with error {1}.", pid, err);
            }
        }
    }

    for (auto& procInfo : resilientProcesses_)
    {
        pid = procInfo.Process.dwProcessId;

        if (svcHostPidSet.find(pid) != svcHostPidSet.end())
        {
            WriteInfo(
                TraceType,
                this->traceId_,
                "Skip closing critical process: '{0}', '{1}'",
                pid,
                procInfo.strAppName);
            continue;
        }

        WriteInfo(
            TraceType,
            this->traceId_,
            "Force closing process: '{0}', '{1}'",
            pid,
            procInfo.strAppName);

        hHandle = ::OpenProcess(PROCESS_ALL_ACCESS, 0, pid);
        if (hHandle == INVALID_HANDLE_VALUE)
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "Could not get handle for terminating process: '{0}', '{1}'",
                pid,
                procInfo.strAppName);
            continue;
        }

        if (!::TerminateProcess(hHandle, dwExitCode))
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "TerminateProcess did not kill: '{0}', '{1}'",
                pid,
                procInfo.strAppName);
        }
        ::CloseHandle(hHandle);

        errorCode = ErrorCode::FromWin32Error(dwExitCode);
        if (!errorCode.IsSuccess())
        {
            WriteWarning(
                TraceType,
                this->traceId_,
                "Force closing process failed: '{0}', '{1}'. Error: {2}",
                pid,
                procInfo.strAppName,
                errorCode);
            break;
        }
    }

    return errorCode;
}

ErrorCode RestartManagerWrapper::CreateNewSessionAndRegisterFiles()
{
#if !DotNetCoreClrIOT
    DWORD dwError = ::RmStartSession(&sessionHandle_, 0, szSessionKey);
    if (dwError != ERROR_SUCCESS)
    {
        WriteWarning(
            TraceType,
            this->traceId_,
            "Opening a restart manager session failed with error {0}",
            dwError);
        return ErrorCode::FromWin32Error(dwError);
    }

    WriteInfo(
        TraceType,
        this->traceId_,
        "Restart Manager session opened with session handle {0} and key {1}",
        sessionHandle_, szSessionKey);

    vector<const wchar_t*> files;
    for (auto& file : registeredFiles_)
    {
        files.push_back(file.c_str());
    }

    dwError = ::RmRegisterResources(sessionHandle_, static_cast<UINT>(files.size()), files.data(), 0, NULL, 0, NULL);
    if (dwError != ERROR_SUCCESS)
    {
        WriteWarning(
            TraceType,
            this->traceId_,
            "Registering resources with restart manager failed with error {0}",
            dwError);
        return ErrorCode::FromWin32Error(dwError);
    }
#endif
    return ErrorCodeValue::Success;
}

void RestartManagerWrapper::ProcessShutdownProgressCallback(UINT progress)
{
    WriteInfo(
        TraceType,
        L"ProcessShutdownProgressCallback",
        "Progress for shutting down processes with restart manager is '{0}%'",
        progress);
}

void RestartManagerWrapper::ProcessRestartProgressCallback(UINT progress)
{
    WriteInfo(
        TraceType,
        L"ProcessRestartProgressCallback",
        "Progress for restarting processes which were previously shutdown by restart manager is '{0}%'",
        progress);
}
