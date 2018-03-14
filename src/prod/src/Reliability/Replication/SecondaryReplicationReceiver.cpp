// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::AsyncCallback;
using Common::ComPointer;
using Common::ErrorCode;
using Common::make_com;
using Common::make_unique;
using Common::ReaderQueue;
using Common::StringWriter;

using std::move;
using std::shared_ptr;
using std::vector;
using std::wstring;

SecondaryReplicationReceiver::SecondaryReplicationReceiver(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    bool requireServiceAck,
    ReplicationEndpointId const & endpointUniqueId,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    bool enableEOSOpAndStreamFaults)
    :   config_(config),
        perfCounters_(perfCounters),
        requireServiceAck_(requireServiceAck),
        endpointUniqueId_(endpointUniqueId),
        partitionId_(partitionId),
        enableEOSOpAndStreamFaults_(enableEOSOpAndStreamFaults),
        // Delay creating the replication queue until
        // the replication start sequence is received
        replicationQueue_(),
        dispatchQueue_(ReaderQueue<ComOperationCPtr>::Create()),
        stream_(),
        streamCreated_(false),
        stateProviderUpdatedEpoch_(Constants::InvalidEpoch),
        startCopyEpoch_(Constants::InvalidEpoch),
        progressVector_(),
        lastReplicationCompletedAck_(Constants::InvalidLSN),
        enqueuedEndOfStreamOperation_(false),
        ackedEndOfStreamOperation_(!requireServiceAck),
        replicationOrCopyStreamFaulted_(false),
        healthReporter_()
{
    healthReporter_ = BatchedHealthReporter::Create(
        partitionId_,
        endpointUniqueId_,
        HealthReportType::SecondaryReplicationQueueStatus,
        config->QueueHealthMonitoringInterval,
        healthClient);
}

SecondaryReplicationReceiver::SecondaryReplicationReceiver(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    bool requireServiceAck,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    Common::Guid const & partitionId,
    bool enableEOSOpAndStreamFaults,
    IReplicatorHealthClientSPtr const & healthClient,
    OperationQueue && primaryQueue)
    :   config_(config),
        perfCounters_(perfCounters),
        requireServiceAck_(requireServiceAck),
        endpointUniqueId_(endpointUniqueId),
        partitionId_(partitionId),
        enableEOSOpAndStreamFaults_(enableEOSOpAndStreamFaults),
        // Create replication queue based on the previous primary one
        replicationQueue_(), 
        dispatchQueue_(ReaderQueue<ComOperationCPtr>::Create()),
        stream_(),
        streamCreated_(false),
        stateProviderUpdatedEpoch_(epoch),
        startCopyEpoch_(epoch),
        progressVector_(),
        lastReplicationCompletedAck_(Constants::InvalidLSN),
        enqueuedEndOfStreamOperation_(false),
        ackedEndOfStreamOperation_(!requireServiceAck),
        replicationOrCopyStreamFaulted_(false),
        healthReporter_()
{
    healthReporter_ = BatchedHealthReporter::Create(
        partitionId_,
        endpointUniqueId_,
        HealthReportType::SecondaryReplicationQueueStatus,
        config->QueueHealthMonitoringInterval,
        healthClient);
    
    wstring queueLog;
    Common::StringWriter writer(queueLog);
    writer << L"Secondary-REPL-" << endpointUniqueId_;
    
    if (config->SecondaryClearAcknowledgedOperations)
    {
        replicationQueue_ = make_unique<OperationQueue>(
            queueLog,
            move(primaryQueue),
            config->InitialSecondaryReplicationQueueSize,
            config->MaxSecondaryReplicationQueueSize,
            config->MaxSecondaryReplicationQueueMemorySize,
            0, 
            0, 
            true, // Clean on complete
            perfCounters_);
    }
    else
    {
        replicationQueue_ = make_unique<OperationQueue>(
            queueLog,
            move(primaryQueue),
            config->InitialSecondaryReplicationQueueSize,
            config->MaxSecondaryReplicationQueueSize,
            config->MaxSecondaryReplicationQueueMemorySize,
            config->MaxPrimaryReplicationQueueSize,
            config->MaxPrimaryReplicationQueueMemorySize,
            false, // Clean on complete
            perfCounters_);
    }

    // At this point, we could possibly have some operations in the queue after the introduction of QC_QUORUM with mustcatchup instead of catchup all
    // That is because all replicas may not have been caught up and hence all operations are not completed from the primary replicator's perspective to discard it.

    // However, having non completed operations in the queue is not good after becoming secondary because any operation that is not completed is categorized as not 
    // being ACK'd by the user service - which is clearly not the case here as the replica has already applied its state
    // This will cause an assert when we call "IgnoreCommit" later below
    // Hence, call complete here to complete any pending operation
    replicationQueue_->Complete();
    
    UpdateReplicationQueue();
}

