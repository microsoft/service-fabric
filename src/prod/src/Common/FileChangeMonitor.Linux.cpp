// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include <sys/inotify.h>

using namespace Common;
using namespace std;

static StringLiteral const INotifyTrace("INotify");

class FileChangeMonitor2::INotify : TextTraceComponent<TraceTaskCodes::Common>
{
	DENY_COPY(INotify)

public:
	static INotify & GetSingleton();
    INotify();

    ErrorCode AddWatch(FileChangeMonitor2* fcm, string const & path, int & wd);
    ErrorCode RemoveWatch(FileChangeMonitor2 const* fcm, int wd);

private:
    void WaitForNextChange();
    void RetryWithDelay();
    static void OnAioComplete(sigval_t sigval);
    void ProcessNotifications();

	int inotifyFd_;
	unordered_map<int, unordered_map<FileChangeMonitor2 const*, FileChangeMonitor2WPtr>> watchMap_;
    RwLock lock_;
    struct aiocb aiocb_;
    ByteBuffer eventBuffer_;

	static BOOL InitFunction(PINIT_ONCE, PVOID, PVOID *); 
	static INIT_ONCE initOnce_;
	static Global<INotify> singleton_;
};

INIT_ONCE FileChangeMonitor2::INotify::initOnce_ = INIT_ONCE_STATIC_INIT;
Global<FileChangeMonitor2::INotify> FileChangeMonitor2::INotify::singleton_;

BOOL FileChangeMonitor2::INotify::InitFunction(PINIT_ONCE, PVOID, PVOID *)
{
	singleton_ = make_global<FileChangeMonitor2::INotify>();
	return TRUE;
}

FileChangeMonitor2::INotify & FileChangeMonitor2::INotify::GetSingleton()
{
	PVOID lpContext = NULL;
	Invariant(::InitOnceExecuteOnce(&initOnce_, InitFunction, NULL, &lpContext));
	return *singleton_;
}

FileChangeMonitor2::INotify::INotify() : inotifyFd_(inotify_init1(IN_CLOEXEC))
{
    ASSERT_IF(inotifyFd_ < 0, "inotify_init failed: {0}", ErrorCode::FromErrno());
    WriteInfo(INotifyTrace, "inotify_init created descriptor {0}", inotifyFd_);

    bzero(&aiocb_, sizeof(aiocb_));
    aiocb_.aio_fildes = inotifyFd_;
    aiocb_.aio_sigevent.sigev_notify = SIGEV_THREAD;
    aiocb_.aio_sigevent.sigev_notify_function = OnAioComplete;
    aiocb_.aio_sigevent.sigev_value.sival_ptr = this;

    eventBuffer_.resize(1024 * (sizeof(inotify_event) + 16));
    aiocb_.aio_buf = eventBuffer_.data();
    aiocb_.aio_nbytes = eventBuffer_.size();

    WaitForNextChange();
}

void FileChangeMonitor2::INotify::WaitForNextChange()
{
    if (aio_read(&aiocb_) == 0) return;

    auto error = ErrorCode::FromErrno();
    WriteError(INotifyTrace, "aio_read({0}) failed: {1}", inotifyFd_, error);
    RetryWithDelay();
}

void FileChangeMonitor2::INotify::RetryWithDelay()
{
    Threadpool::Post([this] { WaitForNextChange(); }, TimeSpan::FromMilliseconds(Random().Next(5000)));
}

void FileChangeMonitor2::INotify::OnAioComplete(sigval_t sigval)
{
    auto thisPtr = (FileChangeMonitor2::INotify*)(sigval.sival_ptr);
    thisPtr->ProcessNotifications();
}

void FileChangeMonitor2::INotify::ProcessNotifications()
{
    WriteNoise(INotifyTrace, "INotify::ProcessNotifications");

    auto aioError = aio_error(&aiocb_);
    if (aioError != 0)
    {
        auto error = ErrorCode::FromErrno(aioError);
        WriteError(INotifyTrace, "aio_error returned {0}", aioError);

        if (aioError == ECANCELED) return;

        RetryWithDelay();
        return;
    }

    auto resultSize = aio_return(&aiocb_);
    int processed = 0;
    int count = 0;

    while (processed < resultSize)
    {
        inotify_event const * event = (inotify_event const*)(eventBuffer_.data() + processed);
        processed += (sizeof(inotify_event) + event->len);
        ++count;
        WriteInfo(
                INotifyTrace,
                "event retrieved: wd = {0}, mask = {1:x}, name = '{2}', count = {3}",
                event->wd,
                event->mask,
                (event->len > 0) ? string(event->name) : "",
                count);

        unordered_map<FileChangeMonitor2 const *, FileChangeMonitor2WPtr> subscribers;
        {
            AcquireReadLock grab(lock_);

            auto iter = watchMap_.find(event->wd);
            if (iter == watchMap_.cend())
            {
                WriteInfo(INotifyTrace, "wd {0} not found in map", event->wd);
                continue;
            }

            subscribers = iter->second;
        }

        for (auto const & subscriber : subscribers)
        {
            auto fcmSPtr = subscriber.second.lock();
            if (fcmSPtr)
            {
                fcmSPtr->DispatchChange();
            }
        }
    }

    WaitForNextChange();
}

