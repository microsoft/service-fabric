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
using Common::AsyncManualResetEvent;
using Common::ComponentRoot;
using Common::ErrorCode;
using Common::Threadpool;
using Common::TimeSpan;
using Common::DateTime;
using Common::JobQueue;

using Transport::Message;
using Transport::MessageUPtr;
using Transport::ISendTarget;

using std::move;
using std::shared_ptr;
using std::vector;
using std::wstring;

struct OperationDispatchJobPtr
{
public:
    OperationDispatchJobPtr()
    {
    }

    explicit OperationDispatchJobPtr(DispatchQueueSPtr && dispatchQueue)
        : dispatchQueue_(move(dispatchQueue))
    {
    }
    
    bool ProcessJob(SecondaryReplicator & replicator)
    {
        UNREFERENCED_PARAMETER(replicator);
        dispatchQueue_->Dispatch();
        return true;
    }

private:
    DispatchQueueSPtr dispatchQueue_;
};

class SecondaryReplicator::OperationDispatchJobQueue
    : public JobQueue<OperationDispatchJobPtr, SecondaryReplicator>
{
    DENY_COPY(OperationDispatchJobQueue);

public:
    static void Create(std::wstring const & name, SecondaryReplicator & root, int threadCount, _Out_ OperationDispatchJobQueueUPtr & ptr)
    {
        ptr = std::unique_ptr<OperationDispatchJobQueue>(new OperationDispatchJobQueue(name, root, threadCount));
    }

private:
    OperationDispatchJobQueue(std::wstring const & name, SecondaryReplicator & root, int threadCount)
        : JobQueue(name, root, false, threadCount)
    {
    }
};

// Create the initial Idle replica
SecondaryReplicatorSPtr SecondaryReplicator::CreateSecondary(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor)
{
    auto secondary = std::shared_ptr<SecondaryReplicator>(new SecondaryReplicator(
        config,
        perfCounters,
        replicaId,
        hasPersistedState,
        endpointUniqueId,
        stateProvider,
        partition,
        version,
        transport,
        partitionId,
        healthClient,
        apiMonitor));

    secondary->Open();
    return secondary;
}

// Create a secondary using an old primary's replication queue
SecondaryReplicatorSPtr SecondaryReplicator::CreateSecondary(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    OperationQueue && primaryQueue)
{
    auto secondary = std::shared_ptr<SecondaryReplicator>(new SecondaryReplicator(
        config,
        perfCounters,
        replicaId, 
        hasPersistedState,
        endpointUniqueId,
        epoch,
        stateProvider,
        partition,
        version,
        transport,
        partitionId,
        healthClient,
        apiMonitor,
        move(primaryQueue)));

    secondary->Open();

    // Since copy is done, there will be no copy operations to trigger dispatch. 
    // Instead, trigger it now, so the service can pick up the last empty operation
    // if it needs it.
    secondary->ScheduleDispatchCopy();
    return secondary;
}

// Create Idle on initial open
SecondaryReplicator::SecondaryReplicator(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor)
    :   ComponentRoot(),
        config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        hasPersistedState_(hasPersistedState),
        requireServiceAck_(hasPersistedState || config->RequireServiceAck),
        endpointUniqueId_(endpointUniqueId),
        enableEOSOpAndStreamFaults_(config->UseStreamFaultsAndEndOfStreamOperationAck),
        minAllowedEpoch_(Constants::InvalidEpoch),
        startCopyEpoch_(Constants::InvalidEpoch),
        stateProvider_(stateProvider),
        partition_(partition),
        version_(version),
        transport_(transport),
        partitionId_(partitionId),
        replicationReceiver_(
            config,
            perfCounters,
            hasPersistedState || config->RequireServiceAck,
            endpointUniqueId,
            partitionId,
            healthClient,
            config->UseStreamFaultsAndEndOfStreamOperationAck),
        copyReceiver_(
            config,
            hasPersistedState || config->RequireServiceAck,
            endpointUniqueId,
            partitionId,
            config->UseStreamFaultsAndEndOfStreamOperationAck,
            false /*copyDone*/),
        copySender_(),
        copyErrorCodeValue_(0),
        faultErrorCode_(),
        faultType_(FABRIC_FAULT_TYPE_TRANSIENT),
        faultDescription_(),
        faultErrorLock_(),
        copyAckCallback_(),
        replicationAckCallback_(),
        closeAsyncOperation_(),
        drainCopyOperationAsyncOperation_(),
        drainReplOperationAsyncOperation_(),
        primaryAddress_(),
        primaryDemuxerActor_(),
        primaryTarget_(),
        ackSender_(config, partitionId, endpointUniqueId),
        isActive_(true),
        queuesLock_(),
        finishedStateProviderUpdateEpoch_(make_shared<AsyncManualResetEvent>()),
        doNotInvokeUEOnStateProvider_(false),
        finishedStateProviderGetCopyContext_(make_shared<AsyncManualResetEvent>()),
        lastReplicationOperationReceivedAt_(DateTime::Zero),
        lastCopyOperationReceivedAt_(DateTime::Zero),
        isPromotedToActiveSecondary_(false),
        healthClient_(healthClient),
        apiMonitor_(apiMonitor),
        dispatchJobQueue_(nullptr),
        replicationAckHeadersSPtr_(),
        copyContextHeadersSPtr_(),
        replicationTraceHandler_(static_cast<ULONG64>(config->SecondaryReplicatorBatchTracingArraySize)),
        copyTraceHandler_(static_cast<ULONG64>(config->SecondaryReplicatorBatchTracingArraySize))
{
    // Do not create the replication queue 
    // because we don't know the starting sequence number.
    // The copy queue and the temporary buffer are constructed;
    // they will be needed to bring the replica up-to-date.
    
    ReplicatorEventSource::Events->SecondaryCtorInitial(
        partitionId_, 
        endpointUniqueId_, 
        L"initial open",
        endpointUniqueId_.IncarnationId);
}

SecondaryReplicator::SecondaryReplicator(
    REInternalSettingsSPtr const & config,
    REPerformanceCountersSPtr const & perfCounters,
    FABRIC_REPLICA_ID replicaId,
    bool hasPersistedState,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_EPOCH const & epoch,
    ComProxyStateProvider const & stateProvider,
    ComProxyStatefulServicePartition const & partition,
    Replicator::ReplicatorInternalVersion version,
    ReplicationTransportSPtr const & transport,
    Common::Guid const & partitionId,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    OperationQueue && primaryQueue)
    :   ComponentRoot(),
        config_(config),
        perfCounters_(perfCounters),
        replicaId_(replicaId),
        hasPersistedState_(hasPersistedState),
        requireServiceAck_(hasPersistedState || config->RequireServiceAck),
        endpointUniqueId_(endpointUniqueId),
        enableEOSOpAndStreamFaults_(config->UseStreamFaultsAndEndOfStreamOperationAck),
        minAllowedEpoch_(epoch),
        startCopyEpoch_(epoch),
        stateProvider_(stateProvider),
        partition_(partition),
        version_(version),
        transport_(transport),
        partitionId_(partitionId),
        replicationReceiver_(
            config, 
            perfCounters,
            hasPersistedState || config->RequireServiceAck,
            endpointUniqueId,
            epoch,
            partitionId,
            config->UseStreamFaultsAndEndOfStreamOperationAck,
            healthClient,
            move(primaryQueue)),
        copyReceiver_(
            config,
            hasPersistedState || config->RequireServiceAck,
            endpointUniqueId,
            partitionId,
            config->UseStreamFaultsAndEndOfStreamOperationAck,
            true /*copyDone*/),
        copySender_(),
        copyErrorCodeValue_(0),
        faultErrorCode_(),
        faultType_(FABRIC_FAULT_TYPE_TRANSIENT),
        faultDescription_(),
        faultErrorLock_(),
        copyAckCallback_(),
        replicationAckCallback_(),
        closeAsyncOperation_(),
        primaryAddress_(),
        primaryDemuxerActor_(),
        primaryTarget_(nullptr),
        ackSender_(config, partitionId, endpointUniqueId),
        isActive_(true),
        queuesLock_(),
        finishedStateProviderUpdateEpoch_(make_shared<AsyncManualResetEvent>()),
        doNotInvokeUEOnStateProvider_(false),
        finishedStateProviderGetCopyContext_(make_shared<AsyncManualResetEvent>()),
        lastReplicationOperationReceivedAt_(DateTime::Zero),
        lastCopyOperationReceivedAt_(DateTime::Zero),
        isPromotedToActiveSecondary_(true),
        healthClient_(healthClient),
        apiMonitor_(apiMonitor),
        dispatchJobQueue_(nullptr),
        replicationAckHeadersSPtr_(),
        copyContextHeadersSPtr_(),
        replicationTraceHandler_(static_cast<ULONG64>(config->SecondaryReplicatorBatchTracingArraySize)),
        copyTraceHandler_(static_cast<ULONG64>(config->SecondaryReplicatorBatchTracingArraySize))
{
    ReplicatorEventSource::Events->SecondaryCtor(
        partitionId_, 
        endpointUniqueId_, 
        L"demote to secondary",
        epoch.DataLossNumber,
        epoch.ConfigurationNumber,
        endpointUniqueId_.IncarnationId);
}

