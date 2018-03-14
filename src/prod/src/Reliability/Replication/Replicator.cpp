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
using Common::MoveUPtr;

using Common::AcquireReadLock;
using Common::AcquireWriteLock;
using Common::FormatOptions;
using Common::make_unique;
using Common::StringWriter;
using Common::TextWriter;
using Common::JobQueue;

using Transport::Message;
using Transport::MessageUPtr;
using Transport::ReceiverContextUPtr;
using Transport::ISendTarget;
using Transport::SecuritySettings;

using std::move;
using std::shared_ptr;
using std::unique_ptr;
using std::vector;
using std::wstring;

Replicator::MessageJobItem::MessageJobItem()
    : message_(nullptr)
{
}

Replicator::MessageJobItem::MessageJobItem(MessageJobItem && other)
    : message_(move(other.message_))
{
}

Replicator::MessageJobItem::MessageJobItem(MessageUPtr && message)
    : message_(move(message))
{
}

bool Replicator::MessageJobItem::ProcessJob(Replicator & replicator)
{
    replicator.ProcessMessageInternal(move(message_));
    return true;
}

Replicator::MessageJobItem& Replicator::MessageJobItem::operator=(MessageJobItem && other)
{
    if (this != &other)
    {
        message_ = move(other.message_);
    }

    return *this;
}

Replicator::Replicator(
    REInternalSettingsSPtr && config, 
    FABRIC_REPLICA_ID replicaId,
    Common::Guid const & partitionId,
    bool hasPersistedState,
    ComProxyStatefulServicePartition && partition,
    ComProxyStateProvider && stateProvider,
    ReplicatorInternalVersion version,
    ReplicationTransportSPtr && transport,
    IReplicatorHealthClientSPtr && healthClient)
    :   Common::ComponentRoot(),
        config_(move(config)),
        hasPersistedState_(hasPersistedState),
        partition_(move(partition)),
        stateProvider_(move(stateProvider)),
        partitionId_(partitionId),
        version_(version),
        transport_(move(transport)),
        endpoint_(),
        endpointUniqueId_(partitionId, replicaId),
        replicaId_(replicaId), 
        state_(),
        primary_(),
        secondary_(),
        stateProviderInitialProgress_(Constants::NonInitializedLSN),
        lock_(),
        perfCounters_(),
        healthClient_(move(healthClient)),
        apiMonitor_(),
        messageProcessorJobQueueUPtr_(nullptr)
{
    ASSERT_IFNOT(transport_, "{0}: .ctor: The transport should exist", endpointUniqueId_);
    ReplicatorEventSource::Events->ReplicatorCtor(
        partitionId_,
        endpointUniqueId_,
        *config_.get());
}

Replicator::~Replicator()
{
    ReplicatorEventSource::Events->ReplicatorDtor(
        partitionId_,
        endpointUniqueId_);
    
    AcquireReadLock lock(lock_);
    if ((state_.Value & ReplicatorState::DestructEntryMask) == 0)
    {
        Assert::CodingError(
            "{0}: Replicator should be in state Created | Closed | Faulted | Aborted | None when destructing",
            ToString());
    }
}

void Replicator::CreateMessageProcessorCallerHoldsLock()
{
    messageProcessorJobQueueUPtr_ = make_unique<Common::JobQueue<MessageJobItem, Replicator>>(endpointUniqueId_.ToString(), *this);
}

void Replicator::ReportFault(
    ComProxyStatefulServicePartition const & partition,
    Common::Guid const & partitionId,
    ReplicationEndpointId const & description,
    std::wstring const & failureDesc,
    ErrorCode error,
    FABRIC_FAULT_TYPE faultType)
{
    ReplicatorEventSource::Events->ReplicatorReportFault(
        partitionId,
        description, 
        faultType == FABRIC_FAULT_TYPE_TRANSIENT ? L"Transient" : L"Permanent",
        failureDesc,
        error.ReadValue());

    partition.ReportFault(faultType);
}

// *****************************
// IFabricReplicator methods
// *****************************
// ******* Open *******
AsyncOperationSPtr Replicator::BeginOpen(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<OpenAsyncOperation>(
        *this, 
        callback, 
        state); 
}

ErrorCode Replicator::EndOpen(AsyncOperationSPtr const & asyncOperation, __out wstring & endpoint)
{
    return OpenAsyncOperation::End(asyncOperation, endpoint);
}

