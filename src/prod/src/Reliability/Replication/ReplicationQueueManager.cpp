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
using Common::ErrorCode;
using Common::ComPointer;
using Common::make_com;

using Common::AcquireWriteLock;
using Common::AcquireReadLock;
using Common::DateTime;
using Common::StringWriter;

using std::map;
using std::move;
using std::vector;
using std::wstring;

ReplicationQueueManager::ReplicationQueueManager(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    bool requireServiceAck,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_SEQUENCE_NUMBER initialProgress,
    Common::Guid const & partitionId)
    :   config_(config),
        perfCounters_(perfCounters),
        partitionId_(partitionId),
        endpointUniqueId_(endpointUniqueId),
        replicationQueue_(
            partitionId_, 
            GetQueueDescription(endpointUniqueId),
            config->InitialPrimaryReplicationQueueSize, 
            config->MaxPrimaryReplicationQueueSize, 
            config->MaxPrimaryReplicationQueueMemorySize,
            0, /*maxCompletedOperationsCount*/
            0, /*maxCompletedOperationsMemorySize*/
            requireServiceAck, 
            true, /*cleanOnComplete*/
            false, 
            initialProgress + 1,
            perfCounters),
        nextSequenceNumber_(initialProgress + 1),
        previousConfigCatchupLsn_(Constants::InvalidLSN),
        previousConfigQuorumLsn_(Constants::InvalidLSN)
{
}

ReplicationQueueManager::ReplicationQueueManager(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    ReplicationEndpointId const & endpointUniqueId,
    Common::Guid const & partitionId,
    OperationQueue && queue)
    :   config_(config),
        perfCounters_(perfCounters),
        partitionId_(partitionId),
        endpointUniqueId_(endpointUniqueId),
        replicationQueue_(
            GetQueueDescription(endpointUniqueId),
            move(queue),
            config->InitialPrimaryReplicationQueueSize,
            config->MaxPrimaryReplicationQueueSize,
            config->MaxPrimaryReplicationQueueMemorySize,
            0, /*maxCompletedOperationsCount*/
            0, /*maxCompletedOperationsMemorySize*/
            true /*cleanOnComplete */,
            perfCounters),
        nextSequenceNumber_(replicationQueue_.LastSequenceNumber + 1),
        previousConfigCatchupLsn_(Constants::InvalidLSN),
        previousConfigQuorumLsn_(Constants::InvalidLSN)
{
    replicationQueue_.IgnoreCommit = false;
    // Discard any pending replication operations from previous configuration versions
    replicationQueue_.DiscardNonCompletedOperations();
    // Everything in the previous secondary queue is considered committed
    // on the primary.
    replicationQueue_.ResetCompleted();
    previousConfigCatchupLsn_ = replicationQueue_.LastSequenceNumber;
    previousConfigQuorumLsn_ = replicationQueue_.LastCommittedSequenceNumber;
    replicationQueue_.SetCommitCallback(nullptr);

    UpdatePerfCounters();
}

ReplicationQueueManager::~ReplicationQueueManager()
{
}