void SecondaryReplicator::Open()
{
    ackSender_.Open(
        *this,
        [this](bool shouldTrace) { return this->SendAck(shouldTrace); });

    if (requireServiceAck_)
    {
        copyAckCallback_ = [this] (ComOperation & operation)
        {
            this->OnAckCopyOperation(operation);
        };

        replicationAckCallback_ = [this] (ComOperation & operation)
        {
            this->OnAckReplicationOperation(operation);
        };

        endOfStreamCopyAckCallback_ = [this](ComOperation & operation)
        {
            this->OnAckEndOfStreamCopyOperation(operation);
        };

        endOfStreamReplicationAckCallback_ = [this](ComOperation & operation)
        {
            this->OnAckEndOfStreamReplicationOperation(operation);
        };
    }

    finishedStateProviderUpdateEpoch_->Set();
    finishedStateProviderGetCopyContext_->Set();

    OperationDispatchJobQueue::Create(
        endpointUniqueId_.ToString(),
        *this,
        0,
        dispatchJobQueue_);
}
    
SecondaryReplicator::~SecondaryReplicator()
{
    ReplicatorEventSource::Events->SecondaryDtor(
        partitionId_, 
        endpointUniqueId_);
}

void SecondaryReplicator::CreatePrimary(
    FABRIC_EPOCH const & epoch,
    REPerformanceCountersSPtr const & perfCounters,
    IReplicatorHealthClientSPtr const & healthClient,
    ApiMonitoringWrapperSPtr const & apiMonitor,
    __out PrimaryReplicatorSPtr & primary)
{
    primary = PrimaryReplicator::CreatePrimary(
        config_,
        perfCounters, // Pass in the function parameter rather than member variable as the member is reset in close/abort
        replicaId_,
        hasPersistedState_,
        endpointUniqueId_, 
        epoch, 
        stateProvider_, 
        partition_,
        version_,
        transport_, 
        partitionId_,
        healthClient,
        apiMonitor,
        move(replicationReceiver_.Queue));
}

// ******* Close *******
AsyncOperationSPtr SecondaryReplicator::BeginClose(
    bool isPromoting,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    AsyncOperationSPtr closeAsyncOpCopy;

    {
        AcquireWriteLock lock(queuesLock_);
        isActive_ = false;
        // Replicator ensures there is only one call to close happening
        ASSERT_IF(
            closeAsyncOperation_,
            "{0}: Can't start multiple secondary Close operations",
            endpointUniqueId_);
        
        closeAsyncOperation_ = enableEOSOpAndStreamFaults_ ? 
            AsyncOperation::CreateAndStart<CloseAsyncOperation2>(*this, isPromoting, callback, state) :
            AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, isPromoting, callback, state); 
        closeAsyncOpCopy = closeAsyncOperation_;
    }

    // OnStart does minimal work under lock--resume the operation outside the lock
    if (enableEOSOpAndStreamFaults_)
    {
        AsyncOperation::Get<CloseAsyncOperation2>(closeAsyncOpCopy)->ResumeOutsideLock(closeAsyncOpCopy);
    }
    else
    {
        AsyncOperation::Get<CloseAsyncOperation>(closeAsyncOpCopy)->ResumeOutsideLock(closeAsyncOpCopy);
    }
    
    return closeAsyncOpCopy;
}

ErrorCode SecondaryReplicator::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    ErrorCode error = enableEOSOpAndStreamFaults_ ? 
        CloseAsyncOperation2::End(asyncOperation) : 
        CloseAsyncOperation::End(asyncOperation);

    if (enableEOSOpAndStreamFaults_)
    {
        ackSender_.Close();
    }

    {
        AcquireWriteLock lock(queuesLock_);
        closeAsyncOperation_.reset();
        // Reset the primary target, so no messages are sent to the primary 
        // past this point
        primaryTarget_.reset();
        perfCounters_.reset();
        apiMonitor_.reset();
    }
    
    return error;
}

void SecondaryReplicator::Abort()
{
    // Cancel drain async operations if necessary
    {
        AsyncOperationSPtr drainCopyAsyncOperationCopy;
        AsyncOperationSPtr drainReplAsyncOperationCopy;
        {
            AcquireWriteLock queuesLock(queuesLock_);
            isActive_ = false;
            drainCopyAsyncOperationCopy = drainCopyOperationAsyncOperation_;
            drainReplAsyncOperationCopy = drainReplOperationAsyncOperation_;
        }
        if (drainCopyAsyncOperationCopy)
        {
            drainCopyAsyncOperationCopy->Cancel();
        }
        if (drainReplAsyncOperationCopy)
        {
            drainReplAsyncOperationCopy->Cancel();
        }
    }

    CopySenderSPtr cachedCopySender;
    {
        AcquireWriteLock queuesLock(queuesLock_);

        cachedCopySender = copySender_;

        // NOTE - The order of these calls is important
        // 1. Enqueue the end of stream operation
        // 2. Clean queue on close will mark the end of stream operation as acked and ignore acks from other operations
        EnqueueEndOfStreamOperations(); // This will only enqueue the operation if config is enabled
        replicationReceiver_.CleanQueueOnClose(*this, true, true);
        copyReceiver_.CleanQueueOnClose(*this, true);
    }

    // Abort both replication and copy dispatch queues.
    // 3. Finally, dispatch the operation which will dispatch the end of stream operation
    CloseDispatchQueues(true, true);

    ackSender_.Close();

    if (cachedCopySender)
    {
        cachedCopySender->FinishOperation(ErrorCode(Common::ErrorCodeValue::OperationCanceled));
    }
    
    // Ignore any errors
    finishedStateProviderGetCopyContext_->WaitOne();

    shared_ptr<AsyncManualResetEvent> event;
    {
        AcquireWriteLock queuesLock(queuesLock_);
        doNotInvokeUEOnStateProvider_ = true;
        event = finishedStateProviderUpdateEpoch_;
    }

    event->WaitOne();
}

void SecondaryReplicator::CloseDispatchQueues(bool abortCopy, bool abortRepl)
{
    ReplicatorEventSource::Events->SecondaryCloseDispatchQueues(
        partitionId_,
        endpointUniqueId_,
        abortCopy,
        abortRepl);
    
    dispatchJobQueue_->Close();

    if (abortRepl)
    {
        replicationReceiver_.DispatchQueue->Abort();
    }
    else
    {
        replicationReceiver_.DispatchQueue->Close();
    }

    if (abortCopy)
    {
        copyReceiver_.DispatchQueue->Abort();
    }
    else
    {
        copyReceiver_.DispatchQueue->Close();    
    }

    auto root = this->CreateComponentRoot();
    Threadpool::Post([this, root]() 
    { 
        ReplicatorEventSource::Events->SecondaryDispatch(
            partitionId_, 
            endpointUniqueId_);

        copyReceiver_.DispatchQueue->Dispatch();
        replicationReceiver_.DispatchQueue->Dispatch();
    });
}

// ******* GetStatus *******
ErrorCode SecondaryReplicator::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber)
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }

    AcquireReadLock lock(queuesLock_);
    if (copyReceiver_.IsCopyInProgress() || 
        !replicationReceiver_.IsReplicationQueueCreated())
    {
        lastSequenceNumber = Constants::NonInitializedLSN;
        SetReportedFault(
            ErrorCode(Common::ErrorCodeValue::NotReady),
            L"GetCurrentProgress during copy");

        return Common::ErrorCodeValue::NotReady;
    }
    else 
    {
        lastSequenceNumber = replicationReceiver_.GetCurrentProgress();
    }

    ReplicatorEventSource::Events->SecondaryGetInfo(
        partitionId_, 
        endpointUniqueId_, 
        L"GetCurrentProgress",
        lastSequenceNumber,
        minAllowedEpoch_.DataLossNumber,
        minAllowedEpoch_.ConfigurationNumber);
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode SecondaryReplicator::GetCatchUpCapability(
    __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber)
{
    if (CheckReportedFault())
    {
        return Common::ErrorCodeValue::OperationFailed;
    }

    AcquireReadLock lock(queuesLock_);
    if (copyReceiver_.IsCopyInProgress() || 
        !replicationReceiver_.IsReplicationQueueCreated())
    {
        firstSequenceNumber = Constants::NonInitializedLSN;
        SetReportedFault(
            ErrorCode(Common::ErrorCodeValue::NotReady),
            L"GetCatchUpCapability during copy");

        return Common::ErrorCodeValue::NotReady;
    }
    else 
    {
        firstSequenceNumber = replicationReceiver_.GetCatchUpCapability();
    }

    ReplicatorEventSource::Events->SecondaryGetInfo(
        partitionId_, 
        endpointUniqueId_, 
        L"GetCatchUpCapability",
        firstSequenceNumber,
        minAllowedEpoch_.DataLossNumber,
        minAllowedEpoch_.ConfigurationNumber);
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode SecondaryReplicator::GetReplicatorStatus(
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    ServiceModel::ReplicatorQueueStatusSPtr replicationQueueStatus;
    ServiceModel::ReplicatorQueueStatusSPtr copyQueueStatus;
    DateTime lastReplicationOperationReceivedTime;
    DateTime lastCopyOperationReceivedTime;
    DateTime lastAckSentTime;
    bool isInBuild = false;
    
    {
        AcquireReadLock lock(queuesLock_);
        lastReplicationOperationReceivedTime = lastReplicationOperationReceivedAt_;
        lastCopyOperationReceivedTime = lastCopyOperationReceivedAt_;
        lastAckSentTime = ackSender_.LastAckSentAt;
        replicationQueueStatus = move(replicationReceiver_.GetQueueStatusForQuery());
        copyQueueStatus = move(copyReceiver_.GetQueueStatusForQuery());
        isInBuild = !isPromotedToActiveSecondary_.load();
    }
    
    result = move(ServiceModel::SecondaryReplicatorStatusQueryResult::Create(
        move(replicationQueueStatus),
        move(lastReplicationOperationReceivedTime),
        isInBuild,
        move(copyQueueStatus),
        move(lastCopyOperationReceivedTime),
        move(lastAckSentTime)));

    return ErrorCode();
}

// ******* UpdateEpoch *******
AsyncOperationSPtr SecondaryReplicator::BeginUpdateEpoch(
    FABRIC_EPOCH const & epoch,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
        *this, epoch, callback, state);
}