SecondaryReplicationReceiver::~SecondaryReplicationReceiver()
{
    healthReporter_->Close();
}

OperationQueue && SecondaryReplicationReceiver::get_Queue() 
{ 
    ASSERT_IFNOT(
        replicationQueue_, 
        "{0}: Replication queue should have been created before promoting S->P",
        endpointUniqueId_);
    
    return std::move(*(replicationQueue_.get())); 
}

OperationStreamSPtr SecondaryReplicationReceiver::GetStream(
    SecondaryReplicatorSPtr const & secondary)
{
    auto streamSPtr = stream_.lock();
    if (!streamSPtr)
    {
        streamSPtr = std::make_shared<OperationStream>(
            true,           // this is a replication stream
            secondary,
            dispatchQueue_,
            partitionId_,
            endpointUniqueId_,
            enableEOSOpAndStreamFaults_);
        stream_ = streamSPtr;
        streamCreated_.store(true);
    }
    
    return streamSPtr;
}

FABRIC_SEQUENCE_NUMBER SecondaryReplicationReceiver::GetCurrentProgress() const
{
    ASSERT_IFNOT(
        replicationQueue_, 
        "{0}: GetCurrentProgress: Replication queue doesn't exist", 
        endpointUniqueId_);
    // Fix for bug 808654
    // For persisted service, during GetLSN phase, we return the highest received in-order LSN,
    // instead of service ACK'ed LSN.  If this replica is promoted to primary, it will wait
    // until all received operation be drained (completed) by the service when changing role.
    // If this replica goes down, the second most advanced replica will be picked as new primary
    // and it will do the same thing.  This is to avoid the replica with highest ACK'ed LSN but 
    // not highest received LSN being selected as new primary.  As will cause false progress 
    // in the latter, which is not handled and will assert.
    // For volatile service, the last committed LSN is equal to last completed LSN.
    return replicationQueue_->LastCommittedSequenceNumber;
}

FABRIC_SEQUENCE_NUMBER SecondaryReplicationReceiver::GetCatchUpCapability() const
{
    ASSERT_IFNOT(
        replicationQueue_, 
        "{0}: GetCatchUpCapability: Replication queue doesn't exist", 
        endpointUniqueId_);
    return replicationQueue_->FirstAvailableCompletedSequenceNumber;
}

FABRIC_SEQUENCE_NUMBER SecondaryReplicationReceiver::GetNextToBeCompleted() const
{
    if (!replicationQueue_)
    {
        return Constants::NonInitializedLSN;
    }
    else
    {
        return replicationQueue_->NextToBeCompletedSequenceNumber;
    }
}

void SecondaryReplicationReceiver::DiscardOutOfOrderOperations()
{
    if (replicationQueue_)
    {
        // Clean any out-of-order operations in the replication queue
        if (requireServiceAck_)
        {
            // Out of order are items that are not yet committed (given to the service)
            replicationQueue_->DiscardNonCommittedOperations();
        }
        else
        {
            replicationQueue_->DiscardNonCompletedOperations();
        }

        UpdatePerfCountersAndReportHealthIfNeeded();
    }
}

void SecondaryReplicationReceiver::EnqueueEndOfStreamOperationAndKeepRootAlive(OperationAckCallback const & callback, Common::ComponentRoot const & root)
{
    ASSERT_IFNOT(enableEOSOpAndStreamFaults_, "{0}: Cannot enqueue end of stream operation if config is not enabled", endpointUniqueId_);
    if (enqueuedEndOfStreamOperation_)
    {
        return;
    }

    enqueuedEndOfStreamOperation_ = true;
    dispatchQueue_->EnqueueWithoutDispatch(
        make_unique<ComOperationCPtr>(ComFromBytesOperation::CreateEndOfStreamOperationAndKeepRootAlive(callback, root)));

    ReplicatorEventSource::Events->SecondaryEOSOperation(partitionId_, endpointUniqueId_, Constants::ReplOperationTrace, Constants::EnqueueOperationTrace);
}

