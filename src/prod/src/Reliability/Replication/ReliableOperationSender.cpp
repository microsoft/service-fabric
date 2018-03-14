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
using Common::DateTime;
using Common::Stopwatch;
using Common::StringWriter;
using Common::Timer;
using Common::TimerSPtr;
using Common::TimeSpan;

using std::list;
using std::move;
using std::wstring;

ULONGLONG const ReliableOperationSender::DEFAULT_MAX_SWS_WHEN_0 = 1024;
int const ReliableOperationSender::DEFAULT_MAX_SWS_FACTOR_WHEN_0 = 4;

ReliableOperationSender::ReliableOperationSender(
    REInternalSettingsSPtr const & config,
    ULONGLONG startSendWindowSize,
    ULONGLONG maxSendWindowSize,
    Common::Guid const & partitionId,
    std::wstring const & purpose,
    ReplicationEndpointId const & endpointUniqueId,
    FABRIC_REPLICA_ID const & replicaId,
    FABRIC_SEQUENCE_NUMBER lastAckedSequenceNumber)
    : ComponentRoot(),
    config_(config),
    maxSendWindowSize_(maxSendWindowSize),
    partitionId_(partitionId),
    purpose_(purpose),
    endpointUniqueId_(endpointUniqueId),
    replicaId_(replicaId),
    pendingOperations_(),
    opCallback_(),
    lastAckedReceivedSequenceNumber_(lastAckedSequenceNumber),
    lastAckedApplySequenceNumber_(lastAckedSequenceNumber),
    highestOperationSequenceNumber_(lastAckedSequenceNumber),
    sendWindowSize_(static_cast<size_t>(startSendWindowSize)),
    noAckSinceLastCallback_(false),
    isActive_(false),
    retrySendTimer_(),
    timerActive_(false),
    lock_(),
    lastAckProcessedTime_(DateTime::Zero),
    averageReceiveAckDuration_(config->SecondaryProgressRateDecayFactor, config->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation),
    averageApplyAckDuration_(config->SecondaryProgressRateDecayFactor, config->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation),
    ackDurationList_(),
    timeSinceLastAverageUpdate_(),
    completedSeqNumber_(Constants::InvalidLSN)
{
    timeSinceLastAverageUpdate_.Start();

    if (maxSendWindowSize_ == 0)
    {
        // this value can be 0 if the user specified no queue limits based on the queue size, but rather based on memory size 
        // It is hard to predict what is a good max sending window size. so use 1024 as the max. If that is lesser than the initial sending size, use 4 times the value
        maxSendWindowSize_ = DEFAULT_MAX_SWS_WHEN_0;
        if (maxSendWindowSize_ < sendWindowSize_)
            maxSendWindowSize_ = sendWindowSize_*DEFAULT_MAX_SWS_FACTOR_WHEN_0;
    }
}

ReliableOperationSender::ReliableOperationSender(ReliableOperationSender && other)
    : ComponentRoot(other),
    config_(other.config_),
    maxSendWindowSize_(other.maxSendWindowSize_),
    partitionId_(move(other.partitionId_)),
    purpose_(move(other.purpose_)),
    endpointUniqueId_(move(other.endpointUniqueId_)),
    replicaId_(other.replicaId_),
    pendingOperations_(move(other.pendingOperations_)),
    opCallback_(move(other.opCallback_)),
    lastAckedReceivedSequenceNumber_(other.lastAckedReceivedSequenceNumber_),
    lastAckedApplySequenceNumber_(other.lastAckedApplySequenceNumber_),
    highestOperationSequenceNumber_(other.highestOperationSequenceNumber_),
    sendWindowSize_(other.sendWindowSize_),
    noAckSinceLastCallback_(other.noAckSinceLastCallback_),
    isActive_(other.isActive_),
    retrySendTimer_(move(other.retrySendTimer_)),
    timerActive_(other.timerActive_),
    lock_(),
    lastAckProcessedTime_(move(other.lastAckProcessedTime_)),
    averageReceiveAckDuration_(move(other.averageReceiveAckDuration_)),
    averageApplyAckDuration_(move(other.averageApplyAckDuration_)),
    ackDurationList_(other.ackDurationList_),
    timeSinceLastAverageUpdate_(other.timeSinceLastAverageUpdate_),
    completedSeqNumber_(other.completedSeqNumber_)
{
}