ErrorCode SecondaryReplicator::EndUpdateEpoch(
    AsyncOperationSPtr const & asyncOperation)
{
    return UpdateEpochAsyncOperation::End(asyncOperation);
}

// ******* Get copy and replication streams *******
OperationStreamSPtr SecondaryReplicator::GetCopyStream(SecondaryReplicatorSPtr const & thisSPtr)
{
    AcquireWriteLock lock(queuesLock_);
    return copyReceiver_.GetStream(thisSPtr);
}

OperationStreamSPtr SecondaryReplicator::GetReplicationStream(SecondaryReplicatorSPtr const & thisSPtr)
{
    AcquireWriteLock lock(queuesLock_);
    return replicationReceiver_.GetStream(thisSPtr);
}

bool SecondaryReplicator::SendAck(bool shouldTrace)
{
    FABRIC_SEQUENCE_NUMBER copyCommittedLSN;
    FABRIC_SEQUENCE_NUMBER copyCompletedLSN;
    FABRIC_SEQUENCE_NUMBER replicationCommittedLSN;
    FABRIC_SEQUENCE_NUMBER replicationCompletedLSN;
    wstring primaryTarget;
    KBuffer::SPtr headersStream;
    ISendTarget::SPtr target;
    int errorCodeValue;
    std::vector<SecondaryReplicatorTraceInfo> pendingReplOpTraces;
    std::vector<SecondaryReplicatorTraceInfo> pendingCopyOpTraces;

    GetAck(
        replicationCommittedLSN,
        replicationCompletedLSN,
        copyCommittedLSN,
        copyCompletedLSN,
        primaryTarget,
        headersStream,
        target,
        errorCodeValue,
        pendingReplOpTraces,
        pendingCopyOpTraces);

    if (!target)
    {
        // Do not send ACK if the primary target is not set
        // (the secondary must have been closed)
        return false;
    }
    
    MessageUPtr message = ReplicationTransport::CreateAckMessage(
        replicationCommittedLSN,
        replicationCompletedLSN,
        copyCommittedLSN,
        copyCompletedLSN,
        errorCodeValue);

    if (shouldTrace)
    {
        ReplicatorEventSource::Events->SecondarySendVerboseAcknowledgement(
            partitionId_,
            endpointUniqueId_,
            primaryTarget,
            replicationCommittedLSN,
            replicationCompletedLSN,
            copyCommittedLSN,
            copyCompletedLSN,
            errorCodeValue,
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index),
            pendingReplOpTraces,
            pendingCopyOpTraces,
            config_->ToString());
    }
    else
    {
        ReplicatorEventSource::Events->SecondarySendAcknowledgement(
            partitionId_,
            endpointUniqueId_,
            primaryTarget,
            replicationCommittedLSN,
            replicationCompletedLSN,
            copyCommittedLSN,
            copyCompletedLSN,
            errorCodeValue,
            message->MessageId.Guid,
            static_cast<uint32>(message->MessageId.Index),
            pendingReplOpTraces,
            pendingCopyOpTraces);
    }

    return SendTransportMessage(
        move(headersStream),
        target,
        move(message),
        false);
}

bool SecondaryReplicator::SendCopyContextOperation(
    ComOperationCPtr const & operationPtr,
    bool isLast)
{
    MessageUPtr message = ReplicationTransport::CreateCopyContextOperationMessage(
        operationPtr, 
        isLast);
    
    ReplicatorEventSource::Events->SecondarySendCC(
        partitionId_,
        endpointUniqueId_, 
        operationPtr->SequenceNumber,
        isLast,
        message->MessageId.Guid,
        static_cast<uint32>(message->MessageId.Index));
    
    KBuffer::SPtr headersStream;
    ISendTarget::SPtr target;
    {
        AcquireReadLock lock(queuesLock_);
        headersStream = copyContextHeadersSPtr_;
        target = primaryTarget_;
    }

    if (!target)
    {
        // Do not send the message if the primary target is not set
        // (the secondary must have been closed)
        return false;
    }

    return SendTransportMessage(move(headersStream), target, move(message), true);
}

void SecondaryReplicator::GetAck(
    __out FABRIC_SEQUENCE_NUMBER & replicationCommittedLSN, 
    __out FABRIC_SEQUENCE_NUMBER & replicationCompletedLSN, 
    __out FABRIC_SEQUENCE_NUMBER & copyCommittedLSN,
    __out FABRIC_SEQUENCE_NUMBER & copyCompletedLSN,
    __out wstring & primaryTarget,
    __out KBuffer::SPtr & headersStream,
    __out ISendTarget::SPtr & target,
    __out int & errorCodeValue,
    __out std::vector<SecondaryReplicatorTraceInfo> & pendingReplOpMessages,
    __out std::vector<SecondaryReplicatorTraceInfo> & pendingCopyOpMessages)
{
    AcquireWriteLock lock(queuesLock_);
    // Look at the copy and replication queues 
    // and send ACK with the latest progress.
    copyReceiver_.GetAck(copyCommittedLSN, copyCompletedLSN);
    replicationReceiver_.GetAck(replicationCommittedLSN, replicationCompletedLSN);

    if (primaryTarget_)
    {
        primaryTarget = primaryTarget_->Address();
    }

    headersStream = replicationAckHeadersSPtr_;
    target = primaryTarget_;
    errorCodeValue = copyErrorCodeValue_;

    pendingReplOpMessages = replicationTraceHandler_.GetTraceMessages();
    pendingCopyOpMessages = copyTraceHandler_.GetTraceMessages();
}

// ******* Process Start Copy Message *******
void SecondaryReplicator::StartCopyMessageHandler(
    __in Message & message,
    ReplicationFromHeader const & fromHeader)
{
    FABRIC_EPOCH epoch;
    FABRIC_REPLICA_ID replicaId;
    FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber;
    ReplicationTransport::GetStartCopyFromMessage(message, epoch, replicaId, replicationStartSequenceNumber);

    ReplicatorEventSource::Events->SecondaryReceive(
        partitionId_,
        endpointUniqueId_,
        Constants::StartCopyOperationTrace,
        replicationStartSequenceNumber,
        fromHeader.Address,
        fromHeader.DemuxerActor);

    bool shouldDispatch;
    bool startCopyContext;

    if (ProcessStartCopy(
        fromHeader,
        epoch,
        replicaId,
        replicationStartSequenceNumber,
        shouldDispatch,
        startCopyContext))
    {
        if (shouldDispatch || startCopyContext)
        {
            FinishProcessStartCopy(shouldDispatch, startCopyContext);
        }
    }
}

void SecondaryReplicator::CopyContextCallback(AsyncOperationSPtr const & asyncOperation)
{
    CopySenderSPtr localCopySender;
    {
        AcquireWriteLock lock(queuesLock_);
        localCopySender = move(this->copySender_);
    }

    if (localCopySender)
    {
        ErrorCode faultErrorCode;
        wstring faultDescription;
        ErrorCode error = localCopySender->EndCopy(asyncOperation, faultErrorCode, faultDescription);
        if (!faultErrorCode.IsSuccess())
        {
            SetReportedFault(faultErrorCode, faultDescription);
            return;
        }
        
        if (!error.IsSuccess())
        {
            ProcessCopyContextFailure(error);
        }
    }
}

