// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace std;

static StringLiteral const TraceType = "ProcessWait";

namespace Common
{
    class ProcessExitCache : public enable_shared_from_this<ProcessExitCache>, TextTraceComponent<TraceTaskCodes::Common>
    {
    public:
        void AddEvent(pid_t pid, int status);
        bool FindEvent(pid_t pid, int& status);
        void RemoveEvent(pid_t pid);

        size_t Test_Size() const;
        void Test_Reset();

    private:
        void PurgeIfOverSizeLimit();
        void ScheduleToPurgeOldEvents();
        void PurgeOldEvents();
        void PurgeEvent();
        void ReconstructPurgeQueue();

        struct ProcessExitEvent
        {
            ProcessExitEvent(pid_t pid, int status, StopwatchTime time) : Pid(pid), Status(status), Time(time) {}
            pid_t Pid;
            int Status;
            StopwatchTime Time;
        };
        map<pid_t, ProcessExitEvent> exitEvents_; 

        struct CompareExitByTime
        {
            bool operator() (ProcessExitEvent const & e1, ProcessExitEvent const & e2)
            {
                return e1.Time > e2.Time;
            }
        };
        using ProcessEventPair = pair<pid_t, ProcessExitEvent>;
        using PurgeQueue = priority_queue<ProcessExitEvent, vector<ProcessExitEvent>, CompareExitByTime>;
        PurgeQueue purgeQueue_; 
    };

    class ProcessWaitImpl : public TimedAsyncOperation, TextTraceComponent<TraceTaskCodes::Common>
    {
    public:
        ProcessWaitImpl(Handle && handle, pid_t pid, ProcessWait::WaitCallback const & waitCallback, TimeSpan timeout);
        void TryCompleteAsync(AsyncOperationSPtr const & thisSPtr, int exitCode);

        static void InitOnce();
        static Global<RwLock> Lock;

        static size_t Test_CacheSize();
        static void Test_Reset();

    protected:
        void OnStart(AsyncOperationSPtr const & thisSPtr) override;
        void OnCompleted() override;

    private:
        static BOOL InitWaitLoop(PINIT_ONCE, PVOID, PVOID*);
        static void* WaitLoop(void*);
        static INIT_ONCE initOnce_;

        static void SigChildHandler(int sig, siginfo_t *si, void*);
        static int pipeFd_[2];

        static int TryGetExitCode(pid_t pid, int & status);
        static void AddWaiter(pid_t pid, AsyncOperationSPtr const & thisSPtr);
        static void RemoveWaiter(pid_t pid, AsyncOperationSPtr const & thisSPtr);
        static size_t TryCompleteWaiters();
        static Global<map<pid_t, set<AsyncOperationSPtr>>> waiters_;
        static Global<ProcessExitCache> processExitCache_;

        Handle handle_;
        pid_t pid_;
        ProcessWait::WaitCallback callback_;
        int exitCode_;
    };

#if DBG
    struct SigChildInfo
    {
        int signo;
        int code;
        int status;
        pid_t pid;
    };
#else
    typedef byte SigChildInfo;
#endif

    INIT_ONCE ProcessWaitImpl::initOnce_ = INIT_ONCE_STATIC_INIT;
    int ProcessWaitImpl::pipeFd_[2];
    decltype(ProcessWaitImpl::waiters_) ProcessWaitImpl::waiters_ = make_global<map<pid_t, set<AsyncOperationSPtr>>>();
    Global<ProcessExitCache> ProcessWaitImpl::processExitCache_ = make_global<ProcessExitCache>();
    Global<RwLock> ProcessWaitImpl::Lock = make_global<RwLock>();
}

size_t ProcessExitCache::Test_Size() const
{
    Invariant(exitEvents_.size() == purgeQueue_.size());
    return exitEvents_.size();
}

void ProcessExitCache::Test_Reset()
{
    Invariant(exitEvents_.size() == purgeQueue_.size());
    exitEvents_.clear();
    purgeQueue_ = PurgeQueue();  
}

