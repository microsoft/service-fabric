// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Hosting2;
using namespace ServiceModel;

StringLiteral const TraceType_Activator("ProcessActivator");

// The HRESULT values of the fabric error codes are of 0x8007XXXX format. The win32 error values of these
// error codes can be got by dropping the 0x8007 part of the HRESULT.
UINT ProcessActivator::ProcessDeactivateExitCode = ErrorCodeValue::ProcessDeactivated & 0x0000FFFF;
UINT ProcessActivator::ProcessAbortExitCode = ErrorCodeValue::ProcessAborted & 0x0000FFFF;

// ********************************************************************************************************************
// ProcessActivator::ActivateAsyncOperation Implementation
//
class ProcessActivator::ActivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(ActivateAsyncOperation)

public:
    ActivateAsyncOperation(
        __in ProcessActivator & owner,
        SecurityUserSPtr const & runAs,
        ProcessDescription const & processDescription,
        wstring const & fabricbinPath,
        ProcessWait::WaitCallback const & processExitCallback,
        bool enableWinfabAdminSynchronize,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),
        timeoutHelper_(timeout),
        runAs_(runAs),
        processDescription_(processDescription),
        processExitCallback_(processExitCallback),
        envBlock_(),
        commandLine_(),
        userToken_(),
        workstationHandle_(),
        desktopHandle_(),
        desktopName_(),
        hasConsole_(false),
        fabricBinFolder_(fabricbinPath),
        debuggerProcId_(0),
        enableWinfabAdminSynchronize_(enableWinfabAdminSynchronize)
    {
    }

    virtual ~ActivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation, 
        __out IProcessActivationContextSPtr & activationContext)
    {
        auto thisPtr = AsyncOperation::End<ActivateAsyncOperation>(operation);
        if (thisPtr->Error.IsSuccess())
        {
            ProcessActivationContextSPtr winActivationContext;

            ProcessActivationContext::CreateProcessActivationContext(
                thisPtr->processMonitor_,
                move(thisPtr->processHandle_),
                move(thisPtr->jobHandle_),
                move(thisPtr->threadHandle_),                
                move(thisPtr->workstationHandle_),
                move(thisPtr->desktopHandle_),
                L"", // container id
                thisPtr->processId_,
                move(thisPtr->stdoutRedirector_),
                move(thisPtr->stderrRedirector_),
                thisPtr->hasConsole_,
                thisPtr->debuggerProcId_,
                move(thisPtr->debuggerProcessHandle_),
                move(thisPtr->debuggerThreadHandle_),
                thisPtr->debugprocessMonitor_,
                winActivationContext);

            activationContext = move(winActivationContext);
        }
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error(ErrorCodeValue::Success);

        if (this->runAs_)
        {
            error = CreateSandboxedProcess(thisSPtr);
        }
        else
        {
            error = CreateNonSandboxedProcess(thisSPtr);
        }
        if (error.IsSuccess() &&
            enableWinfabAdminSynchronize_)
        {
            SidSPtr winfabAdminSid;
            error = BufferedSid::CreateSPtr(FabricConstants::WindowsFabricAdministratorsGroupName, winfabAdminSid);
            if (!error.IsSuccess())
            {
                TryComplete(thisSPtr, error);
                return;
            }
            
            error = SecurityUtility::UpdateProcessAcl(
                processHandle_->Value,
                winfabAdminSid,
                SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION);
        }
        TryComplete(thisSPtr, error);
    }

private:
    ErrorCode CreateNonSandboxedProcess(AsyncOperationSPtr const & thisSPtr)
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Creating NonSandboxedProcess. ProcessDescription={0}", 
            this->processDescription_);
        ErrorCode error(ErrorCodeValue::Success);

        CreateCommandLine();
        
        if (HostingConfig::GetConfig().EnableProcessDebugging &&
            !processDescription_.DebugParameters.EnvironmentBlock.empty())
        {
            error = CreateEnvironmentBlock();
            if (!error.IsSuccess())
            {
                return error;
            }
        }
        else
        {
            envBlock_ = processDescription_.EnvironmentBlock;
        }

        if(processDescription_.NotAttachedToJob)
        {
            WriteNoise(
                TraceType_Activator,
                owner_.Root.TraceId,
                "No job created for process. ProcessDescription={0}", 
                this->processDescription_);
        }
        else
        {
            // create job 
            error = CreateJobObject(
                processDescription_.JobObjectName,
                processDescription_.IsHostedServiceProcess);
            if (!error.IsSuccess()) { return error; }
        }

        if (processDescription_.RedirectConsole)
        {
            AcquireExclusiveLock lock(owner_.consoleRedirectionLock_);
            error = this->CreateProcess();
            this->stdoutHandle_.reset();
            this->stderrHandle_.reset();
            if (!error.IsSuccess())
            {
                return error;
            }
        }
        else
        {
            error = this->CreateProcess();
            if (!error.IsSuccess()) { return error; }
        }
        if(!processDescription_.NotAttachedToJob)
        {
            error = this->AssignJobToProcess(processHandle_);
            if (!error.IsSuccess()) { return error; }
        }
        error = this->MonitorProcess(processHandle_);
        if (!error.IsSuccess()) { return error; }

        if (HostingConfig::GetConfig().EnableProcessDebugging &&
            !this->processDescription_.DebugParameters.ExePath.empty())
        {
            error = SetupDebugger(thisSPtr);
        }
        else
        {
            error = this->ResumeProcess(threadHandle_);
        }
        return error; 
    }