OperationQueue && ReplicationQueueManager::get_Queue() 
{ 
    // Before changing role to secondary, this queue must be trimmed to the secondary queue limits
    // The way we trim the queue is by calling "Complete" on operations as they will release the operations until the secondary queue size is reached.

    ASSERT_IFNOT(replicationQueue_.LastSequenceNumber == replicationQueue_.LastCommittedSequenceNumber, "{0}: Primary had non committed operations during swap", endpointUniqueId_);

    if (config_->MaxSecondaryReplicationQueueSize !=0 &&
        replicationQueue_.OperationCount > static_cast<ULONGLONG>(config_->MaxSecondaryReplicationQueueSize))
    {
        FABRIC_SEQUENCE_NUMBER completeUpto = replicationQueue_.LastRemovedSequenceNumber + config_->MaxSecondaryReplicationQueueSize;

        ReplicatorEventSource::Events->PrimaryDemoteOperationQueueCount(
            partitionId_,
            endpointUniqueId_,
            replicationQueue_.OperationCount,
            config_->MaxSecondaryReplicationQueueSize,
            replicationQueue_.LastRemovedSequenceNumber + 1,
            completeUpto);

        replicationQueue_.Complete(completeUpto);

        TESTASSERT_IFNOT(
            replicationQueue_.OperationCount == static_cast<ULONGLONG>(config_->MaxSecondaryReplicationQueueSize), "{0}: MaxSecondary queue size is {1}, but operation count is {2} during demotion",
            endpointUniqueId_,
            replicationQueue_.OperationCount,
            config_->MaxSecondaryReplicationQueueSize);
    }

    if (config_->MaxSecondaryReplicationQueueMemorySize != 0 &&
        replicationQueue_.TotalMemorySize > static_cast<ULONGLONG>(config_->MaxSecondaryReplicationQueueMemorySize))
    {
        while(replicationQueue_.TotalMemorySize > static_cast<ULONGLONG>(config_->MaxSecondaryReplicationQueueMemorySize))
        {
            ReplicatorEventSource::Events->PrimaryDemoteOperationQueueMemory(
                partitionId_,
                endpointUniqueId_,
                replicationQueue_.TotalMemorySize,
                config_->MaxSecondaryReplicationQueueMemorySize,
                replicationQueue_.LastRemovedSequenceNumber + 1,
                replicationQueue_.LastRemovedSequenceNumber + 1);

            replicationQueue_.Complete(replicationQueue_.LastRemovedSequenceNumber + 1);
        }
    }

    return std::move(replicationQueue_);
}

// The caller must make sure there are no pending operations on the old queue
// while resetting the queue
void ReplicationQueueManager::ResetQueue(
    FABRIC_SEQUENCE_NUMBER newInitialProgress)
{    
    // Dump all operations in old queue and create a new one
    replicationQueue_.Reset(newInitialProgress + 1);

    // These statements should execute after Reset on the queue
    previousConfigCatchupLsn_ = replicationQueue_.LastSequenceNumber;
    previousConfigQuorumLsn_ = replicationQueue_.LastCommittedSequenceNumber;
    nextSequenceNumber_ = previousConfigCatchupLsn_ + 1;

    UpdatePerfCounters();
}

bool ReplicationQueueManager::GetPendingOperationsToSendDuringBuild(
    __in bool stateproviderSupportsBuildUntilLatestLsn,
    __out FABRIC_SEQUENCE_NUMBER & fromSequenceNumber,
    __out ComOperationRawPtrVector & pendingOperations) const 
{
    // If state provider is V2 replicator which supports copying until latest LSN (unlike KVS), do so
    if (stateproviderSupportsBuildUntilLatestLsn)
    {
        fromSequenceNumber = replicationQueue_.LastSequenceNumber + 1;
        return true;
    }

    if (replicationQueue_.LastCommittedSequenceNumber >= previousConfigQuorumLsn_)
    {
        // If the CC quorum LSN is greater than PC quorum LSN, use it -> this has always been the case and will be the most likely code path 
        fromSequenceNumber = replicationQueue_.LastCommittedSequenceNumber + 1;
    }
    else
    {
        // If the PC quorum is greater than the current quorum (Due to pending catchup of old replicas), let build send all the operations instead of
        // replication stream

        // This is to address a behavior in the V2 replicator which currently cannot handle the case where we ask it to build upto an older state as
        // UpdateEpoch records get missed in the build 
        fromSequenceNumber = previousConfigQuorumLsn_ + 1;
    }

    return replicationQueue_.GetOperations(
        fromSequenceNumber,
        pendingOperations);
}

bool ReplicationQueueManager::GetOperations(
    FABRIC_SEQUENCE_NUMBER start,
    __out ComOperationRawPtrVector & pendingOperations) const 
{
    return replicationQueue_.GetOperations(
        start,
        pendingOperations);
}

FABRIC_SEQUENCE_NUMBER ReplicationQueueManager::GetCurrentProgress() const
{
    auto lastSequenceNumber = replicationQueue_.LastCommittedSequenceNumber;

    ReplicatorEventSource::Events->PrimaryGetInfo(
        partitionId_,
        endpointUniqueId_,
        L"GetCurrentProgress", 
        lastSequenceNumber);
    return lastSequenceNumber;
}

