// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::AcquireReadLock;
using Common::AcquireWriteLock;
using Common::Assert;
using Common::AsyncCallback;
using Common::AsyncOperation;
using Common::AsyncOperationSPtr;
using Common::ErrorCode;

Replicator::OnDataLossAsyncOperation::OnDataLossAsyncOperation(
    __in Replicator & parent,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        isStateChanged_(FALSE),
        apiNameDesc_(Common::ApiMonitoring::ApiNameDescription(Common::ApiMonitoring::InterfaceName::IStateProvider, Common::ApiMonitoring::ApiName::OnDataLoss, L""))
{
}

void Replicator::OnDataLossAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
{
    ASSERT_IFNOT(
        parent_.primary_,
        "{0}: OnDataLoss: primary should exist",
        parent_.partitionId_);
    if (parent_.primary_->CheckReportedFault())
    {
        TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
        return;
    }

    ErrorCode error;
        
    {
        AcquireWriteLock lock(parent_.lock_);
        error = parent_.state_.TransitionToCheckingDataLoss();
    }
                
    if (!error.IsSuccess())
    {
        // Complete outside the lock
        TryComplete(thisSPtr, error);
    }
    else
    {
        auto callDesc = ApiMonitoringWrapper::GetApiCallDescriptionFromName(parent_.endpointUniqueId_, apiNameDesc_, parent_.config_->SlowApiMonitoringInterval);
        parent_.apiMonitor_->StartMonitoring(callDesc);

        auto inner = parent_.stateProvider_.BeginOnDataLoss(
            [this, callDesc](AsyncOperationSPtr const & asyncOperation) 
            { 
                this->OnDataLossCallback(asyncOperation, callDesc); 
            }, 
            thisSPtr);
        if (inner->CompletedSynchronously)
        {
            FinishOnDataLoss(inner, callDesc);
        }
    }
}
    
void Replicator::OnDataLossAsyncOperation::OnDataLossCallback(
    AsyncOperationSPtr const & asyncOperation,
    Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        FinishOnDataLoss(asyncOperation, callDesc);
    }
}

void Replicator::OnDataLossAsyncOperation::FinishOnDataLoss(
    AsyncOperationSPtr const & asyncOperation,
    Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
{
    ErrorCode error = parent_.stateProvider_.EndOnDataLoss(
        asyncOperation, 
        isStateChanged_);

    parent_.apiMonitor_->StopMonitoring(callDesc, error);

    if (!error.IsSuccess())
    {
        this->TryComplete(asyncOperation->Parent, error);
        return;
    }

    if (isStateChanged_ == TRUE)
    {
        // Get the latest status from state provider 
        // and clean up the replication queue
        FABRIC_SEQUENCE_NUMBER newProgress;
        error = parent_.stateProvider_.GetLastCommittedSequenceNumber(newProgress);
        if (!error.IsSuccess())
        {
            this->TryComplete(asyncOperation->Parent, error);
            return;
        }

        {
            // Reset the replication queue under the lock;
            // since no progress should happen meanwhile,
            // it's ok to do it under the lock.
            AcquireWriteLock lock(parent_.lock_);
            ReplicatorEventSource::Events->ReplicatorDataLossStateChanged(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                parent_.stateProviderInitialProgress_,
                newProgress);

            ASSERT_IFNOT(
                parent_.primary_,
                "{0}: OnDataLoss: primary should exist",
                parent_.partitionId_);
            parent_.stateProviderInitialProgress_ = newProgress;
            error = parent_.primary_->ResetReplicationQueue(parent_.stateProviderInitialProgress_);
        }
    }
    else
    {
        ReplicatorEventSource::Events->ReplicatorDataLoss(
            parent_.partitionId_,
            parent_.endpointUniqueId_);
    }

    this->TryComplete(asyncOperation->Parent, error);
}

ErrorCode Replicator::OnDataLossAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation, 
    __out BOOLEAN & isStateChanged)
{
    auto casted = AsyncOperation::End<OnDataLossAsyncOperation>(asyncOperation);
    isStateChanged = casted->isStateChanged_;
    
    {
        AcquireWriteLock lock(casted->parent_.lock_);
        // Restore the replicator state to original one
        ErrorCode error = casted->parent_.state_.TransitionToPrimary();
        if (casted->Error.IsSuccess())
        {
            return error;
        }
        // Ignore the new error, return the original one
        error.ReadValue();
    }
    
    return casted->Error;
}
    
} // end namespace ReplicationComponent
} // end namespace Reliability