private:
    ErrorCode CreateSandboxedProcess(AsyncOperationSPtr const & thisSPtr)
    {
#if DBG
        bool interactiveMode = HostingConfig::GetConfig().InteractiveRunAsEnabled;
        if(interactiveMode)
        {
            bool isSystem = false;
            auto error = owner_.IsSystemAccount(isSystem);
            if(!error.IsSuccess())
            {
                return error;
            }
            //CreateProcessWithLogonW can only be called from processes not running as system
            interactiveMode = !isSystem && 
                (runAs_->AccountType == SecurityPrincipalAccountType::LocalUser ||
                runAs_->AccountType == SecurityPrincipalAccountType::DomainUser);
        }
#else
        bool interactiveMode = false;
#endif

        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Creating Sandboxed Process: ProcessDescription={0}, User={1}, InteractiveMode={2}.", 
            this->processDescription_, 
            this->runAs_->AccountName,
            interactiveMode);

        CreateCommandLine();

        auto error = this->CreateUserTokens();
        if (!error.IsSuccess()) { return error ; }

        error = this->CreateRestrictedEnvironmentBlock();
        if (!error.IsSuccess()) { return error; }

        error = owner_.EnsureSeDebugPrivilege();
        if (!error.IsSuccess()) { return error; }

        if (interactiveMode)
        {
            error = this->CreateProcessWithLogon();
            if (!error.IsSuccess()) { return error; }
        }
        else
        {
            error = owner_.EnsureAssignPrimaryTokenPrivilege();
            if (!error.IsSuccess()) { return error; }

#if !DotNetCoreClrIOT
            error = this->AddDesktopRestrictions();
            if (!error.IsSuccess()) { return error; }
#endif

            if(processDescription_.NotAttachedToJob)
            {
                WriteNoise(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "Job object not created for ProcessDescription={0}.", 
                    this->processDescription_);   
            }
            else
            {
        error = this->CreateJobObject(
            processDescription_.JobObjectName,
                    processDescription_.IsHostedServiceProcess);

                if (!error.IsSuccess()) { return error; }
            }

            if (processDescription_.RedirectConsole)
            {
                AcquireExclusiveLock lock(owner_.consoleRedirectionLock_);
                error = this->CreateProcessAsUser();
                this->stdoutHandle_.reset();
                this->stderrHandle_.reset();
                if (!error.IsSuccess())
                {
                    return error;
                }
            }
            else
            {
                error = this->CreateProcessAsUser();
            }
            if (!error.IsSuccess()) { return error; }

            if(!processDescription_.NotAttachedToJob)
            {
                error = this->AssignJobToProcess(processHandle_);
                if (!error.IsSuccess()) { return error; }
            }
        }

        error = this->MonitorProcess(processHandle_);
        if (!error.IsSuccess()) { return error; }

        //Setup debugger only if this setting is enabled in cluster manifest.
        if (HostingConfig::GetConfig().EnableProcessDebugging &&
            !this->processDescription_.DebugParameters.ExePath.empty())
        {
            error = SetupDebugger(thisSPtr);
        }
        else
        {
            error = this->ResumeProcess(threadHandle_);
        }
        return error;
    }

    ErrorCode CreateUserTokens()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Creating user tokens.");
        return runAs_->CreateLogonToken(userToken_);
    }

    ErrorCode AddDesktopRestrictions()
    {
        {
            AcquireExclusiveLock lock(owner_.desktopRestricationLock_);
            WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Adding desktop restrications.");

            if (!::ImpersonateLoggedOnUser(userToken_->TokenHandle->Value))
            {
                auto error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "ImpersonateLoggedOnUser failed. ErrorCode={0}",
                    error);
                return error;
            }
            // create impersonation scope, so that we revert back to ourself when we come out it
            ResourceHolder<PVOID> impersonationScope(
                NULL,
                [this](ResourceHolder<PVOID> *)
            { 
                if (!::RevertToSelf())
                {
                    auto error = ErrorCode::FromWin32Error();
                    WriteError(
                        TraceType_Activator,
                        owner_.Root.TraceId,
                        "Failed to revert an impersonation scope. ErrorCode={0}.",
                        error);
                    Assert::CodingError("This is a failfast, because we failed to revert impersonated user scope.");
                }
            });

            // create new workstation
            DWORD winstaAccess = 
                STANDARD_RIGHTS_REQUIRED | WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS |
                WINSTA_CREATEDESKTOP | WINSTA_ENUMDESKTOPS | WINSTA_EXITWINDOWS |
                WINSTA_READATTRIBUTES | WINSTA_WRITEATTRIBUTES;

            DWORD desktopAll = 
                DESKTOP_READOBJECTS | DESKTOP_CREATEWINDOW | DESKTOP_CREATEMENU | 
                DESKTOP_HOOKCONTROL | DESKTOP_JOURNALRECORD | DESKTOP_JOURNALPLAYBACK | 
                DESKTOP_ENUMERATE | DESKTOP_WRITEOBJECTS | DESKTOP_SWITCHDESKTOP | STANDARD_RIGHTS_REQUIRED;

            SecurityDescriptorSPtr securityDescriptor;
            auto error = userToken_->CreateSecurityDescriptor(winstaAccess, securityDescriptor);
            if (!error.IsSuccess()) { return error; }

            SecurityDescriptorSPtr deskTopDescriptor;
            error = userToken_->CreateSecurityDescriptor(desktopAll, deskTopDescriptor);
            if (!error.IsSuccess()) { return error; }

            SECURITY_ATTRIBUTES securityAttributes = { sizeof(SECURITY_ATTRIBUTES), securityDescriptor->PSecurityDescriptor, FALSE };
            HWINSTA wkHandle = ::CreateWindowStation(NULL, 0, MAXIMUM_ALLOWED, (PSECURITY_ATTRIBUTES)&securityAttributes);
            if (wkHandle == NULL)
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "CreateWindowStation failed. ErrorCode={0}",
                    error);
                return error;
            }
            this->workstationHandle_ = WorkstationHandle::CreateUPtr(wkHandle);

            // get the name of the workstation
            WCHAR wszName[MAX_PATH];
            ::ZeroMemory(wszName, sizeof(wszName));
            DWORD sizeWinSta = (_countof(wszName) - 1) * sizeof(WCHAR);
            if (!::GetUserObjectInformation(this->workstationHandle_->Value, UOI_NAME, wszName, sizeWinSta, &sizeWinSta))
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "GetUserObjectInformation failed for workstation. ErrorCode={0}",
                    error);
                return error;
            }

            // The desktop name will be appended to the new window station
            desktopName_.clear();
            desktopName_.append(wszName, sizeWinSta/sizeof(WCHAR) - 1);
            desktopName_.append(L"\\");


            // set the current process's workstation to be the new workstation and then create desktop in it
            // setup the scope so that we revert to the right workstation when we come out of it
            HWINSTA currentWkHandle = ::GetProcessWindowStation();
            if (currentWkHandle == NULL)
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "GetProcessWindowStation failed. ErrorCode={0}",
                    error);
                return error;
            }
            ResourceHolder<HWINSTA> currentWorkstation(
                currentWkHandle,
                [this](ResourceHolder<HWINSTA> * thisPtr)
            { 
                if (!::SetProcessWindowStation(thisPtr->Value))
                {
                    auto error = ErrorCode::FromWin32Error();
                    WriteError(
                        TraceType_Activator,
                        owner_.Root.TraceId,
                        "Failed to revert workstation scope. ErrorCode={0}.",
                        error);
                    Assert::CodingError("This is a failfast, because we failed to revert to the right workstation after changing the workstation for this process.");
                }
            });

            if (!::SetProcessWindowStation(workstationHandle_->Value))
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "SetProcessWindowStation failed. ErrorCode={0}",
                    error);
                return error;
            }

            securityAttributes.nLength = sizeof(SECURITY_ATTRIBUTES);
            securityAttributes.lpSecurityDescriptor = deskTopDescriptor->PSecurityDescriptor;
            securityAttributes.bInheritHandle = FALSE;

            // create new desktop in the workstation
            wstring desktopTitle = L"Fabric Worker Process Desktop";
            HDESK dtHandle = ::CreateDesktop(desktopTitle.c_str(), NULL, NULL, 0, GENERIC_ALL, (PSECURITY_ATTRIBUTES)&securityAttributes);
            if (dtHandle == NULL)
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "CreateDesktop failed. ErrorCode={0}",
                    error);
                return error;
            }
            this->desktopHandle_ = DesktopHandle::CreateUPtr(dtHandle);
            desktopName_.append(desktopTitle);

            return ErrorCode(ErrorCodeValue::Success);
        }
    }

    ErrorCode SetupDebugger(AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        if (HostingConfig::GetConfig().EnableProcessDebugging)
        {
            WriteInfo(TraceType_Activator, owner_.Root.TraceId, "Debugging enabled. setting up debugger for procId {0} threadId {1}", processId_, threadId_);
            wstring args = processDescription_.DebugParameters.Arguments;
            if (StringUtility::ContainsCaseInsensitive(processDescription_.DebugParameters.Arguments, *Constants::DebugProcessIdParameter))
            {
                wstring processId = StringUtility::ToWString<DWORD>(processId_);
                StringUtility::Replace<wstring>(args, *Constants::DebugProcessIdParameter, processId);
            }
            if (StringUtility::ContainsCaseInsensitive(processDescription_.DebugParameters.Arguments, *Constants::DebugThreadIdParameter))
            {
                wstring threadId = StringUtility::ToWString<DWORD>(threadId_);
                StringUtility::Replace<wstring>(args, *Constants::DebugThreadIdParameter, threadId);
            }
            CreateDebuggerCommandLine(processDescription_.DebugParameters.ExePath, args);
            error = CreateDebuggerProcess();
            if (!error.IsSuccess()) { TryComplete(thisSPtr, error); return error; }
            if (!processDescription_.NotAttachedToJob)
            {
                error = this->AssignJobToProcess(debuggerProcessHandle_);
                if (!error.IsSuccess()) { TryComplete(thisSPtr, error); return error; }
            }

            error = this->MonitorProcess(debuggerProcessHandle_, true);
            if (!error.IsSuccess()) { TryComplete(thisSPtr, error); return error; }

            error = this->ResumeProcess(debuggerThreadHandle_);

        }
        return error;
    }

    ErrorCode CreateEnvironmentBlock()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Creating environment block.");

        EnvironmentMap envMap;

        vector<wchar_t> envBlock = processDescription_.EnvironmentBlock;
        LPVOID penvBlock = envBlock.data();
        Environment::FromEnvironmentBlock(penvBlock, envMap);
        
        if (!processDescription_.DebugParameters.EnvironmentBlock.empty())
        {
            EnvironmentMap debugMap;
            vector<wchar_t> debugEnvBlock = processDescription_.DebugParameters.EnvironmentBlock;
            LPVOID pdebugEnv = debugEnvBlock.data();
            Environment::FromEnvironmentBlock(pdebugEnv, debugMap);
            for (auto iter = debugMap.begin(); iter != debugMap.end(); ++iter)
            {
                envMap[iter->first] = iter->second;
            }
        }

        auto error = DecryptEnvironmentVariables(envMap);
        if (!error.IsSuccess())
        {
            return error;
        }

        Environment::ToEnvironmentBlock(envMap, envBlock_);
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CreateRestrictedEnvironmentBlock()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Creating restricted environment block.");

        EnvironmentMap activatorEnvMap;
        if (!Environment::GetEnvironmentMap(activatorEnvMap))
        {
            return ErrorCode(ErrorCodeValue::OperationFailed);
        }

        EnvironmentMap fabricEnvMap;
        vector<wchar_t> envBlock = processDescription_.EnvironmentBlock;
        LPVOID penvBlock = envBlock.data();
        Environment::FromEnvironmentBlock(penvBlock, fabricEnvMap);
        EnvironmentMap userEnvMap;
        auto error = Environment::GetEnvironmentMap(userToken_->TokenHandle->Value, false, userEnvMap);
        if (!error.IsSuccess()) { return error; }

        auto pathIter = fabricEnvMap.find(L"Path");
        auto currPath = userEnvMap.find(L"Path");

        if (currPath != userEnvMap.end())
        {
            if(pathIter != fabricEnvMap.end())
            {
                wstring path = currPath->second;
                path.append(L";");
                path.append(pathIter->second);
                fabricEnvMap.erase(pathIter);
                userEnvMap[L"Path"] = path;
            }
        }

        for(auto iter = fabricEnvMap.begin(); iter != fabricEnvMap.end(); ++iter)
        {
            userEnvMap[iter->first] = iter->second;
        }

        if (!processDescription_.DebugParameters.EnvironmentBlock.empty())
        {
            EnvironmentMap debugMap;
            vector<wchar_t> debugEnvBlock = processDescription_.DebugParameters.EnvironmentBlock;
            LPVOID pdebugEnv = debugEnvBlock.data();
            Environment::FromEnvironmentBlock(pdebugEnv, debugMap);
            for (auto iter = debugMap.begin(); iter != debugMap.end(); ++iter)
            {
                userEnvMap[iter->first] = iter->second;
            }
        }

        error = DecryptEnvironmentVariables(userEnvMap);
        if (!error.IsSuccess())
        {
            return error;
        }

        Environment::ToEnvironmentBlock(userEnvMap, envBlock_);
        return error;
    }

    ErrorCode DecryptEnvironmentVariables(_Inout_ EnvironmentMap & userEnvMap)
    {
        ErrorCode error(ErrorCodeValue::Success);
        wstring value;
        for (auto const& setting : processDescription_.EncryptedEnvironmentVariables)
        {
            error = ContainerConfigHelper::DecryptValue(setting.second, value);
            if (!error.IsSuccess())
            {
                return error;
            }

            userEnvMap[setting.first] = value;
        }

        return error;
    }

    ErrorCode CreateJobObject(wstring const & jobObjectName = L"", bool isHostedService = false)
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Creating job object.");

        HANDLE handle = NULL;
        ErrorCode error = owner_.CreateJobObjectImpl(handle, jobObjectName);
        if (handle == NULL || !error.IsSuccess())
        {
            return error;
        }
        jobHandle_ = JobHandle::CreateUPtr(handle, true);

        JOBOBJECT_EXTENDED_LIMIT_INFORMATION limitInfo = { 0 };

        if (!processDescription_.AllowChildProcessDetach &&
            HostingConfig::GetConfig().ActivatedProcessDieOnUnhandledExceptionEnabled &&
            !IsCrashDumpCollectionEnabled())
        {
            limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_DIE_ON_UNHANDLED_EXCEPTION;
        }

        limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
        if(processDescription_.AllowChildProcessDetach)
        {
            limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_BREAKAWAY_OK;
        }

        // Set governance to job object structures.
        ResourceGovernancePolicyDescription const & rgPolicy = processDescription_.ResourceGovernancePolicy;
        if (!HostingConfig::GetConfig().LocalResourceManagerTestMode)
        {
            //this is for windows 2012
            if (!rgPolicy.CpusetCpus.empty())
            {
                owner_.SetupCpuSetCpusLimit(limitInfo, rgPolicy.CpusetCpus);
            }

            // If the JobObject is created for the hosted service,
            // then MemoryInMB caps only working mamory set (main memory),
            // and MemorySwapInMB caps commited memory (main mamory + disk)
            if (rgPolicy.MemoryInMB > 0 ||
                (isHostedService && rgPolicy.MemorySwapInMB > 0))
            {
                owner_.SetupMemoryLimit(limitInfo, rgPolicy.MemoryInMB, rgPolicy.MemorySwapInMB, isHostedService);
            }

            if (rgPolicy.CpuShares > 0)
            {
                JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpuRateControl = { 0 };

                owner_.SetupCpuSharesLimit(cpuRateControl, rgPolicy.CpuShares, isHostedService);

                if (!::SetInformationJobObject(
                    jobHandle_->Value,
                    JobObjectCpuRateControlInformation,
                    (void*)&cpuRateControl,
                    sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION)))
                {
                    error = ErrorCode::FromWin32Error();

                    WriteWarning(
                        TraceType_Activator,
                        owner_.Root.TraceId,
                        "SetInformationJobObject (JOBOBJECT_CPU_RATE_CONTROL_INFORMATION) failed. ErrorCode={0}",
                        error);

                    return error;
                }
            }
        }

        if (!::SetInformationJobObject(
            jobHandle_->Value, 
            JobObjectExtendedLimitInformation, 
            (void*)&limitInfo, 
            sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
        {
            error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "SetInformationJobObject:JobObjectExtendedLimitInformation failed. ErrorCode={0}",
                error);
            return error;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CreateProcess()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Launching process via CreateProcessW");

        STARTUPINFO startupInfo = { 0 };
        startupInfo.cb = sizeof(STARTUPINFO);
        DWORD startupFlags = 
            CREATE_UNICODE_ENVIRONMENT |
            CREATE_SUSPENDED | 
            CREATE_BREAKAWAY_FROM_JOB | 
            CREATE_NEW_PROCESS_GROUP;

        if (processDescription_.ShowNoWindow)
        {
            startupFlags |= CREATE_NO_WINDOW;
            startupInfo.wShowWindow = SW_HIDE;
            startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        }
        else
        {
            startupFlags |= CREATE_NEW_CONSOLE;
        }

        hasConsole_ = !processDescription_.ShowNoWindow;

        bool inheritHandles = SetupConsoleRedirection(startupInfo);

        PROCESS_INFORMATION processInfo = { 0 };
        LPVOID penvBlock = NULL;
        if (!envBlock_.empty() && envBlock_.size() > 1)
        {
            penvBlock = &envBlock_[0];
        }

        BOOL success = FALSE;
        success = ::CreateProcessW(
            NULL,
            &commandLine_[0],
            NULL,
            NULL,
            inheritHandles,
            startupFlags,
            penvBlock,
            processDescription_.StartInDirectory.c_str(),
            &startupInfo,
            &processInfo);
        if (!success)
        {
            auto win32Err = ::GetLastError();
            auto error = ErrorCode::FromWin32Error(win32Err);

            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessW({0}) failed. ErrorCode={1}",
                commandLine_,
                error);

            if (win32Err == ERROR_FILE_NOT_FOUND)
            {
                // https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx
                // 
                // MSDN document is not explicit about CreateProcess supporting UNC paths and it
                // does not seem to work in practice regardless of whether the path is part of
                // lpApplicationName or lpCommandLine. In lpCommandLine, ERROR_FILE_NOT_FOUND
                // occurs if the executable path is too long. Return a PathTooLong error
                // with details for Hosting to report health and aid customer self-debugging.
                //
                auto msg = wformatString(GET_RC( ActivationPathTooLong ), commandLine_);

                WriteWarning(
                    TraceType_Activator,
                    owner_.Root.TraceId,
                    "{0}",
                    msg);

                error = ErrorCode(ErrorCodeValue::PathTooLong, move(msg));
            }

            return error;
        }
        else
        {
            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessW({0}) successful. ProcessId={1}, ThreadId={2}", 
                commandLine_,
                processInfo.dwProcessId,
                processInfo.dwThreadId);
        }

        try
        {
            processHandle_ = ProcessHandle::CreateUPtr(processInfo.hProcess, true);
            threadHandle_ = make_unique<Handle>(processInfo.hThread);
            processId_ = processInfo.dwProcessId;
            threadId_ = processInfo.dwThreadId;
        }
        catch(...)
        {
            ::CloseHandle(processInfo.hThread);
            throw;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CreateProcessWithLogon()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Launching process via CreateProcessWithLogon");

        STARTUPINFO startupInfo = { 0 };
        startupInfo.cb = sizeof(STARTUPINFO);
        DWORD startupFlags =
            CREATE_UNICODE_ENVIRONMENT | 
            CREATE_SUSPENDED | 
            CREATE_BREAKAWAY_FROM_JOB | 
            CREATE_NEW_PROCESS_GROUP |
            CREATE_NEW_CONSOLE;

        SetupConsoleRedirection(startupInfo);
        hasConsole_ = true;
        // ignore ShowNoWindow for interactive sandbox

        PROCESS_INFORMATION processInfo = { 0 };
        BOOL success = FALSE;

        {
            // create process with lognon tries to start secondary logon service
            // concurrent invocation of this API can result in greater change of failure
            AcquireExclusiveLock lock(owner_.logonAsUserLock_);

            success = ::CreateProcessWithLogonW(
                runAs_->AccountName.c_str(),
                (runAs_->AccountType == SecurityPrincipalAccountType::LocalUser) ? L"." : NULL,
                runAs_->Password.c_str(),
                LOGON_WITH_PROFILE, 
                NULL, 
                &commandLine_[0],
                startupFlags,
                envBlock_.data(),
                processDescription_.StartInDirectory.c_str(),
                &startupInfo,
                &processInfo);
        }

        if (!success)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessWithLogonW failed. ErrorCode={0}",
                error);
            return error;
        }
        else
        {
            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessWithLogonW successful. ProcessId={0}, ThreadId={0}", 
                processInfo.dwProcessId,
                processInfo.dwThreadId);
        }

        try
        {
            processHandle_ = ProcessHandle::CreateUPtr(processInfo.hProcess, true);
            threadHandle_ = make_unique<Handle>(processInfo.hThread);
            processId_ = processInfo.dwProcessId;
            threadId_ = processInfo.dwThreadId;
        }
        catch(...)
        {
            ::CloseHandle(processInfo.hThread);
            throw;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CreateProcessAsUser()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Launching process via CreateProcessAsUser");

        STARTUPINFO startupInfo = { 0 };
        startupInfo.cb = sizeof(STARTUPINFO);
        DWORD startupFlags = 
            CREATE_UNICODE_ENVIRONMENT | 
            CREATE_SUSPENDED | 
            CREATE_BREAKAWAY_FROM_JOB |
            CREATE_NEW_PROCESS_GROUP;
        startupInfo.lpDesktop = &desktopName_[0];
        if (processDescription_.ShowNoWindow)
        {
            startupFlags |= CREATE_NO_WINDOW;
            startupInfo.wShowWindow = SW_HIDE;
            startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        }
        else
        {
            startupFlags |= CREATE_NEW_CONSOLE;
            hasConsole_ = true;
        }
        bool inheritHandles = SetupConsoleRedirection(startupInfo);

        PROCESS_INFORMATION processInfo = { 0 };
        BOOL success = FALSE;

        LPVOID penvBlock = NULL;
        if(!envBlock_.empty() &&
            envBlock_.size() > 1)
        {
            penvBlock = envBlock_.data();
        }

        success = ::CreateProcessAsUser(
            this->userToken_->TokenHandle->Value,
            NULL, 
            &commandLine_[0],
            NULL,       
            NULL,
            inheritHandles,
            startupFlags,
            penvBlock,
            processDescription_.StartInDirectory.c_str(),
            &startupInfo,
            &processInfo);

        if (!success)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessAsUser failed. ErrorCode={0}",
                error);
            return error;
        }
        else
        {
            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "CreateProcessAsUser successful. ProcessId={0}, ThreadId={1}", 
                processInfo.dwProcessId,
                processInfo.dwThreadId);
        }

        try
        {
            processHandle_ = ProcessHandle::CreateUPtr(processInfo.hProcess, true);
            threadHandle_ = make_unique<Handle>(processInfo.hThread);
            processId_ = processInfo.dwProcessId;
            threadId_ = processInfo.dwThreadId;
        }
        catch(...)
        {
            ::CloseHandle(processInfo.hThread);
            throw;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode CreateDebuggerProcess()
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Launching debugger process via CreateProcessAsUser");

        STARTUPINFO startupInfo = { 0 };
        startupInfo.cb = sizeof(STARTUPINFO);
        DWORD startupFlags =
            CREATE_UNICODE_ENVIRONMENT |
            CREATE_SUSPENDED |
            CREATE_BREAKAWAY_FROM_JOB |
            CREATE_NEW_PROCESS_GROUP;
        startupInfo.lpDesktop = &desktopName_[0];
        if (processDescription_.ShowNoWindow)
        {
            startupFlags |= CREATE_NO_WINDOW;
            startupInfo.wShowWindow = SW_HIDE;
            startupInfo.dwFlags |= STARTF_USESHOWWINDOW;
        }
        else
        {
            startupFlags |= CREATE_NEW_CONSOLE;
            hasConsole_ = true;
        }
        bool inheritHandles = false;

        LPVOID penvBlock = NULL;
        if (!envBlock_.empty() &&
            envBlock_.size() > 1)
        {
            penvBlock = &envBlock_[0];
        }
        PROCESS_INFORMATION processInfo = { 0 };
        BOOL success = FALSE;

        if (runAs_)
        {
            success = ::CreateProcessAsUser(
                this->userToken_->TokenHandle->Value,
                NULL,
                &debuggerCommandLine_[0],
                NULL,
                NULL,
                inheritHandles,
                startupFlags,
                penvBlock,
                NULL,
                &startupInfo,
                &processInfo);
        }
        else
        {
            success = ::CreateProcessW(
                NULL,
                &debuggerCommandLine_[0],
                NULL,
                NULL,
                inheritHandles,
                startupFlags,
                penvBlock,
                NULL,
                &startupInfo,
                &processInfo);
        }
        if (!success)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "DebuggerProcess: CreateProcess failed. ErrorCode={0} commandline {1}",
                error,
                debuggerCommandLine_);
            return error;
        }
        else
        {
            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "DebuggerProcess: CreateProcess successful. ProcessId={0}, ThreadId={1} commandline {2}",
                processInfo.dwProcessId,
                processInfo.dwThreadId,
                debuggerCommandLine_);
        }

        try
        {
            debuggerProcessHandle_ = ProcessHandle::CreateUPtr(processInfo.hProcess, true);
            debuggerThreadHandle_ = make_unique<Handle>(processInfo.hThread);
            debuggerProcId_ = processInfo.dwProcessId;
        }
        catch (...)
        {
            ::CloseHandle(processInfo.hThread);
            throw;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode AssignJobToProcess(ProcessHandleUPtr const & processHandle)
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Assigning job to the process.");

        if (!::AssignProcessToJobObject(jobHandle_->Value, processHandle->Value))
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "AssignProcessToJobObject failed. ErrorCode={0}",
                error);
            return error;
        }

        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode MonitorProcess(ProcessHandleUPtr const & processHandle, bool isDebug = false)
    {
        if (isDebug)
        {
            debugprocessMonitor_ = ProcessWait::CreateAndStart(
                Handle(processHandle->Value, Handle::DUPLICATE()),
                processExitCallback_);
        }
        else
        {
            // start process monitoring
            processMonitor_ = ProcessWait::CreateAndStart(
                Handle(processHandle->Value, Handle::DUPLICATE()),
                processExitCallback_);
        }
        return ErrorCode(ErrorCodeValue::Success);
    }

    ErrorCode ResumeProcess(HandleUPtr const & threadHandle)
    {
        WriteNoise(TraceType_Activator, owner_.Root.TraceId, "Resuming the process.");

        auto prevSuspendCount = ::ResumeThread(threadHandle->Value);

        // > 1 indicates that suspended count did not get decremented to 0 after this call. Process still suspended.
        // == -1 indicates that the API failed.
        if(prevSuspendCount != 1)
        {
            auto error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "ResumeProcess failed. ErrorCode={0}, PreviousSuspendCount={1}",
                error,
                prevSuspendCount);

            return error;
        }

        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Process successfully resumed");

        if (stdoutRedirector_ && stderrRedirector_)
        {
            stdoutRedirector_->Open();
            stderrRedirector_->Open();
        }

        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Process successfully resumed");

        return ErrorCode(ErrorCodeValue::Success);
    }

    void CreateCommandLine()
    {
        commandLine_ = ProcessUtility::GetCommandLine(processDescription_.ExePath, processDescription_.Arguments);
    }

    void CreateDebuggerCommandLine(
        wstring const & exePath,
        wstring const & arguments)
    {
        debuggerCommandLine_ = ProcessUtility::GetCommandLine(
            exePath,
            arguments);
    }

    bool SetupConsoleRedirection(STARTUPINFO & startupInfo)
    {
        if (!this->processDescription_.RedirectConsole) return false;

        ErrorCode error = ProcessConsoleRedirector::CreateOutputRedirector(
            this->processDescription_.LogDirectory, 
            this->processDescription_.RedirectedConsoleFileNamePrefix, 
            this->processDescription_.ConsoleRedirectionFileRetentionCount,
            this->processDescription_.ConsoleRedirectionFileMaxSizeInKb,
            stdoutHandle_, 
            stdoutRedirector_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Error '{0}' setting up redirection for console output. Console logs will not be available.",
                error);
            return false;
        }

        error = ProcessConsoleRedirector::CreateErrorRedirector(
            this->processDescription_.LogDirectory, 
            this->processDescription_.RedirectedConsoleFileNamePrefix, 
            this->processDescription_.ConsoleRedirectionFileRetentionCount,
            this->processDescription_.ConsoleRedirectionFileMaxSizeInKb,
            stderrHandle_, 
            stderrRedirector_);
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Error '{0}' setting up redirection for console error. Console logs will not be available.",
                error);                
            return false;
        }

        HANDLE hStdInputHandle = ::GetStdHandle(STD_INPUT_HANDLE);
        if (hStdInputHandle == INVALID_HANDLE_VALUE)
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Error {0} during GetStdHandle. Console logs will not be available.",
                ErrorCode::FromWin32Error());
            return false;
        }

        startupInfo.hStdOutput = stdoutHandle_->Value;
        startupInfo.hStdError = stderrHandle_->Value;
        startupInfo.hStdInput = hStdInputHandle;
        startupInfo.dwFlags |= STARTF_USESTDHANDLES;
        return true;
    }

    bool IsCrashDumpCollectionEnabled()
    {
        wstring exeName = Path::GetFileName(this->processDescription_.ExePath);
        return CrashDumpConfigurationManager::IsCrashDumpCollectionEnabled(exeName);
    }