void SecondaryReplicator::ProcessCopyContextFailure(ErrorCode const & error)
{
    ASSERT_IF(
        error.IsSuccess(),
        "{0}: ProcessCopyContextFailure shouldn't be called on success",
        endpointUniqueId_);
    bool sendAck = false;
    {
        // If copy is not finished, set the copy error, 
        // so next ACK sent to primary
        // will let it know that the secondary copy context failed.
        AcquireWriteLock lock(queuesLock_);
        if (copyReceiver_.IsCopyInProgress())
        {
            copyErrorCodeValue_ = error.ReadValue();
            sendAck = true;
        }
    }

    if (sendAck)
    {
        // The primary should be notified immediately of failure,
        // so force the ACK to be sent now
        ackSender_.ScheduleOrSendAck(true);
    }
}

bool SecondaryReplicator::ProcessStartCopy(
    ReplicationFromHeader const & fromHeader,
    FABRIC_EPOCH const & primaryEpoch,
    FABRIC_REPLICA_ID replicaId,
    FABRIC_SEQUENCE_NUMBER replicationStartSequenceNumber,
    __out bool & progressDone,
    __out bool & startCopyContext)
{
    progressDone = false;
    startCopyContext = false;

    if (replicaId != replicaId_)
    {
        ReplicatorEventSource::Events->SecondaryDropMsgDueToReplicaId(
            partitionId_,
            endpointUniqueId_,
            Constants::StartCopyOperationTrace,
            replicationStartSequenceNumber,
            replicaId,
            replicaId_);

        return false;
    }

    {
        AcquireWriteLock lock(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_, 
                endpointUniqueId_,
                Constants::StartCopyOperationTrace);

            return false;
        }
        
        // If min epoch is established, we have already accepted a copy request.
        // We will check if there is a new primary trying to build and if so restart. Else this is a stale message
        if (this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            if (primaryEpoch < startCopyEpoch_)
            {
                // Drop messages received from an old primary
                ReplicatorEventSource::Events->SecondaryDropMsgDueToEpoch(
                    partitionId_,
                    endpointUniqueId_,
                    Constants::StartCopyOperationTrace,
                    replicationStartSequenceNumber,
                    primaryEpoch.DataLossNumber,
                    primaryEpoch.ConfigurationNumber,
                    startCopyEpoch_.DataLossNumber,
                    startCopyEpoch_.ConfigurationNumber);

                return false;
            }

            if (startCopyEpoch_ < primaryEpoch)
            {
                // There were updates to the system (epoch) that weren't propagated to this replica.
                SetReportedFault(
                    ErrorCode(Common::ErrorCodeValue::InvalidState),
                    L"Secondary received StartCopy with higher epoch than expected");

                return false;
            }
        }

        if (!replicationReceiver_.IsReplicationQueueCreated())
        {
            replicationReceiver_.ProcessStartCopyOperation(
                replicationStartSequenceNumber,
                primaryEpoch,
                progressDone);

            minAllowedEpoch_ = primaryEpoch;
            startCopyEpoch_ = primaryEpoch;

            ReplicatorEventSource::Events->SecondaryEstablishMinAndStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber);

            copyErrorCodeValue_ = 0;

            // We need to send ACK, so update the primary target and demuxer id
            SetPrimaryInfoCallerHoldsLock(fromHeader.DemuxerActor, fromHeader.Address);

            startCopyContext = hasPersistedState_;
        }
    }
    
    ackSender_.ScheduleOrSendAck(true);

    return true;
}

void SecondaryReplicator::FinishProcessStartCopy(
    bool shouldDispatch,
    bool startCopyContext)
{
    if (shouldDispatch)
    {
        // Dispatch all items in the dispatch queue
        // outside the queues lock,
        // after the timer was started, to avoid send delays.
        replicationReceiver_.DispatchQueue->Dispatch();
    }

    if (startCopyContext)
    {
        auto localCopySender = std::make_shared<CopySender>(
            config_,
            partition_,
            partitionId_,
            Constants::CopyContextOperationTrace,
            endpointUniqueId_,
            primaryDemuxerActor_.ReplicaId,
            false,
            apiMonitor_,
            Common::ApiMonitoring::ApiName::GetNextCopyContext);

        {
            AcquireWriteLock lock(queuesLock_);
            if (!isActive_)
            {
                ReplicatorEventSource::Events->SecondaryNotActive(
                    partitionId_,
                    endpointUniqueId_,
                    Constants::CopyContextAckOperationTrace);

                return;
            }

            ASSERT_IF(copySender_, "{0}: Copy manager already exists, but replication queue was not created", endpointUniqueId_);
            copySender_ = localCopySender;
        }

        // Finish processing on a different thread,
        // as user code may be executed
        auto root = this->CreateComponentRoot();
        Threadpool::Post([this, root, localCopySender]()
        {
            this->BeginCopyContext(localCopySender);
        });
    }
}

void SecondaryReplicator::BeginCopyContext(CopySenderSPtr copySender)
{
    {
        AcquireWriteLock grab(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_,
                endpointUniqueId_,
                L"BeginCopyContext");
            return;
        }

        finishedStateProviderGetCopyContext_->Reset();
    }
    
    // Ask the state provider for copy context
    ComProxyAsyncEnumOperationData enumerator;
    ErrorCode error = stateProvider_.GetCopyContext(enumerator);
    finishedStateProviderGetCopyContext_->Set();

    if (!error.IsSuccess())
    {
        SetReportedFault(error, L"GetCopyContext", ::FABRIC_FAULT_TYPE_TRANSIENT);
        ProcessCopyContextFailure(error);
        return;
    }
            
    // Start the copy operations that takes operations
    // from the enumerator and sends them to the primary
    EnumeratorLastOpCallback enumLastOpCallback = 
        [this, copySender] (FABRIC_SEQUENCE_NUMBER lastCopySequenceNumber)
        {
            copySender->OnLastEnumeratorCopyOp(lastCopySequenceNumber, Constants::NonInitializedLSN);
        };

    SendOperationCallback copySendCallback = 
        [this, copySender](ComOperationCPtr const & op, bool requestAck, FABRIC_SEQUENCE_NUMBER completedSeqNumber)
        {
            UNREFERENCED_PARAMETER(completedSeqNumber);

            ASSERT_IF(requestAck, "{0}: Secondary CopyCONTEXT shouldn't ask for Ack", endpointUniqueId_);
            bool isLast;
            {
                isLast = copySender->IsOperationLast(op);
            }

            return this->SendCopyContextOperation(op, isLast);
        };
    
    copySender->BeginCopy(
        move(enumerator),
        copySendCallback,
        enumLastOpCallback,
        [this](AsyncOperationSPtr const & asyncOperation) 
        { 
            this->CopyContextCallback(asyncOperation);
        },
        this->CreateAsyncOperationRoot());
}

// ******* Process Copy Context Ack Message *******
void SecondaryReplicator::CopyContextAckMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader)
{
    wstring const & toAddress = fromHeader.Address;
    ReplicationEndpointId const & toActor = fromHeader.DemuxerActor;

    CopySenderSPtr cachedCopySender;
    {
        AcquireReadLock queuesLock(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_, 
                endpointUniqueId_,
                Constants::CopyContextAckOperationTrace);

            return;
        }

        if (!this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToNoStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::ReplOperationTrace,
                -1,
                Constants::InvalidEpoch.DataLossNumber,
                Constants::InvalidEpoch.ConfigurationNumber);

            return;
        }

        // TODO: This check is safe since Failover gurantee's that a primary will never cancel the build and restart it without either the idle restarting or primary itself restarting
        // TODO: If idle restarts, we are fine. If primary restarts, its address changes so we are fine.
        // The primary address shouldn't change while copy is in progress
        if (toAddress != primaryAddress_ || !(toActor == primaryDemuxerActor_))
        {
            ReplicatorEventSource::Events->SecondaryDropMsgPrimaryAddress(
                partitionId_,
                endpointUniqueId_,
                Constants::CopyContextAckOperationTrace,
                toAddress,
                toActor,
                primaryAddress_,
                primaryDemuxerActor_);

            return;
        }

        cachedCopySender = copySender_;
    }

    // The following operations do not call any user code,
    // so there's no need to execute them on a different thread
    // than the dispatch thread
    if (cachedCopySender)
    {
        FABRIC_SEQUENCE_NUMBER lsn;
        int errorCodeValue;
        ReplicationTransport::GetCopyContextAckFromMessage(message, lsn, errorCodeValue);

        ReplicatorEventSource::Events->SecondaryCCAckReceived(
            partitionId_,
            endpointUniqueId_,
            lsn,
            errorCodeValue);

        if (errorCodeValue != 0)
        {
            // The primary encountered an error and cancelled copy,
            // so the copy context can be stopped with the error
            cachedCopySender->FinishOperation(
                ErrorCode(static_cast<Common::ErrorCodeValue::Enum>(errorCodeValue)));
        }
        else
        {
            cachedCopySender->ProcessCopyAck(lsn, lsn);
        }
    }
    else
    {
        ReplicatorEventSource::Events->SecondaryCCAckMissingCopySender(
            partitionId_,
            endpointUniqueId_);
    }
}