ReliableOperationSender::~ReliableOperationSender()
{
    AcquireReadLock lock(lock_);
    ASSERT_IF(
        isActive_,
        "{0}->{1}:{2}: ReliableOperationSender.dtor: isActive is true, Close should have been called",
        endpointUniqueId_,
        replicaId_,
        purpose_);
    ASSERT_IF(
        retrySendTimer_,
        "{0}->{1}:{2}: ReliableOperationSender.dtor: Timer not null, Close should have been called",
        endpointUniqueId_,
        replicaId_,
        purpose_);
}

void ReliableOperationSender::Close()
{
    AcquireWriteLock lock(lock_);
    if (isActive_)
    {
        ReplicatorEventSource::Events->OpSenderClose(
            partitionId_, 
            endpointUniqueId_,
            replicaId_,
            purpose_,
            lastAckedReceivedSequenceNumber_,
            lastAckedApplySequenceNumber_);

        ackDurationList_.Clear();
        pendingOperations_.clear();
        isActive_ = false;
        retrySendTimer_->Cancel();
        retrySendTimer_ = nullptr;
        timerActive_ = false;
    }
}

FABRIC_SEQUENCE_NUMBER ReliableOperationSender::get_LastAckedSequenceNumber() const 
{
    AcquireReadLock lock(lock_);
    return lastAckedApplySequenceNumber_; 
}

void ReliableOperationSender::GetProgress(
    __out FABRIC_SEQUENCE_NUMBER & lastAckedReceivedSequenceNumber,
    __out FABRIC_SEQUENCE_NUMBER & lastAckedQuorumSequenceNumber,
    __out DateTime & lastAckProcessedTime,
    __in FABRIC_SEQUENCE_NUMBER completedLsn)
{
    AcquireWriteLock lock(lock_);

    UpdateCompletedLsnCallerHoldsLock(completedLsn);

    lastAckedReceivedSequenceNumber = lastAckedReceivedSequenceNumber_;
    lastAckedQuorumSequenceNumber = lastAckedApplySequenceNumber_;
    lastAckProcessedTime = lastAckProcessedTime_;
}

TimeSpan ReliableOperationSender::get_AvgReceiveAckDuration() const
{
    AcquireReadLock lock(lock_);
    return averageReceiveAckDuration_.Value;
}

TimeSpan ReliableOperationSender::get_AvgApplyAckDuration() const
{
    AcquireReadLock lock(lock_);
    return averageApplyAckDuration_.Value;
}

FABRIC_SEQUENCE_NUMBER ReliableOperationSender::get_NotReceivedCount() const
{
    AcquireReadLock lock(lock_);
    return NotReceivedCountCallerHoldsLock();
}

FABRIC_SEQUENCE_NUMBER ReliableOperationSender::get_ReceivedAndNotAppliedCount() const
{
    AcquireReadLock lock(lock_);
    return ReceivedAndNotAppliedCountCallerHoldsLock();
}

FABRIC_SEQUENCE_NUMBER ReliableOperationSender::NotReceivedCountCallerHoldsLock() const
{
    auto notReceivedCount = highestOperationSequenceNumber_ - lastAckedReceivedSequenceNumber_;
    return notReceivedCount > 0 ? notReceivedCount : 0;
}

FABRIC_SEQUENCE_NUMBER ReliableOperationSender::ReceivedAndNotAppliedCountCallerHoldsLock() const
{
    auto receivedAndNotApplied = lastAckedReceivedSequenceNumber_ - lastAckedApplySequenceNumber_;
    return receivedAndNotApplied > 0 ? receivedAndNotApplied : 0;
}

void ReliableOperationSender::ResetAverageStatistics()
{
    AcquireWriteLock lock(lock_);

    averageReceiveAckDuration_.Reset();
    averageApplyAckDuration_.Reset();
    timeSinceLastAverageUpdate_.Restart();
}