private:
    ProcessActivator & owner_;
    SecurityUserSPtr const runAs_;
    ProcessWait::WaitCallback const processExitCallback_;
    ProcessDescription const processDescription_;
    TimeoutHelper timeoutHelper_;
    ProcessWaitSPtr processMonitor_;
    ProcessWaitSPtr debugprocessMonitor_;
    JobHandleUPtr jobHandle_;
    ProcessHandleUPtr processHandle_;
    HandleUPtr threadHandle_;
    HandleUPtr stdoutHandle_;
    HandleUPtr stderrHandle_;    
    ProcessConsoleRedirectorSPtr stdoutRedirector_;
    ProcessConsoleRedirectorSPtr stderrRedirector_;
    DWORD processId_;
    DWORD threadId_;
    DWORD debuggerProcId_;
    ProcessHandleUPtr debuggerProcessHandle_;
    HandleUPtr debuggerThreadHandle_;
    vector<wchar_t> envBlock_;
    wstring commandLine_;
    wstring debuggerCommandLine_;
    AccessTokenSPtr userToken_;
    WorkstationHandleUPtr workstationHandle_;
    DesktopHandleUPtr desktopHandle_;
    wstring desktopName_;
    bool hasConsole_;
    wstring fabricBinFolder_;
    bool enableWinfabAdminSynchronize_;
};

