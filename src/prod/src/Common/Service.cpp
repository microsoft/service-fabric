// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"


#include "Common/WaitHandle.h"
#include "Common/Service.h"

using namespace Common;

DWORD ONESECONDINMILLISECONDS = 1000;
DWORD TENSECONDSINMILLISECONDS = 10000;
ServiceBase* ServiceBase::storedService = nullptr;

// Rather than port cxlrtl\thread.* for just this one function, I'm going to convert it to an old-fashioned static thread routine.
static bool stopThread = false;
static AutoResetEvent wakeUpEvent(false);  // signalled after a new control code has been added

static DWORD WINAPI ControlHandlerThread(void * param)
{
    ServiceBase* service = (ServiceBase *)param;

    while (!stopThread)
    {
        DWORD currentControlCode = service->GetControlCode();
        if (currentControlCode > 0)
        {
            // Found a control code. Process it.
            service->ProcessControlCode(currentControlCode);
        }
        else
        {
            // No more control codes. If we were signaled to terminate
            // earlier, do so now. Otherwise, sleep.
            if (stopThread)
                break;

            wakeUpEvent.WaitOne(INFINITE);
        }
    }

    return 0;
}

static void WakeUpControlHandler()
{
    wakeUpEvent.Set();
}

static void SignalStopThread()
{
    stopThread = true;
    WakeUpControlHandler();
}

ServiceBase::~ServiceBase()
{
    if (controlCodeHandlerThread_ != nullptr)
    {
        HANDLE tempCopy = controlCodeHandlerThread_;
        controlCodeHandlerThread_ = nullptr;  // set this to null so another thread won't use it during destruction

        SignalStopThread();

        WaitForSingleObject( tempCopy, INFINITE );

        CloseHandle( tempCopy );
    }
}

void ServiceBase::SetServiceState(
    DWORD currentState,
    DWORD checkPoint,
    DWORD waitHint,
    DWORD errorCode)
{
    //
    // If there is no error, we set dwWin32ExitCode to NO_ERROR
    // and dwServiceSpecificExitCode is ignored.
    // If there is an error, we set dwWin32ExitCode to
    // ERROR_SERVICE_SPECIFIC_ERROR
    //
    DWORD error = (errorCode == NO_ERROR) ?
    NO_ERROR :
             ERROR_SERVICE_SPECIFIC_ERROR;

    SERVICE_STATUS serviceStatus =
    {
        SERVICE_WIN32_OWN_PROCESS,
        currentState,
        SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN | SERVICE_ACCEPT_PRESHUTDOWN,
        error,
        errorCode,
        checkPoint,
        waitHint
    };

    if (this->serviceStopped_)
    {
        return;
    }

    if (currentState == SERVICE_STOPPED)
    {
        this->serviceStopped_ = true;
    }

    this->currentState_ = currentState;
    this->checkPoint_ = checkPoint;

    CHK_WBOOL(SetServiceStatus(handle_, &serviceStatus));
}

void ServiceBase::SetControlCode(DWORD newControlCode)
{
    AcquireWriteLock lockHolder(controlCodeLock_);
    controlCode_ = newControlCode;

    WakeUpControlHandler();
}

DWORD ServiceBase::GetControlCode()
{
    DWORD currentControlCode = 0;

    AcquireWriteLock lockHolder(controlCodeLock_);
    currentControlCode = controlCode_;
    controlCode_ = 0;

    return currentControlCode;
}

void ServiceBase::CreateHandlerThread()
{
    if (controlCodeHandlerThread_ == nullptr)
    {
        controlCodeHandlerThread_ = ::CreateThread(
            NULL, // security attributes
            0,      // stack size
            ControlHandlerThread,
            this,
            0,
            NULL);
    }
}

void ServiceBase::Run(ServiceBase& service)
{
    storedService = &service;
    SERVICE_TABLE_ENTRY ServiceTable[] =
    {
        { const_cast<::LPWSTR>(service.serviceName_.c_str()), service.ServiceMain },
        { nullptr, nullptr },
    };

    CHK_WBOOL(StartServiceCtrlDispatcher(ServiceTable));
}