void ReliableOperationSender::Add(
    ComOperationRawPtrVector const & operations,
    FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    if (operations.empty())
    {
        return;
    }

    auto now = DateTime::Now();
    
    // Copy the data to send it outside the lock
    std::vector<ComOperationCPtr> copies;
    
    {
        AcquireWriteLock lock(lock_);
        ReplicatorEventSource::Events->OpSenderAddOps(
            partitionId_,
            endpointUniqueId_,
            replicaId_,
            purpose_,
            lastAckedReceivedSequenceNumber_, 
            lastAckedApplySequenceNumber_,
            operations[0]->SequenceNumber,
            operations[operations.size() - 1]->SequenceNumber,
            static_cast<uint64>(sendWindowSize_));

        if (!isActive_)
        {
            ReplicatorEventSource::Events->OpSenderNotActive(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                L"AddOps");
            return;
        }
        
        UpdateCompletedLsnCallerHoldsLock(completedSeqNumber);

        // The provided data is passed when a replica is added
        // and is a sorted vector with no gaps.
        // pendingOperations_ is a sorted list that may have gaps.
        // It's possible that we have duplicate operations in the 2 lists,
        // if we receive replicate operations at the same time with Update configuration.
        auto itNewOps = operations.begin();
        auto itDest = pendingOperations_.begin();
        ComOperationCPtr opPointer;
        
        size_t initialListSize = pendingOperations_.size();
        bool send = initialListSize < sendWindowSize_;

        while (itNewOps != operations.end() && itDest != pendingOperations_.end())
        {
            if (itDest->first->SequenceNumber > (*itNewOps)->SequenceNumber)
            {
                ackDurationList_.Add((*itNewOps)->SequenceNumber);
                // If there are too many operations pending in the list, 
                // do not send the operation immediately
                if (!send)
                {
                    pendingOperations_.insert(itDest, RawOperationPair(*itNewOps, DateTime::Zero));
                }
                else
                {
                    opPointer.SetAndAddRef(*itNewOps);
                    pendingOperations_.insert(itDest, RawOperationPair(*itNewOps, now));
                    send = pendingOperations_.size() < sendWindowSize_;
                    copies.push_back(move(opPointer));
                }
  
                if ((*itNewOps)->SequenceNumber > highestOperationSequenceNumber_)
                {
                    highestOperationSequenceNumber_ = (*itNewOps)->SequenceNumber;
                }
               
                ++itNewOps;
            }
            else if (itDest->first->SequenceNumber == (*itNewOps)->SequenceNumber)
            {
                ++itNewOps;
                ++itDest;
            }
            else
            {
                ++itDest;
            }
        }
            
        // Insert the remaining operations at the end of the list
        for (; itNewOps != operations.end(); ++itNewOps)
        {
            ackDurationList_.Add((*itNewOps)->SequenceNumber);

            if (!send)
            {
                pendingOperations_.push_back(RawOperationPair(*itNewOps, DateTime::Zero));
            }
            else
            {
                opPointer.SetAndAddRef(*itNewOps);
                pendingOperations_.push_back(RawOperationPair(*itNewOps, now));
                send = pendingOperations_.size() < sendWindowSize_;
                copies.push_back(move(opPointer));
            }

            if ((*itNewOps)->SequenceNumber > highestOperationSequenceNumber_)
            {
                highestOperationSequenceNumber_ = (*itNewOps)->SequenceNumber;
            }
        }

        if (pendingOperations_.size() == initialListSize)
        {
            // Nothing new was added
            return;
        }

        if (!copies.empty())
        {
            FABRIC_SEQUENCE_NUMBER minAddedLSN = copies.front()->SequenceNumber;
            // The operations that are added must be greater than the last ACKed operation.
            ASSERT_IF(
                minAddedLSN <= lastAckedReceivedSequenceNumber_,
                "{0}->{1}:{2}: ReliableOperationSender.Add: {3} should be greater than the current ACK {4}",
                endpointUniqueId_,
                replicaId_,
                purpose_,
                minAddedLSN,
                lastAckedReceivedSequenceNumber_);
        }

        // Start the timer if not already active
        ASSERT_IFNOT(
            retrySendTimer_,
            "{0}->{1}:{2}: ReliableOperationSender.Add: Timer should exist",
            endpointUniqueId_,
            replicaId_,
            purpose_);
        if (!timerActive_)
        {
            retrySendTimer_->Change(config_->RetryInterval);
            timerActive_ = true;
        }
    }
       
    for(ComOperationCPtr const & opPtr : copies)
    {
        if(!this->opCallback_(opPtr, false, completedSeqNumber))
        {
            // Stop sending rest of the operations if there are errors 
            break;
        }
    }
}