// ********************************************************************************************************************
// ProcessActivator::DeactivateAsyncOperation Implementation
//
class ProcessActivator::DeactivateAsyncOperation : public AsyncOperation
{
    DENY_COPY(DeactivateAsyncOperation)

public:
    DeactivateAsyncOperation(
        __in ProcessActivator & owner,
        IProcessActivationContextSPtr const & activationContext,        
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        owner_(owner),   
        activationContext_(static_pointer_cast<ProcessActivationContext>(activationContext)),
        timeoutHelper_(timeout),        
        ctrlCProcessWait_(),
        hostProcessWait_(),
        sharedPtrLock_(),
        isCancelled_(false)
    {
    }

    virtual ~DeactivateAsyncOperation()
    {
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<DeactivateAsyncOperation>(operation);
        return thisPtr->Error;
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        HandleUPtr ctrlCSenderThreadHandle;
        HandleUPtr ctrlCSenderProcessHandle;
        vector<wchar_t> envBlock;
        if (!activationContext_->HasConsole)
        {
            FinishTerminateProcess(thisSPtr, false);
            return;
        }
        // Create CtrlCProcess. This will be associated with the fabric job object.
        // That will ensure that we do not have orphan CtrlC processes when fabric crashes.
        ErrorCode error = ProcessUtility::CreateProcessW(
            ProcessUtility::GetCommandLine(Constants::CtrlCSenderExeName, StringUtility::ToWString<DWORD>(activationContext_->ProcessId)),
            L"",
            envBlock,
            DETACHED_PROCESS /* Do not inherit console */,
            ctrlCSenderProcessHandle,
            ctrlCSenderThreadHandle);    
        if (!error.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "{0} process could not be created. Attempting to terminate the process. ErrorCode={1}",
                Constants::CtrlCSenderExeName,
                error);
            FinishTerminateProcess(thisSPtr, false);
            return;
        }

        {
            AcquireExclusiveLock lock(sharedPtrLock_);
            if (isCancelled_) { return; }

            ctrlCProcessWait_ = ProcessWait::CreateAndStart(
                Handle(ctrlCSenderProcessHandle->Value, Handle::DUPLICATE()),
                [this, thisSPtr](pid_t, ErrorCode const & waitResult, DWORD exitCode) { OnCtrlCProcessTerminated(thisSPtr, waitResult, exitCode); },
                timeoutHelper_.GetRemainingTime());
        }
    }

