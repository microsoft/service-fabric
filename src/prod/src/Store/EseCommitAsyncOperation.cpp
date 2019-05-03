// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;

namespace Store
{
    EseCommitAsyncOperation::EseCommitAsyncOperation(
        EseSessionSPtr const & session,
        EseLocalStoreSettings const & settings,
        EseStorePerformanceCounters const & perfCounters,
        TimeSpan const timeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        session_(session),
        perfCounters_(perfCounters),
        stopwatch_(),
        error_(ErrorCodeValue::Success),
        commitId_(),
        timeout_(),
        completionCount_(2)
    {
        ZeroMemory(&commitId_, sizeof(commitId_));

        TimeSpan maxDelay = settings.MaxAsyncCommitDelay;

        if (timeout == TimeSpan::Zero || timeout == TimeSpan::MaxValue || timeout.TotalMilliseconds() < 0 || timeout.TotalMilliseconds() > maxDelay.TotalMilliseconds())
        {
            timeout_ = static_cast<ULONG>(maxDelay.TotalMilliseconds());
        }
        else
        {
            timeout_ = static_cast<ULONG>(timeout.TotalMilliseconds());
        }
    }

    EseCommitAsyncOperation::EseCommitAsyncOperation(
        ErrorCode const & error,
        EseStorePerformanceCounters const & perfCounters,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & parent)
        : AsyncOperation(callback, parent),
        session_(),
        perfCounters_(perfCounters),
        stopwatch_(),
        error_(error),
        commitId_(),
        timeout_(),
        completionCount_(2)
    {
    }

    void EseCommitAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!error_.IsSuccess())
        {
            this->TryComplete(thisSPtr, error_);

            return;
        }

        JET_ERR jetErr = JET_errSuccess;
        {
            BIND_TO_THREAD_SET_ERR( *session_, jetErr )

            if (JET_errSuccess == jetErr)
            {
                stopwatch_.Start();

                jetErr = CALL_ESE_NOTHROW(
                    JetCommitTransaction2(
                    session_->SessionId,
                    JET_bitCommitLazyFlush,
                    timeout_,
                    &commitId_));

                // Remove this session as the commit call into ESE has been made
                session_->Instance->RemoveActiveSession(*session_);
            }
        }

        // Once this async operation completes, the EseLocalStore::Transaction may destruct.
        // EsePool::ReturnToPool verifies that there's no outstanding work by checking
        // the value of EseSession::currentThread_, which is released by the BindToThread
        // scope. Completion must occur outside the BindToThread scope.
        // 
        if (JET_errSuccess != jetErr)
        {
            this->TryComplete(
                thisSPtr, 
                session_->Instance ? session_->Instance->JetToErrorCode(jetErr) : EseError::GetErrorCode(jetErr));
        }
        else
        {
            this->DecrementAndTryComplete(thisSPtr);
        }
    }

    void EseCommitAsyncOperation::OnCompleted()
    {
        if (stopwatch_.IsRunning)
        {
            stopwatch_.Stop();

            perfCounters_.AvgLatencyOfCommitsBase.Increment(); 
            perfCounters_.AvgLatencyOfCommits.IncrementBy(stopwatch_.ElapsedMicroseconds); 
        }

        __super::OnCompleted();
    }

    ErrorCode EseCommitAsyncOperation::End(AsyncOperationSPtr const & operation)
    {
        return AsyncOperation::End<EseCommitAsyncOperation>(operation)->Error;
    }

    void EseCommitAsyncOperation::ExternalComplete(
        AsyncOperationSPtr const & operation, 
        ErrorCode const & error)
    {
        error_ = ErrorCode::FirstError(error_, error);

        this->DecrementAndTryComplete(operation);
    }

    void EseCommitAsyncOperation::DecrementAndTryComplete(AsyncOperationSPtr const & operation)
    {
        if (--completionCount_ == 0)
        {
            this->TryComplete(operation, error_);
        }
    }
}
