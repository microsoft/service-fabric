// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

static StringLiteral const TraceType = "ProcessWait";

namespace Common
{
    class ProcessWaitImpl : 
        public std::enable_shared_from_this<ProcessWaitImpl>,
        TextTraceComponent<TraceTaskCodes::Common>
    {
        DENY_COPY(ProcessWaitImpl);

    public:
        ProcessWaitImpl(Handle && handle, pid_t pid, ProcessWait::WaitCallback const & callback, TimeSpan timeout);
        ~ProcessWaitImpl();

        static ErrorCode GetWaitResult(TP_WAIT_RESULT result);

        void Initialize();
        void Cancel(); 

        static void Callback(PTP_CALLBACK_INSTANCE, void* lpParameter, PTP_WAIT, TP_WAIT_RESULT);

    private:
        Handle handle_;
        pid_t pid_;
        ProcessWait::WaitCallback callback_;
        TimeSpan timeout_;
        PTP_WAIT wait_;
        atomic_bool cancelCalled_;
    };
}

ProcessWaitImpl::ProcessWaitImpl(Handle && handle, pid_t pid, ProcessWait::WaitCallback const & callback, TimeSpan timeout)
    : handle_(std::move(handle)), pid_(pid), callback_(callback), timeout_(timeout), wait_(nullptr), cancelCalled_(false)
{
    CODING_ERROR_ASSERT(handle_.Value != nullptr);
    CODING_ERROR_ASSERT(callback_ != nullptr);
    CODING_ERROR_ASSERT(timeout_ >= TimeSpan::Zero);
    WriteNoise(TraceType, "{0} created.", static_cast<void*>(this));
}


ProcessWaitImpl::~ProcessWaitImpl()
{
    ASSERT_IFNOT(this->cancelCalled_.load(), "Cancel() must be called before destruction");
    WriteNoise(TraceType, "{0} destructed.", static_cast<void*>(this));
}

void ProcessWaitImpl::Initialize()
{
    int64 ticks = -this->timeout_.Ticks;
    PFILETIME timeoutPtr = (this->timeout_ == TimeSpan::MaxValue) ? NULL : reinterpret_cast<PFILETIME>(&ticks);

    wait_ = CreateThreadpoolWait(&Threadpool::WaitCallback<ProcessWaitImpl>, this, NULL /*no specific environment*/);
    CHK_WBOOL(wait_ != nullptr);
    SetThreadpoolWait(wait_, handle_.Value, timeoutPtr);
}

void ProcessWaitImpl::Cancel()
{
    WriteNoise(TraceType, "{0}: Cancel called", static_cast<void*>(this));

    bool expected = false;
    if (!cancelCalled_.compare_exchange_weak(expected, true))
    {
        // cancelled has already been called.
        WriteNoise(TraceType, "{0}: Cancel has already been called before, returning", static_cast<void*>(this));
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

void ProcessWaitImpl::Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_WAIT, TP_WAIT_RESULT waitResult)
{
    // Increment ref count to make sure ProcessWaitImpl object is alive 
    ProcessWaitImplSPtr thisSPtr = reinterpret_cast<ProcessWaitImpl*>(callbackParameter)->shared_from_this();
    WriteNoise(TraceType, "{0} : enter ProcessWaitImpl::Callback()", static_cast<void*>(thisSPtr.get()));

    auto callback = thisSPtr->callback_;

    // The following call makes sure WaitForThreadpoolWaitCallbacks() won't wait for this callback to complete. This way,
    // Delete() won't block on callbacks, since it only needs to wait for the completion of minimal work above.
    DisassociateCurrentThreadFromCallback(pci);

    auto error = GetWaitResult(waitResult);
    DWORD processExitCode = 0;
    if (error.IsSuccess())
    {
        error = ProcessUtility::GetProcessExitCode(thisSPtr->handle_, processExitCode);
    }

    callback(thisSPtr->pid_, error, processExitCode);
    WriteNoise(TraceType, "{0} : client callback returned.", static_cast<void*>(thisSPtr.get()));

    thisSPtr->Cancel();
}

ErrorCode ProcessWaitImpl::GetWaitResult(TP_WAIT_RESULT waitResult)
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

ProcessWaitSPtr ProcessWait::CreateAndStart(Handle && handle, WaitCallback const & callback, TimeSpan timeout)
{
    auto processId = ::GetProcessId(handle.Value);
    return make_shared<ProcessWait>(move(handle), processId, callback, timeout);
}

ProcessWaitSPtr ProcessWait::CreateAndStart(Handle && handle, pid_t pid, WaitCallback const & callback, TimeSpan timeout)
{
    return make_shared<ProcessWait>(move(handle), pid, callback, timeout);
}

ProcessWait::ProcessWait(Handle && handle, pid_t pid, WaitCallback const & waitCallback, TimeSpan timeout) : impl_(make_shared<ProcessWaitImpl>(move(handle), pid, waitCallback, timeout))
{
    this->impl_->Initialize();
}

ProcessWait::~ProcessWait()
{
    this->Cancel();
}

void ProcessWait::Cancel()
{
    this->impl_->Cancel();
}