// ******* Close *******
AsyncOperationSPtr Replicator::BeginClose(
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<CloseAsyncOperation>(*this, callback, state); 
}

ErrorCode Replicator::EndClose(AsyncOperationSPtr const & asyncOperation)
{
    return CloseAsyncOperation::End(asyncOperation);
}

void Replicator::Abort()
{
    PrimaryReplicatorSPtr primaryCopy;
    SecondaryReplicatorSPtr secondaryCopy;

    {
        AcquireWriteLock lock(lock_);
        ErrorCode error = state_.TransitionToAborting();
        if (!error.IsSuccess())
        {
            ReplicatorEventSource::Events->ReplicatorInvalidState(
                partitionId_,
                *this, // need to print the current state
                L"Abort",
                (const int)ReplicatorState::Opened);
        }
        else 
        {
            if (primary_ && secondary_)
            {
                Assert::CodingError(
                    "{0}: Close: Can't have both primary and secondary",
                    ToString());
            }
            else if (primary_)
            {
                primaryCopy = move(primary_);
            }
            else if (secondary_)
            {
                // On the secondary, some operations are allowed when state is Aborting
                // (eg. GetOperation)
                // so make a copy of the secondary shared_ptr 
                secondaryCopy = secondary_;
            }
        }
    }

    if (primaryCopy)
    {
        primaryCopy->Abort();
    }
    else if (secondaryCopy)
    {
        secondaryCopy->Abort();
    }
    
    if (apiMonitor_)
    {
        apiMonitor_->Close();
        apiMonitor_.reset();
    }

    if (messageProcessorJobQueueUPtr_)
    {
        // Do not reset the job queue because there could be pending items being processed and resetting the job queue results in asserting if that is the case
        messageProcessorJobQueueUPtr_->Close();
    }

    perfCounters_.reset();
    transport_->UnregisterMessageProcessor(*this);
    ComProxyStateProvider cleanup(move(stateProvider_));

    {
        AcquireWriteLock lock(lock_);
        ErrorCode error = state_.TransitionToAborted();
        if (!error.IsSuccess())
        {
            ReplicatorEventSource::Events->ReplicatorInvalidState(
                partitionId_,
                *this, // need to print the current state
                L"Abort",
                (const int) ReplicatorState::Opened);
        }
        secondary_.reset();
    }
}

// ******* ChangeRole *******
AsyncOperationSPtr Replicator::BeginChangeRole(
    FABRIC_EPOCH const & epoch,
    FABRIC_REPLICA_ROLE newRole,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<ChangeRoleAsyncOperation>(
        *this, epoch, newRole, callback, state); 
}

ErrorCode Replicator::EndChangeRole(AsyncOperationSPtr const & asyncOperation)
{
    auto error = ChangeRoleAsyncOperation::End(asyncOperation);
    
    if (error.IsSuccess())
    {
        // Update the Role Perf counter only if the change role was successful
        perfCounters_->Role.RawValue = static_cast<int>(state_.Role);
    }

    return error;
}

AsyncOperationSPtr Replicator::BeginUpdateEpoch(
    FABRIC_EPOCH const & epoch,
    Common::AsyncCallback const & callback,
    Common::AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<UpdateEpochAsyncOperation>(
        *this, epoch, callback, state);
}

ErrorCode Replicator::EndUpdateEpoch(
    Common::AsyncOperationSPtr const & asyncOperation)
{
    return UpdateEpochAsyncOperation::End(asyncOperation);
}