void SecondaryReplicationReceiver::CleanQueue(
    Common::ComponentRoot const & root,
    bool clearQueueItems,
    bool ignoreEOSOperationAck)
{
    DiscardOutOfOrderOperations();
    if (replicationQueue_ && requireServiceAck_)
    {
        // For persisted services, use the operation queue to mark 
        // all committed operations to ignore Acknowledge callback
        ComOperationRawPtrVector committedOperations;
        replicationQueue_->GetOperations(
            replicationQueue_->LastCompletedSequenceNumber + 1,
            committedOperations);

        for (auto const & operation : committedOperations)
        {
            operation->SetIgnoreAckAndKeepParentAlive(root);
        }

        if (clearQueueItems)
        {
            // Release ref to the operations in the queue.
            // The operations that are pending service ACKs
            // are going to keep the secondary alive.
            // If the queue keeps the operations alive, 
            // there is a circular dependency that may cause a leak
            // if the service doesn't ack an operation.    
            replicationQueue_->Reset(replicationQueue_->NextToBeCompletedSequenceNumber);
        }
    }

    if (ignoreEOSOperationAck)
    {
        MarkEOSOperationACK(true /* ackIgnored */);
    }
}

void SecondaryReplicationReceiver::CleanQueueOnClose(
    Common::ComponentRoot const & root,
    bool clearQueueItems,
    bool ignoreEOSOperationAck)
{
    //closing the health reporter when the replica is closing, as the object may stay longer in the memory until the destructor gets called.
    healthReporter_->Close();
    CleanQueue(root, clearQueueItems, ignoreEOSOperationAck);
}

void SecondaryReplicationReceiver::CleanQueueOnUpdateEpochFailure(Common::ComponentRoot const & root)
{
    CleanQueue(root, true, true);
}

void SecondaryReplicationReceiver::MarkEOSOperationACK(bool ackIgnored)
{
    if (enqueuedEndOfStreamOperation_)
    {
        // If end of stream was enqueued already, we have ignored its ACK above. If we dont mark this, we will be stuck waiting for the ACK forever
        if (!ackedEndOfStreamOperation_)
        {
            if (ackIgnored)
            {
                ReplicatorEventSource::Events->SecondaryEOSOperation(
                    partitionId_, endpointUniqueId_, Constants::ReplOperationTrace, L"Ignore ACK");
            }
            else
            {
                ReplicatorEventSource::Events->SecondaryEOSOperation(
                    partitionId_, endpointUniqueId_, Constants::ReplOperationTrace, Constants::AckOperationTrace);
            }
            ackedEndOfStreamOperation_ = true;
        }
    }
}

void SecondaryReplicationReceiver::GetAck(
    __out FABRIC_SEQUENCE_NUMBER & replicationCommittedLSN, 
    __out FABRIC_SEQUENCE_NUMBER & replicationCompletedLSN)
{
    if (replicationQueue_)
    {
        replicationCommittedLSN = replicationQueue_->LastCommittedSequenceNumber;
        replicationCompletedLSN = replicationQueue_->LastCompletedSequenceNumber;
    }
    else
    {
        replicationCommittedLSN = Constants::NonInitializedLSN;
        replicationCompletedLSN = Constants::NonInitializedLSN;
    }

    lastReplicationCompletedAck_ = replicationCompletedLSN;
}

void SecondaryReplicationReceiver::ProcessStartCopyOperation(
    FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber,
    FABRIC_EPOCH const & startCopyEpoch,
    __out bool & progressDone)
{
    progressDone = false;
    
    ASSERT_IF(replicationQueue_, "{0}: Replication Queue should not exist", endpointUniqueId_);
     
    startCopyEpoch_ = startCopyEpoch;
    CreateReplicationQueue(replicationStartSequenceNumber);
}

