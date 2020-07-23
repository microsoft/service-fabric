// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;
using namespace Store;
using namespace Management::CentralSecretService;

StringLiteral const TraceComponent("SecretManager::AutoRetryAsyncOperation");

int const SecretManager::AutoRetryAsyncOperation::MaxRetryTimes(3);
TimeSpan const SecretManager::AutoRetryAsyncOperation::MaxRetryDelay = TimeSpan::FromSeconds(5);

SecretManager::AutoRetryAsyncOperation::AutoRetryAsyncOperation(
    CreateAndStartCallback const & createAndStartCallback,
    TimeSpan const & timeout,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & parent,
    Common::ActivityId const & activityId)
    : TimedAsyncOperation(timeout, callback, parent)
    , createAndStartCallback_(createAndStartCallback)
    , activityId_(activityId)
    , retriedTimes_(0)
    , innerOperationSPtr_(nullptr)
    , retryTimer_(nullptr)
    , cancellationRequested_(false)
{
}

void SecretManager::AutoRetryAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    auto operationSPtr =
        this->createAndStartCallback_(
            [this](AsyncOperationSPtr const & operationSPtr)
    {
        this->CompleteOperation(operationSPtr, false);
    },
            thisSPtr);

    this->CompleteOperation(operationSPtr, true);
}

void SecretManager::AutoRetryAsyncOperation::CompleteOperation(
    AsyncOperationSPtr const & operationSPtr,
    bool expectedCompletedSynchronously)
{
    if (operationSPtr->CompletedSynchronously != expectedCompletedSynchronously) { return; }

    // Innter operation failed.
    if (!operationSPtr->Error.IsSuccess())
    {
        auto error = AsyncOperation::End(operationSPtr);

        if (this->cancellationRequested_
            || this->retriedTimes_ >= SecretManager::AutoRetryAsyncOperation::MaxRetryTimes
            || !this->IsRetryable(error))
        {
            Trace.WriteError(
                TraceComponent,
                "{0}: [Retry:{1}] Inner operation failed. Error={2}",
                this->ActivityId,
                this->retriedTimes_,
                error);

            if (!this->TryComplete(operationSPtr->Parent, error))
            {
                Assert::CodingError("SecretManager::AutoRetryAsyncOperation failed to complete with error.");
            }
        }
        else
        {
            Trace.WriteWarning(
                TraceComponent,
                "{0}: [Retry:{1}] Inner operation failed. Error={2}",
                this->ActivityId,
                this->retriedTimes_,
                error);

            this->Retry(operationSPtr->Parent, error);
        }
    }
    // Inner operation succeeded.
    else
    {
        this->innerOperationSPtr_ = operationSPtr;

        if (!this->TryComplete(operationSPtr->Parent, ErrorCode::Success()))
        {
            Assert::CodingError("SecretManager::AutoRetryAsyncOperation failed to complete with success.");
        }
    }
}

void SecretManager::AutoRetryAsyncOperation::OnCancel()
{
    this->cancellationRequested_ = true;
    TimedAsyncOperation::OnCancel();
}

void SecretManager::AutoRetryAsyncOperation::OnTimeout(AsyncOperationSPtr const & thisSPtr)
{
    this->cancellationRequested_ = true;
    TimedAsyncOperation::OnTimeout(thisSPtr);
}

bool SecretManager::AutoRetryAsyncOperation::IsRetryable(ErrorCode const & error)
{
    switch (error.ReadValue())
    {
    case ErrorCodeValue::NoWriteQuorum:
    case ErrorCodeValue::ReconfigurationPending:
        //case ErrorCodeValue::StoreRecordAlreadyExists: // same as StoreWriteConflict
    case ErrorCodeValue::StoreOperationCanceled:
    case ErrorCodeValue::StoreWriteConflict:
    case ErrorCodeValue::StoreSequenceNumberCheckFailed:
        return true;

    default:
        return false;
    }
}

void SecretManager::AutoRetryAsyncOperation::Retry(AsyncOperationSPtr const & thisSPtr, ErrorCode const & error)
{
    this->retriedTimes_++;

    this->retryTimer_ = Timer::Create(
        TimerTagDefault,
        [this, thisSPtr](TimerSPtr const & timer)
    {
        timer->Cancel();

        auto operationSPtr = this->createAndStartCallback_(
            [this](AsyncOperationSPtr const & operationSPtr)
            {
                this->CompleteOperation(operationSPtr, false);
            },
            thisSPtr);

        this->CompleteOperation(operationSPtr, true);
    },
        true);

    this->retryTimer_->Change(
        StoreTransaction::GetRandomizedOperationRetryDelay(
            error,
            SecretManager::AutoRetryAsyncOperation::MaxRetryDelay));
}

ErrorCode SecretManager::AutoRetryAsyncOperation::End(
    AsyncOperationSPtr const & operationSPtr,
    __out AsyncOperationSPtr & innerOperationSPtr)
{
    auto retryOperationSPtr = AsyncOperation::End<SecretManager::AutoRetryAsyncOperation>(operationSPtr);

    innerOperationSPtr = retryOperationSPtr->innerOperationSPtr_;

    return retryOperationSPtr->Error;
}