void ReliableOperationSender::UpdateCompletedLsnCallerHoldsLock(FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    if (completedSeqNumber != Constants::InvalidLSN)
        completedSeqNumber_ = completedSeqNumber;
}

void ReliableOperationSender::Add(
    ComOperationRawPtr const operationPtr,
    FABRIC_SEQUENCE_NUMBER completedSeqNumber)
{
    ComOperationCPtr copy;
    bool send;
    
    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->OpSenderNotActive(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                L"AddOp");
            return;
        }

        UpdateCompletedLsnCallerHoldsLock(completedSeqNumber);

        ASSERT_IF(operationPtr == nullptr, "{0}->{1}:{2}: Add: operation should not be null", endpointUniqueId_, replicaId_, purpose_);
        ReplicatorEventSource::Events->OpSenderAddOp(
            partitionId_,
            endpointUniqueId_,
            replicaId_,
            purpose_,
            lastAckedReceivedSequenceNumber_, 
            lastAckedApplySequenceNumber_,
            operationPtr->SequenceNumber,
            static_cast<uint64>(sendWindowSize_));
    
        // Insert at the correct position to keep the list sorted:
        // Since it's more probable to have the operations in order, 
        // start at the end and search for the first operation with LSN smaller 
        // than the new oeration's LSN.
        // If the operation already exists, there is nothing left to do.
        // Otherwise, insert the operation and then call the callback outside the lock.
        auto it = pendingOperations_.rbegin();
        while (it != pendingOperations_.rend())
        {
            if (it->first->SequenceNumber == operationPtr->SequenceNumber)
            {
                // The data is already in the list, NOP
                return;
            }
            if (it->first->SequenceNumber > operationPtr->SequenceNumber)
            {
                ++it;
            }
            else
            {
                break;
            }
        }

        // The new operation must be greater than the last ACKed operation.
        ASSERT_IF(
            operationPtr->SequenceNumber <= lastAckedReceivedSequenceNumber_,
            "{0}->{1}:{2}: ReliableOperationSender.Add: {3} should be greater than the current ACK {4}",
            endpointUniqueId_,
            replicaId_,
            purpose_,
            operationPtr->SequenceNumber,
            lastAckedReceivedSequenceNumber_);
        
        ackDurationList_.Add(operationPtr->SequenceNumber);
        // If there are too many operations pending in the list, 
        // do not send the operation immediately
        if (pendingOperations_.size() >= sendWindowSize_)
        {
            pendingOperations_.insert(it.base(), RawOperationPair(operationPtr, DateTime::Zero));
            send = false;
        }
        else
        {
            copy.SetAndAddRef(operationPtr);
            DateTime now = DateTime::Now();
            pendingOperations_.insert(it.base(), RawOperationPair(operationPtr, now));
            send = true;
        }
        
        if (operationPtr->SequenceNumber > highestOperationSequenceNumber_)
        {
            highestOperationSequenceNumber_ = operationPtr->SequenceNumber;
        }
                
        // Start the timer if not already active
        ASSERT_IFNOT(
            retrySendTimer_,
            "{0}->{1}:{2}: ReliableOperationSender.Add: Timer should exist",
            endpointUniqueId_, replicaId_, purpose_);
        if (!timerActive_)
        {
            retrySendTimer_->Change(config_->RetryInterval);
            timerActive_ = true;
        }
    }
     
    if (send)
    {
        opCallback_(copy, false, completedSeqNumber);
    }
}

