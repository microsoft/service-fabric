// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

StringLiteral const TraceType = "AsyncWaitHandle";

class WaitAsyncOperation : public AsyncOperation
{
    DENY_COPY(WaitAsyncOperation)

public:
    WaitAsyncOperation(
        HANDLE handle,
        TimeSpan const timeout,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        handle_(handle),
        timeout_(timeout)
    {
        CODING_ERROR_ASSERT(timeout_ >= TimeSpan::Zero);
    }

    static ErrorCode End(AsyncOperationSPtr const & operation)
    {
        auto thisPtr = AsyncOperation::End<WaitAsyncOperation>(operation);
        return thisPtr->Error;
    }

    static void Callback(PTP_CALLBACK_INSTANCE pci, void* callbackParameter, PTP_WAIT, TP_WAIT_RESULT waitResult)
    {
        AsyncOperationSPtr thisSPtr = reinterpret_cast<WaitAsyncOperation*>(callbackParameter)->shared_from_this();

        // With the following call to DisassociateCurrentThreadFromCallback(), future calls to WaitForThreadpoolWaitCallbacks()
        // won't wait for this callback instance to complete, and this callback instance has finished its minimal work above.
        DisassociateCurrentThreadFromCallback(pci);

        if (waitResult == WAIT_TIMEOUT)
        {
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
        else
        {
            thisSPtr->TryComplete(thisSPtr, ErrorCodeValue::Success);
        }
    }

protected:
    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        int64 ticks = -this->timeout_.Ticks;
        PFILETIME timeoutPtr = (this->timeout_ == TimeSpan::MaxValue) ? NULL : reinterpret_cast<PFILETIME>(&ticks);

        wait_ = ::CreateThreadpoolWait(&Threadpool::WaitCallback<WaitAsyncOperation>, this, NULL /*no specific environment*/);
        if (wait_ == nullptr)
        {
            this->TryComplete(thisSPtr, ErrorCode::FromHResult(HRESULT_FROM_WIN32(::GetLastError())));
            return;
        }

        self_ = this->shared_from_this();
        ::SetThreadpoolWait(wait_, handle_, timeoutPtr);
    }

    void OnCompleted()
    {
        if (wait_)
        {
            // We don't have to wait for callback completion here because we can only come here when threadpool 
            // callback is being called. If this is called as part of cancel, then wait_ would have been set to null.
            CloseThreadpoolWait(wait_);
            wait_ = nullptr;
        }

        self_ = nullptr;
    }

    void OnCancel()
    {
        if (wait_ != nullptr)
        {
            WaitForThreadpoolWaitCallbacks(wait_, TRUE /*cancel pending callbacks*/);
            CloseThreadpoolWait(wait_);
            wait_ = nullptr;
        }
    }

private:

    HANDLE handle_;
    TimeSpan const timeout_;
    PTP_WAIT wait_;
    AsyncOperationSPtr self_; // keep this operation alive
};

template <bool ManualReset> 
AsyncWaitHandle<ManualReset>::AsyncWaitHandle(bool initialState, wstring const & eventName) : WaitHandle<ManualReset>(initialState, eventName)
{
}

template <bool ManualReset> 
AsyncOperationSPtr AsyncWaitHandle<ManualReset>::BeginWaitOne(
    TimeSpan const timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent)
{
    return AsyncOperation::CreateAndStart<WaitAsyncOperation>(
        this->handle_,
        timeout,
        callback,
        parent);
}

template <bool ManualReset> 
ErrorCode AsyncWaitHandle<ManualReset>::EndWaitOne(
    AsyncOperationSPtr const & operation)
{
    return WaitAsyncOperation::End(operation);
}

namespace Common
{
    template class AsyncWaitHandle<false>;
    template class AsyncWaitHandle<true>;
}
