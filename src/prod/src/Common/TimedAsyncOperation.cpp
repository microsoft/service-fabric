// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Common
{
    TimedAsyncOperation::TimedAsyncOperation(
            TimeSpan const timeout, 
            AsyncCallback const & callback, 
            AsyncOperationSPtr const & parent)
        :   AsyncOperation(callback, parent),
            timeout_(timeout),
            expireTime_(Stopwatch::Now() + timeout),
            timer_(nullptr)
    {
    }

    TimedAsyncOperation::~TimedAsyncOperation()
    {
    }

    Common::TimeSpan TimedAsyncOperation::get_RemainingTime() const 
    {
        return expireTime_.RemainingDuration();
    }

    void TimedAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        this->InternalStartTimer(thisSPtr);
    }

    void TimedAsyncOperation::OnCompleted()
    {
        AsyncOperation::OnCompleted();
        if (this->timer_) { this->timer_->Cancel(); }
    }

    void TimedAsyncOperation::StartTimerCallerHoldsLock(AsyncOperationSPtr const & thisSPtr)
    {
        this->InternalStartTimer(thisSPtr);
    }

    void TimedAsyncOperation::InternalStartTimer(AsyncOperationSPtr const & thisSPtr)
    {
        if (this->timeout_ != TimeSpan::MaxValue)
        {
            this->timer_ = Timer::Create(
                "TimedAsyncOperation",
                [thisSPtr](TimerSPtr const &) -> void { TimedAsyncOperation::OnTimerCallback(thisSPtr); });
#ifdef PLATFORM_UNIX
            this->timer_->LimitToOneShot();
#endif
            this->timer_->Change(this->timeout_);
        }
    }

    void TimedAsyncOperation::OnTimerCallback(AsyncOperationSPtr const & thisSPtr)
    {
        if (thisSPtr->TryStartComplete())
        {
            auto timedAsyncOperation = AsyncOperation::Get<TimedAsyncOperation>(thisSPtr);
            timedAsyncOperation->OnTimeout(thisSPtr);
            timedAsyncOperation->FinishComplete(thisSPtr, ErrorCodeValue::Timeout);
        }
    }

    void TimedAsyncOperation::OnTimeout(AsyncOperationSPtr const &)
    {
    }

    ErrorCode TimedAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
    {
        auto thisPtr = AsyncOperation::End<TimedAsyncOperation>(asyncOperation);
        return thisPtr->Error;
    }
}