// ******* GetStatus *******
ErrorCode Replicator::GetCurrentProgress(
    __out FABRIC_SEQUENCE_NUMBER & lastSequenceNumber)
{
    bool needStateProviderCurrentProgress = false;
        
    {
        AcquireReadLock lock(lock_);
        switch(state_.Value)
        {
        case ReplicatorState::Opened:
            // Ask the service for initial value or return the cached value
            if (stateProviderInitialProgress_ == Constants::NonInitializedLSN)
            {
                needStateProviderCurrentProgress = true;
            }
            else
            {
                lastSequenceNumber = stateProviderInitialProgress_;
                return ErrorCode(Common::ErrorCodeValue::Success);
            }
            break;
        case ReplicatorState::Primary:
            ASSERT_IFNOT(primary_, "{0}: GetCurrentProgress: Primary can't be null", ToString());
            return primary_->GetCurrentProgress(lastSequenceNumber);
            break;
        case ReplicatorState::SecondaryActive:
        case ReplicatorState::SecondaryIdle:
            ASSERT_IFNOT(secondary_, "{0}: GetCurrentProgress: Secondary can't be null", ToString());
            return secondary_->GetCurrentProgress(lastSequenceNumber);
            break;
        default:
            return ErrorCode(Common::ErrorCodeValue::InvalidState);
        }
    }

    if (needStateProviderCurrentProgress)
    {
        // GetCurrentProgress outside the lock
        ErrorCode error = stateProvider_.GetLastCommittedSequenceNumber(lastSequenceNumber);
        if (!error.IsSuccess())
        {
            ReplicatorEventSource::Events->StateProviderError(
                partitionId_,
                ToString(), 
                L"GetLastCommittedSequenceNumber",
                error.ReadValue());

            return error;
        }
       
        ReplicatorEventSource::Events->StateProviderProgress(
            partitionId_,
            endpointUniqueId_, 
            lastSequenceNumber);
            
        {
            // Update the state provider initial progress on the replicator,
            // if not changed
            AcquireWriteLock lock(lock_);
            if (stateProviderInitialProgress_ == Constants::NonInitializedLSN)
            {
                stateProviderInitialProgress_ = lastSequenceNumber;
            }
        }
    }

    return ErrorCode(Common::ErrorCodeValue::Success);
}

ErrorCode Replicator::UpdateSecuritySettings(
    __in SecuritySettings const & securitySettings)
{
    return transport_->SetSecurity(securitySettings);
}

ErrorCode Replicator::GetCatchUpCapability(
    __out FABRIC_SEQUENCE_NUMBER & firstSequenceNumber)
{
    AcquireReadLock lock(lock_);
    switch(state_.Value)
    {
    case ReplicatorState::Primary:
        ASSERT_IFNOT(primary_, "{0}: GetCatchUpCapability: Primary can't be null", ToString());
        return primary_->GetCatchUpCapability(firstSequenceNumber);
    case ReplicatorState::SecondaryActive:
    case ReplicatorState::SecondaryIdle:
        ASSERT_IFNOT(secondary_, "{0}: GetCatchUpCapability: Secondary can't be null", ToString());
        return secondary_->GetCatchUpCapability(firstSequenceNumber);
    case ReplicatorState::Opened:
        // There are no operations initially, so return 0
        firstSequenceNumber = 0;
        return ErrorCode(Common::ErrorCodeValue::Success);
    default:
        return ErrorCode(Common::ErrorCodeValue::InvalidState);
    }
}

// *****************************
// IFabricPrimaryReplicator methods
// *****************************
// ******* UpdateCatchUpConfiguration *******
ErrorCode Replicator::UpdateCatchUpReplicaSetConfiguration(
    ReplicaInformationVector const & previousActiveSecondaryReplicas, 
    ULONG previousWriteQuorum,
    ReplicaInformationVector const & currentActiveSecondaryReplicas, 
    ULONG currentWriteQuorum)
{
    ErrorCode error;
    PrimaryReplicatorSPtr primaryCopy;
    
    {
        AcquireWriteLock lock(lock_);
        if (VerifyIsPrimaryCallerHoldsLock(L"UpdateCatchUpReplicaSetConfiguration", error))
        {
            primaryCopy = primary_;
        }
        else
        {
            return error;
        }
    }

    error = primaryCopy->UpdateCatchUpReplicaSetConfiguration(
        previousActiveSecondaryReplicas,
        previousWriteQuorum,
        currentActiveSecondaryReplicas, 
        currentWriteQuorum);
    return error;
}

// ******* CatchupReplicaSet *******
AsyncOperationSPtr Replicator::BeginWaitForCatchUpQuorum(
    FABRIC_REPLICA_SET_QUORUM_MODE catchUpMode,
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<CatchupReplicaSetAsyncOperation>(
        *this, catchUpMode, callback, state); 
}

ErrorCode Replicator::EndWaitForCatchUpQuorum(AsyncOperationSPtr const & asyncOperation)
{
    return CatchupReplicaSetAsyncOperation::End(asyncOperation);
}

