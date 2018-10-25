// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

using namespace Common;
using namespace ServiceModel;
using namespace std;
using namespace Store;

StringLiteral const TraceComponent("OpenAsyncOperation");

ReplicatedStore::OpenAsyncOperation::OpenAsyncOperation(
    PartitionedReplicaTraceComponent const & trace,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent, true) // skipCompleteOnCancel_
    , PartitionedReplicaTraceComponent(trace)
    , openError_(ErrorCodeValue::Success)
    , completionEvent_(false)
{
}

ReplicatedStore::OpenAsyncOperation::OpenAsyncOperation(
    PartitionedReplicaTraceComponent const & trace,
    ErrorCode const & error,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & parent)
    : AsyncOperation(callback, parent, true) // skipCompleteOnCancel_
    , PartitionedReplicaTraceComponent(trace)
    , openError_(error)
    , completionEvent_(false)
{
}

ErrorCode ReplicatedStore::OpenAsyncOperation::End(AsyncOperationSPtr const & operation)
{
    return AsyncOperation::End<OpenAsyncOperation>(operation)->Error;
}

void ReplicatedStore::OpenAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    if (!openError_.IsSuccess())
    {
        this->TryComplete(thisSPtr, openError_);
    }

    // Otherwise, externally completed by ReplicatedStore on state change
}

void ReplicatedStore::OpenAsyncOperation::OnCancel()
{
    // RA can potentially call Open->Cancel->Open->Close when deleting a replica.
    // The local store may still be open (or opening) when the cancel call aborts
    // the first replica open, causing the subsequent re-open to fail.
    //
    // Although the Open->Cancel->Open sequence should be optimized by RA, protect
    // against it at the store level by blocking cancel.
    //
    // We can block cancel in this case because skipCompleteOnCancel_ is being set 
    // to true and cancel will be called with forceComplete = false. Otherwise,
    // OnCancel() would deadlock waiting for OnCompleted().
    //
    WriteInfo(TraceComponent, "{0} blocking cancel for completion", this->TraceId);

    completionEvent_.WaitOne();

    WriteInfo(TraceComponent, "{0} cancel unblocked", this->TraceId);
}

void ReplicatedStore::OpenAsyncOperation::OnCompleted()
{
    completionEvent_.Set();
}