void ReliableOperationSender::RemoveOperationsCallerHoldsLock(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    //
    // Tracks the removed range
    //
    FABRIC_SEQUENCE_NUMBER from = FABRIC_INVALID_SEQUENCE_NUMBER;
    FABRIC_SEQUENCE_NUMBER to = FABRIC_INVALID_SEQUENCE_NUMBER;

    //
    // Tracks the remaining range
    //
    FABRIC_SEQUENCE_NUMBER remainingFrom = FABRIC_INVALID_SEQUENCE_NUMBER;
    FABRIC_SEQUENCE_NUMBER remainingTo = FABRIC_INVALID_SEQUENCE_NUMBER;

    if (pendingOperations_.empty())
    {
        ReplicatorEventSource::Events->OpSenderRemoveOpsEmpty(
            partitionId_, 
            endpointUniqueId_,
            replicaId_,
            purpose_,
            sequenceNumber);

        return;
    }

    pendingOperations_.erase(
        std::remove_if(
            pendingOperations_.begin(),
            pendingOperations_.end(),
            [&](RawOperationPair it)
            { 
                if (FABRIC_INVALID_SEQUENCE_NUMBER == from)
                {
                    from = it.first->SequenceNumber;
                }

                to = it.first->SequenceNumber;

                return it.first->SequenceNumber <= sequenceNumber;
            }),
        pendingOperations_.end());

    if (FABRIC_INVALID_SEQUENCE_NUMBER == from)
    {
        remainingFrom = pendingOperations_.front().first->SequenceNumber;
        remainingTo = pendingOperations_.back().first->SequenceNumber;
    }

    if (FABRIC_INVALID_SEQUENCE_NUMBER != from)
    {
        ReplicatorEventSource::Events->OpSenderRemoveOps(
            partitionId_, 
            endpointUniqueId_,
            replicaId_,
            purpose_,
            from,
            to,
            sequenceNumber);
    }
    else
    {
        ReplicatorEventSource::Events->OpSenderRemoveOpsNothingRemoved(
            partitionId_, 
            endpointUniqueId_,
            replicaId_,
            purpose_,
            remainingFrom,
            remainingTo,
            sequenceNumber);
    }
}

void ReliableOperationSender::OnTimerCallback()
{
    // Copy the data to send it outside the lock
    std::vector<ComOperationCPtr> copies;
    FABRIC_SEQUENCE_NUMBER completedSeqNumber;
    bool requestAck;

    {
        AcquireWriteLock lock(lock_);
        if (!isActive_)
        {
            ReplicatorEventSource::Events->OpSenderNotActive(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                L"OnTimerCallback");
            return;
        }
        
        completedSeqNumber = completedSeqNumber_;

        if (noAckSinceLastCallback_ && sendWindowSize_ > 1)
        {
            // Since it's been a while since the 
            // other side ACKed operations, it may be overwhelmed.
            // To avoid putting even more pressure on it,
            // send only half the operations previously allowed.
            sendWindowSize_ /= 2;
        }

        noAckSinceLastCallback_ = true;
        GetNextSendAndSetTimerCallerHoldsLock(requestAck, copies);
    }

    if (requestAck)
    {
        ASSERT_IFNOT(copies.empty(), "{0}->{1}:{2}: Can't request ACK if there are pending ops", endpointUniqueId_, replicaId_, purpose_);
        opCallback_(ComOperationCPtr(), requestAck, completedSeqNumber);
    }
    else
    {
        for(ComOperationCPtr const & opPtr : copies)
        {
            if(!this->opCallback_(opPtr, false, completedSeqNumber))
            {
                // Stop sending rest of the operations if there are errors 
                break;
            }
        }
    }
}