// ******* Process Request Ack Message *******
void SecondaryReplicator::RequestAckMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader)
{
    UNREFERENCED_PARAMETER(message);

    {
        AcquireReadLock queuesLock(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_, 
                endpointUniqueId_,
                Constants::RequestAckTrace);
            return;
        }

        if (!this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToNoStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                L"RequestAckMessage",
                -1,
                Constants::InvalidEpoch.DataLossNumber,
                Constants::InvalidEpoch.ConfigurationNumber);

            return;
        }

        // The primary address should have been set in processing Replication or StartCopy.
        // We dont update primary address and demuxer id to prevent a stale message updating them.
        if (fromHeader.Address != primaryAddress_ || !(fromHeader.DemuxerActor == primaryDemuxerActor_))
        {
            ReplicatorEventSource::Events->SecondaryDropMsgPrimaryAddress(
                partitionId_,
                endpointUniqueId_,
                Constants::RequestAckTrace,
                fromHeader.Address,
                fromHeader.DemuxerActor,
                primaryAddress_,
                primaryDemuxerActor_);

            return;
        }
    }

    ackSender_.ScheduleOrSendAck(false);
}

// ******* Process Replication Operation *******
void SecondaryReplicator::ReplicationOperationMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader)
{
    std::vector<ComOperationCPtr> batchOperation;
    FABRIC_EPOCH epoch;
    FABRIC_SEQUENCE_NUMBER completedSequenceNumber;

    if (ReplicationTransport::GetReplicationBatchOperationFromMessage(
        message, replicationAckCallback_, batchOperation, epoch, completedSequenceNumber))
    {
        ReplicatorEventSource::Events->SecondaryReceiveBatch(
            partitionId_,
            endpointUniqueId_,
            Constants::ReplOperationTrace,
            batchOperation.front()->SequenceNumber,
            batchOperation.back()->SequenceNumber,
            batchOperation.back()->LastOperationInBatch,
            fromHeader.Address,
            fromHeader.DemuxerActor);

        if (ProcessReplicationOperation(
                fromHeader,
                move(batchOperation),
                epoch,
                completedSequenceNumber))
        {
            ScheduleDispatchReplication();
        }
    }
    else
    {
        ReplicatorEventSource::Events->ReplicatorInvalidMessage(
            partitionId_,
            endpointUniqueId_,
            Constants::ReplOperationTrace,
            fromHeader.Address,
            fromHeader.DemuxerActor);
    }
}

bool SecondaryReplicator::ProcessReplicationOperation(
    ReplicationFromHeader const & fromHeader,
    std::vector<ComOperationCPtr> && batchOperation,
    FABRIC_EPOCH const & primaryEpoch,
    FABRIC_SEQUENCE_NUMBER completedSequenceNumber)
{
    bool forceSendAck = false;

    {
        AcquireWriteLock lock(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_, 
                endpointUniqueId_,
                Constants::ReplOperationTrace);

            return false;
        }

        if (!this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToNoStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::ReplOperationTrace,
                batchOperation.front()->SequenceNumber,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber);

            return false;
        }

        if (primaryEpoch < minAllowedEpoch_)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::ReplOperationTrace,
                batchOperation.front()->SequenceNumber,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber,
                minAllowedEpoch_.DataLossNumber,
                minAllowedEpoch_.ConfigurationNumber);

            return false;
        }
 
        replicationTraceHandler_.AddBatchOperation(
            batchOperation.front()->SequenceNumber,
            batchOperation.back()->SequenceNumber);

        // If we got an operation from a new primary, update the minAllowedEpoch_
        if (minAllowedEpoch_ < primaryEpoch)
        {
            ReplicatorEventSource::Events->SecondaryUpdateMinEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::ReplOperationTrace,
                minAllowedEpoch_.DataLossNumber,
                minAllowedEpoch_.ConfigurationNumber,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber);

            minAllowedEpoch_ = primaryEpoch;
        }

        lastReplicationOperationReceivedAt_ = DateTime::Now();
        bool checkForceSendAck;

        if (!replicationReceiver_.ProcessReplicationOperation(
            move(batchOperation),
            completedSequenceNumber,
            checkForceSendAck))
        {
            return false;
        }
        
        if (checkForceSendAck)
        {
            forceSendAck = ShouldForceSendAck();
        }

        // We need to send ACK, so update the primary target and demuxer id
        SetPrimaryInfoCallerHoldsLock(fromHeader.DemuxerActor, fromHeader.Address);
    }

    ackSender_.ScheduleOrSendAck(forceSendAck);

    return true;
}

// ******* Process Copy Operation *******
void SecondaryReplicator::CopyOperationMessageHandler(
    __in Message & message, 
    ReplicationFromHeader const & fromHeader)
{
    ComOperationCPtr operation;
    FABRIC_REPLICA_ID replicaId;
    FABRIC_EPOCH epoch;
    bool isLast;
    if (ReplicationTransport::GetCopyOperationFromMessage(
        message, copyAckCallback_, operation, replicaId, epoch, isLast))
    {
        ReplicatorEventSource::Events->SecondaryReceive(
            partitionId_,
            endpointUniqueId_,
            Constants::CopyOperationTrace,
            operation->SequenceNumber,
            fromHeader.Address,
            fromHeader.DemuxerActor);

        bool shouldDispatch;
        if (ProcessCopyOperation(
                move(operation),
                replicaId,
                epoch,
                isLast,
                shouldDispatch))
        {
            if (shouldDispatch)
            {
                ScheduleDispatchCopy();
            }
        }
    }
    else
    {
        ReplicatorEventSource::Events->ReplicatorInvalidMessage(
            partitionId_,
            endpointUniqueId_,
            Constants::CopyOperationTrace,
            fromHeader.Address,
            fromHeader.DemuxerActor);
    }
}

bool SecondaryReplicator::ProcessCopyOperation(
    ComOperationCPtr && operation,
    FABRIC_REPLICA_ID replicaId, 
    FABRIC_EPOCH const & primaryEpoch,
    bool isLast,
    __out bool & progressDone)
{
    progressDone = false;
    if (replicaId != replicaId_)
    {
        ReplicatorEventSource::Events->SecondaryDropMsgDueToReplicaId(
            partitionId_,
            endpointUniqueId_,
            Constants::CopyOperationTrace,
            operation->SequenceNumber,
            replicaId,
            replicaId_);

        return false;
    }

    CopySenderSPtr cachedCopySender;
    bool forceSendAck = false;
    bool shouldCloseDispatchQueue = false;

    {
        AcquireWriteLock lock(queuesLock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->SecondaryNotActive(
                partitionId_, 
                endpointUniqueId_,
                Constants::CopyOperationTrace);
            return false;
        }

        if (!this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToNoStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::CopyOperationTrace,
                operation->SequenceNumber,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber);

            return false;
        }

        if (primaryEpoch != startCopyEpoch_)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToEpoch(
                partitionId_,
                endpointUniqueId_,
                Constants::CopyOperationTrace,
                operation->SequenceNumber,
                primaryEpoch.DataLossNumber,
                primaryEpoch.ConfigurationNumber,
                startCopyEpoch_.DataLossNumber,
                startCopyEpoch_.ConfigurationNumber);

            if (primaryEpoch < startCopyEpoch_)
            {
                // drop the stale COPY message
                return false;
            }
            
            // This is not a stale COPY message, but rather a newer message from a new primary which is not expected
            SetReportedFault(
                Common::ErrorCodeValue::InvalidState,
                L"Secondary received COPY operation from different primary epoch",
                FABRIC_FAULT_TYPE_TRANSIENT);

            return false;
        }

        lastCopyOperationReceivedAt_ = DateTime::Now();

        copyTraceHandler_.AddOperation(operation->SequenceNumber);

        bool checkForceSendAck;
        bool checkCopySender;
        if (!copyReceiver_.ProcessCopyOperation(
            move(operation),
            isLast, 
            progressDone,
            shouldCloseDispatchQueue,
            checkForceSendAck,
            checkCopySender))
        {
            return false;
        }

        if (checkCopySender)
        {
            cachedCopySender = copySender_;
        }

        if (checkForceSendAck)
        {
            forceSendAck = ShouldForceSendAck();
        }
    }

    if (shouldCloseDispatchQueue)
    {
        copyReceiver_.DispatchQueue->Close();        
    }

    ackSender_.ScheduleOrSendAck(forceSendAck);

    if (cachedCopySender)
    {
        // We complete here with an error code instead of success to indicate that the copy sender is being closed
        // even though it may not have given the last operation
        // This is because we already received all the required ops from the primary
        cachedCopySender->FinishOperation(ErrorCode(Common::ErrorCodeValue::EnumerationCompleted));
    }

    return true;
}

// Helper methods
bool SecondaryReplicator::HasPrimaryInfoChangesCallerHoldsLock(
    ReplicationEndpointId const & primaryDemuxer,
    std::wstring const & primaryAddress)
{
    // Check whether the target is already resolved and the primary changed
    if (!primaryTarget_)
    {
        return false;
    }
    
    if (Common::StringUtility::AreEqualCaseInsensitive(primaryAddress, primaryAddress_) && 
        primaryDemuxer == primaryDemuxerActor_)
    {
        return false;
    }

    return true;
}

