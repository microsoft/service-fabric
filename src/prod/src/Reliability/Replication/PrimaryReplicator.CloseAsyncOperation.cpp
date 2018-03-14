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
    using Common::TimeSpan;

    PrimaryReplicator::CloseAsyncOperation::CloseAsyncOperation(
        __in PrimaryReplicator & parent,
        bool isCreatingSecondary,
        TimeSpan const & waitForQuorumTimeout,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state, true),
            parent_(parent),
            waitForQuorumTimeout_(waitForQuorumTimeout),
            isCreatingSecondary_(isCreatingSecondary)
    {
    }

    PrimaryReplicator::CloseAsyncOperation::~CloseAsyncOperation()
    {
    }

    void PrimaryReplicator::CloseAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (waitForQuorumTimeout_ == TimeSpan::Zero)
        {
            PerformClose(thisSPtr);
        }
        else
        {
            ReplicatorEventSource::Events->PrimaryPreCloseWait(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                waitForQuorumTimeout_);

            auto inner = parent_.replicaManagerSPtr_->BeginWaitForReplicateOperationsToComplete(
                waitForQuorumTimeout_,
                [this](AsyncOperationSPtr const & innerAsyncOp)
                {
                    if (!innerAsyncOp->CompletedSynchronously)
                    {
                        this->FinishBeginWaitForReplicateOperationsToComplete(innerAsyncOp);
                    }
                },
                thisSPtr);

            if (inner->CompletedSynchronously)
            {
                this->FinishBeginWaitForReplicateOperationsToComplete(inner);
            }
        }
    }

    void PrimaryReplicator::CloseAsyncOperation::FinishBeginWaitForReplicateOperationsToComplete(AsyncOperationSPtr const & innerAsyncOp)
    {
        auto error = parent_.replicaManagerSPtr_->EndWaitForReplicateOperationsToComplete(innerAsyncOp);

        ReplicatorEventSource::Events->PrimaryPreCloseWaitDone(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            error);

        // Intentionally ignore error code returned and continue with the actual close
        PerformClose(innerAsyncOp->Parent);
    }

    void PrimaryReplicator::CloseAsyncOperation::PerformClose(AsyncOperationSPtr const & thisSPtr)
    {
        ReplicatorEventSource::Events->PrimaryClose(
            parent_.partitionId_,
            parent_.endpointUniqueId_);

        // Cancel all pending replicate async operations and
        // Close replicas, 
        // which will cancel any pending BuildIdle async operations
        ErrorCode error = parent_.replicaManagerSPtr_->Close(isCreatingSecondary_);
        if (!error.IsSuccess())
        {
            TryComplete(thisSPtr, error);
            return;
        }

        // Cancel the catchup async operation (if in progress)
        AsyncOperationSPtr copy;
        {
            AcquireWriteLock lock(parent_.catchupLock_);
            copy = std::move(parent_.catchupAsyncOp_);
        }

        if (copy != nullptr)
        {
            copy->Cancel();
        }

        AsyncOperationSPtr inner = parent_.replicaManagerSPtr_->BeginWaitForReplicateOperationsCallbacksToComplete(
            [this] (AsyncOperationSPtr const & innerAsyncOp)
            {
                if (!innerAsyncOp->CompletedSynchronously)
                {
                    this->FinishBeginWaitForReplicateOperationsCallbacksToComplete(innerAsyncOp);
                }
            },
            thisSPtr);

        if (inner->CompletedSynchronously)
        {
            FinishBeginWaitForReplicateOperationsCallbacksToComplete(inner);
        }
    }

    void PrimaryReplicator::CloseAsyncOperation::FinishBeginWaitForReplicateOperationsCallbacksToComplete(AsyncOperationSPtr const & innerAsyncOp)
    {
        ErrorCode error = parent_.replicaManagerSPtr_->EndWaitForReplicateOperationsCallbacksToComplete(innerAsyncOp);
        ReplicatorEventSource::Events->PrimaryCloseDone(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            error.ReadValue());
        TryComplete(innerAsyncOp->Parent, error);
    }

    ErrorCode PrimaryReplicator::CloseAsyncOperation::End(AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<CloseAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