bool ProcessExitCache::FindEvent(pid_t pid, int& status)
{
    auto match = exitEvents_.find(pid);
    if (match == exitEvents_.cend()) return false;

    status = match->second.Status;
    return true;
}

void ProcessExitCache::PurgeEvent()
{
    auto pid = purgeQueue_.top().Pid;
    purgeQueue_.pop();

    auto iter = exitEvents_.find(pid);
    ASSERT_IF(iter == exitEvents_.cend(), "purgeQueue_.size() = {0}, exitEvents_.size() = {1}", purgeQueue_.size(), exitEvents_.size());
    exitEvents_.erase(iter);
    WriteNoise(TraceType, "PurgeEvent: remove process {0}", pid);
}

void ProcessExitCache::AddEvent(pid_t pid, int status)
{
    WriteNoise(TraceType, "AddEvent: pid = {0}, status = {1}", pid, status);

    bool wasEmpty = exitEvents_.empty();

    ProcessExitEvent evt(pid, status, Stopwatch::Now());
    auto added = exitEvents_.emplace(pid, evt);
    if(!added.second)
    {
        WriteInfo(TraceType, "AddEvent: found duplicate of pid {0}, replacing status {1} with {2}", pid, added.first->second.Status, status);
        added.first->second = evt; 

        // Need to reconstruct purgeQueue_ as the event time changed for existing pid
        ReconstructPurgeQueue();
   }
    else
    {
        purgeQueue_.emplace(evt);
    }

    Invariant(exitEvents_.size() == purgeQueue_.size());
    PurgeIfOverSizeLimit();

    if (wasEmpty)
    {
        ScheduleToPurgeOldEvents();
    }
}

void ProcessExitCache::RemoveEvent(pid_t pid)
{
    WriteInfo(TraceType, "RemoveEvent: pid = {0}");
    auto erased = exitEvents_.erase(pid);
    Invariant(erased == 1);
    ReconstructPurgeQueue();
}

void ProcessExitCache::ReconstructPurgeQueue()
{
    PurgeQueue newQueue;
    for(auto const & e : exitEvents_)
    {
        newQueue.emplace(e.second);
    }

    purgeQueue_ = move(newQueue);
}

void ProcessExitCache::PurgeIfOverSizeLimit()
{
    auto sizeLimit = CommonConfig::GetConfig().ProcessExitCacheSizeLimit;
    while (exitEvents_.size() > sizeLimit)
    {
        WriteInfo(
            TraceType, "remove process {0} from ProcessExitCache, cacheSize = {1}, limit = {2}",
            purgeQueue_.top().Pid, exitEvents_.size(), sizeLimit);

        PurgeEvent();
    }
}

void ProcessExitCache::PurgeOldEvents()
{
    auto ageLimit = CommonConfig::GetConfig().ProcessExitCacheAgeLimit;
    AcquireWriteLock grab(*ProcessWaitImpl::Lock);

    while(!purgeQueue_.empty() && (purgeQueue_.top().Time + ageLimit) <= Stopwatch::Now())
    {
        WriteInfo(
            TraceType,
            "remove process {0} from ProcessExitCache, age limit = {1}, exit time = {2}",
            purgeQueue_.top().Pid, ageLimit, purgeQueue_.top().Time);
        PurgeEvent();
    }

    Invariant(exitEvents_.size() == purgeQueue_.size());
    ScheduleToPurgeOldEvents();
}

void ProcessExitCache::ScheduleToPurgeOldEvents()
{
    if (exitEvents_.empty()) return;

    auto delay = CommonConfig::GetConfig().ProcessExitCacheAgeLimit;
    WriteInfo(TraceType, "schedule purging in {0}", delay);
    Threadpool::Post([this] { PurgeOldEvents(); }, delay);
}

ProcessWaitImpl::ProcessWaitImpl(Handle && handle, pid_t pid, ProcessWait::WaitCallback const & waitCallback, TimeSpan timeout)
    : TimedAsyncOperation(timeout, nullptr, nullptr), handle_(move(handle)), pid_(pid), callback_(waitCallback), exitCode_(0)
{
    Invariant(pid_ > 0);
}