void SecondaryReplicator::SetPrimaryInfoCallerHoldsLock(
    ReplicationEndpointId const & primaryDemuxer,
    std::wstring const & primaryAddress)
{
    // If the target is already resolved and the 
    // primary hasn't changed, use the old address
    if (primaryTarget_ == nullptr ||
        !Common::StringUtility::AreEqualCaseInsensitive(primaryAddress, primaryAddress_))
    {
        ReplicatorEventSource::Events->SecondaryUpdatePrimaryAddress(
            partitionId_,
            endpointUniqueId_,
            primaryAddress_,
            primaryDemuxerActor_,
            primaryAddress,
            primaryDemuxer);

        // Update primary address and resolve new address
        primaryAddress_ = primaryAddress;
        //Here we don't set the Id for the primary because passing this value to the secondary is a bigger change and so
        //we have decided to skip it for now. It will be done if ever the need to drop message from S->P (Acks) arises
        primaryTarget_ = transport_->ResolveTarget(primaryAddress);
    }

    if (primaryDemuxerActor_ != primaryDemuxer)
    {
        primaryDemuxerActor_ = primaryDemuxer;

        replicationAckHeadersSPtr_ = transport_->CreateSharedHeaders(endpointUniqueId_, primaryDemuxerActor_, ReplicationTransport::ReplicationAckAction);
        copyContextHeadersSPtr_ = transport_->CreateSharedHeaders(endpointUniqueId_, primaryDemuxerActor_, ReplicationTransport::CopyContextOperationAction);
    }
}

void SecondaryReplicator::Test_ForceDispatch()
{
    replicationReceiver_.DispatchQueue->Dispatch();
    copyReceiver_.DispatchQueue->Dispatch();
}

void SecondaryReplicator::OnAckReplicationOperation(ComOperation & operation)
{
    if (!requireServiceAck_)
    {
        return;
    }

    // The operations can be acked out of order.
    // Complete will inspect if operations with lower LSN
    // are acked before completing, and it will stop at the first non-acked one
    // (if any).
    ProgressVectorEntryUPtr pendingUpdateEpoch = nullptr;
    AsyncOperationSPtr cachedDrainReplOperationAsyncOp;
    OperationQueueEventSource::Events->OperationAckCallback(
        partitionId_,
        endpointUniqueId_.ToString(),
        operation.SequenceNumber);

    bool forceSendAck;
    Common::ComponentRootSPtr replicatorSPtr = nullptr;

    {
        AcquireWriteLock lock(queuesLock_);

        operation.OnAckCallbackStartedRunning();

        // Check if Close is in progress
        if (!isActive_)
        {
            if (!closeAsyncOperation_)
            {
                // The service was already closed, so the ACK can be ignored
                ReplicatorEventSource::Events->SecondaryNotActive(
                    partitionId_,
                    endpointUniqueId_,
                    L"OnAckReplicationOperation");
                return;
            }
        }
        cachedDrainReplOperationAsyncOp = drainReplOperationAsyncOperation_;

        if (!replicationReceiver_.OnAckReplicationOperation(operation, pendingUpdateEpoch))
        {
            // No new progress, nothing to do
            return;
        }

        if (pendingUpdateEpoch)
        {
            // If we are scheduling an update epoch, reset the wait event so that close/abort waits
            finishedStateProviderUpdateEpoch_->Reset();
        }

        forceSendAck = ShouldForceSendAck();

        // After releasing the queuesLock_, the replicator object can get destructed (RDBUG 4780473). This is because all operations could have been ack'd and there is nothing
        // that is holding on to the root
        // Since we need to access the ackSender_, lets capture the root
        replicatorSPtr = this->CreateComponentRoot();
    }

    ackSender_.ScheduleOrSendAck(forceSendAck);
    
    if (pendingUpdateEpoch)
    {
        std::shared_ptr<ProgressVectorEntry> pendingUpdateEpochSPtr(std::move(pendingUpdateEpoch));
        Threadpool::Post([this, replicatorSPtr, pendingUpdateEpochSPtr]()
            {
                this->ScheduleUpdateStateProviderEpochOnReplicationAck(pendingUpdateEpochSPtr);
            });
    }

    if (cachedDrainReplOperationAsyncOp)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(cachedDrainReplOperationAsyncOp);
        op->CheckIfAllReplicationOperationsAcked(cachedDrainReplOperationAsyncOp);
    }

    // NOTE - Do NOT perform any additional work here since the secondary replicator object might have been free'd by now
}

void SecondaryReplicator::OnAckEndOfStreamReplicationOperation(ComOperation & operation)
{
    if (!requireServiceAck_)
    {
        return;
    }

    OperationQueueEventSource::Events->OperationAckCallback(
        partitionId_,
        endpointUniqueId_.ToString(),
        operation.SequenceNumber);

    ErrorCode faultErrorCode;
    {
        AcquireReadLock grab(faultErrorLock_);
        faultErrorCode = faultErrorCode_;
    }

    AsyncOperationSPtr cachedDrainReplOperationAsyncOp;
    {
        AcquireWriteLock lock(queuesLock_);
        operation.OnAckCallbackStartedRunning();
        ASSERT_IF(isActive_ && faultErrorCode.IsSuccess(), "{0}: Secondary Received end of stream Operation ACK when active and the replicator is not faulted", endpointUniqueId_);

        cachedDrainReplOperationAsyncOp = drainReplOperationAsyncOperation_;

        replicationReceiver_.OnAckEndOfStreamOperation(operation);
    }
    
    if (cachedDrainReplOperationAsyncOp)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(cachedDrainReplOperationAsyncOp);
        op->CheckIfAllReplicationOperationsAcked(cachedDrainReplOperationAsyncOp);
    }
}

class SecondaryReplicator::StateProviderUpdateEpochAsyncOperation 
    : public AsyncOperation
{
public:
    StateProviderUpdateEpochAsyncOperation(
        ErrorCode const & ec,
        bool updateQueueState,
        SecondaryReplicator & parent,
        FABRIC_EPOCH const & epoch,
        FABRIC_SEQUENCE_NUMBER lsn,
        AsyncCallback const & callback,
        AsyncOperationSPtr const & state)
        : AsyncOperation(callback, state, true)
        , ec_(ec)
        , epoch_(epoch)
        , lsn_(lsn)
        , updateQueueState_(updateQueueState)
        , parent_(parent)
    {
    }

    void FinishUpdateSecondaryEpoch(
        AsyncOperationSPtr const & asyncOperation,
        bool completedSynchronously,
        Common::ApiMonitoring::ApiCallDescriptionSPtr callDesc)
    {
        if (asyncOperation->CompletedSynchronously != completedSynchronously)
        {
            return;
        }

        ErrorCode error = parent_.stateProvider_.EndUpdateEpoch(asyncOperation);

        parent_.apiMonitor_->StopMonitoring(callDesc, error);

        if (!updateQueueState_)
        {
            if (!error.IsSuccess())
            { 
                parent_.SetReportedFault(error, L"Secondary StateProvider EndUpdateEpoch", ::FABRIC_FAULT_TYPE_TRANSIENT);
            }

            parent_.finishedStateProviderUpdateEpoch_->Set();
            TryComplete(asyncOperation->Parent);

            return;
        }

        ASSERT_IFNOT(updateQueueState_, "{0}:{1} Queue state must be updated only if needed", parent_.partitionId_, parent_.endpointUniqueId_);

        if (!error.IsSuccess())
        {
            // Cleans the operation queues here. So, when close being called, drainQueueAsync would not get stucked.
            {
                AsyncOperationSPtr drainCopyAsyncOperationCopy;
                AsyncOperationSPtr drainReplAsyncOperationCopy;
                {
                    AcquireWriteLock queuesLock(parent_.queuesLock_);
                    drainCopyAsyncOperationCopy = parent_.drainCopyOperationAsyncOperation_;
                    drainReplAsyncOperationCopy = parent_.drainReplOperationAsyncOperation_;

                    // 1. Enqueue the end of stream operation
                    // 2. Clean queue on fault will mark the end of stream operation as acked and ignore acks from other operations
                    parent_.EnqueueEndOfStreamOperations(); // This will only enqueue the operation if config is enabled
                    parent_.replicationReceiver_.CleanQueueOnUpdateEpochFailure(parent_);
                    parent_.copyReceiver_.CleanQueueOnUpdateEpochFailure(parent_);
                }
                TESTASSERT_IF(drainCopyAsyncOperationCopy, "Drain Copy operation found during completion of update epoch on state provider");
                if (drainReplAsyncOperationCopy)
                {
                    drainReplAsyncOperationCopy->TryComplete(drainReplAsyncOperationCopy, error);
                }
            }

            parent_.SetReportedFault(error, L"Secondary StateProvider EndUpdateEpoch", ::FABRIC_FAULT_TYPE_TRANSIENT);
        }
        else
        {
            ReplicatorEventSource::Events->StateProviderUpdateEpochCompleted(parent_.partitionId_, parent_.endpointUniqueId_);

            bool closeInProgress;
            AsyncOperationSPtr cachedDrainReplOperationAsyncOp;
            AsyncOperationSPtr cachedDrainCopyOperationAsyncOp;
            {
                AcquireWriteLock lock(parent_.queuesLock_);
                closeInProgress = (parent_.closeAsyncOperation_ != nullptr);
                cachedDrainReplOperationAsyncOp = parent_.drainReplOperationAsyncOperation_;
                cachedDrainCopyOperationAsyncOp = parent_.drainCopyOperationAsyncOperation_;
                parent_.replicationReceiver_.UpdateStateProviderEpochDone(closeInProgress);
            }

            if (!closeInProgress)
            {
                // Something may have been enqueued in the dispatch queue, so dispatch
                parent_.replicationReceiver_.DispatchQueue->Dispatch();
            }
            // If close is pending, check if there is a pending drain replication operation to be completed
            // We dont need to check for pending drain copy operation as copy queue is guaranteed to be drained during an update epoch call on state provider
            else
            {
                TESTASSERT_IF(cachedDrainCopyOperationAsyncOp, "Drain Copy operation found during completion of update epoch on state provider");
                if (cachedDrainReplOperationAsyncOp)
                {
                    auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(cachedDrainReplOperationAsyncOp);
                    op->CheckIfAllReplicationOperationsAcked(cachedDrainReplOperationAsyncOp);
                }
            }
        }

        parent_.finishedStateProviderUpdateEpoch_->Set();
        TryComplete(asyncOperation->Parent);
    }

    void OnStart(AsyncOperationSPtr const & thisSPtr)
    {
        if (!ec_.IsSuccess())
        {
            TryComplete(thisSPtr, ec_);
            return;
        }

        auto apiNameDesc = Common::ApiMonitoring::ApiNameDescription(Common::ApiMonitoring::InterfaceName::IStateProvider, Common::ApiMonitoring::ApiName::UpdateEpoch, L"");
        auto callDesc = ApiMonitoringWrapper::GetApiCallDescriptionFromName(parent_.endpointUniqueId_, apiNameDesc, parent_.config_->SlowApiMonitoringInterval);

        parent_.apiMonitor_->StartMonitoring(callDesc);

        ReplicatorEventSource::Events->StateProviderUpdateEpoch(
            parent_.partitionId_,
            parent_.endpointUniqueId_,
            epoch_.DataLossNumber,
            epoch_.ConfigurationNumber,
            lsn_);

        AsyncOperationSPtr inner = parent_.stateProvider_.BeginUpdateEpoch(
            epoch_,
            lsn_,
            [this, callDesc](AsyncOperationSPtr const & asyncOperation)
            {
                FinishUpdateSecondaryEpoch(asyncOperation, false, callDesc);
            },
            thisSPtr);

        FinishUpdateSecondaryEpoch(inner, true, callDesc);
    }

    static ErrorCode End(AsyncOperationSPtr const & asyncOperation)
    {
        auto casted = AsyncOperation::End<StateProviderUpdateEpochAsyncOperation>(asyncOperation);
        return casted->Error;
    }

private:
    ErrorCode const ec_;
    FABRIC_EPOCH const epoch_;
    FABRIC_SEQUENCE_NUMBER const lsn_;
    bool updateQueueState_;
    SecondaryReplicator & parent_;
};