bool SecondaryReplicationReceiver::ProcessReplicationOperation(
    std::vector<ComOperationCPtr> && batchOperation,
    FABRIC_SEQUENCE_NUMBER completedSequenceNumber,
    __out bool & checkForceSendAck)
{
    checkForceSendAck = false;
    bool progressMade = false;

    ASSERT_IFNOT(replicationQueue_, "{0}: Replication Queue should exist", endpointUniqueId_);

    ErrorCode error;
    for (auto item = batchOperation.begin(); item != batchOperation.end(); item++)
    {
        if (stateProviderUpdatedEpoch_ == Constants::InvalidEpoch &&
            ((*item)->SequenceNumber == (replicationQueue_->LastRemovedSequenceNumber + 1)))
        {
            // This is the first replication operation received by this secondary
            // Based on the operation epoch, establish the state provider epoch

            auto firstOperationEpoch = (*item)->Epoch;

            // If the first operation has an epoch smaller than start copy, it is possibly due to a cancelled swap. Hence initialize the SP epoch to this
            // smaller epoch so that we call UE on SP when an operation in a higher epoch comes in
            // RD: RDBug 7248183: Assert "this.lastStableLsn <= tailLsn. last stable lsn :{0}" during false progress in v2 replicator

            // If the first operation has an epoch higher than start copy, initialize to the original start copy epoch to ensure we call UE on the first 
            // dispatch itself

            stateProviderUpdatedEpoch_ =
                firstOperationEpoch < startCopyEpoch_ ?
                firstOperationEpoch :
                startCopyEpoch_;

            ReplicatorEventSource::Events->SecondaryEstablishStateProviderEpoch(
                partitionId_,
                endpointUniqueId_,
                stateProviderUpdatedEpoch_.DataLossNumber,
                stateProviderUpdatedEpoch_.ConfigurationNumber);
        }

        error = replicationQueue_->TryEnqueue(move(*item));

        if (error.IsSuccess())
        {
            if (!TryDispatchReplication(checkForceSendAck))
            {
                UpdatePerfCountersAndReportHealthIfNeeded();
                return progressMade;
            }
            else
            {
                // Dispatch is made only when operations are received in order. We must assert that the state provider epoch has been established
                ASSERT_IF(
                    stateProviderUpdatedEpoch_ == Constants::InvalidEpoch,
                    "{0}: State Provider Epoch is Invalid, but dispatch of operation successful. Queue = {1}",
                    endpointUniqueId_,
                    replicationQueue_->ToString());

                // Atleast one operation was dispatched
                progressMade = true;
            }

            // continue enqueing all items we got, regardless of whether we can dispatch them or not.
            // we do not want to waste the network send that has come in
        }
        else if (error.IsError(Common::ErrorCodeValue::REQueueFull))
        {
            healthReporter_->ScheduleWarningReport(Common::wformatString("Secondary Queue Full : {0}", replicationQueue_->ToString()));
            // stop trying to enqueue more as we have hit a queue full error
            break;
        }
        // continue enqueing more
    }

    if (completedSequenceNumber != Constants::InvalidLSN && replicationQueue_)
    {
        // 'CompleteHead' corresponds to the first available sequence number in the operation queue
        // Since the primary is indicating that all operations including "completedSequenceNumber" have been ack'd by all secondary replicas
        // we can safely remove all operations including the LSN "completedSequenceNumber"
        //
        // In order to do so, we increment the LSN by 1 as that will be the first operation in the queue

        // This call only succeeds if the operations have been committed and completed in the first place
        replicationQueue_->UpdateCompleteHead(completedSequenceNumber + 1);
    }

    UpdatePerfCountersAndReportHealthIfNeeded();
    return true;
}

void SecondaryReplicationReceiver::DispatchReplicationOperation(ComOperationCPtr const & operation)
{
    // Check whether primary epoch changed 
    // If yes, enqueue a special operation in the queue
    // that will be trapped after the previous operation is taken by the service
    // and will be transformed into an UpdateEpoch called on state provider
    bool needFullBarrier;
    UpdateEpochIfNeededCallerHoldsLock(operation, needFullBarrier);
    if (needFullBarrier)
    {
        // For persisted services, a new operation epoch acts as a full barrier:
        // the service can't receive any more operations 
        // until it ACKs any pending ones.
        // To achieve this, disable the replication callback.
        // It will be re-enabled when all pending operations are ACKed.
        SetReplicationCallback(false);
        return;
    }

    // Enqueue without dispatch because we don't want 
    // the caller to execute callbacks inside the queues lock.
    // We will call Dispatch after Complete returns.
    bool success = dispatchQueue_->EnqueueWithoutDispatch(make_unique<ComOperationCPtr>(operation));
    ASSERT_IFNOT(success, "{0}: EnqueueWithoutDispatch Replication should succeed", endpointUniqueId_);
    ReplicatorEventSource::Events->SecondaryEnqueueWithoutDispatch(
        partitionId_, 
        endpointUniqueId_, 
        Constants::ReplOperationTrace,
        operation->SequenceNumber);
}