size_t ProcessWaitImpl::Test_CacheSize()
{
    AcquireReadLock grab(*Lock);
    return processExitCache_->Test_Size();
}

void ProcessWaitImpl::Test_Reset()
{
    AcquireWriteLock grab(*Lock);
    return processExitCache_->Test_Reset();
}

void ProcessWaitImpl::TryCompleteAsync(AsyncOperationSPtr const& thisSPtr, int exitCode)
{
    if (!TryStartComplete()) return;

    Threadpool::Post([thisSPtr, this, exitCode]
    {
        Invariant(exitCode_ == 0);
        exitCode_ = exitCode;
        FinishComplete(thisSPtr);
    });
}

void* ProcessWaitImpl::WaitLoop(void*)
{
    SigUtil::BlockSignalOnCallingThread(SIGCHLD); //block SIGCHLD signal in pipe reading thread to avoid deadlock when pipe is full

    WriteInfo(TraceType, "start wait loop");
    SigChildInfo si[1024];
    for(;;)
    {
        auto retval = read(pipeFd_[0], si, sizeof(si)); 
        if (retval < 0)
        {
            ASSERT_IF(errno != EINTR, "{0}: read failed with errno = {1:x}", __FUNCTION__, errno);
            continue;
        }

        if (retval == 0)
        {
            WriteInfo(TraceType, "WaitLoop: read returned 0, stop loop");
            break;
        }

        Invariant((retval % sizeof(SigChildInfo)) == 0);
#if DBG
        for (uint i = 0; i < retval/sizeof(SigChildInfo); ++i)
        {
            WriteInfo(
                TraceType,
                "SIGCHLD reported: si_signo = {0}, si_code = {1}, si_pid = {2}, si_status = {3}",
                si[i].signo, si[i].code, si[i].pid, si[i].status);
            Invariant(si[i].signo == SIGCHLD);
        }
#endif

        size_t completed = 0;
        {
            AcquireWriteLock grab(*Lock);
            completed = TryCompleteWaiters();
        }
        WriteInfo(TraceType, "completed {0} waiters", completed);
    }

    return nullptr;
}

void ProcessWaitImpl::AddWaiter(pid_t pid, AsyncOperationSPtr const & thisSPtr)
{
    auto iter = waiters_->find(pid);
    if (iter != waiters_->cend())
    {
        iter->second.emplace(thisSPtr);
        WriteNoise(TraceType, "added {0} as waiter for process {1}, {2} waiters in total", TextTracePtr(thisSPtr.get()), pid, iter->second.size());
        return;
    }

    set<AsyncOperationSPtr> opSet;
    opSet.emplace(thisSPtr);
    waiters_->emplace(pid, move(opSet));
    WriteNoise(TraceType, "added {0} as waiter for process {1}, new waiter set", TextTracePtr(thisSPtr.get()), pid);
}

void ProcessWaitImpl::RemoveWaiter(pid_t pid, AsyncOperationSPtr const & thisSPtr)
{
    auto iter = waiters_->find(pid);
    if (iter == waiters_->cend())
    {
        WriteNoise(TraceType, "process {0} not found in waiters_", pid);
        return;
    }

    auto erased = iter->second.erase(thisSPtr);
    Invariant(erased == 1);
    if (iter->second.empty())
    {
        waiters_->erase(iter);
        WriteNoise(TraceType, "removed waiter set for process {0}", pid);
    }
    else
    {
        WriteNoise(TraceType, "removed waiter {0} from waiter set for process {1}", TextTracePtr(thisSPtr.get()), pid);
    }
}

size_t ProcessWaitImpl::TryCompleteWaiters()
{
    // Need to go through all entries as there may be SIGCHLD loss
    size_t completed = 0;
    for(auto iter = waiters_->cbegin(); iter != waiters_->cend(); )
    {
        int status = 0;
        auto err = TryGetExitCode(iter->first, status);
        if (!err)
        {
            for(auto & op : iter->second)
            {
                auto waiter = move(op);
                AsyncOperation::Get<ProcessWaitImpl>(waiter)->TryCompleteAsync(waiter, status);
                ++completed;
            }

            iter = waiters_->erase(iter);
            continue;
        }

        ++iter;
    }

    return completed;
}