AsyncOperationSPtr SecondaryReplicator::BeginUpdateStateProviderEpochFromOperationStream(
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER lsn,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    return BeginUpdateStateProviderEpochPrivate(
        false, // do not update queue states when operation stream invokes update epoch
        epoch, lsn, callback, state);
}

AsyncOperationSPtr SecondaryReplicator::BeginUpdateStateProviderEpochPrivate(
    bool updateQueueStates,
    FABRIC_EPOCH const & epoch,
    FABRIC_SEQUENCE_NUMBER lsn,
    AsyncCallback const & callback,
    AsyncOperationSPtr const & state)
{
    ErrorCode ec = ErrorCodeValue::Success;

    {
        AcquireExclusiveLock grab(queuesLock_);

        // If close has already completed, fail this call with an error code that is identified by the operation stream
        if (doNotInvokeUEOnStateProvider_)
        {
            ec = ErrorCodeValue::ReplicatorInternalError;
        }

        finishedStateProviderUpdateEpoch_->Reset();
    }

    return AsyncOperation::CreateAndStart<StateProviderUpdateEpochAsyncOperation>(
        ec, updateQueueStates, *this, epoch, lsn, callback, state);
}

ErrorCode SecondaryReplicator::EndUpdateStateProviderEpochFromOperationStream(
    AsyncOperationSPtr const & asyncOperation)
{
    return StateProviderUpdateEpochAsyncOperation::End(asyncOperation);
}

void SecondaryReplicator::ScheduleUpdateStateProviderEpochOnReplicationAck(std::shared_ptr<ProgressVectorEntry> const & pendingUpdateEpochSPtr)
{
    BeginUpdateStateProviderEpochPrivate(
        true, // Update Queue States
        pendingUpdateEpochSPtr->first,
        pendingUpdateEpochSPtr->second,
        [this](AsyncOperationSPtr const & asyncOperation)
        {
            FinishUpdateStateProviderEpochOnReplicationAck(asyncOperation, false);
        },
        CreateAsyncOperationRoot());
}

void SecondaryReplicator::FinishUpdateStateProviderEpochOnReplicationAck(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSync)
{
    if (asyncOperation->CompletedSynchronously != completedSync)
    {
        return;
    }

    EndUpdateStateProviderEpochFromOperationStream(asyncOperation);
}

void SecondaryReplicator::OnAckCopyOperation(ComOperation & operation)
{
    if (!requireServiceAck_)
    {
        return;
    }

    CopySenderSPtr cachedCopySender;
    AsyncOperationSPtr cachedDrainCopyOperationAsyncOp;

    if (operation.IsEndOfStreamOperation)
    {
        OnAckEndOfStreamCopyOperation(operation);
        // Continue processing to remove this from the operation queue if it exists
    }
    else
    {
        OperationQueueEventSource::Events->OperationAckCallback(
            partitionId_,
            endpointUniqueId_.ToString(),
            operation.SequenceNumber);
    }

    Common::ComponentRootSPtr replicatorSPtr = nullptr;
    bool forceSendAck;

    {
        AcquireWriteLock lock(queuesLock_);

        if (!operation.IsEndOfStreamOperation)
        {
            operation.OnAckCallbackStartedRunning();
        }

        // Check if Close is in progress
        if (!isActive_)
        {
            if (!closeAsyncOperation_)
            {
                // The service was already closed, so the ACK can be ignored
                ReplicatorEventSource::Events->SecondaryNotActive(
                    partitionId_,
                    endpointUniqueId_,
                    L"OnAckCopyOperation");
                return;
            }
        }
        cachedDrainCopyOperationAsyncOp = drainCopyOperationAsyncOperation_;

        bool checkCopySender;
        if (!copyReceiver_.OnAckCopyOperation(operation, checkCopySender))
        {
            // No new progress, nothing to do
            return;
        }
        
        if (checkCopySender)
        {
            cachedCopySender = copySender_;
        }

        forceSendAck = ShouldForceSendAck();
        // After releasing the queuesLock_, the replicator object can get destructed (RDBUG 4780473). This is because all operations could have been ack'd and there is nothing
        // that is holding on to the root
        // Since we need to access the ackSender_, lets capture the root
        replicatorSPtr = this->CreateComponentRoot();
    }

    if (cachedCopySender)
    {
        // We complete here with an error code instead of success to indicate that the copy sender is being closed
        // even though it may not have given the last operation
        // This is because we already received all the required ops from the primary
        cachedCopySender->FinishOperation(ErrorCode(Common::ErrorCodeValue::EnumerationCompleted));
    }
    
    // Try to schedule send ACK before checking if all copy operations are ACK'd since the latter might result in releasing the secondaryreplicator object
    // as it posts the completion of the async operation in a thread pool
    ackSender_.ScheduleOrSendAck(forceSendAck);

    if (cachedDrainCopyOperationAsyncOp)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(cachedDrainCopyOperationAsyncOp);
        op->CheckIfAllCopyOperationsAcked(cachedDrainCopyOperationAsyncOp);
    }

    // NOTE - Do NOT perform any additional work here since the secondary replicator object might have been free'd by now
}

void SecondaryReplicator::OnAckEndOfStreamCopyOperation(ComOperation & operation)
{
    if (!requireServiceAck_)
    {
        return;
    }

    OperationQueueEventSource::Events->OperationAckCallback(
        partitionId_,
        endpointUniqueId_.ToString(),
        operation.SequenceNumber);

    AsyncOperationSPtr cachedDrainCopyOperationAsyncOp;
    {
        AcquireWriteLock lock(queuesLock_);
        operation.OnAckCallbackStartedRunning();
        // When primary changes role to secondary, we enqueue a end of stream operation in the copy queue for the 
        // service to pick up the last operation. The secondary is active at this point.
        // Hence there is no assert that allows this ack only when the secondary is not active (like it is there in the end of stream replication ack path)
        cachedDrainCopyOperationAsyncOp = drainCopyOperationAsyncOperation_;

        copyReceiver_.OnAckEndOfStreamOperation(operation);
    }
    
    if (cachedDrainCopyOperationAsyncOp)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(cachedDrainCopyOperationAsyncOp);
        op->CheckIfAllCopyOperationsAcked(cachedDrainCopyOperationAsyncOp);
    }
}