ErrorCode Replicator::UpdateCurrentReplicaSetConfiguration(
    ReplicaInformationVector const & currentActiveSecondaryReplicas, 
    ULONG currentWriteQuorum)
{
    ErrorCode error;
    PrimaryReplicatorSPtr primaryCopy;
    
    {
        AcquireWriteLock lock(lock_);
        if (VerifyIsPrimaryCallerHoldsLock(L"UpdateCurrentReplicaSetConfiguration", error))
        {
            primaryCopy = primary_;
        }
        else
        {
            return error;
        }
    }

    error = primaryCopy->UpdateCurrentReplicaSetConfiguration(
        currentActiveSecondaryReplicas, 
        currentWriteQuorum);
    return error;
}

// ******* BuildIdle *******
AsyncOperationSPtr Replicator::BeginBuildReplica(
    ReplicaInformation const & toReplica, 
    AsyncCallback const & callback, 
    AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<BuildIdleAsyncOperation>(
        *this, toReplica, callback, state); 
}

ErrorCode Replicator::EndBuildReplica(AsyncOperationSPtr const & asyncOperation)
{
    return BuildIdleAsyncOperation::End(asyncOperation);
}

// ******* RemoveReplica *******
Common::ErrorCode Replicator::RemoveReplica(FABRIC_REPLICA_ID replicaId)
{
    ErrorCode error;
    PrimaryReplicatorSPtr primaryCopy;
        
    {
        AcquireReadLock lock(lock_);
        if (VerifyIsPrimaryCallerHoldsLock(L"RemoveReplica", error))
        {
            primaryCopy = primary_;
        }
        else
        {
            return error;
        }
    }
        
    error = primaryCopy->RemoveReplica(replicaId);
    return error;
}

// ******* OnDataLoss *******
AsyncOperationSPtr Replicator::BeginOnDataLoss(
    Common::AsyncCallback const & callback, 
    Common::AsyncOperationSPtr const & state)
{
    return AsyncOperation::CreateAndStart<OnDataLossAsyncOperation>(
        *this, callback, state); 
}

ErrorCode Replicator::EndOnDataLoss(
    Common::AsyncOperationSPtr const & asyncOperation, 
    __out BOOLEAN & isStateChanged)
{
    return OnDataLossAsyncOperation::End(asyncOperation, isStateChanged);
}

// *****************************
// IFabricStateReplicator methods
// *****************************
AsyncOperationSPtr Replicator::BeginReplicate(
     ComPointer<IFabricOperationData> && comOperationPointer,
     __out FABRIC_SEQUENCE_NUMBER & sequenceNumber,
     AsyncCallback const & callback, 
     AsyncOperationSPtr const & state)
{
    auto inner = std::make_shared<ReplicateAsyncOperation>(
        *this, 
        move(comOperationPointer), 
        callback, 
        state); 

    inner->Start(inner);
    sequenceNumber = inner->SequenceNumber;
    return inner;
}

ErrorCode Replicator::EndReplicate(
    AsyncOperationSPtr const & asyncOperation)
{
    return ReplicateAsyncOperation::End(asyncOperation);
}

// ******* Get Copy and Replication Streams *******
ErrorCode Replicator::GetCopyStream(
    __out OperationStreamSPtr & stream)
{
    ErrorCode error;
    AcquireWriteLock lock(lock_);
    if (VerifyIsSecondaryCallerHoldsLock(L"GetCopyStream", error))
    {
        stream = secondary_->GetCopyStream(secondary_);
    }

    return error;
}

ErrorCode Replicator::GetReplicationStream(
    __out OperationStreamSPtr & stream)
{
    ErrorCode error;
    AcquireWriteLock lock(lock_);
    if (VerifyIsSecondaryCallerHoldsLock(L"GetReplicationStream", error))
    {
        stream = secondary_->GetReplicationStream(secondary_);
    }

    return error;
}

ErrorCode Replicator::GetReplicationQueueCounters(
    __out FABRIC_INTERNAL_REPLICATION_QUEUE_COUNTERS & counters)
{
    ErrorCode error;

    AcquireReadLock grab(lock_);

    if (VerifyIsPrimaryAndAccessGrantedCallerHoldsLock(L"GetReplicationQueueCounters", true, error))
    {
        error = primary_->GetReplicationQueueCounters(counters);
    }

    return error;
}