void ProcessWaitImpl::SigChildHandler(int sig, siginfo_t *si, void*)
{
    auto savedErrno = errno;

#if DBG
    SigChildInfo sigInfo = { .signo = si->si_signo, .code = si->si_code, .pid = si->si_pid, .status = si->si_status };
#else
    SigChildInfo sigInfo = 0;
#endif

    auto written = write(pipeFd_[1], &sigInfo, sizeof(sigInfo));
    Invariant(written == sizeof(sigInfo));

    errno = savedErrno;
}

BOOL ProcessWaitImpl::InitWaitLoop(PINIT_ONCE, PVOID, PVOID*)
{
    ProcessWait::WriteInfo(TraceType, "enter InitWaitLoop");

    ZeroRetValAssert(pipe2(pipeFd_, O_CLOEXEC));
    pthread_t tid;
    pthread_attr_t pthreadAttr;
    ZeroRetValAssert(pthread_attr_init(&pthreadAttr));
    ZeroRetValAssert(pthread_attr_setdetachstate(&pthreadAttr, PTHREAD_CREATE_DETACHED));
    ZeroRetValAssert(pthread_create(&tid, nullptr, &WaitLoop, nullptr));
    pthread_attr_destroy(&pthreadAttr);

    struct sigaction sa = {};
    sa.sa_flags = SA_NOCLDSTOP | SA_SIGINFO | SA_RESTART; //LINUXTODO should we also report stop events?
    sa.sa_sigaction = SigChildHandler;
    sigemptyset(&sa.sa_mask);
    ZeroRetValAssert(sigaction(SIGCHLD, &sa, nullptr));

    sigset_t mask = {};
    sigemptyset(&mask);
    sigaddset(&mask, SIGCHLD);
    ZeroRetValAssert(sigprocmask(SIG_UNBLOCK, &mask, nullptr));

    ProcessWait::WriteInfo(TraceType, "leave InitWaitLoop");
    return TRUE;
}

void ProcessWaitImpl::InitOnce()
{
    Invariant(::InitOnceExecuteOnce(
        &initOnce_,
        InitWaitLoop,
        nullptr,
        nullptr));
}

void ProcessWaitImpl::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    InitOnce();

    WriteNoise(TraceType, "{0}: {1} starts waiting for process {2}", TextTracePtr(thisSPtr.get()), getpid(), pid_);
    if (getpid() == pid_)
    {
        // Self-wait can happen in FabricTest
        WriteInfo(TraceType, "getpid() == pid == {0}, self wait is no-op", pid_);
        TimedAsyncOperation::OnStart(thisSPtr);
        return;
    }

    {
        AcquireWriteLock grab(*Lock);

        int cachedStatus = 0;
        bool foundInCache = processExitCache_->FindEvent(pid_, cachedStatus);
        if (foundInCache)
        {
            WriteNoise(TraceType, "{0}: found {1} in cache, status = {2}", TextTracePtr(thisSPtr.get()), pid_, cachedStatus);
        }

        // process exit is either reported by the following TryGetExitCode call, or it will be
        // reported by WaitLoop, as this operation wll be added to waiters_ below under *Lock.
        int status = 0;
        auto err = TryGetExitCode(pid_, status);
        if (!err)
        {
            WriteNoise(TraceType, "{0}: complete synchronously for process {1}", TextTracePtr(thisSPtr.get()), pid_);
            // Must complete async while holding *Lock, which is acquired in OnCompleted
            TryCompleteAsync(thisSPtr, status);
            return;
        }

        if (foundInCache)
        {
            if (err == ECHILD)
            {
                // pid_ is not reused yet
                // Must complete async while holding *Lock, which is acquired in OnCompleted
                TryCompleteAsync(thisSPtr, cachedStatus);
                return;
            }

            // pid_ is already reused, so the cached event is obsolete
            processExitCache_->RemoveEvent(pid_);
        }
        else if (err == ECHILD)
        {
            WriteError(TraceType, "process {0} doesn't exist or isn't a child of current process {1}", pid_, getpid());
            // Must complete async while holding *Lock, which is acquired in OnCompleted
            Threadpool::Post([thisSPtr] { thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::InvalidArgument); });
            return;
        }

        // cannot complete in OnStart()
        AddWaiter(pid_, thisSPtr);
    }

    TimedAsyncOperation::OnStart(thisSPtr);
}