void SecondaryReplicator::ScheduleDispatchReplication()
{
    DispatchQueueSPtr replQueue = this->replicationReceiver_.DispatchQueue;
    dispatchJobQueue_->Enqueue(OperationDispatchJobPtr(move(replQueue)));
}

void SecondaryReplicator::ScheduleDispatchCopy()
{
    DispatchQueueSPtr copyQueue = this->copyReceiver_.DispatchQueue;
    dispatchJobQueue_->Enqueue(OperationDispatchJobPtr(move(copyQueue)));
}

bool SecondaryReplicator::ShouldForceSendAck()
{
    if (!ackSender_.ForceSendAckOnMaxPendingItems)
    {
        return false;
    }

    FABRIC_SEQUENCE_NUMBER progress = copyReceiver_.GetAckProgress() + replicationReceiver_.GetAckProgress();
    return progress >= config_->MaxPendingAcknowledgements;
}

void SecondaryReplicator::PromoteToActiveSecondary(
    AsyncOperationSPtr const & promoteIdleAsyncOperation)
{
    if (!requireServiceAck_)
    {
        ReplicatorEventSource::Events->SecondaryDrainDispatchQueue(
            partitionId_,
            endpointUniqueId_,
            Constants::CopyOperationTrace,
            Constants::PromoteIdleDrainQueue,
            copyReceiver_.DispatchQueue->ItemCount);

        auto inner = copyReceiver_.DispatchQueue->BeginWaitForQueueToDrain(
            [this](AsyncOperationSPtr const & asyncOperation)
            {
                this->FinishWaitForCopyQueueToDrain(asyncOperation, false);
            },
            promoteIdleAsyncOperation);

        FinishWaitForCopyQueueToDrain(inner, true);
    }
    else
    {
        promoteIdleAsyncOperation->TryComplete(promoteIdleAsyncOperation);
    }

    isPromotedToActiveSecondary_.store(true);
}

void SecondaryReplicator::FinishWaitForCopyQueueToDrain(
    AsyncOperationSPtr const & asyncOperation,
    bool completedSynchronously)
{
    if (completedSynchronously == asyncOperation->CompletedSynchronously)
    {
        ErrorCode error = copyReceiver_.DispatchQueue->EndWaitForQueueToDrain(asyncOperation);
        ReplicatorEventSource::Events->SecondaryDrainDispatchQueueDone(
            partitionId_, 
            endpointUniqueId_,
            Constants::CopyOperationTrace,
            static_cast<int>(error.ReadValue()),
            Constants::PromoteIdleDrainQueue);
            
        asyncOperation->Parent->TryComplete(asyncOperation->Parent, error);
    }
}

void SecondaryReplicator::SetReportedFault(
    ErrorCode const & error,
    wstring const & faultDescription,
    FABRIC_FAULT_TYPE faultType)
{
    AcquireWriteLock grab(faultErrorLock_);
    if (faultErrorCode_.IsSuccess())
    {
        faultErrorCode_ = error;
        faultDescription_ = faultDescription;
        faultType_ = faultType;

        Replicator::ReportFault(
            partition_,
            partitionId_,
            endpointUniqueId_,
            faultDescription_,
            faultErrorCode_,
            faultType_);
    }
}

bool SecondaryReplicator::CheckReportedFault(bool callReportFault) const
{
    AcquireReadLock grab(faultErrorLock_);
    if (!faultErrorCode_.IsSuccess())
    {
        if (callReportFault)
        {
            Replicator::ReportFault(
                partition_,
                partitionId_,
                endpointUniqueId_,
                L"Secondary CheckReportedFault",
                faultErrorCode_,
                faultType_);
        }
        return true;
    }
    return false;
}

bool SecondaryReplicator::SendTransportMessage(
    KBuffer::SPtr && sharedHeaders,
    ISendTarget::SPtr const & receiverTarget, 
    MessageUPtr && message,
    bool setExpiration)
{
    auto expiration = setExpiration ?
        config_->RetryInterval :
        TimeSpan::MaxValue;

    auto errorCode = transport_->SendMessage(
        *sharedHeaders, endpointUniqueId_.PartitionId, receiverTarget, move(message), expiration);

    bool ret = true;

    switch(errorCode.ReadValue())
    {
        case Common::ErrorCodeValue::MessageTooLarge:
            SetReportedFault(
                errorCode,
                L"SendTransportMessage failed");
            break;
        case Common::ErrorCodeValue::TransportSendQueueFull:
            // We only return failure if we hit send queue full error so that 
            // the sender stops retrying
            //
            // Other error codes are ignored by upper layer
            ret = false;
            break;
        default:
            break;
    }
    
    return ret;
}

void SecondaryReplicator::EnqueueEndOfStreamOperations()
{
    if (!enableEOSOpAndStreamFaults_)
    {
        return;
    }
    
    // These are idempotent. 
    replicationReceiver_.EnqueueEndOfStreamOperationAndKeepRootAlive(endOfStreamReplicationAckCallback_, *this);
    copyReceiver_.EnqueueEndOfStreamOperationAndKeepRootAlive(endOfStreamCopyAckCallback_, *this);
}

void SecondaryReplicator::OnStreamFault(FABRIC_FAULT_TYPE faultType, std::wstring const & errorMessage)
{
    AsyncOperationSPtr drainCopyAsyncOperationCopy;
    AsyncOperationSPtr drainReplAsyncOperationCopy;

    this->SetReportedFault(
        Common::ErrorCodeValue::OperationStreamFaulted,
        errorMessage,
        faultType);

    // Complete drain async operations if necessary
    {
        AcquireWriteLock queuesLock(queuesLock_);
        drainReplAsyncOperationCopy = drainReplOperationAsyncOperation_;
        drainCopyAsyncOperationCopy = drainCopyOperationAsyncOperation_;

        replicationReceiver_.OnStreamReportFault(*this);
        copyReceiver_.OnStreamReportFault(*this);
    }

    if (drainReplAsyncOperationCopy)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(drainReplAsyncOperationCopy);
        op->OnDrainStreamFaulted();
        op->CheckIfAllReplicationOperationsAcked(drainReplAsyncOperationCopy);
    }

    if (drainCopyAsyncOperationCopy)
    {
        auto op = AsyncOperation::Get<DrainQueueAsyncOperation>(drainCopyAsyncOperationCopy);
        op->OnDrainStreamFaulted();
        op->CheckIfAllCopyOperationsAcked(drainCopyAsyncOperationCopy);
    }
}

void SecondaryReplicator::InduceFaultMessageHandler(
    __in Transport::Message & message,
    ReplicationFromHeader const & fromHeader)
{
    UNREFERENCED_PARAMETER(fromHeader);
    FABRIC_REPLICA_ID replicaId;
    Common::Guid incarnationId;
    wstring reason;
    
    ReplicationTransport::GetInduceFaultFromMessage(message, replicaId, incarnationId, reason);
    
    // TODO: Remove these 2 fields before release after running enough coverage tests to assert this 
    // The following 2 fields are not needed in the message. However, it is being passed to assert that we can never get stale data in the message
    // This is because of the incarnation id being passed back to the primary on open of the replicator, which should always have the latest end point DataLossNumber

    if (replicaId != replicaId_)
    {
        ReplicatorEventSource::Events->SecondaryDropMsgDueToReplicaId(
            partitionId_,
            endpointUniqueId_,
            reason,
            Constants::NonInitializedLSN,
            replicaId,
            replicaId_);

        Assert::TestAssert("InduceFaultMessageHandler Replica Id's do not match: {0} and {1}", replicaId, replicaId_);
        return;
    }

    if (incarnationId != endpointUniqueId_.IncarnationId)
    {
        Assert::TestAssert("InduceFaultMessageHandler Incarnation Id's do not match: {0} and {1}", incarnationId, endpointUniqueId_.IncarnationId);
        return;
    }

    {
        AcquireReadLock grab(queuesLock_);

        if (!this->IsStartCopyEpochEstablishedCallerHoldsLock)
        {
            ReplicatorEventSource::Events->SecondaryDropMsgDueToNoStartCopyEpoch(
                partitionId_,
                endpointUniqueId_,
                reason,
                -1,
                Constants::InvalidEpoch.DataLossNumber,
                Constants::InvalidEpoch.ConfigurationNumber);

            return;
        }
    }
   
    ProcessInduceFault(reason);
}

void SecondaryReplicator::ProcessInduceFault(wstring const & reason)
{
    // Report fault if this replica is not already faulted
    if (!CheckReportedFault(false))
    {
        SetReportedFault(Common::ErrorCodeValue::ReplicatorInternalError, reason);
    }
}


} // end namespace ReplicationComponent
} // end namespace Reliability