    void OnCtrlCProcessTerminated(AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & waitResult, DWORD ctrlCProcessExitCode)
    {
        if (!waitResult.IsSuccess())
        {
            WriteWarning(
                TraceType_Activator,
                owner_.Root.TraceId,
                "{0} didn't shutdown timely. Attempting to force terminate the application host process.",
                Constants::CtrlCSenderExeName);  
            FinishTerminateProcess(thisSPtr, false);
            return;
        }

        // CtrlC process didn't succeed
        if (ctrlCProcessExitCode != 0)        
        {
            WriteInfo(
                TraceType_Activator,
                owner_.Root.TraceId,
                "{0} exited with return code {1}. Attempting to force terminate the application host process.",
                Constants::CtrlCSenderExeName,
                ctrlCProcessExitCode);  
            FinishTerminateProcess(thisSPtr, false);
            return;
        }

        {
            AcquireExclusiveLock lock(sharedPtrLock_);            
            if (isCancelled_) { return; }
            hostProcessWait_ = ProcessWait::CreateAndStart(
                Handle(activationContext_->ProcessHandle->Value, Handle::DUPLICATE()),
                [this, thisSPtr](pid_t, ErrorCode const & waitResult, DWORD) { OnProcessTerminated(thisSPtr, waitResult); },
                timeoutHelper_.GetRemainingTime());
        }
    }

