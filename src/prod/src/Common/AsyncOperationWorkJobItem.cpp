// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace std;
using namespace Common;

AsyncOperationWorkJobItem::AsyncOperationWorkJobItem()
    : AsyncWorkJobItem()
    , beginCallback_()
    , endCallback_()
    , startTimeoutCallback_()
    , operation_()
{
}

AsyncOperationWorkJobItem::AsyncOperationWorkJobItem(
    BeginAsyncWorkCallback const & beginCallback,
    EndAsyncWorkCallback const & endCallback)
    : AsyncWorkJobItem()
    , beginCallback_(beginCallback)
    , endCallback_(endCallback)
    , startTimeoutCallback_()
    , operation_()
{
}

AsyncOperationWorkJobItem::AsyncOperationWorkJobItem(
    BeginAsyncWorkCallback const & beginCallback,
    EndAsyncWorkCallback const & endCallback,
    OnStartWorkTimedOutCallback const & startTimeoutCallback)
    : AsyncWorkJobItem()
    , beginCallback_(beginCallback)
    , endCallback_(endCallback)
    , startTimeoutCallback_(startTimeoutCallback)
    , operation_()
{
}

AsyncOperationWorkJobItem::~AsyncOperationWorkJobItem()
{
}

AsyncOperationWorkJobItem::AsyncOperationWorkJobItem(AsyncOperationWorkJobItem && other)
    : AsyncWorkJobItem(move(other))
    , beginCallback_(move(other.beginCallback_))
    , endCallback_(move(other.endCallback_))
    , startTimeoutCallback_(move(other.startTimeoutCallback_))
    , operation_(move(other.operation_))
{
}

AsyncOperationWorkJobItem & AsyncOperationWorkJobItem::operator=(AsyncOperationWorkJobItem && other)
{
    if (this != &other)
    {
        beginCallback_ = move(other.beginCallback_);
        endCallback_ = move(other.endCallback_);
        startTimeoutCallback_ = move(other.startTimeoutCallback_);
        operation_ = move(other.operation_);
    }

    __super::operator=(move(other));
    return *this;
}

void AsyncOperationWorkJobItem::StartWork(
    AsyncWorkReadyToCompleteCallback const & completeCallback,
    __out bool & completedSync)
{
    ASSERT_IFNOT(beginCallback_, "BeginWork callback should be set, sequence number {0}, state {1}", this->SequenceNumber, this->State);

    auto inner = beginCallback_(
        [this, completeCallback](AsyncOperationSPtr const & operation)
        {
            if (!operation->CompletedSynchronously)
            {
                // Call the job queue to let it know the operation completed async
                completeCallback(this->SequenceNumber);
            }
        });

    ASSERT_IFNOT(inner, "{0}: beginCallback_: operation not set, sequence number {0}", this->SequenceNumber);
    completedSync = inner->CompletedSynchronously;
    operation_ = move(inner);
    beginCallback_ = nullptr;
}

void AsyncOperationWorkJobItem::EndWork()
{
    TESTASSERT_IFNOT(endCallback_, "Valid EndWork callback should be set");

    if (endCallback_)
    {
        ASSERT_IFNOT(operation_, "AsyncOperationWorkJobItem::EndWork operation is not set, sequence number {0}, state {1}", this->SequenceNumber, this->State);
        endCallback_(operation_);
        endCallback_ = nullptr;
    }
}

void AsyncOperationWorkJobItem::OnStartWorkTimedOut()
{
    if (startTimeoutCallback_)
    {
        startTimeoutCallback_();
        startTimeoutCallback_ = nullptr;
    }
}
