// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType("FileChangeMonitor");

const int FileChangeMonitor2::FileReadMaxRetryIntervalInMillis = 1000;
const int FileChangeMonitor2::FileReadMaxRetryCount = 10;

FileChangeMonitor2SPtr FileChangeMonitor2::Create(wstring const & filePath, bool const supportFileDelete)
{
    return make_shared<FileChangeMonitor2>(filePath, supportFileDelete);
}

FileChangeMonitor2::FileChangeMonitor2(wstring const & filePath, bool const supportFileDelete)
    : StateMachine(Created),
    filePath_(filePath),
    supportFileDelete_(supportFileDelete),
    directory_(Path::GetDirectoryName(filePath)),
    directoryChangeHandle_(INVALID_HANDLE_VALUE),
    lastWriteTime_(DateTime::Zero),
    wait_(nullptr),
    callback_(nullptr),
    traceId_(),
    failedEvent_()
{
    StringWriter writer(traceId_);
    writer.Write("<{0}>", reinterpret_cast<void*>(this));
}

FileChangeMonitor2::~FileChangeMonitor2()
{
    ASSERT_IF(this->wait_ != nullptr, "The wait object must not be valid in the dtor.");
    ASSERT_IF(this->directoryChangeHandle_ != INVALID_HANDLE_VALUE, "The change handle must not be valid in the dtor.");
}

FileChangeMonitor2::FailedEventHHandler FileChangeMonitor2::RegisterFailedEventHandler(
    FileChangeMonitor2::FailedEventHandler const & handler)
{
    return this->failedEvent_.Add(handler);
}

bool FileChangeMonitor2::UnregisterFailedEventHandler(
    FileChangeMonitor2::FailedEventHHandler const & hHandler)
{
    return this->failedEvent_.Remove(hHandler);
}


ErrorCode FileChangeMonitor2::Open(ChangeCallback const & callback)
{
    auto error = this->TransitionToOpening();
    if (!error.IsSuccess()) { return error; }

    error = UpdateLastWriteTime();
    if (!error.IsSuccess())
    { 
        this->OnFailure(error);
        return error; 
    }

    error = InitializeChangeHandle();
    if (!error.IsSuccess()) 
    { 
        this->OnFailure(error);
        return error; 
    }

    error = WaitForNextChange();
    if (!error.IsSuccess()) 
    { 
        this->OnFailure(error);
        return error; 
    }

    error = this->TransitionToOpened();
    if (error.IsSuccess())
    {
        AcquireWriteLock lock(this->Lock);
        this->callback_ = callback;
        if (this->GetState_CallerHoldsLock() == FileChangeMonitor2::Opened)
        {
            ::SetThreadpoolWait(this->wait_, this->directoryChangeHandle_, NULL);
        }
    }
    else
    {
        this->OnFailure(error);
    }

    return error;
}

void FileChangeMonitor2::Close()
{
    Abort();
}

void FileChangeMonitor2::OnAbort()
{
    this->failedEvent_.Close();
    this->CancelThreadpoolWait();
    this->CloseChangeHandle();
    {
        AcquireWriteLock lock(this->Lock);
        this->callback_ = nullptr;
    }
}

ErrorCode FileChangeMonitor2::UpdateLastWriteTime()
{
    DateTime lastWriteTime;
    auto error = GetLastWriteTime(filePath_, lastWriteTime);
    if (error.IsSuccess())
    {
        AcquireWriteLock lock(this->Lock);
        this->lastWriteTime_ = lastWriteTime;
    }

    return error;
}

ErrorCode FileChangeMonitor2::InitializeChangeHandle()
{
    AcquireWriteLock lock(this->Lock);

    DWORD notifyFilter;
    if (this->supportFileDelete_)
    {
        notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_FILE_NAME;
    }
    else 
    {
        notifyFilter = FILE_NOTIFY_CHANGE_LAST_WRITE;
    }

    this->directoryChangeHandle_ = ::FindFirstChangeNotification(
        directory_.c_str(),
        FALSE,
        notifyFilter);
    if (this->directoryChangeHandle_ == INVALID_HANDLE_VALUE)
    {
        return OnWin32Error(L"FindFirstChangeNotification");
    }
    else
    {
        return ErrorCode(ErrorCodeValue::Success);
    }
}

ErrorCode FileChangeMonitor2::WaitForNextChange()
{
    AcquireWriteLock lock(this->Lock);

    BOOL success = ::FindNextChangeNotification(this->directoryChangeHandle_);
    if (success == FALSE)
    {
        return OnWin32Error(L"FindNextChangeNotification");
    }

    ASSERT_IFNOT(this->wait_ == nullptr, "The wait object must be null.");

    this->wait_ = ::CreateThreadpoolWait(&Threadpool::WaitCallback<FileChangeMonitor2>, this, NULL);
    if (this->wait_ == nullptr)
    {
        return OnWin32Error(L"CreateThreadpoolWait");
    }

    return ErrorCode(ErrorCodeValue::Success);
}

void FileChangeMonitor2::CancelThreadpoolWait()
{
    AcquireWriteLock lock(this->Lock);

    if (this->wait_ != nullptr)
    {
        ::SetThreadpoolWait(this->wait_, nullptr, nullptr); 
        ::WaitForThreadpoolWaitCallbacks(wait_, TRUE /*cancel pending callbacks*/);
        ::CloseThreadpoolWait(this->wait_);
        this->wait_ = nullptr;
    }
}