int ProcessWaitImpl::TryGetExitCode(pid_t pid, int& status)
{
    auto retval = waitpid(pid, &status, WNOHANG);
    if (retval < 0)
    {
        WriteNoise(TraceType, "TryGetExitCode({0}): errno = {1}", pid, errno);
        Invariant(errno != EINVAL);
        Invariant(errno != 0);
        return errno;
    }

    if (retval == 0)
    {
        WriteNoise(TraceType, "TryGetExitCode({0}): no state change", pid);
        return EAGAIN;
    }

    WriteNoise(TraceType, "TryGetExitCode: waitpid returned {0}", retval);
    Invariant(pid == retval);
    processExitCache_->AddEvent(pid, status);
    return 0;
}

void ProcessWaitImpl::OnCompleted()
{
    if (!Error.IsSuccess())
    {
        // In case of failure (timeout/cancel), need to remove this op from waiters_
        WriteNoise(TraceType, "remove unsuccessful wait {0} on process {1}", TextTracePtr((AsyncOperation*)this), pid_);
        AcquireWriteLock grab(*Lock);
        RemoveWaiter(pid_, shared_from_this()); 
   }

    callback_(pid_, this->Error, exitCode_);
    callback_ = nullptr;
}

ProcessWaitSPtr ProcessWait::Create()
{
    return make_shared<ProcessWait>();
}

ProcessWaitSPtr ProcessWait::CreateAndStart(pid_t pid, WaitCallback const & callback, TimeSpan timeout)
{
    return CreateAndStart(Handle(NULL), pid, callback, timeout);
}

ProcessWaitSPtr ProcessWait::CreateAndStart(Handle&& handle, pid_t pid, WaitCallback const & callback, TimeSpan timeout)
{
    auto pw = make_shared<ProcessWait>(move(handle), pid, callback, timeout);
    pw->StartWait();
    return pw;
}

ProcessWait::ProcessWait()
{
}

ProcessWait::ProcessWait(pid_t pid, WaitCallback const & waitCallback, TimeSpan timeout)
{
    CreateImpl(Handle(NULL), pid, waitCallback, timeout);
}

ProcessWait::ProcessWait(Handle && handle, pid_t pid, WaitCallback const & waitCallback, TimeSpan timeout)
{
    CreateImpl(move(handle), pid, waitCallback, timeout);
}

ProcessWait::~ProcessWait()
{
    Cancel();
}

void ProcessWait::StartWait(pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout)
{
    CreateImpl(Handle(NULL), pid, callback, timeout);
    StartWait();
}

void ProcessWait::StartWait(Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout)
{
    CreateImpl(move(handle), pid, callback, timeout); 
    StartWait();
}

void ProcessWait::CreateImpl(Handle && handle, pid_t pid, WaitCallback const & callback, Common::TimeSpan timeout)
{
    Invariant(!impl_);
    impl_ = make_shared<ProcessWaitImpl>(move(handle), pid, callback, timeout);
}

void ProcessWait::StartWait()
{
    Invariant(impl_);
    impl_->Start(impl_);
}

void ProcessWait::Cancel()
{
    if (impl_)
    {
        impl_->Cancel();
    }
}

void ProcessWait::Setup()
{
    ProcessWaitImpl::InitOnce();
}

size_t ProcessWait::Test_CacheSize()
{
    return ProcessWaitImpl::Test_CacheSize();
}

void ProcessWait::Test_Reset()
{
    ProcessWaitImpl::Test_Reset();
}