void SecondaryReplicationReceiver::UpdateEpochIfNeededCallerHoldsLock(
    ComOperationCPtr const & operation,
    __out bool & needFullBarrier)
{
    needFullBarrier = false;
    FABRIC_EPOCH const & operationEpoch = operation->Epoch;

    ASSERT_IF(stateProviderUpdatedEpoch_ == Constants::InvalidEpoch, "{0} State Provider Epoch is not yet established", endpointUniqueId_);

    if (!(operationEpoch <= stateProviderUpdatedEpoch_))
    {
        if (!requireServiceAck_ || 
            replicationQueue_->NextToBeCompletedSequenceNumber == operation->SequenceNumber)
        {
            // For optimistic acks, update epoch when the 
            // service picks up the operation from the dispatch queue
            auto opPtr = make_com<ComUpdateEpochOperation,ComOperation>(
                operationEpoch, 
                operation->SequenceNumber - 1);
            // Enqueue a special operation to signify update epoch
            bool success = dispatchQueue_->EnqueueWithoutDispatch(
                make_unique<ComOperationCPtr>(move(opPtr)));

            ASSERT_IFNOT(success, "{0}: EnqueueWithoutDispatch Replication should succeed", endpointUniqueId_);
        }
        else
        {
            // UpdateEpoch must be called after service ACKs the previous operations.
            // Since the operation ACKs can come out of order,
            // remember the progress vector entry when the operations are enqueued
            // in dispatch queue (in order);
            // When Acks from the service arrive, 
            // the entry will be used to update the state provider
            ASSERT_IF(
                progressVector_,
                "{0}: Dispatch REPL: Progress vector already exists {1}-{2}, can't update it to {3}-{4}",
                endpointUniqueId_,
                progressVector_->first,
                progressVector_->second,
                operationEpoch,
                operation->SequenceNumber - 1);

            progressVector_ = make_unique<ProgressVectorEntry>(
                operationEpoch, 
                operation->SequenceNumber - 1);
            needFullBarrier = true;
        }

        stateProviderUpdatedEpoch_ = operationEpoch;
    }
}

bool SecondaryReplicationReceiver::OnAckReplicationOperation(
    ComOperation const & operation,
    __out ProgressVectorEntryUPtr & pendingUpdateEpoch)
{
    // The operations can be acked out of order.
    // Complete will inspect if operations with lower LSN
    // are acked before completing, and it will stop at the first non-acked one
    // (if any).
    ASSERT_IFNOT(
        replicationQueue_,
        "{0}: Repl operation {1} ACKed, but replication queue doesn't exist",
        endpointUniqueId_,
        operation.SequenceNumber);
    
    if (!replicationQueue_->Complete())
    {
        // No new progress, nothing to do
        return false;
    }
    
    UpdatePerfCountersAndReportHealthIfNeeded();
                
    if (progressVector_)
    {
        // Compare the progress vector entry with the epoch and lsn 
        // of the latest completed operation.
        FABRIC_EPOCH const & progressVectorEpoch = progressVector_->first;
        FABRIC_SEQUENCE_NUMBER lastCompletedOpLSN = replicationQueue_->LastCompletedSequenceNumber;
        FABRIC_EPOCH lastCompletedOpEpoch = progressVectorEpoch;
        bool performAssertCheck = true;
        // Check whether the current operation is the last completed one;
        // if it's not, it means we received previous out of order ACKs
        // and the current operation filled in the gap.
        if (operation.SequenceNumber != lastCompletedOpLSN)
        {
            // Take the epoch of the latest completed operation from the queue
            ComOperationRawPtr lastCompletedOp = replicationQueue_->GetOperation(lastCompletedOpLSN);
            ASSERT_IF(lastCompletedOp == nullptr && config_->SecondaryClearAcknowledgedOperations == false, "{0}: Last completed op can't be null when secondary does NOT clear operations on acks", endpointUniqueId_);
            
            if (lastCompletedOp == nullptr)
            {
                performAssertCheck = false;
            }
            else
            {
                lastCompletedOpEpoch = lastCompletedOp->Epoch;
            }
        }
        else
        {
            // The current operation is the latest completed, 
            // so there's no need to look at the item in the queue
            lastCompletedOpEpoch = operation.Epoch;
        }

        if (performAssertCheck)
        {
            ASSERT_IF(
                progressVectorEpoch <= lastCompletedOpEpoch ||
                progressVector_->second < lastCompletedOpLSN,
                "{0}: StateProvider UpdateEpoch {1}-{2} should have been called before Ack {3}-{4} is received",
                endpointUniqueId_,
                progressVectorEpoch,
                progressVector_->second,
                lastCompletedOpEpoch,
                lastCompletedOpLSN);
        }
            
        if (progressVector_->second == lastCompletedOpLSN)
        {
            pendingUpdateEpoch = move(progressVector_);
        }
    }

    return true;
}

