// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::AcquireReadLock;

    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ErrorCode;
    using Common::ComPointer;

    using std::move;

    Replicator::ReplicateAsyncOperation::ReplicateAsyncOperation(
       __in Replicator & parent,
       ComPointer<IFabricOperationData> && comOperationPointer,
       AsyncCallback const & callback, 
       AsyncOperationSPtr const & state)
       :   AsyncOperation(callback, state),
           parent_(parent),
           comOperationPointer_(move(comOperationPointer)),
           sequenceNumber_(Constants::InvalidLSN)
    {
    }

    void Replicator::ReplicateAsyncOperation::OnStart(
        AsyncOperationSPtr const & thisSPtr)
    {
        ErrorCode error;
        bool shouldComplete = false;
        PrimaryReplicatorSPtr primaryCopy;
        AsyncOperationSPtr inner;
        bool shouldFinish = false;

        {
            AcquireReadLock lock(parent_.lock_);

            // First verify the partition write status, before checking replicator's internal state.
            // This is to ensure we return back the right error code to the caller as RAP has a more up to date view of the partition than the replicator
            if (parent_.VerifyAccessGranted(error))
            {
                // If the write access is granted, replicator must have a primary because we are performing the above check in a read lock and no one could have acquired a write lock to change the state
                bool isPrimary = parent_.VerifyIsPrimaryCallerHoldsLock(L"BeginReplicate", error);
                TESTASSERT_IFNOT(isPrimary, "{0}:BeginReplicate: Replicator is not primary, while partition write access is granted", parent_.ToString());

                if (isPrimary)
                {
                    primaryCopy = parent_.primary_;
                    error.ReadValue();
                    inner = primaryCopy->BeginReplicate(
                        move(comOperationPointer_),
                        sequenceNumber_,
                        [this, primaryCopy](AsyncOperationSPtr const & asyncOperation)
                        {
                            this->ReplicateCallback(asyncOperation, primaryCopy);
                        },
                        thisSPtr);

                    if (inner->CompletedSynchronously)
                    {
                        shouldFinish = true;
                    }
                }
                else
                {
                    shouldComplete = true;
                }
            }
            else
            {
                 shouldComplete = true;
            }
        }
                
        if (shouldComplete)
        {
            // Complete outside the lock
            TryComplete(thisSPtr, error);
        }
        else if (shouldFinish)
        {
            FinishReplicate(inner, primaryCopy);
        }
    }

    void Replicator::ReplicateAsyncOperation::ReplicateCallback(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        if (!asyncOperation->CompletedSynchronously)
        {
            FinishReplicate(asyncOperation, primary);
        }
    }

    void Replicator::ReplicateAsyncOperation::FinishReplicate(
        AsyncOperationSPtr const & asyncOperation,
        PrimaryReplicatorSPtr const & primary)
    {
        ErrorCode error = primary->EndReplicate(asyncOperation);
        this->TryComplete(asyncOperation->Parent, error);
    }
    
    ErrorCode Replicator::ReplicateAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<ReplicateAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