bool ReliableOperationSender::ProcessOnAck(
    FABRIC_SEQUENCE_NUMBER ackedReceivedSequenceNumber,
    FABRIC_SEQUENCE_NUMBER ackedQuorumSequenceNumber)
{
    ASSERT_IF(
        ackedReceivedSequenceNumber < ackedQuorumSequenceNumber,
        "{0}->{1}:{2}: Ack received {3} < ack quorum {4}",
        endpointUniqueId_,
        replicaId_,
        purpose_,
        ackedReceivedSequenceNumber,
        ackedQuorumSequenceNumber);

    bool progressQuorumDone = false;
    bool progressReceiveDone = false;
    std::vector<ComOperationCPtr> copies;
    bool requestAck = false;
    FABRIC_SEQUENCE_NUMBER completedSeqNumber;
            
    {
        AcquireWriteLock lock(lock_);
        if(!isActive_)
        {
            ReplicatorEventSource::Events->OpSenderNotActive(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                L"ProcessOnAck");
            return false;
        }

        completedSeqNumber = completedSeqNumber_;
        lastAckProcessedTime_ = DateTime::Now();

        if (ackedReceivedSequenceNumber > lastAckedReceivedSequenceNumber_ || 
            ackedQuorumSequenceNumber > lastAckedApplySequenceNumber_)
        {
            ReplicatorEventSource::Events->OpSenderAck(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                lastAckedReceivedSequenceNumber_,
                ackedReceivedSequenceNumber,
                lastAckedApplySequenceNumber_,
                ackedQuorumSequenceNumber);
        }
        
        if (ackedReceivedSequenceNumber > lastAckedReceivedSequenceNumber_)
        {
            // Remove the operations that are receive ACKed, since we don't have to send them anymore
            RemoveOperationsCallerHoldsLock(ackedReceivedSequenceNumber);
            lastAckedReceivedSequenceNumber_ = ackedReceivedSequenceNumber;

            // Though there is progress only in receivedACK and not quorum ACK, we mark this as
            // some progress because this could lead to clearing out more operations in the queue
            // if this replica is an idle
            progressReceiveDone = true;
        }

        if (ackedQuorumSequenceNumber > lastAckedApplySequenceNumber_)
        {
            progressQuorumDone = true;
            lastAckedApplySequenceNumber_ = ackedQuorumSequenceNumber;
        }

        if (progressQuorumDone || progressReceiveDone)
        {
            noAckSinceLastCallback_ = false;
            if (sendWindowSize_ < static_cast<size_t>(maxSendWindowSize_))
            {
                // Since the session on the other side is making progress,
                // allow to send more operations that the current window size.
                sendWindowSize_ *= 2;
            }

            ackDurationList_.OnAck(lastAckedReceivedSequenceNumber_, lastAckedApplySequenceNumber_);
        }

        GetNextSendAndSetTimerCallerHoldsLock(requestAck, copies);
    }

    // Do not send request ACK, it will be sent on callback only.
    // Otherwise, if the other side didn't make progress,
    // it will send ACK back with the same numbers and the behavior will be repeated in a loop.
    if (!requestAck)
    {
        for(ComOperationCPtr const & opPtr : copies)
        {
            if(!this->opCallback_(opPtr, false, completedSeqNumber))
            {
                // Stop sending rest of the operations if there are errors 
                break;
            }
        }
    }

    return progressQuorumDone || progressReceiveDone;
}