void FileChangeMonitor2::CloseChangeHandle()
{
    AcquireWriteLock lock(this->Lock);
    if (this->directoryChangeHandle_ != INVALID_HANDLE_VALUE)
    {
        BOOL success = ::FindCloseChangeNotification(directoryChangeHandle_);
        if (success == FALSE)
        {
            this->OnWin32Error(L"FindCloseChangeNotification").ReadValue();
        }

        this->directoryChangeHandle_ = INVALID_HANDLE_VALUE;
    }
}

void FileChangeMonitor2::Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_WAIT, TP_WAIT_RESULT)
{
    FileChangeMonitor2SPtr thisSPtr = reinterpret_cast<FileChangeMonitor2*>(callbackParameter)->shared_from_this();
    DisassociateCurrentThreadFromCallback(pci);
    thisSPtr->CancelThreadpoolWait();
    thisSPtr->OnWaitCompleted();
}

void FileChangeMonitor2::OnWaitCompleted()
{
    // try to transition to updating state
    auto error = this->TransitionToUpdating();
    if (!error.IsSuccess())
    {
        TraceWarning(
            TraceTaskCodes::Common,
            TraceType,
            traceId_,
            "Error while transitioning to updating; Ignoring the notification");

        this->OnFailure(error);
        return;
    }

    bool myChange = false;
    error = this->IsMyChange(myChange);
    if (!error.IsSuccess())
    {
        this->OnFailure(error);
        return;
    }

    if (myChange)
    {
        this->DispatchChange();
    }
    else
    {
        TraceNoise(
            TraceTaskCodes::Common,
            TraceType,
            traceId_,
            "this->lastWriteTime >= File::LastWriteTime. Ignoring this old notification");
    }

    error = WaitForNextChange();
    if (!error.IsSuccess()) 
    { 
        this->OnFailure(error);
        return; 
    }

    error = this->TransitionToOpened();
    if (error.IsSuccess())
    {
        AcquireWriteLock lock(this->Lock);
        if (this->GetState_CallerHoldsLock() == FileChangeMonitor2::Opened)
        {
            ::SetThreadpoolWait(this->wait_, this->directoryChangeHandle_, NULL);
        }
    }
    else
    {
        this->OnFailure(error);
    }
}

ErrorCode FileChangeMonitor2::IsMyChange(__out bool & myChange)
{
    if (this->supportFileDelete_ && !File::Exists(filePath_))
    {
        AcquireWriteLock lock(this->Lock);

        TraceNoise(
            TraceTaskCodes::Common,
            TraceType,
            traceId_,
            "file no longer exists");

        myChange = TRUE;
        return ErrorCode(ErrorCodeValue::Success);
    }
    else
    {
        DateTime lastWriteTime;
        auto error = GetLastWriteTime(filePath_, lastWriteTime);

        if (error.IsSuccess())
        {
            AcquireWriteLock lock(this->Lock);

            TraceNoise(
                TraceTaskCodes::Common,
                TraceType,
                traceId_,
                "this->lastWriteTime:{0} File::lastWriteTime:{1}", this->lastWriteTime_, lastWriteTime);

            myChange = (this->lastWriteTime_.Ticks < lastWriteTime.Ticks);
            this->lastWriteTime_ = lastWriteTime;
        }

        return error;
    }
}

void FileChangeMonitor2::DispatchChange()
{
    FileChangeMonitor2::ChangeCallback snap;
    {
        AcquireReadLock lock(this->Lock);
        snap = this->callback_;
    }

    if (snap)
    {
        TraceNoise(
            TraceTaskCodes::Common,
            TraceType,
            traceId_,
            "Invoking callback");

        snap(this->shared_from_this());
    }
}

void FileChangeMonitor2::OnFailure(ErrorCode const error)
{
    TraceWarning(
        TraceTaskCodes::Common,
        TraceType,
        traceId_,
        "FileChangeMonitor failed file {0} with ErrorCode {1}.",
        filePath_,
        error);

    // it is possible for the state to transition to 'Aborted' anywhere during the processing of the notification.
    // For such cases, Failed Event should not be fired.
    if(StateMachine::Aborted != this->GetState())
    {
        this->TransitionToFailed().ReadValue();
        this->failedEvent_.Fire(error);
    }
}

ErrorCode FileChangeMonitor2::OnWin32Error(wstring const & win32FunctionName)
{
    DWORD winError = ::GetLastError();
    ErrorCode error = ErrorCode::FromWin32Error(winError);

    TraceError(
        TraceTaskCodes::Common,
        TraceType,
        traceId_,
        "{0}() failed with {1} ({2}) for file {3}.",
        win32FunctionName,
        error,
        winError,
        filePath_);
    return error;
}

ErrorCode FileChangeMonitor2::GetLastWriteTime(wstring const & filePath, __out DateTime & lastWriteTime)
{
    Random rand;
    ErrorCode lastError = ErrorCode(ErrorCodeValue::OperationFailed);
    for(int i = 0; i < FileChangeMonitor2::FileReadMaxRetryCount; i++)
    {
        auto error = File::GetLastWriteTime(filePath, lastWriteTime);
        if (error.IsSuccess())
        {
            return error;
        }
        else
        {
            lastError.Overwrite(error);
            ::Sleep(rand.Next(FileChangeMonitor2::FileReadMaxRetryIntervalInMillis));
        }
    }

    return lastError;
}