    void OnProcessTerminated(AsyncOperationSPtr const & thisSPtr, Common::ErrorCode const & waitResult)
    {
        bool isProcessTerminated = true;

        // If we timed out, there is a possibility that the process didn't shutdown as a result of Ctrl-C
        // We will force shutdown it using Terminate
        if (!waitResult.IsSuccess())
        {
            WriteNoise(
                TraceType_Activator,
                owner_.Root.TraceId,
                "Process didn't shutdown as a result of CtrlC. Attempting to terminate the process");
            isProcessTerminated = false;
        }

        FinishTerminateProcess(thisSPtr, isProcessTerminated);
    }

    void FinishTerminateProcess(AsyncOperationSPtr const & thisSPtr, bool isProcessTerminated)
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "Process {0} using CtrlC.", (isProcessTerminated ? L"terminated": L"did not terminate"));
        ErrorCode error = owner_.Cleanup(activationContext_, isProcessTerminated, ProcessActivator::ProcessDeactivateExitCode);
        TryComplete(thisSPtr, error);
    }

    void OnCancel()
    {
        WriteNoise(
            TraceType_Activator,
            owner_.Root.TraceId,
            "ProcessActivator::DeactivateAsyncOperation: Cancel called.");

        AcquireExclusiveLock lock(sharedPtrLock_);

        if (ctrlCProcessWait_) { ctrlCProcessWait_->Cancel(); }
        if (hostProcessWait_) { hostProcessWait_->Cancel(); }

        isCancelled_ = true;
    }

private:
    ProcessActivator & owner_;
    shared_ptr<ProcessActivationContext> const activationContext_;        
    TimeoutHelper timeoutHelper_;
    ProcessWaitSPtr ctrlCProcessWait_;
    ProcessWaitSPtr hostProcessWait_;
    ExclusiveLock sharedPtrLock_;    
    bool isCancelled_;
};

// ********************************************************************************************************************
// ProcessActivator Implementation
//

ProcessActivator::ProcessActivator(ComponentRoot const & root) : RootedObject(root),
    seDebugPrivilegeCheckLock_(),
    hasSeDebugPrivilege_(false),
    assignPrimaryTokenPrivilegeCheckLock_(),
    hasAssignPrimaryTokenPrivilege_(false),
    desktopRestricationLock_(),
    logonAsUserLock_(),
    isSystem_(false),
    accountChecked_(false),
    systemAccountCheckLock_()
{
}

ProcessActivator::~ProcessActivator()
{
}

AsyncOperationSPtr ProcessActivator::BeginActivate(
    SecurityUserSPtr const & runAs,
    ProcessDescription const & processDescription,
    wstring const & fabricbinPath,
    ProcessWait::WaitCallback const & processExitCallback,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    bool allowWinfabAdminSync)
{
    return AsyncOperation::CreateAndStart<ActivateAsyncOperation>(
        *this,
        runAs,
        processDescription,
        fabricbinPath,
        processExitCallback,
        allowWinfabAdminSync,
        timeout,
        callback,
        parent);
}

ErrorCode ProcessActivator::EndActivate(
    AsyncOperationSPtr const & operation,
    __out IProcessActivationContextSPtr & activationContext)
{
    return ActivateAsyncOperation::End(operation, activationContext);
}

AsyncOperationSPtr ProcessActivator::BeginDeactivate(
    IProcessActivationContextSPtr const & activationContext,
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<DeactivateAsyncOperation>(
        *this,
        activationContext,        
        timeout,
        callback,
        parent);
}

ErrorCode ProcessActivator::EndDeactivate(
    AsyncOperationSPtr const & operation)
{
    return DeactivateAsyncOperation::End(operation);
}

ErrorCode ProcessActivator::Terminate(IProcessActivationContextSPtr const & activationContext, UINT uExitCode)
{
    return Cleanup(activationContext, false, uExitCode);
}

ErrorCode ProcessActivator::Cleanup(IProcessActivationContextSPtr const & activationContext, bool processClosedUsingCtrlC, UINT uExitCode)
{
    return activationContext->TerminateAndCleanup(processClosedUsingCtrlC, uExitCode);
}

