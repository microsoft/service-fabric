// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType_ThreadpoolWait = "ThreadpoolWait";

class ThreadpoolWait::ThreadpoolWaitImpl : 
    public std::enable_shared_from_this<ThreadpoolWaitImpl>,
    TextTraceComponent<TraceTaskCodes::Common>
{
    DENY_COPY(ThreadpoolWaitImpl);

public:
    ThreadpoolWaitImpl(Handle &&, WaitCallback const & callback, TimeSpan timeout);
    ~ThreadpoolWaitImpl();

    static ErrorCode GetWaitResult(TP_WAIT_RESULT result);

    void Initialize();
    void Cancel(); 

    static void Callback(PTP_CALLBACK_INSTANCE, void* lpParameter, PTP_WAIT, TP_WAIT_RESULT);

private:
    Handle handle_;
    WaitCallback callback_;
    TimeSpan timeout_;
    PTP_WAIT wait_;
    atomic_bool cancelCalled_;
};

ThreadpoolWait::ThreadpoolWaitImpl::ThreadpoolWaitImpl(Handle && handle, WaitCallback const & callback, TimeSpan timeout)
    : handle_(std::move(handle)), callback_(callback), timeout_(timeout), wait_(nullptr), cancelCalled_(false)
{
    CODING_ERROR_ASSERT(handle_.Value != nullptr);
    CODING_ERROR_ASSERT(callback_ != nullptr);
    CODING_ERROR_ASSERT(timeout_ >= TimeSpan::Zero);
    WriteNoise(TraceType_ThreadpoolWait, "{0} created.", static_cast<void*>(this));
}


ThreadpoolWait::ThreadpoolWaitImpl::~ThreadpoolWaitImpl()
{
    ASSERT_IFNOT(this->cancelCalled_.load(), "Cancel() must be called before destruction");
    WriteNoise(TraceType_ThreadpoolWait, "{0} destructed.", static_cast<void*>(this));
}

void ThreadpoolWait::ThreadpoolWaitImpl::Initialize()
{
    int64 ticks = -this->timeout_.Ticks;
    PFILETIME timeoutPtr = (this->timeout_ == TimeSpan::MaxValue) ? NULL : reinterpret_cast<PFILETIME>(&ticks);

    wait_ = CreateThreadpoolWait(&Threadpool::WaitCallback<ThreadpoolWaitImpl>, this, NULL /*no specific environment*/);
    CHK_WBOOL(wait_ != nullptr);
    SetThreadpoolWait(wait_, handle_.Value, timeoutPtr);
}

void ThreadpoolWait::ThreadpoolWaitImpl::Cancel()
{
    WriteNoise(TraceType_ThreadpoolWait, "{0}: Cancel called", static_cast<void*>(this));

    bool expected = false;
    if (!cancelCalled_.compare_exchange_weak(expected, true))
    {
        // cancelled has already been called.
        WriteNoise(TraceType_ThreadpoolWait, "{0}: Cancel has already been called before, returning", static_cast<void*>(this));
        return;
    }

    CODING_ERROR_ASSERT(wait_ != nullptr);
    CODING_ERROR_ASSERT(callback_ != nullptr);

    SetThreadpoolWait(this->wait_, nullptr, nullptr); 
    WaitForThreadpoolWaitCallbacks(wait_, TRUE /*cancel pending callbacks*/);
    CloseThreadpoolWait(wait_);

    wait_ = nullptr;
    callback_ = nullptr;
}

void ThreadpoolWait::ThreadpoolWaitImpl::Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_WAIT, TP_WAIT_RESULT waitResult)
{
    // Increment ref count to make sure ThreadpoolWaitImpl object is alive 
    ThreadpoolWaitImplSPtr thisSPtr = reinterpret_cast<ThreadpoolWaitImpl*>(callbackParameter)->shared_from_this();
    WriteNoise(TraceType_ThreadpoolWait, "{0} : enter ThreadpoolWait::ThreadpoolWaitImpl::Callback()", static_cast<void*>(thisSPtr.get()));

    auto callback = thisSPtr->callback_;

    // The following call makes sure WaitForThreadpoolWaitCallbacks() won't wait for this callback to complete. This way,
    // Delete() won't block on callbacks, since it only needs to wait for the completion of minimal work above.
    DisassociateCurrentThreadFromCallback(pci);

    callback(thisSPtr->handle_, GetWaitResult(waitResult));
    WriteNoise(TraceType_ThreadpoolWait, "{0} : client callback returned.", static_cast<void*>(thisSPtr.get()));

    thisSPtr->Cancel();
}

ErrorCode ThreadpoolWait::ThreadpoolWaitImpl::GetWaitResult(TP_WAIT_RESULT waitResult)
{
    if (waitResult == WAIT_OBJECT_0)
    {
        return ErrorCodeValue::Success;
    }

    if (waitResult == WAIT_TIMEOUT)
    {
        return ErrorCodeValue::Timeout;
    }

    // WAIT_ABANDONED is only meaningful for mutex. It means the previous owner thread exit without calling release, so the state
    // protected by the mutext may be in an inconsistent state
    CODING_ERROR_ASSERT(waitResult == WAIT_ABANDONED);
    return ErrorCode::FromHResult(HRESULT_FROM_WIN32(WAIT_ABANDONED));
}

ThreadpoolWaitSPtr ThreadpoolWait::Create(Handle && handle, WaitCallback const & callback, TimeSpan timeout)
{
    return make_shared<ThreadpoolWait>(move(handle), callback, timeout);
}

ThreadpoolWait::ThreadpoolWait(Handle && handle, WaitCallback const & waitCallback, TimeSpan timeout) : impl_(make_shared<ThreadpoolWaitImpl>(move(handle), waitCallback, timeout))
{
    this->impl_->Initialize();
}

ThreadpoolWait::~ThreadpoolWait()
{
    this->Cancel();
}

void ThreadpoolWait::Cancel()
{
    this->impl_->Cancel();
}

