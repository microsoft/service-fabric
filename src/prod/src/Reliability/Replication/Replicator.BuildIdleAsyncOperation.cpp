// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AcquireWriteLock;

    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ErrorCode;

    Replicator::BuildIdleAsyncOperation::BuildIdleAsyncOperation(
        __in Replicator & parent,
        ReplicaInformation const & replica,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            parent_(parent),
            replica_(replica)
    {
    }

    void Replicator::BuildIdleAsyncOperation::OnStart(
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        bool shouldComplete = false;
        PrimaryReplicatorSPtr primaryCopy;
        
        {
            AcquireWriteLock lock(parent_.lock_);
            if(parent_.VerifyIsPrimaryCallerHoldsLock(L"BuildReplica", error))
            {
                primaryCopy = parent_.primary_;
            }
            else
            {
                 shouldComplete = true;
            }
        }

        if(shouldComplete)
        {
            // Complete outside the lock
            TryComplete(thisSPtr, error);
        }
        else
        {
            // primary_ is guaranteed to exist until the inner operation is completed
            auto inner = primaryCopy->BeginBuildReplica(
                replica_,
                [this, primaryCopy](AsyncOperationSPtr const & asyncOperation)                        
                {
                    this->BuildIdleCallback(asyncOperation, primaryCopy);
                }, 
                thisSPtr); 
            if(inner->CompletedSynchronously)
            {
                FinishBuildIdle(inner, primaryCopy);
            }

            error.ReadValue();
        }
    }

    void Replicator::BuildIdleAsyncOperation::BuildIdleCallback(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        if(!asyncOperation->CompletedSynchronously)
        {
            FinishBuildIdle(asyncOperation, primary);
        }
    }

    void Replicator::BuildIdleAsyncOperation::FinishBuildIdle(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        ErrorCode error = primary->EndBuildReplica(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error);
    }
    
    ErrorCode Replicator::BuildIdleAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<BuildIdleAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