ErrorCode ProcessActivator::EnsureSeDebugPrivilege()
{
    {
        AcquireReadLock readLock(seDebugPrivilegeCheckLock_);
        if (hasSeDebugPrivilege_) return ErrorCode(ErrorCodeValue::Success);
    }

    {
        AcquireWriteLock writeLock(seDebugPrivilegeCheckLock_);
        if (hasSeDebugPrivilege_) return ErrorCode(ErrorCodeValue::Success);

        auto error = SecurityUtility::EnsurePrivilege(SE_DEBUG_NAME);
        if (!error.IsSuccess()) { return error; }

        hasSeDebugPrivilege_ = true;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ProcessActivator::EnsureAssignPrimaryTokenPrivilege()
{
    {
        AcquireReadLock readLock(assignPrimaryTokenPrivilegeCheckLock_);
        if (hasAssignPrimaryTokenPrivilege_) return ErrorCode(ErrorCodeValue::Success);
    }

    {
        AcquireWriteLock writeLock(assignPrimaryTokenPrivilegeCheckLock_);
        if (hasAssignPrimaryTokenPrivilege_) return ErrorCode(ErrorCodeValue::Success);

        auto error = SecurityUtility::EnsurePrivilege(SE_ASSIGNPRIMARYTOKEN_NAME);
        if (!error.IsSuccess()) { return error; }

        hasAssignPrimaryTokenPrivilege_ = true;
    }

    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ProcessActivator::IsSystemAccount(bool & isSystem)
{
    if (this->accountChecked_)
    {
        isSystem = this->isSystem_;
        return ErrorCode(ErrorCodeValue::Success);
    }
    {
        AcquireWriteLock writeLock(systemAccountCheckLock_);
        if (!this->accountChecked_)
        {
            SidUPtr currentSid;
            auto error = BufferedSid::GetCurrentUserSid(currentSid);
            if(!error.IsSuccess())
            {
                WriteError(
                    TraceType_Activator,
                    Root.TraceId,
                    "Failed to get current user sid ErrorCode={0}.",
                    error);

                return error;
            }
            this->isSystem_ = currentSid->IsLocalSystem();
            this->accountChecked_ = true;
        }
    }
    isSystem = this->isSystem_;
    return ErrorCode(ErrorCodeValue::Success);
}

ErrorCode ProcessActivator::CreateJobObjectImpl(HANDLE & handle, wstring const & jobObjectName)
{
    wchar_t const * jobObjectNameWinApi = NULL;
    wstring jobGuidName = L"";
    ErrorCode error = ErrorCodeValue::Success;

    // If JO name is specified, create JO with specific name
    if (!jobObjectName.empty())
    {
        jobObjectNameWinApi = jobObjectName.c_str();
    }
    handle = ::CreateJobObject(NULL, jobObjectNameWinApi);

    // If creation of JO without name failed, exit with error
    if (handle == NULL && jobObjectName.empty())
    {
        error = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "CreateJobObject failed. ErrorCode={0}",
            error);
        return error;
    }
    // If creation of Job Object with specific name failed,
    // apply appropriate fallback mechanisms
    else if (handle == NULL)
    {
        error = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "CreateJobObject failed with name {0}. ErrorCode={1}",
            jobObjectName,
            error);

        // First fallback mechanism in case JO name has already been reserved
        jobGuidName = (Guid::NewGuid()).ToString();
        jobObjectNameWinApi = jobGuidName.c_str();
        handle = ::CreateJobObject(NULL, jobObjectNameWinApi);

        if (handle == NULL)
        {
            error = ErrorCode::FromWin32Error();
            WriteWarning(
                TraceType_Activator,
                Root.TraceId,
                "CreateJobObject failed with guid name {0}. ErrorCode={1}",
                jobGuidName,
                error);

            // Second fallback mechanism to create JO without name
            handle = ::CreateJobObject(NULL, NULL);

            if (handle == NULL)
            {
                error = ErrorCode::FromWin32Error();
                WriteWarning(
                    TraceType_Activator,
                    Root.TraceId,
                    "CreateJobObject failed. ErrorCode={0}",
                    error);
                return error;
            }
            else
            {
                WriteInfo(
                    TraceType_Activator,
                    Root.TraceId,
                    "CreateJobObject succeeded without name");
            }
        }
        else
        {
            WriteInfo(
                TraceType_Activator,
                Root.TraceId,
                "CreateJobObject succeeded with name {0}",
                jobGuidName);
        }
    }
    else
    {
        WriteInfo(
            TraceType_Activator,
            Root.TraceId,
            "CreateJobObject succeeded with name {0}",
            jobObjectName);
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void ProcessActivator::SetupCpuSetCpusLimit(JOBOBJECT_EXTENDED_LIMIT_INFORMATION & limitInfo, std::wstring cpusetCpus)
{
    ULONG_PTR affinityMask = 0;
    if (cpusetCpus.empty())
    {
        limitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_AFFINITY;
        limitInfo.BasicLimitInformation.Affinity = affinityMask;
    }
    else
    {
        // Calculate affinity from CpusetCpus
        StringCollection affinityStringCollection;
        StringUtility::Split<std::wstring>(cpusetCpus, affinityStringCollection, L",");
        for (auto coreString : affinityStringCollection)
        {
            int32 coreId = Int32_Parse(coreString);
            affinityMask |= 1ull << coreId;
        }

        // If there is a request to affinitize to a core which doesn't exists on a machine
        // (or it is not allowed), do not set any limits
        ULONG_PTR activeProcessorMask = Environment::GetActiveProcessorsMask();
        if (affinityMask > activeProcessorMask ||
            ((affinityMask & activeProcessorMask) != affinityMask))
        {
            WriteWarning(
                TraceType_Activator,
                Root.TraceId,
                "Set affinity limit failed. Requested affinity mask={0}, system affinity mask={1}",
                affinityMask,
                activeProcessorMask);
            return;
        }

        // Set affinity mask for the process
        limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_AFFINITY;
        limitInfo.BasicLimitInformation.Affinity = affinityMask;
    }
}

void ProcessActivator::SetupCpuSharesLimit(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION & cpuRateInfo, UINT cpuShares, bool isHostedService)
{
    UNREFERENCED_PARAMETER(isHostedService);

    if (cpuShares > 0)
    {
        // If the value is above maximal allowed value,
        // do not set limit
        if (cpuShares > 10000)
        {
            WriteWarning(
                TraceType_Activator,
                Root.TraceId,
                "Set cpuShares limit failed. Value is above maximal allowed value which is 10000");
            return;
        }

        cpuRateInfo.ControlFlags |= JOB_OBJECT_CPU_RATE_CONTROL_ENABLE;
        cpuRateInfo.ControlFlags |= JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        cpuRateInfo.CpuRate = cpuShares;
    }
    else
    {
        cpuRateInfo.ControlFlags &= ~JOB_OBJECT_CPU_RATE_CONTROL_ENABLE;
        cpuRateInfo.ControlFlags &= ~JOB_OBJECT_CPU_RATE_CONTROL_HARD_CAP;
        cpuRateInfo.CpuRate = 0;
    }
}

void ProcessActivator::SetupMemoryLimit(JOBOBJECT_EXTENDED_LIMIT_INFORMATION & limitInfo, UINT memoryInMB, UINT memorySwapInMB, bool isHostedService)
{
    // In case of hosted services, main memory and swap space are always governed separately.
    // In case of regular services, if depends on configuration switch GovernOnlyMainMemoryForProcesses:
    //  - If set to true, memoryInMB will govern only the amount of RAM (swap is ignored in this case).
    //  - Otherwise, memoryInMB will govern the amount of RAM + swap (default behavior)
    bool splitRAMAndSwap = isHostedService || HostingConfig::GetConfig().GovernOnlyMainMemoryForProcesses;

    if (splitRAMAndSwap)
    {
        // Working memory limits
        if (memoryInMB > 0)
        {
            // If there is a request to limit memory on value which exceeds machine memory, do not set any limits
            uint64 systemMemoryInMB = 0;
            Common::ErrorCode errorMemory = Environment::GetAvailableMemoryInBytes(systemMemoryInMB);
            if (errorMemory.IsSuccess())
            {
                if (memoryInMB > systemMemoryInMB)
                {
                    WriteWarning(
                        TraceType_Activator,
                        Root.TraceId,
                        "Set memory limit failed. Requested memoryLimit={0}, available machineMemory={1}",
                        memoryInMB,
                        systemMemoryInMB);
                    return;
                }
            }

            limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_WORKINGSET;
            // Minimal required memory to launch a process is fixed, and it is 1MB
            limitInfo.BasicLimitInformation.MinimumWorkingSetSize = 1024 * 1024;
            SIZE_T memoryInBytes = static_cast<SIZE_T>(memoryInMB) * 1024 * 1024;
            limitInfo.BasicLimitInformation.MaximumWorkingSetSize = memoryInBytes;
        }
        else
        {
            limitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_WORKINGSET;
            limitInfo.BasicLimitInformation.MinimumWorkingSetSize = 0;
            limitInfo.BasicLimitInformation.MaximumWorkingSetSize = 0;
        }

        // Committed memory limits (only for hosted services)
        if (isHostedService && memorySwapInMB > 0)
        {
            limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            limitInfo.ProcessMemoryLimit = static_cast<SIZE_T>(memorySwapInMB) * 1024 * 1024;
        }
        else
        {
            limitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            limitInfo.ProcessMemoryLimit = 0;
        }
    }
    else
    {
        // If GovernOnlyMainMemoryForProcesses is set to false, then keep the existing behavior for non-hosted services,
        // and set committed memory limit.
        if (memoryInMB > 0)
        {
            limitInfo.BasicLimitInformation.LimitFlags |= JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            limitInfo.ProcessMemoryLimit = static_cast<SIZE_T>(memoryInMB) * 1024 * 1024;
        }
        else
        {
            limitInfo.BasicLimitInformation.LimitFlags &= ~JOB_OBJECT_LIMIT_PROCESS_MEMORY;
            limitInfo.ProcessMemoryLimit = 0;
        }
    }
}

Common::ErrorCode ProcessActivator::UpdateRGPolicy(
    IProcessActivationContextSPtr & activationContext,
    ProcessDescriptionUPtr & processDescription)
{
    // Get the Job Object handle
    HANDLE jobHandle = nullptr;
    try
    {
        if (activationContext != nullptr)
        {
            jobHandle = dynamic_cast<ProcessActivationContext &>(*activationContext).JobHandle->Value;
        }
        else
        {
            return (ErrorCodeValue::OperationFailed);
        }
    }
    catch (...)
    {
        return (ErrorCodeValue::OperationFailed);
    }

    if (jobHandle == nullptr)
    {
        return (ErrorCodeValue::OperationFailed);
    }

    bool isHostedService = processDescription->IsHostedServiceProcess;

    // Currently update is only used for hosted services
    TESTASSERT_IF(!isHostedService, "Update RG is called for non hosted service: {0}", processDescription->ExePath);

    wstring newCpusetCpus = processDescription->ResourceGovernancePolicy.CpusetCpus;
    UINT newCpuShares = processDescription->ResourceGovernancePolicy.CpuShares;
    UINT newMemoryInMB = processDescription->ResourceGovernancePolicy.MemoryInMB;
    UINT newMemorySwapInMB = processDescription->ResourceGovernancePolicy.MemorySwapInMB;

    WriteInfo(
        TraceType_Activator,
        Root.TraceId,
        "Updating Job Object {0} with CpusetCpus={1}, CpuShares={2}, MemoryInMB={3}, MemorySwapInMB={4}.",
        processDescription->JobObjectName,
        newCpusetCpus,
        newCpuShares,
        newMemoryInMB,
        newMemorySwapInMB);

    // Get the current JO limits and state
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limitInfo;
    if (!QueryInformationJobObject(jobHandle, JobObjectExtendedLimitInformation, &limitInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION), nullptr))
    {
        auto win32Err = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(win32Err);
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "Update RG policy (JOBOBJECT_EXTENDED_LIMIT_INFORMATION) query phase failed. ErrorCode={0}",
            error);
        return error;
    }

    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpuRateInfo;
    if (!QueryInformationJobObject(jobHandle, JobObjectCpuRateControlInformation, &cpuRateInfo, sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION), nullptr))
    {
        auto win32Err = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(win32Err);
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "Update RG policy (JOBOBJECT_CPU_RATE_CONTROL_INFORMATION) query phase failed. ErrorCode={0}",
            error);
        return error;
    }

    // Update CpusetCpus policy
    SetupCpuSetCpusLimit(limitInfo, newCpusetCpus);

    // Update Memory policies
    SetupMemoryLimit(limitInfo, newMemoryInMB, newMemorySwapInMB, isHostedService);

    // Update CpuShares policy
    SetupCpuSharesLimit(cpuRateInfo, newCpuShares, isHostedService);

    // Set new/updated JO limits
    if (!::SetInformationJobObject(
        jobHandle,
        JobObjectExtendedLimitInformation,
        (void*)&limitInfo,
        sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)))
    {
        auto error = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "Update RG policy (JOBOBJECT_EXTENDED_LIMIT_INFORMATION) set phase failed. ErrorCode={0}",
            error);
        return error;
    }

    if (!::SetInformationJobObject(
        jobHandle,
        JobObjectCpuRateControlInformation,
        (void*)&cpuRateInfo,
        sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION)))
    {
        auto error = ErrorCode::FromWin32Error();
        WriteWarning(
            TraceType_Activator,
            Root.TraceId,
            "Update RG policy (JOBOBJECT_CPU_RATE_CONTROL_INFORMATION) set phase failed. ErrorCode={0}",
            error);
        return error;
    }

    return ErrorCode(ErrorCodeValue::Success);
}