void SecondaryReplicationReceiver::OnAckEndOfStreamOperation(ComOperation const & operation)
{
    ASSERT_IFNOT(operation.IsEndOfStreamOperation, "{0}: Unknown end of stream Operation Type {1}", endpointUniqueId_, operation.Metadata.Type);
    ASSERT_IFNOT(enqueuedEndOfStreamOperation_, "{0}: Received sentionel operation ACK without enqueing it", endpointUniqueId_);
    
    MarkEOSOperationACK();
}


void SecondaryReplicationReceiver::UpdateStateProviderEpochDone(bool closeInProgress)
{
    if (closeInProgress && replicationQueue_)
    {
        // The other operations have not been dispatched to the service yet;
        // Since the replicator is closing, it's ok to just discard them
        replicationQueue_->DiscardNonCompletedOperations();
        UpdatePerfCountersAndReportHealthIfNeeded();
    }
    else
    {
        // Enable the replication commit callback,
        // so pending operations are dispatched to the service
        SetReplicationCallback(true);
    }
}

void SecondaryReplicationReceiver::CreateReplicationQueue(
    FABRIC_SEQUENCE_NUMBER replicationStartSeq)
{
    wstring queueLog;
    Common::StringWriter writer(queueLog);
    writer << L"Secondary-REPL-" << endpointUniqueId_;

    // As soon as replication operations arrive in order,
    // they are moved to the dispatch queue.
    // For non-persisted services, the ignore commit flag is enabled.
    // For persisted services, the flag is disabled.
    bool ignoreCommit = !requireServiceAck_;
        
    OperationQueueUPtr queue;
    
    if (config_->SecondaryClearAcknowledgedOperations)
    {
         queue = make_unique<OperationQueue>(
            partitionId_,
            queueLog,
            config_->InitialSecondaryReplicationQueueSize,
            config_->MaxSecondaryReplicationQueueSize, 
            config_->MaxSecondaryReplicationQueueMemorySize,
            0,
            0,
            requireServiceAck_,
            true, /*cleanOnComplete*/
            ignoreCommit,
            replicationStartSeq,
            perfCounters_);
    }
    else
    {
        queue = make_unique<OperationQueue>(
            partitionId_,
            queueLog,
            config_->InitialSecondaryReplicationQueueSize, 
            config_->MaxSecondaryReplicationQueueSize, 
            config_->MaxSecondaryReplicationQueueMemorySize,
            config_->MaxPrimaryReplicationQueueSize, 
            config_->MaxPrimaryReplicationQueueMemorySize, 
            requireServiceAck_,
            false, /*cleanOnComplete*/
            ignoreCommit,
            replicationStartSeq,
            perfCounters_);
   }
    
    // Initially, do not set the commit callback;
    // the callback will be set when all copy operations are received, 
    // so we can start dispatching replication operations.
    replicationQueue_ = move(queue);
    SetReplicationCallback(true);

    UpdatePerfCountersAndReportHealthIfNeeded();
}