void WINAPI ServiceBase::ServiceMain(DWORD argc, __in_ecount(argc) ::LPWSTR* argv)
{
    StringCollection args(argv + 0, argv + argc);

    storedService->handle_ = RegisterServiceCtrlHandlerEx(
        storedService->serviceName_.c_str(),
        ServiceHandler,
        reinterpret_cast<::LPVOID>(storedService));

    CHK_HANDLE_VALID(reinterpret_cast<::HANDLE>(storedService->handle_));
    storedService->checkPoint_ = 1;

#if DBG
    // Check to see if PauseOnStart is enabled.
    HKEY paramsKey;
    std::wstring subkeyName(L"System\\CurrentControlSet\\Services\\");
    subkeyName.append(storedService->serviceName_);
    subkeyName.append(L"\\Parameters");

    if (RegOpenKeyEx(HKEY_LOCAL_MACHINE, subkeyName.c_str(), 0, KEY_READ, &paramsKey) == ERROR_SUCCESS)
    {
        DWORD secondsToPause = 0;
        DWORD continueAfterDebugger = 0;
        DWORD valueSize = sizeof(DWORD);
        DWORD valueType;

        RegQueryValueEx(paramsKey, L"ContinueAfterDebuggerAttached", NULL, &valueType, (PBYTE)&continueAfterDebugger, &valueSize);

        valueSize = sizeof(DWORD);
        if (RegQueryValueEx(paramsKey, L"PauseOnStart", NULL, &valueType, (PBYTE)&secondsToPause, &valueSize) == ERROR_SUCCESS)
        {
            for (DWORD i = 0; i < secondsToPause * 10; ++i)
            {
                if ((i % 10) == 0)
                    storedService->SetServiceState(SERVICE_START_PENDING, storedService->checkPoint_++, 100000);

                if (continueAfterDebugger && IsDebuggerPresent())
                    break;

                Sleep(100);
            }
        }

        RegCloseKey(paramsKey);
    }
#endif

    storedService->CreateHandlerThread();
    storedService->SetServiceState(SERVICE_START_PENDING, storedService->checkPoint_++, storedService->ServiceTimeout);

    //
    // Read note in ServiceBase.cs regarding executing OnStart in a seperate
    // thread pool thread.
    // For now, assume OnStart is going to do sync stuff which will not get
    // affected.
    //
    storedService->OnStart(args);

    storedService->SetServiceState(SERVICE_RUNNING);
}

DWORD WINAPI ServiceBase::ServiceHandler(DWORD dwControl, DWORD, ::LPVOID, ::LPVOID lpContext)
{
    //
    // EventType and EventData are unsupported as yet
    //

    try
    {
        ServiceBase* service = reinterpret_cast<ServiceBase*>(lpContext);

        // no control codes are accepted if we are about to stop
        if (service->currentState_ == SERVICE_STOP_PENDING)
        {
            return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
        }
        if (service->currentState_ == SERVICE_STOPPED)
        {
            return ERROR_SERVICE_NOT_ACTIVE;
        }

        // This shouldn't happen but we can't process control codes if the handler thread is down
        if (service->controlCodeHandlerThread_ == nullptr)
        {
            return ERROR_SERVICE_CANNOT_ACCEPT_CTRL;
        }

        switch (dwControl)
        {
        case SERVICE_CONTROL_PRESHUTDOWN:
        case SERVICE_CONTROL_SHUTDOWN:
        case SERVICE_CONTROL_STOP:
            service->SetServiceState(SERVICE_STOP_PENDING, 1, service->ServiceTimeout);
            // pass through
        case SERVICE_CONTROL_PAUSE:
        case SERVICE_CONTROL_CONTINUE:
            // for all control codes with significant processing, save the control
            // code and wake up the background thread
            service->SetControlCode(dwControl);
            break;

        case SERVICE_CONTROL_INTERROGATE:
            service->SetServiceState(service->currentState_);
            break;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;

        }
    }
    catch (std::system_error & e)
    {
        return e.code().value();
    }

    return ERROR_SUCCESS;
}

DWORD ServiceBase::ProcessControlCode(DWORD controlCode)
{
    try
    {
        bool shutdown = false;

        switch (controlCode)
        {
        case SERVICE_CONTROL_PRESHUTDOWN:
            OnPreshutdown();
            break;

        case SERVICE_CONTROL_SHUTDOWN:
            shutdown = true;
            // fall through

        case SERVICE_CONTROL_STOP:
            OnStop(shutdown);
            SetServiceState(SERVICE_STOPPED);
            SignalStopThread();
            //
            // after this, the handle is no longer valid
            //
            handle_ = nullptr;
            break;

        case SERVICE_CONTROL_PAUSE:
            OnPause();
            SetServiceState(SERVICE_PAUSED);
            break;

        case SERVICE_CONTROL_CONTINUE:
            OnContinue();
            SetServiceState(SERVICE_RUNNING);
            break;

        default:
            return ERROR_CALL_NOT_IMPLEMENTED;

        }
    }
    catch (std::system_error & e)
    {
        return e.code().value();
    }

    return ERROR_SUCCESS;
}

ErrorCode ServiceController::Install(const std::wstring& displayName, const std::wstring& path)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::CreateService(
        scmHandle.Handle,
        serviceName_.c_str(),
        displayName.c_str(),
        0,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START,
        SERVICE_ERROR_NORMAL,
        path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr));
    if (serviceHandle.Handle)
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCode::FromWin32Error(::GetLastError());
}