ErrorCode Replicator::GetReplicatorStatus(
    __out ServiceModel::ReplicatorStatusQueryResultSPtr & result)
{
    PrimaryReplicatorSPtr primaryCopy;
    SecondaryReplicatorSPtr secondaryCopy;

    {
        AcquireReadLock lock(lock_);
        primaryCopy = primary_;
        secondaryCopy = secondary_;
    }

    if (primaryCopy)
    {
        return primaryCopy->GetReplicatorStatus(result);
    }
    else if (secondaryCopy)
    {
        return secondaryCopy->GetReplicatorStatus(result);
    }
    else
    {
        return Common::ErrorCodeValue::NotReady;
    }
}

// *****************************
// Message Processor methods, 
// called by the ReplicationTransport
// when messages are received
// *****************************
void Replicator::ProcessMessage(__in Message & message, __out ReceiverContextUPtr &)
{
    ASSERT_IFNOT(messageProcessorJobQueueUPtr_, "{0}:{1} Message processor queue is null", partitionId_, endpointUniqueId_);
    MessageJobItem tmp(move(message.Clone()));
    messageProcessorJobQueueUPtr_->Enqueue(move(tmp));
}

void Replicator::ProcessMessageInternal(__in MessageUPtr && message)
{
    wstring const & action = message->Action;

    ReplicationFromHeader fromHeader;
    if (!message->Headers.TryReadFirst(fromHeader) ||
        fromHeader.DemuxerActor.IsEmpty() || fromHeader.Address.empty())
    {
        ReplicatorEventSource::Events->ReplicatorDropMsgNoFromHeader(
            partitionId_,
            endpointUniqueId_, 
            action);
        return;
    }

    ReplicationActorHeader receiverActor;
    ASSERT_IF(
        !message->Headers.TryReadFirst(receiverActor) || (receiverActor.Actor != endpointUniqueId_), 
        "{0}: {1}: Demuxer gave a message {1} with no or wrong receiverActor",
        partitionId_,
        endpointUniqueId_,
        action);            
    
    PrimaryReplicatorSPtr primary;
    SecondaryReplicatorSPtr secondary;
    
    {
        AcquireReadLock lock(lock_);
        bool actionMatched = false;
        // Only process messages if the replicator is opened 
        // and the action corresponds with the current role
        // OR
        // If the replicator is closing and we are waiting for pending quorum commits
        if (ReplicatorState::Primary == state_.Value ||
            (ReplicatorState::Closing == state_.Value && state_.Role == FABRIC_REPLICA_ROLE_PRIMARY))
        {
            if (action == ReplicationTransport::ReplicationAckAction ||
                action == ReplicationTransport::CopyContextOperationAction)
            {
                primary = primary_;
                actionMatched = true;
            }
        }
        else if (ReplicatorState::SecondaryActive == state_.Value || ReplicatorState::SecondaryIdle == state_.Value)
        {
            if (action == ReplicationTransport::ReplicationOperationAction ||
                action == ReplicationTransport::CopyOperationAction ||
                action == ReplicationTransport::StartCopyAction ||
                action == ReplicationTransport::CopyContextAckAction ||
                action == ReplicationTransport::RequestAckAction ||
                action == ReplicationTransport::InduceFaultAction)
            {
                secondary = secondary_;
                actionMatched = true;
            }
        }

        if (!actionMatched)
        {
            // At this point, the message can't be processed, so drop it
            ReplicatorEventSource::Events->ReplicatorDropMsgInvalid(
                partitionId_,
                *this,  // need to print the state
                action,
                fromHeader.Address,
                fromHeader.DemuxerActor);

            return;
        }
    } // end AquireReadLock

    // Execute the handlers outside the lock
    if (primary)
    {
        if (!primary->CheckReportedFault(false))
        {
            if (action == ReplicationTransport::CopyContextOperationAction)
            {
                primary->CopyContextMessageHandler(*message, fromHeader);
            }
            else if (action == ReplicationTransport::ReplicationAckAction)
            {
                // We are passing the primary as an input parameter weak ptr so that the async ack processing
                // will not hold on to the object during close/abort
                primary->ReplicationAckMessageHandler(*message, fromHeader, primary);
            }
            else
            {
                Assert::CodingError(
                    "{0}: Primary can't process {1}", 
                    ToString(), 
                    action);
            }
        }
    }
    else if (secondary)
    {
        if (!secondary->CheckReportedFault(false))
        {
            if (action == ReplicationTransport::ReplicationOperationAction)
            {
                secondary->ReplicationOperationMessageHandler(*message, fromHeader);
            }
            else if (action == ReplicationTransport::StartCopyAction)
            {
                secondary->StartCopyMessageHandler(*message, fromHeader);
            }
            else if (action == ReplicationTransport::CopyContextAckAction)
            {
                secondary->CopyContextAckMessageHandler(*message, fromHeader);
            }
            else if (action == ReplicationTransport::CopyOperationAction)
            {
                secondary->CopyOperationMessageHandler(*message, fromHeader);                    
            }
            else if (action == ReplicationTransport::RequestAckAction)
            {
                secondary->RequestAckMessageHandler(*message, fromHeader);                    
            }
            else if (action == ReplicationTransport::InduceFaultAction)
            {
                secondary->InduceFaultMessageHandler(*message, fromHeader);
            }
            else
            {
                Assert::CodingError(
                    "{0}: Secondary can't process {1}",
                    ToString(),
                    action);
            }
        }
    }
    else
    {
        Assert::CodingError( 
            "{0}: Can't process {1}: no primary or secondary exist", 
            ToString(), 
            action);
    }
}