FABRIC_SEQUENCE_NUMBER ReplicationQueueManager::GetCatchUpCapability() const
{
    auto firstSequenceNumber = replicationQueue_.FirstCommittedSequenceNumber;
    
    ReplicatorEventSource::Events->PrimaryGetInfo(
        partitionId_,
        endpointUniqueId_,
        L"GetCatchUpCapability", 
        firstSequenceNumber);
    return firstSequenceNumber;
}

FABRIC_SEQUENCE_NUMBER ReplicationQueueManager::GetLastSequenceNumber() const
{
    auto sequenceNumber = replicationQueue_.LastSequenceNumber;
    
    ReplicatorEventSource::Events->PrimaryGetInfo(
        partitionId_,
        endpointUniqueId_,
        L"GetLastSequenceNumber", 
        sequenceNumber);
    return sequenceNumber;
}

ErrorCode ReplicationQueueManager::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    counters.operationCount = replicationQueue_.OperationCount;
    counters.queueSizeBytes = replicationQueue_.TotalMemorySize;
    counters.allApplyAckLsn = replicationQueue_.LastCompletedSequenceNumber;

    return ErrorCode();
}

ErrorCode ReplicationQueueManager::GetReplicatorStatus(
    __in std::vector<ReplicationSessionSPtr> inBuildReplicas,
    __in std::vector<ReplicationSessionSPtr> readyReplicas,
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    result = move(PrimaryReplicator::GetReplicatorStatusQueryResult(inBuildReplicas, readyReplicas, replicationQueue_));
    return ErrorCode();
}

void ReplicationQueueManager::UpdateCatchupCompletionLSN()
{
    if (previousConfigCatchupLsn_ < replicationQueue_.LastSequenceNumber)
    {
        previousConfigCatchupLsn_ = replicationQueue_.LastSequenceNumber;
    }

    if (previousConfigQuorumLsn_ < replicationQueue_.LastCommittedSequenceNumber)
    {
        previousConfigQuorumLsn_ = replicationQueue_.LastCommittedSequenceNumber;
    }
}

// Update the replication queue;
// the caller should then call CompleteReplicateOperations
// with the oldCommittedLSN and newCommittedLSN numbers.
void ReplicationQueueManager::UpdateQueue(
    FABRIC_SEQUENCE_NUMBER committedLSN,
    FABRIC_SEQUENCE_NUMBER completedLSN,
    __out FABRIC_SEQUENCE_NUMBER & oldCommittedLSN,
    __out FABRIC_SEQUENCE_NUMBER & newCommittedLSN,
    __out bool & removedItemsFromQueue)
{
    removedItemsFromQueue = false;
    ULONGLONG initialItemCount = replicationQueue_.OperationCount;

    oldCommittedLSN = replicationQueue_.LastCommittedSequenceNumber;
    if (committedLSN == Constants::NonInitializedLSN)
    {
        // The primary is the only replica
        replicationQueue_.Commit();
    }
    else
    {
        // Update commit index based on the quorum ACKed
        replicationQueue_.UpdateCommitHead(committedLSN);
    }
    
    // Find the sequence number that is ACKed for all replicas
    // and complete it in the replication queue.
    FABRIC_SEQUENCE_NUMBER completedSeq = replicationQueue_.LastCommittedSequenceNumber;
    if (completedLSN != Constants::NonInitializedLSN && completedLSN < completedSeq)
    {
        completedSeq = completedLSN;
    }
         
    replicationQueue_.UpdateLastCompletedHead(completedSeq);

    newCommittedLSN = replicationQueue_.LastCommittedSequenceNumber;

    if (replicationQueue_.OperationCount < initialItemCount)
    {
        removedItemsFromQueue = true;
    }

    UpdatePerfCounters();
}