ErrorCode FileChangeMonitor2::INotify::AddWatch(FileChangeMonitor2* fcm, const string &path, int & wd)
{
    ErrorCode error;

    AcquireWriteLock grab(lock_);

    auto mask = fcm->supportFileDelete_? (IN_MODIFY|IN_DELETE_SELF|IN_MOVE_SELF) : IN_MODIFY;
    wd = inotify_add_watch(inotifyFd_, path.c_str(), mask);
    if (wd < 0)
    {
        error = ErrorCode::FromErrno();
        WriteError(INotifyTrace, "inotify_add_watch failed: {0}", error);
        return error;
    }

    WriteInfo(INotifyTrace, "inotify_add_watch added {0}, fcm = {1}", wd, TextTracePtr(fcm));

    auto iter = watchMap_.find(wd);
    if (iter == watchMap_.cend())
    {
        unordered_map<FileChangeMonitor2 const *, FileChangeMonitor2WPtr> newWatch;
        newWatch.emplace(make_pair(fcm, fcm->shared_from_this()));
        watchMap_.emplace(make_pair(wd, move(newWatch)));
    }
    else {
        iter->second.emplace(make_pair(fcm, fcm->shared_from_this()));
    }

    return error;
}

ErrorCode FileChangeMonitor2::INotify::RemoveWatch(FileChangeMonitor2 const* fcm, int wd)
{
    AcquireWriteLock grab(lock_);

    auto iter = watchMap_.find(wd);
    if (iter == watchMap_.cend())
    {
        WriteError(INotifyTrace, "wd {0} not found for remove", wd);
        return ErrorCodeValue::NotFound;
    }

    if (iter->second.erase(fcm) == 1)
    {
        WriteInfo(INotifyTrace, "fcm {0} removed", TextTracePtr(fcm));
    }
    else
    {
        WriteWarning(INotifyTrace, "fcm {0} not found for remove", TextTracePtr(fcm));
    }

    if (!iter->second.empty()) return ErrorCode();

    watchMap_.erase(iter);
    auto retval = inotify_rm_watch(inotifyFd_, wd);
    if (retval < 0)
    {
        auto error = ErrorCode::FromErrno();
        WriteWarning(INotifyTrace, "inotify_rm_watch({0}) failed: {1}", wd, error);
        return error; 
    }

    WriteInfo(INotifyTrace, "inotify_rm_watch removed {0}", wd);
    return ErrorCode();
}

static StringLiteral const TraceType("FileChangeMonitor");

FileChangeMonitor2SPtr FileChangeMonitor2::Create(wstring const & filePath, bool supportFileDelete)
{
    return make_shared<FileChangeMonitor2>(filePath, supportFileDelete);
}

FileChangeMonitor2::FileChangeMonitor2(wstring const & filePath, bool supportFileDelete)
    : StateMachine(Created),
    wd_(),
    wdInitialized_(false),
    supportFileDelete_(supportFileDelete),
    callback_(nullptr),
    traceId_(),
    failedEvent_()
{
    StringWriter writer(traceId_);
    writer.Write("{0}", reinterpret_cast<void*>(this));
    StringUtility::Utf16ToUtf8(filePath, filePath_);

    WriteInfo(TraceType, traceId_, "constructed");
}

FileChangeMonitor2::~FileChangeMonitor2()
{
    WriteInfo(TraceType, traceId_, "destructing");
    if (GetState() != StateMachine::Aborted)
    {
        Abort();
    }
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

    error = InitializeChangeHandle();
    if (!error.IsSuccess()) 
    { 
        this->OnFailure(error);
        return error; 
    }

    error = this->TransitionToOpened();
    if (!error.IsSuccess()) return error;

    {
        AcquireWriteLock grab(Lock);
        callback_ = callback;
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
    this->CloseChangeHandle();
}

ErrorCode FileChangeMonitor2::InitializeChangeHandle()
{
    AcquireWriteLock lock(this->Lock);

    auto error = INotify::GetSingleton().AddWatch(this, filePath_, wd_);
    if (!error.IsSuccess())
    {
        WriteError(TraceType, traceId_, "AddWatch failed: {1}", filePath_, error);
        return error; 
    }

    wdInitialized_ = true;
    WriteInfo(TraceType, traceId_, "path = {0}, watch descriptor = {1}", filePath_, wd_);
    return ErrorCode(ErrorCodeValue::Success);
}

void FileChangeMonitor2::CloseChangeHandle()
{
    AcquireWriteLock lock(this->Lock);

    if (wdInitialized_)
    {
        auto error = INotify::GetSingleton().RemoveWatch(this, wd_);
        wdInitialized_ = false;
        WriteInfo(TraceType, traceId_, "RemoveWatch returned {0}", error);
    }

    callback_ = nullptr;
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
        WriteNoise(TraceType, traceId_, "Invoking callback");
        snap(this->shared_from_this());
    }
}

void FileChangeMonitor2::OnFailure(ErrorCode const error)
{
    WriteError(
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