ErrorCode ProcessActivator::Test_QueryJobObject(IProcessActivationContext const & activationContext, uint64 & cpuRate, uint64 & memoryLimit) const
{
    auto jobHandle = dynamic_cast<ProcessActivationContext const &>(activationContext).JobHandle->Value;

    JOBOBJECT_CPU_RATE_CONTROL_INFORMATION cpuRateInfo;
    if (!QueryInformationJobObject(jobHandle, JobObjectCpuRateControlInformation, &cpuRateInfo, sizeof(JOBOBJECT_CPU_RATE_CONTROL_INFORMATION), nullptr))
    {
        auto win32Err = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(win32Err);
        return error;
    }

    cpuRate = cpuRateInfo.CpuRate;

    JOBOBJECT_EXTENDED_LIMIT_INFORMATION memoryInfo;
    if (!QueryInformationJobObject(jobHandle, JobObjectExtendedLimitInformation, &memoryInfo, sizeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION), nullptr))
    {
        auto win32Err = ::GetLastError();
        auto error = ErrorCode::FromWin32Error(win32Err);
        return error;
    }

    if (HostingConfig::GetConfig().GovernOnlyMainMemoryForProcesses)
    {
        memoryLimit = memoryInfo.BasicLimitInformation.MaximumWorkingSetSize;
        if (memoryInfo.ProcessMemoryLimit != 0)
        {
            Common::Assert::TestAssert("Setting process memory limit not allowed when GovernOnlyMainMemoryForProcesses == true.");
        }
    }
    else
    {
        memoryLimit = memoryInfo.ProcessMemoryLimit;
        if (   memoryInfo.BasicLimitInformation.MinimumWorkingSetSize != 0
            || memoryInfo.BasicLimitInformation.MaximumWorkingSetSize != 0)
        {
            Common::Assert::TestAssert("Setting working set size limits not allowed when GovernOnlyMainMemoryForProcesses == false.");
        }
    }

    return ErrorCode(ErrorCodeValue::Success);
}
