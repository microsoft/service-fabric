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
using Common::Assert;
using Common::Threadpool;

using std::move;

Replicator::ChangeRoleAsyncOperation::ChangeRoleAsyncOperation(
    __in Replicator & parent,
    FABRIC_EPOCH const & epoch,
    FABRIC_REPLICA_ROLE newRole,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
    :   AsyncOperation(callback, state, true),
        parent_(parent),
        epoch_(epoch),
        newRole_(newRole),
        primaryCopy_(),
        secondaryCopy_()
{
}

void Replicator::ChangeRoleAsyncOperation::OnStart(
    AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error;
    bool shouldComplete = false;
    FABRIC_SEQUENCE_NUMBER cachedProgress = Constants::NonInitializedLSN;
    ReplicatorState::Action nextAction;
    FABRIC_REPLICA_ROLE presentRole;
        
    {
        AcquireWriteLock lock(parent_.lock_);

        presentRole = parent_.state_.role_;
        error = parent_.state_.TransitionToChangingRole(newRole_, nextAction);
        if (!error.IsSuccess())
        {
            ReplicatorEventSource::Events->ReplicatorChangeRoleInvalid(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                static_cast<int>(presentRole),
                static_cast<int>(newRole_));
            shouldComplete = true;
        }
        else
        {
            ReplicatorEventSource::Events->ReplicatorChangeRole(
                parent_.partitionId_,
                parent_.endpointUniqueId_, 
                static_cast<int>(presentRole),
                static_cast<int>(newRole_), 
                epoch_.DataLossNumber,
                epoch_.ConfigurationNumber);
            
            // Make a copy of all data structures needed outside the lock
            cachedProgress = parent_.stateProviderInitialProgress_;
            secondaryCopy_ = parent_.secondary_;
            primaryCopy_ = parent_.primary_;
        }
    }

    if (shouldComplete)
    {
        TryComplete(thisSPtr, error);
        return;
    }
    else if (nextAction == ReplicatorState::None)
    {
        error = parent_.state_.TransitionToRoleNone();
        TryComplete(thisSPtr, error);
        return;
    }

    switch (nextAction)
    {
    case ReplicatorState::CreateInitialPrimary:
        CreateInitialPrimary(cachedProgress, thisSPtr);
        break;
    case ReplicatorState::CreateInitialSecondary:
        error = CreateInitialSecondary();
        TryComplete(thisSPtr, error);
        break;
    case ReplicatorState::PromoteSecondaryToPrimary:
        // Promote Idle/Active Secondary to Primary
        // Old secondary must be closed;
        // after close, the new primary will be created using the secondary queue
        CloseSecondary(thisSPtr, true);
        break;
    case ReplicatorState::DemotePrimaryToSecondary:
        // Promote Primary to secondary
        // Old primary must be closed, and a secondary will be created
        ClosePrimary(thisSPtr, true);
        break;
    case ReplicatorState::PromoteIdleToActive:
        PromoteIdleToActive(thisSPtr);
        break;
    case ReplicatorState::ClosePrimary:
        ASSERT_IFNOT(parent_.state_.Role == FABRIC_REPLICA_ROLE_NONE, "{0}: Replicator ChangeRole can close the primary only when transitioning to role NONE", parent_.endpointUniqueId_);
        ClosePrimary(thisSPtr);
        break;
    case ReplicatorState::CloseSecondary:
        CloseSecondary(thisSPtr);
        break;
    default:
        // Nothing to do
        break;
    }
}

ErrorCode Replicator::ChangeRoleAsyncOperation::End(
    AsyncOperationSPtr const & asyncOperation)
{
    auto casted = AsyncOperation::End<ChangeRoleAsyncOperation>(asyncOperation);
    return casted->Error;
}

void Replicator::ChangeRoleAsyncOperation::CreateInitialPrimary(
    FABRIC_SEQUENCE_NUMBER cachedProgress,
    AsyncOperationSPtr const & thisSPtr)
{
    bool reportFault = false;

    ASSERT_IF(
        primaryCopy_ || secondaryCopy_,
        "{0}: Primary and secondary shouldn't exist on initial Primary open", 
        parent_.ToString());
        
    // Ask the service for initial value or get the cached value
    FABRIC_SEQUENCE_NUMBER initialProgress = cachedProgress;
    ErrorCode error(Common::ErrorCodeValue::Success);
    if (cachedProgress == Constants::NonInitializedLSN)
    {
        error = parent_.stateProvider_.GetLastCommittedSequenceNumber(initialProgress);
        if (error.IsSuccess())
        {
            ReplicatorEventSource::Events->StateProviderProgress(
                parent_.partitionId_,
                parent_.endpointUniqueId_, 
                initialProgress);
        }
    }

    {
        AcquireWriteLock lock(parent_.lock_);
        if (!error.IsSuccess())
        {
            // Since the service cannot provide a valid initial progress, we will report fault on behalf of it
            // If it has already done so, this is still OK
            parent_.state_.TransitionToFaulted();
            reportFault = true;
        }
        else
        {
            error = parent_.state_.TransitionToPrimary();
            if (error.IsSuccess())
            {
                parent_.stateProviderInitialProgress_ = initialProgress;
                parent_.primary_ = PrimaryReplicator::CreatePrimary(
                    parent_.config_,
                    parent_.perfCounters_,
                    parent_.replicaId_,
                    parent_.hasPersistedState_,
                    parent_.endpointUniqueId_,
                    epoch_,
                    parent_.stateProvider_,
                    parent_.partition_,
                    parent_.version_,
                    parent_.transport_,
                    parent_.stateProviderInitialProgress_,
                    parent_.partitionId_,
                    parent_.healthClient_,
                    parent_.apiMonitor_);
                primaryCopy_ = parent_.primary_;
            }
        }
    }

    // Update epoch outside the lock, since it calls external code
    if (primaryCopy_)
    {
        Threadpool::Post([this, thisSPtr]()
        {
            this->ScheduleOpenPrimaryAndUpdateEpoch(thisSPtr);
        });
    }
    else
    {
        if (reportFault)
        {
            Replicator::ReportFault(
                parent_.partition_, parent_.partitionId_, parent_.endpointUniqueId_,
                L"CreateInitialPrimary->GetLastCommittedSequenceNumber failure", error);
        }

        TryComplete(thisSPtr, error);
    }
}

ErrorCode Replicator::ChangeRoleAsyncOperation::CreateInitialSecondary()
{
    AcquireWriteLock lock(parent_.lock_);
    ASSERT_IF(
        parent_.primary_ || parent_.secondary_, 
        "{0}: The primary and secondary shouldn't exist when changing role to IDLE", 
        parent_.ToString());

    ErrorCode error;
    if (newRole_ == ::FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY)
    {
        error = parent_.state_.TransitionToSecondaryActive();
    }
    else
    {
        error = parent_.state_.TransitionToSecondaryIdle();
    }

    if (!error.IsSuccess())
    {
        return error;
    }

    parent_.secondary_ = move(SecondaryReplicator::CreateSecondary(
        parent_.config_,
        parent_.perfCounters_,
        parent_.replicaId_,
        parent_.hasPersistedState_,
        parent_.endpointUniqueId_,
        parent_.stateProvider_,
        parent_.partition_,
        parent_.version_,
        parent_.transport_,
        parent_.partitionId_,
        parent_.healthClient_,
        parent_.apiMonitor_));

    return ErrorCode(Common::ErrorCodeValue::Success);
}

void Replicator::ChangeRoleAsyncOperation::PromoteIdleToActive(AsyncOperationSPtr const & thisSPtr)
{
    ASSERT_IFNOT(
        secondaryCopy_,
        "{0}: The secondary IDLE should exist to promote to ACTIVE", 
        parent_.ToString());

    ErrorCode error;     

    {
        AcquireWriteLock lock(parent_.lock_);
        error = parent_.state_.TransitionToSecondaryActive();
        if (error.IsSuccess())
        {
            secondaryCopy_->PromoteToActiveSecondary(thisSPtr);
        }        
    }        

    if (!error.IsSuccess())
    {
        thisSPtr->TryComplete(thisSPtr, error);
    }
}

void Replicator::ChangeRoleAsyncOperation::ClosePrimary(AsyncOperationSPtr const & thisSPtr, bool createSecondary)
{
    ASSERT_IFNOT(
        primaryCopy_,
        "{0}: The replicator that must be closed should exist", 
        parent_.ToString());

    if (createSecondary && primaryCopy_->CheckReportedFault())
    {
        {
            AcquireWriteLock lock(parent_.lock_);
            parent_.state_.TransitionToFaulted();
        }
        TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
        return;
    }

    auto inner = primaryCopy_->BeginClose(
        createSecondary,
        Common::TimeSpan::Zero,
        [this, createSecondary](AsyncOperationSPtr const & asyncOperation) 
        {
            this->ClosePrimaryCallback(asyncOperation, createSecondary); 
        },
        thisSPtr);
    if (inner->CompletedSynchronously)
    {
        FinishClosePrimary(inner, createSecondary);
    }
}

void Replicator::ChangeRoleAsyncOperation::ClosePrimaryCallback(AsyncOperationSPtr const & asyncOperation,  bool createSecondary)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        FinishClosePrimary(asyncOperation, createSecondary);
    }
}