// Get next operations to send (upto to a max number of operations)
// if too much time has passed since an ACK was received
// or the operation was never sent.
// If no operations exist, send requestACK if there are pending quorum ACKs.
// Otherwise, disable timer.
void ReliableOperationSender::GetNextSendAndSetTimerCallerHoldsLock(
    __out bool & requestAck,
    __out std::vector<ComOperationCPtr> & operationsToSend)
{
    requestAck = false;
    
    TimeSpan avgReceiveAckDuration;
    TimeSpan avgApplyAckDuration;
    bool updateAverageDurations = false;

    // Update this because the config is dynamic and can change 
    averageReceiveAckDuration_.UpdateDecayFactor(config_->SecondaryProgressRateDecayFactor, config_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation);
    averageApplyAckDuration_.UpdateDecayFactor(config_->SecondaryProgressRateDecayFactor, config_->SlowActiveSecondaryRestartAtAgeOfOldestPrimaryOperation);
    
    if (timeSinceLastAverageUpdate_.Elapsed > config_->RetryInterval)
    {
        updateAverageDurations = true;

        ackDurationList_.ComputeAverageAckDuration(
            avgReceiveAckDuration,
            avgApplyAckDuration);

        timeSinceLastAverageUpdate_.Restart();
    }

    if (pendingOperations_.empty())
    {
        if (lastAckedApplySequenceNumber_ >= lastAckedReceivedSequenceNumber_)
        {
            if (timerActive_)
            {
                // Nothing to send, so disable timer
                timerActive_ = false;
                ReplicatorEventSource::Events->OpSenderTimer(
                    partitionId_, 
                    endpointUniqueId_,
                    replicaId_,
                    purpose_,
                    true);
                retrySendTimer_->Change(TimeSpan::MaxValue);
            }

            return;
        }
        else
        {
            // There are no operationsToSend pending, 
            // but there are pending Quorum ACKs. Enable timer.
            requestAck = true;
            if (updateAverageDurations)
                averageApplyAckDuration_.Update(avgApplyAckDuration);
        }
    }
    else
    {
        if (updateAverageDurations)
        {
            averageApplyAckDuration_.Update(avgApplyAckDuration);
            averageReceiveAckDuration_.Update(avgReceiveAckDuration);
        }

        DateTime now = DateTime::Now();
        ComOperationCPtr opPointer;
            
        // Loop through the operationsToSend and resend the ones that 
        // have been sent more than the retry interval ago
        size_t maxSendOps = 0;
        for (auto it = pendingOperations_.begin(); it != pendingOperations_.end(); ++it)
        {
            if (it->second + config_->RetryInterval <= now)
            {
                if (maxSendOps >= sendWindowSize_)
                {
                    break;
                }

                opPointer.SetAndAddRef(it->first);
                it->second = now;
                operationsToSend.push_back(move(opPointer));
                ++maxSendOps;
            }
        }

        if (operationsToSend.empty())
        {
            ReplicatorEventSource::Events->OpSenderTimer(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                false);
        }
        else
        {
            ReplicatorEventSource::Events->OpSenderSend(
                partitionId_, 
                endpointUniqueId_,
                replicaId_,
                purpose_,
                operationsToSend.front()->SequenceNumber,
                operationsToSend.back()->SequenceNumber,
                static_cast<uint64>(sendWindowSize_));
        }
    }
    
    // Schedule the timer again since there are pending operationsToSend
    ASSERT_IFNOT(
        retrySendTimer_,
        "{0}->{1}:{2}: ReliableOperationSender.OnTimerCallback: Timer should exist",
        endpointUniqueId_,
        replicaId_,
        purpose_);
        
    retrySendTimer_->Change(config_->RetryInterval);
    timerActive_ = true;
}

    wstring ReliableOperationSender::ToString() const
{
    wstring content;
    StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));        
    return content;
}

void ReliableOperationSender::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    AcquireReadLock lock(lock_);
    FABRIC_SEQUENCE_NUMBER notReceivedCount = NotReceivedCountCallerHoldsLock();
    FABRIC_SEQUENCE_NUMBER notAppliedCount = ReceivedAndNotAppliedCountCallerHoldsLock();

    w.Write(
        "{0}:\tNotReceivedCount={1}\tReceivedAndNotAppliedCount={2}\tReceive.ACK={3}(Avg {4})\tApply.ACK={5}(Avg {6})\tSWS={7}\tCompletedLSN={8}",
        purpose_,
        notReceivedCount,
        notAppliedCount,
        lastAckedReceivedSequenceNumber_,
        averageReceiveAckDuration_.ToString(),
        lastAckedApplySequenceNumber_,
        averageApplyAckDuration_.ToString(),
        sendWindowSize_,
        completedSeqNumber_);
}

std::string ReliableOperationSender::AddField(Common::TraceEvent & traceEvent, std::string const & name)
{
    std::string format = "{0}:\tNotReceivedCount={1}\tReceivedAndNotAppliedCount={2}\tReceive.ACK={3}(Avg {4})\tApply.ACK={5}(Avg {6})\tSWS={7}\tCompletedLSN={8}";
    size_t index = 0;

    traceEvent.AddEventField<wstring>(format, name + ".purpose", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".numberofOpsNotReceived", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".numberofOpsNotApplied", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".lastACKedReceived", index);
    traceEvent.AddEventField<wstring>(format, name + ".avgReceiveAckDur", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".lastACKedQuorum", index);
    traceEvent.AddEventField<wstring>(format, name + ".avgApplyAckDur", index);
    traceEvent.AddEventField<uint64>(format, name + ".sws", index);
    traceEvent.AddEventField<FABRIC_SEQUENCE_NUMBER>(format, name + ".completedLSN", index);

    return format;
}

