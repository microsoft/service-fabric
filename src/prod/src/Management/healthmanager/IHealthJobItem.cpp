// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace Federation;
using namespace std;
using namespace Store;
using namespace Management::HealthManager;

IHealthJobItem::IHealthJobItem(HealthEntityKind::Enum entityKind)
    : entityKind_(entityKind)
    , state_(EntityJobItemState::NotStarted)
    , pendingAsyncOperation_()
    , sequenceNumber_(0)
{
}

IHealthJobItem::~IHealthJobItem()
{
}

bool IHealthJobItem::operator > (IHealthJobItem const & other) const
{
    // ReadOnly jobs have already written their info to store, so they should be quick to complete.
    // Give read only job items priority.
    if (this->CanExecuteOnStoreThrottled != other.CanExecuteOnStoreThrottled)
    {
        return other.CanExecuteOnStoreThrottled;
    }

    if (this->JobPriority < other.JobPriority)
    {
        return true;
    }
    else if (this->JobPriority == other.JobPriority)
    {
        return this->SequenceNumber > other.SequenceNumber;
    }
    else
    {
        return false;
    }
}

void IHealthJobItem::ProcessCompleteResult(Common::ErrorCode const & error)
{
    this->OnComplete(error);
}

// Called when work is done, either sync or async
void IHealthJobItem::OnWorkComplete()
{
    ASSERT_IF(
        state_ == EntityJobItemState::NotStarted || state_ == EntityJobItemState::Completed, 
        "{0}: OnWorkComplete can't be called in this state", 
        *this);
    
    state_ = EntityJobItemState::Completed;

    // The pending async operation keeps the job item alive.
    // Release the reference to it here to avoid creating a circular reference.
    pendingAsyncOperation_.reset();
}

AsyncOperationSPtr IHealthJobItem::OnWorkCanceled()
{
    state_ = EntityJobItemState::Completed;
    AsyncOperationSPtr temp;
    pendingAsyncOperation_.swap(temp);
    return temp;
}

void IHealthJobItem::OnAsyncWorkStarted()
{
    ASSERT_IF(
        state_ == EntityJobItemState::NotStarted,
        "{0}: OnAsyncWorkStarted can't be called in this state", 
        *this);
    
    // Mark that async work is started and return
    // When the work is done, the entity will call OnAsyncWorkComplete
    if (state_ == EntityJobItemState::Started)
    {
        state_ = EntityJobItemState::AsyncPending;
    }
}

void IHealthJobItem::OnAsyncWorkReadyToComplete(
    Common::AsyncOperationSPtr const & operation)
{
    // Accepted states: AsyncPending and Started (in rare situations,
    // when the callback is executed quickly and raises method before the async is started) 
    ASSERT_IF(
        state_ == EntityJobItemState::NotStarted || state_ == EntityJobItemState::Completed, 
        "{0}: OnAsyncWorkReadyToComplete can't be called in this state", 
        *this);
    
    // Remember the operation that completed to be executed on post-processing
    pendingAsyncOperation_ = operation;

    if (state_ == EntityJobItemState::Started)
    {
        state_ = EntityJobItemState::AsyncPending;
    }
}

bool IHealthJobItem::ProcessJob(IHealthJobItemSPtr const & thisSPtr, HealthManagerReplica & root)
{
    UNREFERENCED_PARAMETER(root);

    switch (state_)
    {
    case EntityJobItemState::NotStarted:
        {
            // Start actual processing, done in derived classes
            state_ = EntityJobItemState::Started;
            this->ProcessInternal(thisSPtr);
            break;
        }
    case EntityJobItemState::AsyncPending:
        {
            // The result error is already set
            this->FinishAsyncWork();
            break;
        }
    default:
        Assert::CodingError("{0}: ProcessJob can't be called in this state", *this);
    }  

    return true;
}

std::wstring IHealthJobItem::ToString() const
{
    return wformatString(*this);
}

void IHealthJobItem::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
     w.Write(
        "{0}({1},id={2},{3},JI lsn={4},priority={5})",
        this->TypeString,
        this->ReplicaActivityId.TraceId, 
        this->JobId,
        this->State,
        this->SequenceNumber,
        this->JobPriority);
}

void IHealthJobItem::WriteToEtw(uint16 contextSequenceId) const
{
    HMCommonEvents::Trace->JobItemTrace(
        contextSequenceId,
        this->TypeString,
        this->ReplicaActivityId.TraceId, 
        this->JobId,
        this->State,
        this->SequenceNumber,
        this->JobPriority);
}