void Replicator::ChangeRoleAsyncOperation::FinishClosePrimary(AsyncOperationSPtr const & asyncOperation, bool createSecondary)
{
    ErrorCode error = primaryCopy_->EndClose(asyncOperation);
    bool reportFault = false;

    {
        AcquireWriteLock lock(parent_.lock_);
        if (error.IsSuccess())
        {
            if (createSecondary)
            {
                // Promote Primary -> Secondary
                // Pass the new epoch instead of the primary one
                error = parent_.state_.TransitionToSecondaryActive();
                if (error.IsSuccess())
                {
                    primaryCopy_->CreateSecondary(
                        epoch_,
                        parent_.perfCounters_,
                        parent_.healthClient_,
                        parent_.apiMonitor_,
                        /*out*/ parent_.secondary_);

                    parent_.primary_ = nullptr;
                }
            }
            else
            {
                error = parent_.state_.TransitionToRoleNone();
                if (error.IsSuccess())
                {
                    parent_.primary_ = nullptr;
                }
            }
        }

        if (parent_.primary_ != nullptr)
        {
            parent_.state_.TransitionToFaulted();
            reportFault = true;
        }
    }
    
    if (reportFault)
    {
        primaryCopy_->SetReportedFault(error, L"FinishClosePrimary");
    }

    this->TryComplete(asyncOperation->Parent, error);
}

