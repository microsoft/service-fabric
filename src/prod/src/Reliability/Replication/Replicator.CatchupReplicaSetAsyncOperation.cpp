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

    Replicator::CatchupReplicaSetAsyncOperation::CatchupReplicaSetAsyncOperation(
        __in Replicator & parent,
        FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            parent_(parent),
            catchUpMode_(catchUpMode)
    {
    }

    void Replicator::CatchupReplicaSetAsyncOperation::OnStart(
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        bool shouldComplete = false;
        PrimaryReplicatorSPtr primaryCopy;
        AsyncOperationSPtr inner;
        bool shouldFinish = false;
        
        {
            AcquireWriteLock lock(parent_.lock_);
            if(parent_.VerifyIsPrimaryCallerHoldsLock(L"CatchupReplicaSet", error))
            {
                primaryCopy = parent_.primary_;
                error.ReadValue();
                // primary_ is guaranteed to exist until the inner operation is completed
                inner = primaryCopy->BeginWaitForCatchUpQuorum(
                    catchUpMode_,
                    [this, primaryCopy](AsyncOperationSPtr const & asyncOperation)                        
                    {
                        this->CatchupReplicaSetCallback(asyncOperation, primaryCopy);
                    }, 
                    thisSPtr); 
                if(inner->CompletedSynchronously)
                {
                    shouldFinish = true;
                }
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
        else if (shouldFinish)
        {
            FinishCatchupReplicaSet(inner, primaryCopy);
        }
    }

    void Replicator::CatchupReplicaSetAsyncOperation::CatchupReplicaSetCallback(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        if(!asyncOperation->CompletedSynchronously)
        {
            FinishCatchupReplicaSet(asyncOperation, primary);
        }
    }

    void Replicator::CatchupReplicaSetAsyncOperation::FinishCatchupReplicaSet(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        ErrorCode error = primary->EndWaitForCatchUpQuorum(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error);
    }
    
    ErrorCode Replicator::CatchupReplicaSetAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<CatchupReplicaSetAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