void SecondaryReplicationReceiver::UpdateReplicationQueue()
{
    replicationQueue_->CleanOnComplete = config_->SecondaryClearAcknowledgedOperations;
    replicationQueue_->IgnoreCommit = !requireServiceAck_; 
    // Enable replication callback
    SetReplicationCallback(true);
    
    UpdatePerfCountersAndReportHealthIfNeeded();
}

void SecondaryReplicationReceiver::SetReplicationCallback(bool shouldEnable)
{
    ReplicatorEventSource::Events->SecondaryReplDispatchCallback(
        partitionId_,
        endpointUniqueId_,
        shouldEnable);

    if (shouldEnable)
    {
        replicationQueue_->SetCommitCallback(
            [this](ComOperationCPtr const & operation)->void 
            { 
                this->DispatchReplicationOperation(operation); 
            });
    }
    else
    {
        replicationQueue_->SetCommitCallback(nullptr);
    }
}

bool SecondaryReplicationReceiver::TryDispatchReplication(
    __out bool & checkForceSendAck)
{
    checkForceSendAck = false;
    if (requireServiceAck_)
    {
        return replicationQueue_->Commit();
    }
    else
    {
        // Immediately put the items in the dispatch queue, 
        // complete the operation and
        // send ACK to primary that operation is done
        if (replicationQueue_->Complete())
        {
            checkForceSendAck = true;
            return true;
        }

        return false;
    }
}

FABRIC_SEQUENCE_NUMBER SecondaryReplicationReceiver::GetAckProgress()
{
    FABRIC_SEQUENCE_NUMBER progress = 0;
    if (replicationQueue_)
    {
        progress = replicationQueue_->LastCompletedSequenceNumber - lastReplicationCompletedAck_;
    }
    
    return progress;
}

ServiceModel::ReplicatorQueueStatusSPtr SecondaryReplicationReceiver::GetQueueStatusForQuery() const
{
    if (replicationQueue_)
    {
        return move(std::make_shared<ServiceModel::ReplicatorQueueStatus>(Replicator::GetReplicatorQueueStatusForQuery(*replicationQueue_)));
    }
    else
    {
        return move(std::make_shared<ServiceModel::ReplicatorQueueStatus>());
    }
}

inline void SecondaryReplicationReceiver::UpdatePerfCountersAndReportHealthIfNeeded()
{
    // This method must be invoked only in code-paths where the replicationQueue_ exists
    if (perfCounters_) // It could have been reset after a close/abort
    {
        perfCounters_->NumberOfBytesReplicationQueue.Value = replicationQueue_->TotalMemorySize;
        perfCounters_->NumberOfOperationsReplicationQueue.Value = replicationQueue_->OperationCount;
        uint queueUsage = Replicator::GetQueueFullPercentage(*replicationQueue_);
        perfCounters_->ReplicationQueueFullPercentage.Value = queueUsage;

        if (queueUsage < config_->QueueHealthWarningAtUsagePercent)
        {
            healthReporter_->ScheduleOKReport();
        }
        else
        {
            wstring description = GetHealthReportWarningDescription(
                replicationQueue_,
                queueUsage,
                static_cast<uint>(config_->QueueHealthWarningAtUsagePercent));

            healthReporter_->ScheduleWarningReport(description);
        }
    }
}

void SecondaryReplicationReceiver::OnStreamReportFault(Common::ComponentRoot const & root)
{
    replicationOrCopyStreamFaulted_ = true;
    CleanQueueOnClose(root, true/*clearItems*/, false/*ignoreEOSOperationAck*/);
}

wstring SecondaryReplicationReceiver::GetHealthReportWarningDescription(
    __in OperationQueueUPtr const & queue,
    __in uint const & queueUsage,
    __in uint const & queueUsageThreshold)
{
    std::wstring content;
    Common::StringWriter writer(content);

    writer.WriteLine("Secondary Replication Queue Usage of {0}% has reached/exceeded the threshold {1}%. First Replication Operation = {2}, Last Replication Operation = {3}", queueUsage, queueUsageThreshold, queue->LastCompletedSequenceNumber, queue->LastSequenceNumber);
    writer.WriteLine("Detailed Progress Information:");
    writer.WriteLine(queue->ToString());

    return content;
}


} // end namespace ReplicationComponent
} // end namespace Reliability