ErrorCode ServiceController::InstallAuto(const std::wstring& displayName, const std::wstring& path)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::CreateService(
        scmHandle.Handle,
        serviceName_.c_str(),
        displayName.c_str(),
        0,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr,
        nullptr));
    if (serviceHandle.Handle)
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCode::FromWin32Error(::GetLastError());
}

ErrorCode ServiceController::InstallKernelDriver(const std::wstring& displayName, const std::wstring& path)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::CreateService(
        scmHandle.Handle,
        serviceName_.c_str(),
        displayName.c_str(),
        0,
        SERVICE_KERNEL_DRIVER,//
        SERVICE_AUTO_START,
        SERVICE_ERROR_NORMAL,
        path.c_str(),
        nullptr,
        nullptr,
        nullptr,
        nullptr, // what account should the service run under?
        nullptr));
    if (serviceHandle.Handle)
    {
        return ErrorCodeValue::Success;
    }

    return ErrorCode::FromWin32Error(::GetLastError());
}

ErrorCode ServiceController::Uninstall()
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(
        scmHandle.Handle,
        serviceName_.c_str(),
        DELETE));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!DeleteService(serviceHandle.Handle))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::Start()
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_START));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!StartService(serviceHandle.Handle, 0, nullptr))
    {
        auto err = ::GetLastError();
        if (err == ERROR_SERVICE_ALREADY_RUNNING)
        {
            return ErrorCodeValue::Success;
        }
        return ErrorCode::FromWin32Error(err);
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::Stop()
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ALL_ACCESS));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_STOP));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ::SERVICE_STATUS status;

    if (!ControlService(serviceHandle.Handle, SERVICE_CONTROL_STOP, &status))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::GetServiceSIDType(SERVICE_SID_INFO& sidInfo)
{
    DWORD bytesNeeded;
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_QUERY_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!QueryServiceConfig2W(serviceHandle.Handle, SERVICE_CONFIG_SERVICE_SID_INFO, (PBYTE)&sidInfo, sizeof(sidInfo), &bytesNeeded))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetPreshutdownTimeout(DWORD preshutdownTimeout)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    SERVICE_PRESHUTDOWN_INFO preshutdownInfo = { preshutdownTimeout };
    if (!ChangeServiceConfig2W(serviceHandle.Handle, SERVICE_CONFIG_PRESHUTDOWN_INFO, (PBYTE)&preshutdownInfo))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetServiceSIDType(const SERVICE_SID_INFO& sidInfo)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!ChangeServiceConfig2W(serviceHandle.Handle, SERVICE_CONFIG_SERVICE_SID_INFO, (PBYTE)&sidInfo))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::GetState(DWORD & state)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_QUERY_STATUS));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    SERVICE_STATUS status;
    if (!QueryServiceStatus(serviceHandle.Handle, &status))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }
    state = status.dwCurrentState;
    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetServiceAccount(std::wstring const & accountName, SecureString const & password)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!ChangeServiceConfig(
        serviceHandle.Handle,
        SERVICE_NO_CHANGE,
        SERVICE_NO_CHANGE,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        NULL,
        (accountName != L"") ? accountName.c_str() : NULL,
        (!password.IsEmpty()) ? password.GetPlaintext().c_str() : NULL,
        NULL))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetStartupType(DWORD startupType)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    if (!ChangeServiceConfig(
        serviceHandle.Handle,
        SERVICE_NO_CHANGE,
        startupType,
        SERVICE_NO_CHANGE,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL,
        NULL))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetStartupTypeDelayedAuto()
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    SERVICE_DELAYED_AUTO_START_INFO info;
    info.fDelayedAutostart = TRUE;

    if (!ChangeServiceConfig2(
        serviceHandle.Handle,
        SERVICE_CONFIG_DELAYED_AUTO_START_INFO,
        &info))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetServiceDescription(std::wstring const & description)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_CHANGE_CONFIG));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    SERVICE_DESCRIPTION serviceDescription;
    serviceDescription.lpDescription = const_cast<LPWSTR>(description.c_str());

    if (!ChangeServiceConfig2(
        serviceHandle.Handle,
        SERVICE_CONFIG_DESCRIPTION,
        &serviceDescription))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

ErrorCode ServiceController::SetServiceFailureActionsForRestart(DWORD const & delayInSeconds, DWORD const & resetPeriodInDays)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_ALL_ACCESS));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    SERVICE_FAILURE_ACTIONS serviceFailureActions;
    SC_ACTION failureActions[3];
    DWORD delayInMilliSeconds = delayInSeconds * 1000;

    failureActions[0].Type = SC_ACTION_RESTART;
    failureActions[0].Delay = delayInMilliSeconds;
    failureActions[1].Type = SC_ACTION_RESTART;
    failureActions[1].Delay = delayInMilliSeconds;
    failureActions[2].Type = SC_ACTION_RESTART;
    failureActions[2].Delay = delayInMilliSeconds;

    serviceFailureActions.dwResetPeriod = resetPeriodInDays * 24 * 60 * 60;
    serviceFailureActions.lpCommand = NULL;
    serviceFailureActions.lpRebootMsg = NULL;
    serviceFailureActions.cActions = 3;
    serviceFailureActions.lpsaActions = failureActions;

    if (!ChangeServiceConfig2(
        serviceHandle.Handle,
        SERVICE_CONFIG_FAILURE_ACTIONS,
        &serviceFailureActions))
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    return ErrorCodeValue::Success;
}

