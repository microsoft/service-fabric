// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral TraceType_ProcessUtility = "ProcessUtility";

DWORD ProcessUtility::CreationFlags_SuspendedProcessWithJobBreakaway =
    CREATE_NEW_CONSOLE | 
    CREATE_UNICODE_ENVIRONMENT |
    CREATE_NEW_PROCESS_GROUP |
    CREATE_SUSPENDED |
    CREATE_BREAKAWAY_FROM_JOB;

DWORD ProcessUtility::CreationFlags_SuspendedProcessWithJobBreakawayNoWindow =
    CREATE_NO_WINDOW | 
    CREATE_UNICODE_ENVIRONMENT |
    CREATE_NEW_PROCESS_GROUP |
    CREATE_SUSPENDED |
    CREATE_BREAKAWAY_FROM_JOB;


ErrorCode ProcessUtility::OpenProcess(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwProcessId,
    __out HandleUPtr & processHandle)
{
    DWORD osErrorCode = 0;
    return OpenProcess(
        dwDesiredAccess,
        bInheritHandle,
        dwProcessId,
        processHandle,
        osErrorCode);
}

ErrorCode ProcessUtility::OpenProcess(
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    DWORD dwProcessId,
    __out Common::HandleUPtr & processHandle,
    __out DWORD & osErrorCode)
{
    HANDLE result = ::OpenProcess(dwDesiredAccess, bInheritHandle, dwProcessId);

    if (result == NULL)
    {
        osErrorCode = ::GetLastError();
        WriteWarning(
            TraceType_ProcessUtility,
            "OpenProcess failed with {0}: DesiredAccess:{1}, InheritHandle={2}, ProcessId={3}",
            osErrorCode,
            dwDesiredAccess,
            bInheritHandle,
            dwProcessId);
        return ErrorCode(ErrorCodeValue::OperationFailed);
    }

    try
    {
        processHandle = make_unique<Handle>(result);
    }
    catch (...)
    {
        ::CloseHandle(result);
        throw;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

pid_t ProcessUtility::GetProcessId()
{
#ifdef PLATFORM_UNIX
    return getpid();
#else
    return GetCurrentProcessId();
#endif
}

ErrorCode ProcessUtility::CreateProcess(
    wstring const & commandLine,
    wstring const & workingDirectory,
    vector<wchar_t> & environmentBlock,
    DWORD dwCreationFlags,
    __out HandleUPtr & processHandle,
    __out HandleUPtr & threadHandle)
{
    pid_t processId = 0;
    return CreateProcess(
        commandLine,
        workingDirectory,
        environmentBlock,
        dwCreationFlags,
        processHandle,
        threadHandle,
        processId);
};

ErrorCode ProcessUtility::CreateProcess(
    wstring const & commandLine,
    wstring const & workingDirectory,
    vector<wchar_t> & environmentBlock,
    DWORD dwCreationFlags,
    __out HandleUPtr & processHandle,
    __out HandleUPtr & threadHandle,
    pid_t & processId)
{
    processId = 0;
    auto retval = ErrorCode(ErrorCodeValue::OperationFailed);

    // prepare parameters for CreateProcess
    LPCTSTR lpApplicationName = NULL;
    LPTSTR lpCommandLine = const_cast<LPTSTR>(commandLine.c_str());
    LPSECURITY_ATTRIBUTES lpProcessAttributes = NULL;
    LPSECURITY_ATTRIBUTES lpThreadAttributes = NULL;
    BOOL bInheritHandles = false;

    LPVOID lpEnvironment = NULL;
    if (environmentBlock.size() != 0)
    {
        lpEnvironment = &(environmentBlock.front());
    }

    LPCTSTR lpCurrentDirectory = NULL;
    if (workingDirectory.length() != 0)
    {
        lpCurrentDirectory = workingDirectory.c_str();
    }

    STARTUPINFO         startupInfo;
    ::ZeroMemory(&startupInfo, sizeof(startupInfo));
    startupInfo.cb = sizeof(startupInfo);

    PROCESS_INFORMATION processInfo;
    ::ZeroMemory(&processInfo, sizeof(processInfo));

    processHandle.reset();
    threadHandle.reset();

#if defined(PLATFORM_UNIX)
    wchar_t* env = environmentBlock.data();
    vector<char> ansiEnvironment;
    while (*env)
    {
        string enva;
        wstring envw(env);
        StringUtility::Utf16ToUtf8(envw, enva);
        ansiEnvironment.insert(ansiEnvironment.end(), enva.begin(), enva.end());
        ansiEnvironment.push_back(0);
        env += envw.length() + 1;
    }

#endif

    BOOL success = ::CreateProcessW(
        lpApplicationName,
        lpCommandLine,
        lpProcessAttributes,
        lpThreadAttributes,
        bInheritHandles,
#if defined(PLATFORM_UNIX)
        0,
        ansiEnvironment.data(),
#else
        dwCreationFlags,
        lpEnvironment,
#endif  
        lpCurrentDirectory,
        &startupInfo,
        &processInfo);

    try
    {
        if (success)
        {
            processHandle = make_unique<Handle>(processInfo.hProcess);
            threadHandle = make_unique<Handle>(processInfo.hThread);

            WriteInfo(
                TraceType_ProcessUtility,
                "CreateProcess Successful for CommandLine:{0}. ProcessId:{1} MainThreadId:{2} ProcessHandle:{3}",
                commandLine,
                processInfo.dwProcessId,
#if defined(PLATFORM_UNIX)
                processInfo.dwThreadId,//_PAL_Undefined,
#else
                processInfo.dwThreadId,
#endif
                processInfo.hProcess);

            retval = ErrorCode(ErrorCodeValue::Success);
            processId = processInfo.dwProcessId;
        }
        else
        {
            DWORD error = ::GetLastError();
            WriteError(
                TraceType_ProcessUtility,
                "CreateProcess failed with {0}: CommandLine:{1}",
                error, 
                commandLine);
        }
    }
    catch(...)
    {
        CleanupProcess(processHandle, &processInfo);
        CleanupCreateProcessHandles(processHandle, threadHandle, &processInfo);
        throw;
    }

    return retval;
}

#if !defined(PLATFORM_UNIX)
ErrorCode ProcessUtility::ExecuteCommandLine(wstring const & commandLine, DWORD timeoutInMilliseconds)
{
    HandleUPtr procHandle;
    HandleUPtr threadHandle;
    vector<wchar_t> envBlock(0);

    auto error = ProcessUtility::CreateProcessW(
        commandLine,
        Directory::GetCurrentDirectoryW(),
        envBlock,
        CREATE_UNICODE_ENVIRONMENT | CREATE_NO_WINDOW,
        procHandle,
        threadHandle);

    if (!error.IsSuccess())
    {
        WriteWarning(
            TraceType_ProcessUtility,
            "ExecuteCommandLine failed with {0} executing the command \"{1}\"",
            error,
            commandLine);

        return error;
    }

    ::WaitForSingleObject(procHandle->Value, timeoutInMilliseconds);
    DWORD exitCode;
    ::GetExitCodeProcess(procHandle->Value, &exitCode);

    return ErrorCode::FromHResult(exitCode);
}
#endif

#if defined(PLATFORM_UNIX)

ErrorCode ProcessUtility::GetStatusOutput(string const & cmdline, string & output)
{
    vector<string> lines;
    auto error = GetStatusOutput(cmdline, lines);
    StringWriterA sw(output);
    for(auto const & line : lines)
    {
        sw.WriteLine(line);
    }

    return error;
}

ErrorCode ProcessUtility::GetStatusOutput(string const & cmdline, vector<string> & outputLines)
{
    outputLines.clear();

    auto file = popen(cmdline.c_str(), "re");
    if (!file) return ErrorCode::FromErrno();

    char* line = nullptr;
    KFinally([=] { if(line) free(line); });

    size_t len = 0;
    while(getline(&line, &len, file) != -1)
    {
        for (int i = len - 1; i >= 0; --i) //newline may not be at the end
        {
            if (line[i] == '\n')
            {
                line[i] = 0; //erase newline
                break;
            }
        }
        outputLines.emplace_back(line);
    }

    auto status = pclose(file);
    return ErrorCode(ErrorCodeValue::Enum(status));
}

#else
ErrorCode ProcessUtility::CreateAnnonymousJob(
    bool allowBreakaway,
    __out HandleUPtr & jobHandle)
{
    auto retval = ErrorCode(ErrorCodeValue::OperationFailed);

    jobHandle.reset();

    // create job object
    HANDLE handle = NULL;
    try
    {
        handle = ::CreateJobObjectW(NULL, NULL);
        if (!handle)
        {
            DWORD error = ::GetLastError();
            WriteError(
                TraceType_ProcessUtility,
                "CreateJobObject failed with {0}.",
                error);
        }
        else
        {
            jobHandle = make_unique<Handle>(handle);
        }
    }
    catch(...)
    {
        if (handle)
        {
            ::CloseHandle(handle);
        }
        throw;
    }

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION jobInfo;
    ::ZeroMemory(&jobInfo, sizeof(jobInfo));

    if (allowBreakaway)
    {
        jobInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    }
    jobInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;


    // set required limits on the job object
    if (!::SetInformationJobObject(
        jobHandle->Value,
        ::JobObjectExtendedLimitInformation,
        &jobInfo,
        sizeof(jobInfo)))
    {
        DWORD error = ::GetLastError();
        WriteError(
            TraceType_ProcessUtility,
            "SetInformationJobObject failed with {0}: JobHandle={1}",
            error,
            jobHandle->Value);

        jobHandle.reset();
    }
    else
    {
        retval = ErrorCode(ErrorCodeValue::Success);
    }

    return retval;
}

ErrorCode ProcessUtility::ResumeProcess(
    HandleUPtr const & processHandle,
    HandleUPtr const & threadHandle)
{
    auto prevSuspendCount = ::ResumeThread(threadHandle->Value);

    // > 1 indicates that suspended count did not get decremented to 0 after this call. Process still suspended.
    // == -1 indicates that the API failed.
    if (prevSuspendCount > 1 || prevSuspendCount == -1)
    {
        DWORD error = ::GetLastError();
        WriteError(
            TraceType_ProcessUtility,
            "ResumeProcess failed with {0}: ProcessHandle:{1} ThreadHandle:{2} PreviousSuspendCount:{3}",
            error,
            processHandle->Value,
            threadHandle->Value,
            prevSuspendCount);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}
#endif

void ProcessUtility::TerminateProcess(HandleUPtr const & processHandle)
{
    if ((processHandle) && (processHandle->Value))
    {
        TerminateProcess(processHandle->Value);
    }
}

void ProcessUtility::TerminateProcess(HANDLE processHandle)
{
    if (!::TerminateProcess(processHandle, ErrorCodeValue::ProcessDeactivated & 0x0000FFFF))
    {
        WriteNoise(
            TraceType_ProcessUtility,
            "TerminateProcess failed: ProcessHandle:{0}",
            processHandle);
    }
}

void ProcessUtility::CleanupProcess(
    HandleUPtr const & processHandle,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    if (processHandle)
    {
        TerminateProcess(processHandle->Value);
    }
    else
    {
        if (lpProcessInformation->hProcess)
        {
            TerminateProcess(lpProcessInformation->hProcess);
        }
    }
}

#if !defined(PLATFORM_UNIX)
ErrorCode ProcessUtility::AssociateProcessToJob(
    HandleUPtr const & jobHandle,
    HandleUPtr const & processHandle)
{
    if (!::AssignProcessToJobObject(jobHandle->Value, processHandle->Value))
    {
        DWORD error = ::GetLastError();
        WriteError(
            TraceType_ProcessUtility,
            "AssignProcessToJobObject failed with {0}: JobHandle:{1} ProcessHandle:{2}",
            error,
            jobHandle->Value,
            processHandle->Value);

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}
#endif

void ProcessUtility::CleanupCreateProcessHandles(
    HandleUPtr & processHandle,
    HandleUPtr & threadHandle,
    LPPROCESS_INFORMATION lpProcessInformation)
{
    if (processHandle) 
    {
        processHandle.reset();
    }
    else
    {
        if (lpProcessInformation && lpProcessInformation->hProcess)
        {
            ::CloseHandle(lpProcessInformation->hProcess);
        }
    }

    if (threadHandle)
    {
        threadHandle.reset();
    }
    else
    {
        if (lpProcessInformation && lpProcessInformation->hThread)
        {
            ::CloseHandle(lpProcessInformation->hThread);
        }
    }
}

#if !defined(PLATFORM_UNIX)
ErrorCode ProcessUtility::GetProcessExitCode(
    Handle const & processHandle,
    __out DWORD & exitCode)
{
    if (!::GetExitCodeProcess(processHandle.Value, &exitCode))
    {
        DWORD error = ::GetLastError();

        /*
            Getting exit code of the process should not fail except
            in scenarios where the parent is going down due to assert
            (Example: Fabric.exe is crashing at which point GetExitCode of
             the host processes return 5 (AccessDenied))

            Special case that to not trace at error to avoid drowning out
            the actual error trace in noise
        */

        if (error == ERROR_ACCESS_DENIED)
        {
            WriteWarning(
                TraceType_ProcessUtility,
                "GetExitCodeProcess failed with {0}: ProcessHandle:{1}",
                error,
                processHandle.Value);
        }
        else
        {
            WriteError(
                TraceType_ProcessUtility,
                "GetExitCodeProcess failed with {0}: ProcessHandle:{1}",
                error,
                processHandle.Value);            
        }

        return ErrorCode(ErrorCodeValue::OperationFailed);
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}
#endif

ErrorCode ProcessUtility::ParseCommandLine(wstring const & commandLine, wstring & exePath, wstring & arguments)
{
    if (commandLine.empty()) 
    { 
        WriteWarning(
            TraceType_ProcessUtility,
            "ParseCommandLine: Invalid CommandLine {0}",
            commandLine);
        return ErrorCode::FromHResult(E_INVALIDARG); 
    }

    arguments = L"";
    if (commandLine[0] == L'"')
    {
        // find next quote
        size_t i = commandLine.find_first_of(L'"', 1);
        if (i == string::npos)
        {
             WriteWarning(
                TraceType_ProcessUtility,
                "ParseCommandLine: Invalid CommandLine {0}",
                commandLine);
            return ErrorCode::FromHResult(E_INVALIDARG);
        }

        exePath = commandLine.substr(1, i - 1);
        if (commandLine.length() > (i+1))
        {
            arguments = commandLine.substr(i+1);
            StringUtility::TrimWhitespaces(arguments);
        }
    }
    else
    {
        size_t i = commandLine.find_first_of(L' ', 1);
        if (i == string::npos)
        {
            exePath = commandLine;
        }
        else
        {
            exePath = commandLine.substr(0, i);
            if (commandLine.length() > (i+1))
            {
                arguments = commandLine.substr(i+1);
                StringUtility::TrimWhitespaces(arguments);
            }
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}

wstring ProcessUtility::GetCommandLine(wstring const & exePath, wstring const & arguments)
{
    wstring commandLine = L"";

    if (arguments.length() == 0)
    {
        if (!StringUtility::StartsWith<wstring>(exePath, L"\""))
        {
            commandLine.append(L"\"");
        }
        commandLine.append(exePath);

        if (!StringUtility::EndsWith<wstring>(exePath, L"\""))
        {
            commandLine.append(L"\"");
        }

    }
    else
    {
        if (!StringUtility::StartsWith<wstring>(exePath, L"\""))
        {
            commandLine.append(L"\"");
        }
        commandLine.append(exePath);

        if (!StringUtility::EndsWith<wstring>(exePath, L"\""))
        {
            commandLine.append(L"\"");
        }
        commandLine.append(L" ");
        commandLine.append(arguments);
    }

    return commandLine;
}


#if !defined(PLATFORM_UNIX)
// enables synchronize access on the current process to all the users
ErrorCode ProcessUtility::EnableSynchronizeAccess()
{
    ProcessHandle thisProcess(::GetCurrentProcess(), false);
    return ProcessUtility::EnableSynchronizeAccess(thisProcess, WinWorldSid);
}

// enables synchronize access on the specified process to the specified well known user
ErrorCode ProcessUtility::EnableSynchronizeAccess(ProcessHandle const & processHandle, WELL_KNOWN_SID_TYPE wellKnownSidType)
{
    WriteNoise(
        TraceType_ProcessUtility,
        "Enabling SYNCHRONIZE access to everyone for ProcessHandle={0}", 
        processHandle.Value); 

    SidSPtr sid;
    auto error = BufferedSid::CreateSPtr(wellKnownSidType, sid);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ProcessUtility,
        "CreateSid: ErrorCode={0}", 
        error);
    if (!error.IsSuccess()) { return error; }

    error = SecurityUtility::UpdateProcessAcl(processHandle, sid, SYNCHRONIZE);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ProcessUtility,
        "UpdateProcessAcl: ErrorCode={0}",
        error); 
    return error;
}
#endif


#if !defined(PLATFORM_UNIX)
ErrorCode ProcessUtility::GetProcessSidString(__out wstring & stringSid)
{
    ProcessHandle thisProcess(::GetCurrentProcess(), false);
    return ProcessUtility::GetProcessSidString(thisProcess, stringSid);
}

ErrorCode ProcessUtility::GetProcessSidString(ProcessHandle const & processHandle, __out wstring & stringSid)
{
    WriteNoise(
        TraceType_ProcessUtility,
        "GetProcessSidString: ProcessHandle={0}", 
        processHandle.Value);

    AccessTokenSPtr accessToken;
    auto error = AccessToken::CreateProcessToken(processHandle.Value, TOKEN_QUERY, accessToken);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ProcessUtility,
        "CreateProcessToken: ErrorCode={0}", 
        error);
    if (!error.IsSuccess()) { return error; }

    SidSPtr sid;
    error = accessToken->GetUserSid(sid);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ProcessUtility,
        "AccessToken::GetUserSid: ErrorCode={0}", 
        error);
    if (!error.IsSuccess()) { return error; }

    error = sid->ToString(stringSid);
    WriteTrace(
        error.ToLogLevel(),
        TraceType_ProcessUtility,
        "Sid::ToString: ErrorCode={0}", 
        error);
    if (!error.IsSuccess()) { return error; }

    WriteNoise(
        TraceType_ProcessUtility,
        "GetProcessSidString: stringSid={0}", 
        stringSid);
    return ErrorCode(ErrorCodeValue::Success);
}
#endif

ErrorCode ProcessUtility::CreateDefaultEnvironmentBlock(__out vector<wchar_t> & envBlock)
{
    EnvironmentMap envMap;
    if (!Environment::GetEnvironmentMap(envMap)) { return ErrorCode(ErrorCodeValue::OperationFailed); }

    wstring configStoreEnvVarName;
    wstring configStoreEnvVarValue;

    auto error = ComProxyConfigStore::FabricGetConfigStoreEnvironmentVariable(configStoreEnvVarName, configStoreEnvVarValue);
    if (!error.IsSuccess()) { return error; }
    if (!configStoreEnvVarName.empty())
    {
        envMap[configStoreEnvVarName] = configStoreEnvVarValue;
    }

    Environment::ToEnvironmentBlock(envMap, envBlock);
    return ErrorCode(ErrorCodeValue::Success);
}

#if !defined(PLATFORM_UNIX)
_Use_decl_annotations_
ErrorCode ProcessUtility::GetProcessImageName(DWORD processId, wstring & imageName)
{
    imageName.clear();

    HandleUPtr process;
    ErrorCode error = ProcessUtility::OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, processId, process);
    if (!error.IsSuccess())
    {
        return error;
    }

    vector<WCHAR> buffer(1024);
    for (int i = 0; i < 3; ++ i)
    {
        if (::GetProcessImageFileName(process->Value, buffer.data(), (DWORD)buffer.size()) > 0)
        {
            imageName = buffer.data();
            return ErrorCode();
        }

        DWORD lastError = ::GetLastError();
        if (lastError != ERROR_INSUFFICIENT_BUFFER)
        {
            return ErrorCode::FromWin32Error(lastError);
        }

        buffer.resize(buffer.size() * 2);
    }

    return ErrorCodeValue::OperationFailed;
}
#endif