void Replicator::ChangeRoleAsyncOperation::CloseSecondary(AsyncOperationSPtr const & thisSPtr, bool createPrimary)
{
    ASSERT_IFNOT(
        secondaryCopy_,
        "{0}: The replicator that must be closed should exist", 
        parent_.ToString());

    if (createPrimary && secondaryCopy_->CheckReportedFault())
    {
        {
            AcquireWriteLock lock(parent_.lock_);
            parent_.state_.TransitionToFaulted();
        }
        TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
        return;
    }

    auto inner = secondaryCopy_->BeginClose(
        createPrimary /*isPromoting*/,
        [this, createPrimary](AsyncOperationSPtr const & asyncOperation) 
        {
            this->CloseSecondaryCallback(asyncOperation, createPrimary); 
        },
        thisSPtr);
    if (inner->CompletedSynchronously)
    {
        FinishCloseSecondary(inner, createPrimary);
    }
}

void Replicator::ChangeRoleAsyncOperation::CloseSecondaryCallback(AsyncOperationSPtr const & asyncOperation, bool createPrimary)
{
    if (!asyncOperation->CompletedSynchronously)
    {
        FinishCloseSecondary(asyncOperation, createPrimary);
    }
}

void Replicator::ChangeRoleAsyncOperation::FinishCloseSecondary(AsyncOperationSPtr const & asyncOperation, bool createPrimary)
{
    ErrorCode error = secondaryCopy_->EndClose(asyncOperation);
    bool reportFault = false;

    {
        AcquireWriteLock lock(parent_.lock_);
        if (error.IsSuccess())
        {
            if (createPrimary)
            {
                // Promote Secondary -> Primary
                // Pass the specified epoch instead of the secondary one,
                // since the secondary only keeps the invalidated epoch
                error = parent_.state_.TransitionToPrimary();
                if (error.IsSuccess())
                {
                    secondaryCopy_->CreatePrimary(
                        epoch_,
                        parent_.perfCounters_,
                        parent_.healthClient_,
                        parent_.apiMonitor_,
                        /*out*/parent_.primary_);

                    primaryCopy_ = parent_.primary_;
                    parent_.secondary_ = nullptr;
                }
            }
            else
            {
                error = parent_.state_.TransitionToRoleNone();
                if (error.IsSuccess())
                {
                    parent_.secondary_ = nullptr;
                }
            }
        }

        if (parent_.secondary_ != nullptr)
        {
            parent_.state_.TransitionToFaulted();
            reportFault = true;
        }
    }

    if (primaryCopy_)
    {
        Threadpool::Post([this, asyncOperation]()
        {
            this->ScheduleOpenPrimaryAndUpdateEpoch(asyncOperation->Parent);
        });
    }
    else
    {
        if (reportFault)
        {
            secondaryCopy_->SetReportedFault(error, L"FinishCloseSecondary");
        }

        this->TryComplete(asyncOperation->Parent, error);
    }
}

void Replicator::ChangeRoleAsyncOperation::ScheduleOpenPrimaryAndUpdateEpoch(Common::AsyncOperationSPtr const & thisSPtr)
{
    ErrorCode error = primaryCopy_->Open();
    if (!error.IsSuccess())
    {
        TryComplete(thisSPtr, error);
    }
    else
    {
        auto inner = CreateAndStart<UpdateEpochAsyncOperation>(
            parent_,
            epoch_,
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                this->FinishUpdatePrimaryEpoch(asyncOperation, false);
            },
            thisSPtr);
        FinishUpdatePrimaryEpoch(inner, true);
    }
}

void Replicator::ChangeRoleAsyncOperation::FinishUpdatePrimaryEpoch(
    Common::AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (asyncOperation->CompletedSynchronously == completedSynchronously)
    {
        ErrorCode error = UpdateEpochAsyncOperation::End(asyncOperation);

        if (!error.IsSuccess())
        {
            ASSERT_IFNOT(primaryCopy_, "{0}: Primary should exist", parent_.ToString());
            primaryCopy_->SetReportedFault(error, L"FinishUpdatePrimaryEpoch");
        }

        {
            AcquireWriteLock lock(parent_.lock_);
            if (!error.IsSuccess())
            {
                parent_.state_.TransitionToFaulted();
            }
        }
        asyncOperation->Parent->TryComplete(asyncOperation->Parent, error);
    }
}

} // end namespace ReplicationComponent
} // end namespace Reliability
