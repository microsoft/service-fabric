// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

    using Common::Assert;
    using Common::AsyncCallback;
    using Common::AsyncOperation;
    using Common::AsyncOperationSPtr;
    using Common::ComPointer;
    using Common::ErrorCode;

    using std::move;

    PrimaryReplicator::CatchupAsyncOperation::CatchupAsyncOperation(
        __in PrimaryReplicator & parent,
        FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
        AsyncCallback const & callback, 
        AsyncOperationSPtr const & state)
        :   AsyncOperation(callback, state),
            parent_(parent),
            catchUpMode_(catchUpMode)
    {
    }

    void PrimaryReplicator::CatchupAsyncOperation::OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (parent_.CheckReportedFault())
        {
            TryComplete(thisSPtr, Common::ErrorCodeValue::OperationFailed);
            return;
        }
        // Nothing to do on start
        // Note that this runs under a lock.
        // Add another method if we need to do any work outside lock.
    }

    bool PrimaryReplicator::CatchupAsyncOperation::IsAllProgressAchieved(
        FABRIC_SEQUENCE_NUMBER completed,
        FABRIC_SEQUENCE_NUMBER latestSequenceNumber)
    {
        ASSERT_IFNOT(
            CatchupMode == FABRIC_REPLICA_SET_QUORUM_ALL,
            "IsAllProgressAchieved() invoked when catch up mode is not Quorum");

        bool progressAchieved = (completed >= latestSequenceNumber);

        ReplicatorEventSource::Events->PrimaryCatchUpAll(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                progressAchieved,
                latestSequenceNumber,
                completed);
        return progressAchieved;
    }

    bool PrimaryReplicator::CatchupAsyncOperation::IsQuorumProgressAchieved(
        FABRIC_SEQUENCE_NUMBER committed,
        FABRIC_SEQUENCE_NUMBER previousConfigCatchupLsn,
        FABRIC_SEQUENCE_NUMBER lowestLSNAmongstMustCatchupReplicas)
    {
        ASSERT_IFNOT(
            CatchupMode == FABRIC_REPLICA_SET_WRITE_QUORUM,
            "IsQuorumProgressAchieved() invoked when catch up mode is not Quorum");

        bool progressAchieved = (committed >= previousConfigCatchupLsn);

        if (lowestLSNAmongstMustCatchupReplicas == Constants::MaxLSN)
        {
            // This case is for regular catchup quorum during failovers of primary
            ReplicatorEventSource::Events->PrimaryCatchUpQuorum(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                progressAchieved,
                previousConfigCatchupLsn,
                committed);
        }
        else
        {
            // This case is when catchup all is converted by RA into catchup qouorum with target replica
            if (progressAchieved)
            {
                // Even though a quorum of replicas may be caught up, catchup can complete only if the lowest LSN among all replicas that must be caught up is equal to or greater than
                // the current committed

                // today, there is only 1 replica that can have the "Mustcatchup" flag equals to true, but this implementation allows for future changes where more than 1 replica
                // needs to be caught up before completing the async operation
                progressAchieved = (lowestLSNAmongstMustCatchupReplicas >= committed);
            }

            ReplicatorEventSource::Events->PrimaryCatchUpQuorumWithMustCatchupReplicas(
                parent_.partitionId_,
                parent_.endpointUniqueId_,
                progressAchieved,
                previousConfigCatchupLsn,
                committed,
                lowestLSNAmongstMustCatchupReplicas);
        }

        return progressAchieved;
    }

    ErrorCode PrimaryReplicator::CatchupAsyncOperation::End(
        AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<CatchupAsyncOperation>(asyncOperation);
        return casted->Error;
    }
    
} // end namespace ReplicationComponent
} // end namespace Reliability