// General methods
wstring Replicator::ToString() const
{
    wstring content;
    StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return content;
}

void Replicator::WriteTo(__in TextWriter & writer, FormatOptions const &) const
{
    writer << endpointUniqueId_ << ":" << state_;
}

std::string Replicator::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0}:{1}";
    size_t index = 0;

    traceEvent.AddEventField<ReplicationEndpointId>(format, name + ".endpointUniqueId", index);
    traceEvent.AddEventField<ReplicatorState>(format, name + ".state", index);

    return format;
}

void Replicator::FillEventData(Common::TraceEventContext & context) const
{
    context.Write<ReplicationEndpointId>(endpointUniqueId_);
    context.Write<ReplicatorState>(state_);
}

// Helper methods
bool Replicator::VerifyIsPrimaryCallerHoldsLock(
    std::wstring const & checkReason,
    __out ErrorCode & error)
{
    if (!(ReplicatorState::Primary == state_.Value))
    {
        ReplicatorEventSource::Events->ReplicatorInvalidState(
            partitionId_,
            *this, // need to print the current role
            checkReason,
            (const int)ReplicatorState::Primary);

        switch(state_.Value)
        {
            case ReplicatorState::Closed:
            case ReplicatorState::Closing:
            case ReplicatorState::Aborted:
            case ReplicatorState::Aborting:
            case ReplicatorState::Faulted:
                error = ErrorCode(Common::ErrorCodeValue::ObjectClosed);
                break;
            case ReplicatorState::ChangingRole:
            case ReplicatorState::Opened:
            case ReplicatorState::CheckingDataLoss:
                error = ErrorCode(Common::ErrorCodeValue::ReconfigurationPending);
                break;
            case ReplicatorState::SecondaryActive:
            case ReplicatorState::SecondaryIdle:
            case ReplicatorState::RoleNone:
                error = ErrorCode(Common::ErrorCodeValue::NotPrimary);
                break;
            default:
                error = ErrorCode(Common::ErrorCodeValue::ObjectClosed);
                Assert::TestAssert("Invalid Primary state {0}", state_.ToString());
                break;
        }

        return false;
    }

    ASSERT_IFNOT(primary_, "{0}: {1}: Primary should exist", ToString(), checkReason);
    return true;
}