void ReplicationQueueManager::GetQueueProgressForCatchup(
    FABRIC_SEQUENCE_NUMBER replicasCommitted,
    FABRIC_SEQUENCE_NUMBER replicasCompleted,
    __out FABRIC_SEQUENCE_NUMBER & committed,
    __out FABRIC_SEQUENCE_NUMBER & completed,
    __out FABRIC_SEQUENCE_NUMBER & latest,
    __out FABRIC_SEQUENCE_NUMBER & previousLast) const
{
    // Look at the queue and see what progress 
    // would be achieved based on the passed in numbers
    // Check the possible new commit number
    if (replicasCommitted == Constants::NonInitializedLSN)
    {
        committed = replicationQueue_.LastSequenceNumber;
    }
    else
    {
        if (replicasCommitted > replicationQueue_.LastSequenceNumber)
        {
            Assert::CodingError(
                "{0}: GetPossibleQueueProgress: replicasCommitted {1} doesn't respect queue invariants {2}",
                endpointUniqueId_,
                replicasCommitted,
                replicationQueue_.ToString());
        }

        committed = replicasCommitted;
    }

    // Check the possible new complete number
    FABRIC_SEQUENCE_NUMBER completedSeq = committed;
    if (replicasCompleted != Constants::NonInitializedLSN && replicasCompleted < completedSeq)
    {
        completedSeq = replicasCompleted;
    }
    
    latest = replicationQueue_.LastSequenceNumber;
    completed = completedSeq;
    previousLast = previousConfigCatchupLsn_;
}

void ReplicationQueueManager::CheckWithCatchupLsn(
    __inout FABRIC_SEQUENCE_NUMBER & sequenceNumber)
{
    if (sequenceNumber >= previousConfigCatchupLsn_)
    {
        sequenceNumber = previousConfigCatchupLsn_;
    }
}

ErrorCode ReplicationQueueManager::Enqueue(
    ComPointer<IFabricOperationData> && comOperationDataPointer,
    FABRIC_EPOCH const & epoch,
    __out ComOperationCPtr & operationComPtr)
{
    ASSERT_IF(
        nextSequenceNumber_ <= replicationQueue_.LastSequenceNumber, 
        "{0}: Error generating next sequence number {1} <= {2}",
        endpointUniqueId_,
        nextSequenceNumber_,
        replicationQueue_.LastSequenceNumber);

    FABRIC_OPERATION_METADATA metadata;
    metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
    metadata.SequenceNumber = nextSequenceNumber_;
    metadata.AtomicGroupId = FABRIC_INVALID_ATOMIC_GROUP_ID;
    metadata.Reserved = NULL;
    
    ComOperationCPtr opPointer = make_com<ComUserDataOperation, ComOperation>(
        move(comOperationDataPointer), 
        metadata,
        epoch);

    ErrorCode error = replicationQueue_.TryEnqueue(opPointer);
    if (!error.IsSuccess())
    {
        ASSERT_IF(
            nextSequenceNumber_ < replicationQueue_.LastCommittedSequenceNumber, 
            "{0}: Enqueue: The operation {1} was already committed", 
            endpointUniqueId_,
            nextSequenceNumber_);
    }
    else
    {
        ++nextSequenceNumber_;
        operationComPtr = opPointer;
    }

    UpdatePerfCounters();
    return error;
}

void ReplicationQueueManager::UpdatePerfCounters()
{
    // As long as the ReplicationQueueManager(this) object exists, the replicationQueue_ is guaranteed to be non-null
    perfCounters_->NumberOfBytesReplicationQueue.Value = replicationQueue_.TotalMemorySize;
    perfCounters_->NumberOfOperationsReplicationQueue.Value = replicationQueue_.OperationCount;
    perfCounters_->ReplicationQueueFullPercentage.Value = Replicator::GetQueueFullPercentage(replicationQueue_);
}

wstring ReplicationQueueManager::GetQueueDescription(ReplicationEndpointId const & id)
{
    wstring queueDescription;
    StringWriter writer(queueDescription);
    // Set queue description so we can filter events of this queue
    // based on the replicator role (Primary)
    // and the type of the operations it holds (Replication)
    writer << L"Primary-REPL-" << id;
    return queueDescription;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