DWORD ServiceController::GetNotifyMask(ServiceControllerStatus::Enum const status)
{
    switch (status)
    {
    case ServiceControllerStatus::Stopped:
        return SERVICE_NOTIFY_STOPPED;
    case ServiceControllerStatus::StartPending:
        return SERVICE_NOTIFY_START_PENDING;
    case ServiceControllerStatus::StopPending:
        return SERVICE_NOTIFY_STOP_PENDING;
    case ServiceControllerStatus::Running:
        return SERVICE_NOTIFY_RUNNING;
    case ServiceControllerStatus::ContinuePending:
        return SERVICE_NOTIFY_CONTINUE_PENDING;
    case ServiceControllerStatus::PausePending:
        return SERVICE_NOTIFY_PAUSE_PENDING;
    case ServiceControllerStatus::Paused:
        return SERVICE_NOTIFY_PAUSED;
    default:
        Assert::CodingError("Unknown service controller status {0}", static_cast<short>(status));
    }
}

ErrorCode ServiceController::WaitForStatus(ServiceControllerStatus::Enum const expectedStatusEnum, TimeSpan const & timeout)
{
    ServiceHandle scmHandle(::OpenSCManager(nullptr, nullptr, SC_MANAGER_ENUMERATE_SERVICE));
    if (!scmHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    ServiceHandle serviceHandle(::OpenService(scmHandle.Handle, serviceName_.c_str(), SERVICE_QUERY_STATUS));
    if (!serviceHandle.Handle)
    {
        return ErrorCode::FromWin32Error(::GetLastError());
    }

    DateTime startTime = DateTime::Now();
    int64 totalTimeInMilliseconds = timeout.TotalMilliseconds();
    DWORD sleepTime = (DWORD)totalTimeInMilliseconds / 10;
    if (sleepTime > TENSECONDSINMILLISECONDS)
    {
        sleepTime = TENSECONDSINMILLISECONDS;
    }
    else if (sleepTime < ONESECONDINMILLISECONDS)
    {
        sleepTime = ONESECONDINMILLISECONDS;
    }

    while (startTime.AddWithMaxValueCheck(timeout).Ticks > DateTime::Now().Ticks)
    {
        SERVICE_STATUS status;
        if (!QueryServiceStatus(serviceHandle.Handle, &status))
        {
            return ErrorCode::FromWin32Error(::GetLastError());
        }

        DWORD currentStatus = status.dwCurrentState;
        DWORD expectedStatus = static_cast<DWORD>(expectedStatusEnum);
        if (currentStatus == expectedStatus)
        {
            return ErrorCodeValue::Success;
        }
        ::Sleep(sleepTime);
    }

    return ErrorCodeValue::Timeout;
}

ErrorCode ServiceController::WaitForServiceStop(TimeSpan const & timeout)
{
    return WaitForStatus(ServiceControllerStatus::Stopped, timeout);
}

ErrorCode ServiceController::WaitForServiceStart(TimeSpan const & timeout)
{
    return WaitForStatus(ServiceControllerStatus::Running, timeout);
}

bool ServiceController::IsServiceRunning()
{
    DWORD serviceState = 0;
    ErrorCode errorCode(ErrorCodeValue::Success);

    errorCode = this->GetState(serviceState);
    if (!errorCode.IsSuccess())
    {
        return false;
    }

    return serviceState == SERVICE_RUNNING;
}

ErrorCode ServiceController::StartServiceByName(std::wstring const & serviceName, bool waitForServiceStart, TimeSpan const & timeout)
{
    ServiceController sc(serviceName);
    auto error = sc.Start();
    if (error.IsWin32Error(ERROR_SERVICE_ALREADY_RUNNING) || error.IsSuccess())
    {
        if (waitForServiceStart)
        {
            error = sc.WaitForServiceStart(timeout);
        }
    }

    return error;
}

ErrorCode ServiceController::StopServiceByName(std::wstring const & serviceName, bool waitForServiceStop, TimeSpan const & timeout)
{
    ServiceController sc(serviceName);
    auto error = sc.Stop();
    if (error.IsSuccess())
    {
        if (waitForServiceStop)
        {
            error = sc.WaitForServiceStop(timeout);
        }
    }

    return error;
}