bool Replicator::VerifyIsSecondaryCallerHoldsLock(
    std::wstring const & checkReason,
    __out ErrorCode & error)
{
    // Accept operations in secondary state, but also in 
    // ChangingState and Closing
    if ((state_.Value & ReplicatorState::SecondaryOperationsMask) == 0 || !secondary_)
    {
        ReplicatorEventSource::Events->ReplicatorInvalidState(
            partitionId_,
            *this, // need to print the current role
            checkReason,
            (const int) ReplicatorState::SecondaryOperationsMask);

        if (!secondary_)
        {
            error = ErrorCode(Common::ErrorCodeValue::ObjectClosed);
            return false;
        }
        else 
        {
            switch(state_.Value)
            {
                case ReplicatorState::Closed:
                case ReplicatorState::Aborted:
                case ReplicatorState::Faulted:
                case ReplicatorState::RoleNone:
                    error = ErrorCode(Common::ErrorCodeValue::ObjectClosed);
                    break;
                case ReplicatorState::Opened:
                    error = ErrorCode(Common::ErrorCodeValue::ReconfigurationPending);
                    break;
                default:
                    error = ErrorCode(Common::ErrorCodeValue::ObjectClosed);
                    Assert::TestAssert("Invalid Secondary state {0}", state_.ToString());
                    break;
            }
        }

        return false;
    }

    ASSERT_IFNOT(secondary_, "{0}: {1}: Secondary should exist", ToString(), checkReason);
    return true;
}

bool Replicator::VerifyAccessGranted(__out ErrorCode & error)
{
    FABRIC_SERVICE_PARTITION_ACCESS_STATUS writeStatus = partition_.GetWriteStatus();
    switch(writeStatus)
    {
    case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING:
        error = ErrorCode(Common::ErrorCodeValue::ReconfigurationPending);
        return false;
    case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NO_WRITE_QUORUM:
        error = ErrorCode(Common::ErrorCodeValue::NoWriteQuorum);
        return false;
    case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_NOT_PRIMARY:
        error = ErrorCode(Common::ErrorCodeValue::NotPrimary);
        return false;
    case FABRIC_SERVICE_PARTITION_ACCESS_STATUS_GRANTED:
        return true;
    default:
        Assert::CodingError("Unknown service partition access status");
    }
}

bool Replicator::VerifyIsPrimaryAndAccessGrantedCallerHoldsLock(
    __in std::wstring const & checkReason, 
    __in bool alwaysGrantAccessWhenPrimary, 
    __out Common::ErrorCode & error)
{
    if (VerifyIsPrimaryCallerHoldsLock(checkReason, error))
    {
        if (alwaysGrantAccessWhenPrimary)
        {
            return true;
        }
        else
        {
            return VerifyAccessGranted(error);
        }
    }
    else
    {
        return false;
    }
}

ServiceModel::ReplicatorQueueStatus Replicator::GetReplicatorQueueStatusForQuery(
    OperationQueue const & queue)
{
    uint queueFullPercent = GetQueueFullPercentage(queue);

    return ServiceModel::ReplicatorQueueStatus(
        queueFullPercent,
        queue.TotalMemorySize,
        queue.FirstAvailableCompletedSequenceNumber,
        queue.LastCompletedSequenceNumber,
        queue.LastCommittedSequenceNumber,
        queue.LastSequenceNumber);
}

uint Replicator::GetQueueFullPercentage(
    OperationQueue const & queue)
{
    double queueFullMemoryPercent = 0.0;
    double queueFullNumberOfOpsPercent = 0.0;

    // Process only if there are operations in the queue
    if (queue.OperationCount > 0)
    {
        // If queue has size limits set, calculate utilization of number of operations in %age
        if (queue.MaxSize != 0)
        {
            // Use the LastCompletedSequenceNumber, instead of the FirstAvailableSequenceNumber since secondaries
            // keep their operations forever (until there is no more room, when they free up operations)
            queueFullNumberOfOpsPercent =
                static_cast<double>(queue.LastSequenceNumber - queue.LastCompletedSequenceNumber) /
                static_cast<double>(queue.MaxSize);
        }

        // If queue has memory limits set, calculate utlization of memory in %age
        if (queue.MaxMemorySize != 0)
        {
            queueFullMemoryPercent =
                static_cast<double>(queue.TotalMemorySize - queue.CompletedMemorySize) /
                static_cast<double>(queue.MaxMemorySize);
        }
    }

    uint queueFullPercent = 0;
    // Use the higher of the 2 percentages calculated above
    queueFullPercent = queueFullMemoryPercent > queueFullNumberOfOpsPercent ?
        static_cast<uint>(queueFullMemoryPercent * 100) :
        static_cast<uint>(queueFullNumberOfOpsPercent * 100);

    TESTASSERT_IF(queueFullPercent > 100, "QueueFull Percentage cannot be greater than 100");

    return queueFullPercent;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