void ReliableOperationSender::FillEventData(Common::TraceEventContext & context) const
{
    AcquireReadLock lock(lock_);
    context.Write<wstring>(purpose_); // Write is OK since this is a const member
    context.WriteCopy<FABRIC_SEQUENCE_NUMBER>(NotReceivedCountCallerHoldsLock());
    context.WriteCopy<FABRIC_SEQUENCE_NUMBER>(ReceivedAndNotAppliedCountCallerHoldsLock());
    context.WriteCopy<FABRIC_SEQUENCE_NUMBER>(lastAckedReceivedSequenceNumber_);
    context.WriteCopy<wstring>(averageReceiveAckDuration_.ToString());
    context.WriteCopy<FABRIC_SEQUENCE_NUMBER>(lastAckedApplySequenceNumber_);
    context.WriteCopy<wstring>(averageApplyAckDuration_.ToString());
    context.WriteCopy<uint64>(static_cast<uint64>(sendWindowSize_));
    context.WriteCopy<FABRIC_SEQUENCE_NUMBER>(completedSeqNumber_);
}

// Used by CITs only
void ReliableOperationSender::Test_GetPendingOperationsCopy(
    __out std::list<RawOperationPair> & ops,
    __out bool & timerActive,
    __out size_t & sendWindowSize)
{
    AcquireWriteLock lock(lock_);
    for (auto op : pendingOperations_)
    {
        ops.push_back(op);
    }

    timerActive = timerActive_;
    sendWindowSize = sendWindowSize_;
}

// Implementation of the OperationLatencyList Class
ReliableOperationSender::OperationLatencyList::OperationLatencyList()
    : items_()
    , lastApplyAck_(0)
{
}

void ReliableOperationSender::OperationLatencyList::Add(FABRIC_SEQUENCE_NUMBER lsn)
{
    auto it = items_.rbegin();
    while (it != items_.rend())
    {
        if (it->first == lsn)
        {
            return;
        }
        if (it->first > lsn)
        {
            ++it;
        }
        else
        {
            break;
        }
    }

    LSNSendTimePair item(lsn, ReceiveAndApplyTimePair(Stopwatch(), Stopwatch()));
    item.second.first.Start();
    item.second.second.Start();

    items_.insert(it.base(), item);
}

void ReliableOperationSender::OperationLatencyList::Clear()
{
    items_.clear();
}

void ReliableOperationSender::OperationLatencyList::OnAck(
    FABRIC_SEQUENCE_NUMBER receiveAckLsn,
    FABRIC_SEQUENCE_NUMBER applyAckLsn)
{
    ASSERT_IF(receiveAckLsn < applyAckLsn, "ReceiveAck {0} cannot be greater than applyAck {1}", receiveAckLsn, applyAckLsn);

    if (items_.empty())
        return;

    auto jumpToIndex = lastApplyAck_ - items_[0].first + 1;

    if (jumpToIndex < 0)
        jumpToIndex = 0;

    auto it = begin(items_) + jumpToIndex;

    while (it != end(items_) && it->first <= receiveAckLsn)
    {
        it->second.first.Stop(); // Stop the receive ack stopwatch

        if (it->first <= applyAckLsn)
            it->second.second.Stop(); // stop the apply ack stopwatch if needed

        ++it;
    }

    lastApplyAck_ = applyAckLsn;
}

void ReliableOperationSender::OperationLatencyList::ComputeAverageAckDuration(
    TimeSpan& receiveAckDuration,
    TimeSpan& applyAckDuration)
{
    LONGLONG receiveDurationOpCount = 0;
    LONGLONG applyDurationOpCount = 0;

    items_.erase(
        std::remove_if(
            items_.begin(),
            items_.end(),
            [&](LSNSendTimePair it)
            {
                UpdateAverageTime(it.second.first.ElapsedTicks, receiveAckDuration, receiveDurationOpCount);
                UpdateAverageTime(it.second.second.ElapsedTicks, applyAckDuration, applyDurationOpCount);

                return it.second.first.IsRunning == false && it.second.second.IsRunning == false;
            }),
        items_.end());
}

} // end namespace ReplicationComponent
} // end namespace Reliability
