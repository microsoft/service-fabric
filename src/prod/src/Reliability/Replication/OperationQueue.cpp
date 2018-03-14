// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

namespace Reliability {
namespace ReplicationComponent {

using Common::Assert;
using Common::ErrorCode;
using Common::Stopwatch;
using Common::StopwatchTime;
using Common::TimeSpan;
using Common::make_unique;

using std::make_shared;
using std::map;
using std::move;
using std::pair;
using std::wstring;
using std::swap;
using std::vector;

OperationQueue::OperationQueue(
    Common::Guid const & partitionId, 
    std::wstring const & description,
    ULONGLONG initialSize,
    ULONGLONG maxSize,
    ULONGLONG maxMemorySize,
    ULONGLONG maxCompletedOperationsCount,
    ULONGLONG maxCompletedOperationsMemorySize,
    bool requireServiceAck,
    bool cleanOnComplete,
    bool ignoreCommit,
    FABRIC_SEQUENCE_NUMBER startSequence,
    REPerformanceCountersSPtr const & perfCounters)
    :   initialSize_(initialSize),
        maxSize_(maxSize),
        maxMemorySize_(maxMemorySize),
        queue_(),
        partitionId_(partitionId),
        description_(description),
        requireServiceAck_(requireServiceAck),
        commitCallback_(nullptr),
        cleanOnComplete_(cleanOnComplete),
        ignoreCommit_(ignoreCommit),
        capacity_(initialSize),
        head_(startSequence), 
        tail_(startSequence),
        committedHead_(startSequence),
        completedHead_(startSequence),
        expandedLast_(false),
        capacitySum_(initialSize),
        convergentCapacity_(0),
        capacityChangeCount_(1),
        mask_(initialSize - 1),
        totalMemorySize_(0),
        completedMemorySize_(0),
        completedOperationCount_(0),
        operationCount_(0),
        maxCompletedOperationsMemorySize_(maxCompletedOperationsMemorySize),
        maxCompletedOperationsSize_(maxCompletedOperationsCount),
        perfCounters_(perfCounters)
{
    ASSERT_IF(initialSize_ <= 1, "start capacity {0} must be greater than 1", initialSize_);
    ASSERT_IF(maxSize_ > 0 && initialSize_ > maxSize_, 
        "The limits of the queue capacity are not correct. {0} should be smaller or equal than {1}", initialSize_, maxSize_);
    ASSERT_IF(!IsPowerOf2(initialSize_), "The operation queue start capacity ({0}) must be a power of 2", initialSize_);
    ASSERT_IF(maxSize_ > 0 && !IsPowerOf2(maxSize_), "The operation queue max capacity ({0}) must be a power of 2", maxSize_);
    ASSERT_IF(maxSize_ == 0 && maxMemorySize_ == 0, "Either maxSize_ or maxMemorySize_ must be configured");

    ASSERT_IF(startSequence <= Constants::InvalidLSN, "startSequence {0} must be strictly positive", startSequence);

    ASSERT_IF(cleanOnComplete_ && maxCompletedOperationsMemorySize_ != 0 && maxCompletedOperationsSize_ != 0, "Queue {0}: Queue is cleaned up on complete. Cannot set completed operation size limit", ToString());
 
    // Resize from the beginning to the correct capacity,
    // filling the vector with nullptr
    queue_.resize(SafeConvert<size_t>(capacity_));
    OperationQueueEventSource::Events->Ctor(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->maxSize_,
        this->maxMemorySize_,
        this->operationCount_);
}

OperationQueue::OperationQueue(
    wstring const & newDescription,
    OperationQueue && other,
    ULONGLONG initialSize,
    ULONGLONG maxSize,
    ULONGLONG maxMemorySize,
    ULONGLONG maxCompletedOperationsCount,
    ULONGLONG maxCompletedOperationsMemorySize,
    bool cleanOnComplete,
    REPerformanceCountersSPtr const & perfCounters)
    :   initialSize_(initialSize),
        maxSize_(maxSize),
        maxMemorySize_(maxMemorySize),
        queue_(move(other.queue_)),
        partitionId_(move(other.partitionId_)),
        description_(move(other.description_)),
        requireServiceAck_(other.requireServiceAck_),
        commitCallback_(move(other.commitCallback_)),
        cleanOnComplete_(cleanOnComplete),
        ignoreCommit_(other.ignoreCommit_),
        capacity_(other.capacity_),
        head_(other.head_), 
        tail_(other.tail_),
        committedHead_(other.committedHead_),
        completedHead_(other.completedHead_),
        expandedLast_(other.expandedLast_),
        capacitySum_(other.capacitySum_),
        convergentCapacity_(other.convergentCapacity_),
        capacityChangeCount_(other.capacityChangeCount_),
        mask_(other.mask_),
        totalMemorySize_(other.totalMemorySize_),
        completedMemorySize_(other.completedMemorySize_),
        completedOperationCount_(other.completedOperationCount_),
        operationCount_(other.operationCount_),
        maxCompletedOperationsMemorySize_(maxCompletedOperationsMemorySize),
        maxCompletedOperationsSize_(maxCompletedOperationsCount),
        perfCounters_(perfCounters)
{
    description_ = newDescription;

    // When an operation is moved from 1 queue to another, its lifecycle starts again
    // As a result, we must refresh its enqueue time to the current time
    for (ComOperationCPtr op : queue_)
    {
        if(op) op->RefreshEnqueueTime();
    }

    // resize the queue to the correct size
    if (maxSize > 0)
    {
        ASSERT_IF(operationCount_ > maxSize_, "operation count {0} is greater than max size {1}", operationCount_, maxSize_);

        bool prevExpandedLastValue = expandedLast_;
        // This is to force the shrink
        expandedLast_ = true;
        if (!Shrink(tail_ - completedHead_, false /*clearCompletedItems*/))
        {
            // if shrink failed, restore the value of expandedLast_
            expandedLast_ = prevExpandedLastValue;
        }
    
        // Reset the capacity stats
        capacitySum_ = capacity_;
        convergentCapacity_ = 0;
        capacityChangeCount_ = 1;
        mask_ = capacity_ - 1;
    }

    ASSERT_IF(initialSize_ <= 1, "start capacity {0} must be greater than 1", initialSize_);
    ASSERT_IF(maxSize_ > 0 && initialSize_ > maxSize_, 
        "The limits of the queue capacity are not correct. {0} should be smaller or equal than {1}", initialSize_, maxSize_);
    ASSERT_IF(!IsPowerOf2(initialSize_), "The operation queue start capacity ({0}) must be a power of 2", initialSize_);
    ASSERT_IF(maxSize_ > 0 && !IsPowerOf2(maxSize_), "The operation queue max capacity ({0}) must be a power of 2", maxSize_);
    ASSERT_IF(maxSize_ == 0 && maxMemorySize_ == 0, "Either maxSize_ or maxMemorySize_ must be configured");

    ASSERT_IF(cleanOnComplete_ && maxCompletedOperationsMemorySize_ != 0 && maxCompletedOperationsSize_ != 0, "Queue {0}: Queue is cleaned up on complete. Cannot set completed operation size limit", ToString());

    OperationQueueEventSource::Events->Ctor(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->maxSize_,
        this->maxMemorySize_,
        this->operationCount_);
}

OperationQueue::~OperationQueue()
{
    OperationQueueEventSource::Events->Dtor(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->operationCount_);
}

void OperationQueue::set_IgnoreCommit(bool value) 
{
    // Commit and Head should be the same number
    if (committedHead_ != head_)
    {
        Assert::CodingError("Queue {0}: ignoreCommit->{1}: Commit != head", ToString(), value);
    }

    ignoreCommit_ = value; 
}

TimeSpan OperationQueue::get_FirstOperationInReplicationQueueEnqueuedSince() const
{
    if (operationCount_  == 0)
        return TimeSpan::Zero;

    auto op = queue_[GetPosition(completedHead_)];
    return (Stopwatch::Now() - op->EnqueueTime);
}

void OperationQueue::SetCommitCallback(OperationCallback const & callback) 
{ 
    commitCallback_ = callback; 

    for (FABRIC_SEQUENCE_NUMBER i = head_; i < committedHead_; ++i)
    {
        // When executing the callback on an operation, 
        // it may be cleared. So we need to check at every step.
        if (!commitCallback_)
        {
            return;
        }
        // Apply the callback to all already committed operations
        size_t pos = GetPosition(i);
        if (!queue_[pos])
        {
            Assert::CodingError("Queue {0}: SetCommitCallback: Operation {1} doesn't exist", ToString(), i);
        }

        // The callback must execute fast or
        // the user should schedule it on a different thread.
        commitCallback_(queue_[pos]);
    }
}

size_t OperationQueue::GetPosition(FABRIC_SEQUENCE_NUMBER sequenceNumber, FABRIC_SEQUENCE_NUMBER mask)
{
    return SafeConvert<size_t>(sequenceNumber & mask);
}

ULONGLONG OperationQueue::get_ConvergentCapacity() const
{ 
    if (convergentCapacity_ == 0)
    {
        // Cache the value so next time it won't be computed. 
        // The value is invalidated when the sum and change count are modified
        convergentCapacity_ = GetCeilingPowerOf2(static_cast<ULONGLONG>(capacitySum_ / capacityChangeCount_));
    }

    return convergentCapacity_; 
}
      
ErrorCode OperationQueue::MakeRoomForOperations(
    FABRIC_SEQUENCE_NUMBER lastSequenceNumber,
    ULONGLONG totalDataSize)
{
    if (maxMemorySize_ > 0)
    {
        if ((totalMemorySize_ - completedMemorySize_ + totalDataSize) > maxMemorySize_)
        {
            bool queueMemoryFull = true;
            if (lastSequenceNumber < tail_)
            {
                FABRIC_SEQUENCE_NUMBER lsnDropStart = tail_ - 1;
                ULONGLONG memoryAfterDrop = totalMemorySize_ - completedMemorySize_ + totalDataSize;
                while (lsnDropStart > lastSequenceNumber && memoryAfterDrop > maxMemorySize_)
                {
                    memoryAfterDrop -= queue_[GetPosition(lsnDropStart)]->DataSize;
                    while (--lsnDropStart >= committedHead_ && !queue_[GetPosition(lsnDropStart)]);
                }
                if (memoryAfterDrop <= maxMemorySize_)
                {
                    // TODO: drop all in the same batch
                    OperationQueueEventSource::Events->DiscardPendingOps(
                        this->partitionId_,
                        this->description_,
                        this->completedHead_,
                        this->head_,
                        this->committedHead_,
                        this->tail_,
                        lsnDropStart + 1,
                        this->tail_ - 1);
                    DiscardPendingOperations(lsnDropStart+1);
                    queueMemoryFull = false;
                }
            }

            if (queueMemoryFull)
            {
                OperationQueueEventSource::Events->QueueMemoryFull(
                    this->partitionId_,
                    this->description_,
                    this->completedHead_,
                    this->head_,
                    this->committedHead_,
                    this->tail_,
                    lastSequenceNumber,
                    totalDataSize,
                    this->totalMemorySize_);
                return ErrorCode(Common::ErrorCodeValue::REQueueFull);
            }
        }
        ULONGLONG newMemorySize = totalMemorySize_ + totalDataSize;
        if (newMemorySize > maxMemorySize_)
        {
            // Try to remove some completed operations to make room for this new operation.
            FABRIC_SEQUENCE_NUMBER snClearedUpto;
            for (snClearedUpto = completedHead_; snClearedUpto < head_ && newMemorySize > maxMemorySize_; snClearedUpto++)
            {
                newMemorySize -= queue_[GetPosition(snClearedUpto)]->DataSize;
            }
            //TODO: clean all operations in same batch
            ASSERT_IF(newMemorySize > maxMemorySize_, "newMemorySize > maxMemorySize_");
            ClearCompleted(snClearedUpto);
        }
    }

    FABRIC_SEQUENCE_NUMBER missingQueueSlots = lastSequenceNumber - completedHead_ - static_cast<FABRIC_SEQUENCE_NUMBER>(capacity_) + 1;

    if (missingQueueSlots > 0)
    {
        // If the queue is supposed to have completed items (e.g on secondary), we will try to expand the queue without removing any elements
        if (!cleanOnComplete_ &&
           Expand(lastSequenceNumber - completedHead_))
        {
            return Common::ErrorCodeValue::Success;
        }

        // There is not enough room for the new item;
        // First try to remove as few as possible completed items to make room;
        // if there are no sufficient completed items, remove all of them
        // then expand the queue.
        if (missingQueueSlots <= head_ - completedHead_)
        {
            ClearCompleted(completedHead_ + missingQueueSlots);
            // Shrink if possible
            Shrink(lastSequenceNumber - completedHead_, false /*clearCompleted*/);
        }
        else
        {
            // Not enough items to remove, try to expand
            ClearCompleted();

            if (!Expand(lastSequenceNumber - head_))
            {
                OperationQueueEventSource::Events->QueueFull(
                    this->partitionId_,
                    this->description_,
                    this->completedHead_,
                    this->head_,
                    this->committedHead_,
                    this->tail_,
                    lastSequenceNumber,
                    this->totalMemorySize_);

                return ErrorCode(Common::ErrorCodeValue::REQueueFull);
            }
        }
    }

    return Common::ErrorCodeValue::Success;
}
      
ErrorCode OperationQueue::TryEnqueue(ComOperationCPtr const & operationPtr)
{
    FABRIC_SEQUENCE_NUMBER sequenceNumber = operationPtr->SequenceNumber;

    if (sequenceNumber <= Constants::InvalidLSN)
    {
        Assert::CodingError("{0}: TryEnqueue: {1} should be > 0", ToString(), sequenceNumber);
    }

    if (sequenceNumber < committedHead_ || (sequenceNumber < tail_ && queue_[GetPosition(sequenceNumber)]))
    {
        OperationQueueEventSource::Events->Duplicate(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            sequenceNumber);

        return ErrorCode(Common::ErrorCodeValue::REDuplicateOperation);
    }

    ErrorCode error = MakeRoomForOperations(sequenceNumber, operationPtr->DataSize);
    if (!error.IsSuccess())
    {
        return error;
    }

    size_t position = GetPosition(sequenceNumber);
    
    operationCount_ += 1;
    totalMemorySize_ += operationPtr->DataSize;
    if (sequenceNumber < tail_)
    {
        queue_[position] = operationPtr;
    }
    else
    {
        // All previous completed items should be removed before the expansion;
        // so we should have only null between the old tail and the new one
        for (FABRIC_SEQUENCE_NUMBER i = tail_; i <= sequenceNumber; ++i)
        {
            size_t pos = GetPosition(i); 
            if (pos >= queue_.size() || queue_[pos])
            {
                Assert::CodingError(
                    "Queue {0}: The operation at {1} should have been removed", 
                    ToString(), 
                    pos);
            }
        }

        queue_[position] = move(operationPtr);
        // Update tail to the new position
        tail_ = sequenceNumber + 1;
    }


    OperationQueueEventSource::Events->Enqueue(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        sequenceNumber,
        static_cast<int>(operationPtr->Type),
        this->totalMemorySize_);

    UpdateRatePerfCounters(operationPtr->DataSize);

    CheckQueueInvariants();
    return ErrorCode(Common::ErrorCodeValue::Success);
}

ComOperationRawPtr OperationQueue::GetOperation(
    FABRIC_SEQUENCE_NUMBER sequenceNumber) const
{
    if (sequenceNumber < tail_)
    {
        size_t pos = GetPosition(sequenceNumber); 
        if (queue_[pos])
        {
            return queue_[pos].GetRawPointer();
        }
    }

    return nullptr;
}

bool OperationQueue::GetOperations(
    FABRIC_SEQUENCE_NUMBER first,
    __out ComOperationRawPtrVector & operations) const
{
    if (first >= tail_)
    {
        // Nothing to return
        return true;
    }

    OperationQueueEventSource::Events->Query(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        first,
        tail_ - 1);

    for(FABRIC_SEQUENCE_NUMBER i = first; i < tail_; ++i)
    {
        size_t pos = GetPosition(i); 
        if (!queue_[pos])
        {
            OperationQueueEventSource::Events->CancelOp(
                this->partitionId_,
                this->description_,
                this->completedHead_,
                this->head_,
                this->committedHead_,
                this->tail_,
                L"GetOperations",
                i);

            return false;
        }

        operations.push_back(queue_[pos].GetRawPointer());
    }

    return true;
}

bool OperationQueue::Expand(ULONGLONG activeItemsCount)
{
    ASSERT_IFNOT(activeItemsCount >= capacity_, "Queue {0}: Expand is only needed if there is not enough room", ToString());
    ULONGLONG newCapacity;
    if (ConvergentCapacity > activeItemsCount)
    {
        // Increase the capacity directly to the convergent value
        newCapacity = ConvergentCapacity;
    }
    else
    {
        newCapacity = capacity_ << 1;
        while(activeItemsCount >= newCapacity && (maxSize_ == 0 || newCapacity < maxSize_))
        {
            newCapacity <<= 1;
        }
    
        if ((maxSize_ > 0 && newCapacity > maxSize_) || activeItemsCount >= newCapacity)
        {
            // Can't expand the queue, becuase the max size is reached
            return false;
        }
    }

    OperationQueueEventSource::Events->Resize(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->capacity_,
        newCapacity);

    UpdateCapacity(newCapacity);

    expandedLast_ = true;
    return true;
}

void OperationQueue::ClearCompleted(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (completedHead_ >= sequenceNumber)
    {
        return;
    }

    OperationQueueEventSource::Events->Remove(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->completedHead_,
        sequenceNumber - 1);
    
    TimeSpan elapsedTime;
    while(completedHead_ < sequenceNumber)
    {
        // Clean the completed item and move the pointer
        RemoveItem(completedHead_, elapsedTime);
        ++completedHead_;
        UpdatePerfCounter(PerfCounterName::AverageCleanupTime, elapsedTime);
    }

    CheckQueueInvariants();
}
    
bool OperationQueue::Shrink(ULONGLONG activeItemsCount, bool clearCompleted)
{
    if (!expandedLast_)
    {
        return false;
    }

    // Determine the correct size based on the number of active items
    ULONGLONG newCapacity = initialSize_;
    while(newCapacity < capacity_ && newCapacity < activeItemsCount)
    {
        // Increase the capacity until all active items have room
        newCapacity <<= 1;
    }

    if (newCapacity >= capacity_)
    {
        OperationQueueEventSource::Events->ShrinkError(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            this->capacity_,
            activeItemsCount);

        return false;
    }

    if (clearCompleted)
    {
        ClearCompleted();
    }

    OperationQueueEventSource::Events->Resize(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        this->capacity_,
        newCapacity);

    UpdateCapacity(newCapacity);
    expandedLast_ = false;
    return true;
}

bool OperationQueue::UpdateCompleteHead(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (sequenceNumber <= completedHead_)
    {
        OperationQueueEventSource::Events->UpdateCompleteBack(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            this->completedHead_,
            sequenceNumber);

        return false;
    }

    if (sequenceNumber > head_)
    {
        OperationQueueEventSource::Events->UpdateCompleteFront(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            this->head_,
            sequenceNumber);

        return false;
    }

    // completedHead_ < sequenceNumber < head_
    ClearCompleted(sequenceNumber);
    // Try to shrink the queue if it can fit all the items,
    // including the completed ones we want to keep
    if (sequenceNumber > tail_)
    {
        Assert::CodingError(
            "Queue {0}: can't change completed head to {1}, because it's greater than the tail",
            ToString(), 
            sequenceNumber);
    }

    FABRIC_SEQUENCE_NUMBER activeItems = tail_ - sequenceNumber;
    Shrink(activeItems, false);
    return true;
}

void OperationQueue::ResetCompleted()
{
    completedMemorySize_ = 0;
    completedOperationCount_ = 0;
    head_ = completedHead_;
}

bool OperationQueue::Complete() 
{
    FABRIC_SEQUENCE_NUMBER lastSeq = (!ignoreCommit_) ? committedHead_ : tail_;
    if (head_ == lastSeq)
    {
        // There are no items to complete
        return false;
    }

    FABRIC_SEQUENCE_NUMBER i = head_;
    FABRIC_SEQUENCE_NUMBER oldHead = head_;
    for (; i < lastSeq; ++i)
    {
        size_t pos = GetPosition(i);
        if (!queue_[pos])
        {
            // Stop at the first out of order item
            OperationQueueEventSource::Events->Gap(
                this->partitionId_,
                this->description_,
                this->completedHead_,
                this->head_,
                this->committedHead_,
                this->tail_,
                i,
                L"complete");

            break;
        }
        else if (requireServiceAck_ && queue_[pos]->NeedsAck)
        {
            // When sending ACKs after service ACKs operations, 
            // stop at the first item that needs to wait for ACK
            OperationQueueEventSource::Events->CompleteAckGap(
                this->partitionId_,
                this->description_,
                this->completedHead_,
                this->head_,
                this->committedHead_,
                this->tail_,
                i);

            break;
        }

        CompleteItem(pos);
    }

    if (oldHead < head_)
    {
        OperationQueueEventSource::Events->Complete(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            oldHead,
            i - 1);

        CheckQueueInvariants();

        if (cleanOnComplete_)
        {
            // The items have been cleaned up, so we may be able to shrink
            Shrink(tail_ - completedHead_, false /*clearCompleted*/);
        }

        return true;
    }
    
    return false;
}

bool OperationQueue::Complete(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    ASSERT_IF(
        sequenceNumber >= tail_,
        "Queue {0}: Can't complete up to sequence number {1} because it's greater than the tail_ {2}", 
        ToString(),
        sequenceNumber,
        tail_);

    if (head_ == tail_ || sequenceNumber <= Constants::InvalidLSN)
    {
        // No pending items or the sequence is invalid, nothing to complete
        return false;
    }

    if (head_ > sequenceNumber)
    {
        OperationQueueEventSource::Events->Skip(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            L"complete",
            this->head_,
            sequenceNumber);

        return false;
    }

    FABRIC_SEQUENCE_NUMBER i = head_;
    FABRIC_SEQUENCE_NUMBER oldHead = head_;
    FABRIC_SEQUENCE_NUMBER lastSeq = sequenceNumber;

    if (!ignoreCommit_ && lastSeq >= committedHead_)
    {
        lastSeq = committedHead_ - 1;

        OperationQueueEventSource::Events->CompleteLess(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            sequenceNumber,
            i,
            lastSeq);
    }

    for (; i <= lastSeq; ++i)
    {
        size_t pos = GetPosition(i);
        // All items until sequenceNumber should be in order 
        if (!queue_[pos] || (requireServiceAck_ && queue_[pos]->NeedsAck))
        {
            Assert::CodingError(
                "Queue {0}: Operation {1} doesn't exist or needs Ack",
                ToString(),
                i);
        }

        CompleteItem(pos);
    }

    if (oldHead < head_)
    {
        OperationQueueEventSource::Events->Complete(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            oldHead,
            i - 1);

        CheckQueueInvariants();

        if (cleanOnComplete_)
        {
            // The items have been cleaned up, so we may be able to shrink
            Shrink(tail_ - completedHead_, false /*clearCompleted*/);
        }

        return true;
    }
    
    return false;
}

bool OperationQueue::UpdateLastCompletedHead(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (sequenceNumber < Constants::InvalidLSN)
    {
        // The sequence number is negative, nothing to do
        return false;
    }

    FABRIC_SEQUENCE_NUMBER newHead = sequenceNumber + 1;
    if (head_ == newHead)
    {
        // No change, nothing to do
        return true;
    }
    else if (head_ > newHead)
    {
        // Move the head back as much as possible,
        // until we reached the first available completed operation.
        if (completedHead_ == head_)
        {
            // Since no completed items are kept in the queue, 
            // the head can't move back.
            return false;
        }

        if (newHead < completedHead_)
        {
            newHead = completedHead_;
        }

        OperationQueueEventSource::Events->ResetHead(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            L"last completed",
            this->head_,
            newHead);

        while (head_ > newHead)
        {
            completedMemorySize_ -= queue_[GetPosition(--head_)]->DataSize;
            completedOperationCount_ -= 1;
        }
        CheckQueueInvariants();
        return true;
    }
    else
    {
        // Complete more items and move the head forward
        return Complete(sequenceNumber);
    }
}

// Caller should have checked that pos
// contains a valid item
void OperationQueue::CompleteItem(size_t pos)
{
    if (ignoreCommit_)
    {
        ++committedHead_;
        auto elapsedCommit = queue_[pos]->Commit();
        if (commitCallback_)
        {
            // The callback must execute fast or
            // the user should schedule it on a different thread.
            commitCallback_(queue_[pos]);
        }
        
        // Update Average Commit Time
        UpdatePerfCounter(AverageCommitTime, elapsedCommit);
    }
    
    auto elapsedComplete = queue_[pos]->Complete();
    ++head_;

    // Update Average Complete Time
    UpdatePerfCounter(AverageCompleteTime, elapsedComplete);

    if (cleanOnComplete_)
    {
        operationCount_ -= 1;
        totalMemorySize_ -= queue_[pos]->DataSize;
        ++completedHead_;
        auto elapsedCleanup = queue_[pos]->Cleanup();
        
        UpdatePerfCounter(AverageCleanupTime, elapsedCleanup);
        ComOperationCPtr cleanup(move(queue_[pos]));
    }
    else
    {
        completedMemorySize_ += queue_[pos]->DataSize;
        completedOperationCount_ += 1;

        // If the number of completed operations has exceeded the limit, trim the queue
        if (ShouldTrimCompletedOperations())
        {
            TrimCompletedOperations();
        }
    }
}

bool OperationQueue::ShouldTrimCompletedOperations()
{
    if (maxCompletedOperationsMemorySize_ != 0 && completedMemorySize_ > maxCompletedOperationsMemorySize_ ||
        maxCompletedOperationsSize_ != 0 && completedOperationCount_ > maxCompletedOperationsSize_)
    {
        return true;
    }

    return false;
}

void OperationQueue::TrimCompletedOperations()
{
    ASSERT_IF(
        completedMemorySize_ <= maxCompletedOperationsMemorySize_ && completedOperationCount_ <= maxCompletedOperationsSize_, 
        "Queue {0}: TrimCompletedOperations:- Caller should ensure there is atleast 1 item to trim", ToString());

    // start from the 'completedHead_' and cleanup operations until the memory or size restriction is satisfied
    do
    {
        size_t pos = GetPosition(completedHead_);

        ASSERT_IFNOT(queue_[pos], "Queue {0}: TrimCompletedOperations:- Items are Completed in order. Position cannot be null", ToString());
        
        operationCount_ -= 1;
        completedOperationCount_ -= 1;

        completedMemorySize_ -= queue_[pos]->DataSize;
        totalMemorySize_ -= queue_[pos]->DataSize;

        ++completedHead_;
        auto elapsedCleanup = queue_[pos]->Cleanup();
        
        UpdatePerfCounter(AverageCleanupTime, elapsedCleanup);
        ComOperationCPtr cleanup(move(queue_[pos]));
    } while (ShouldTrimCompletedOperations());

    // Removal of items might have resulted in less number of operations. Attempt to shrink the queue here
    Shrink(tail_ - completedHead_, false /*clearCompleted*/);

    CheckQueueInvariants();
}

bool OperationQueue::Commit()
{
    if (committedHead_ == tail_)
    {
        // There are no uncommitted items
        OperationQueueEventSource::Events->Skip(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            L"commit",
            this->committedHead_,
            this->tail_);

        return false;
    }

    FABRIC_SEQUENCE_NUMBER oldCommit = committedHead_;
    FABRIC_SEQUENCE_NUMBER i = committedHead_;

    for (; i < tail_; ++i)
    {
        size_t pos = GetPosition(i);
        if (!queue_[pos])
        {
            // Stop at the first out of order item
            OperationQueueEventSource::Events->Gap(
                this->partitionId_,
                this->description_,
                this->completedHead_,
                this->head_,
                this->committedHead_,
                this->tail_,
                i,
                L"commit");

            break;
        }
        
        CommitItem(pos);
    }

    if (oldCommit < committedHead_)
    {
        OperationQueueEventSource::Events->Commit(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            oldCommit,
            i - 1);

        CheckQueueInvariants();

        return true;
    }

    return false;
}

bool OperationQueue::Commit(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (sequenceNumber <= Constants::InvalidLSN)
    {
        // The sequence is invalid
        return false;
    }
    
    if (sequenceNumber < head_ || sequenceNumber >= tail_)
    {
        Assert::CodingError(
            "Queue {0}: Can't commit upto {1} because it doesn't respect head {2} <= LSN < tail {3}", 
            ToString(), 
            sequenceNumber, 
            head_,
            tail_);
    }

    if (committedHead_ == tail_)
    {
        // There are no pending items to commit
        return false;
    }

    if (sequenceNumber < committedHead_)
    {
        OperationQueueEventSource::Events->Skip(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            L"commit",
            sequenceNumber,
            this->committedHead_);

        return false;
    }

    FABRIC_SEQUENCE_NUMBER oldCommit = committedHead_;
    FABRIC_SEQUENCE_NUMBER i = committedHead_;
    for (; i <= sequenceNumber; ++i)
    {
        size_t pos = GetPosition(i);
        // All items until sequenceNumber should be in order 
        if (!queue_[pos])
        {
            Assert::CodingError("Queue {0}: Commit upto {1}: {2} doesn't exist", ToString(), sequenceNumber, i);
        }

        CommitItem(pos);
    }

    if (oldCommit < committedHead_)
    {
        OperationQueueEventSource::Events->Commit(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            oldCommit,
            i - 1);

        CheckQueueInvariants();

        return true;
    }

    return false;
}

bool OperationQueue::UpdateCommitHead(FABRIC_SEQUENCE_NUMBER sequenceNumber)
{
    if (sequenceNumber < Constants::InvalidLSN)
    {
        // The queue is empty or the sequence is negative, nothing to commit
        return false;
    }

    FABRIC_SEQUENCE_NUMBER newHead = sequenceNumber + 1;
    if (committedHead_ == newHead)
    {
        // No change, nothing to do
        return true;
    }
    else if (committedHead_ > newHead)
    {
        // Move the head back, because the new desired head is smaller than the current one. 
        if (newHead < head_)
        {
            // If new head is smaller than head_, we ignore this call
            OperationQueueEventSource::Events->UpdateCommitBack(
                this->partitionId_,
                this->description_,
                this->completedHead_,
                this->head_,
                this->committedHead_,
                this->tail_,
                L"commit",
                newHead);

            return false;
        }

        OperationQueueEventSource::Events->ResetHead(
            this->partitionId_,
            this->description_,
            this->completedHead_,
            this->head_,
            this->committedHead_,
            this->tail_,
            L"commit",
            committedHead_,
            newHead);

        committedHead_ = newHead;

        CheckQueueInvariants();

        return true;
    }
    else
    {
        // Commit more items and move the committed head forward
        return Commit(sequenceNumber);
    }
}

// Caller should have checked that pos
// contains a valid item
void OperationQueue::CommitItem(size_t pos)
{
    auto elapsedCommit = queue_[pos]->Commit();
    if (commitCallback_)
    {
        // Commit callback must execute fast or
        // the user should schedule it on a different thread.
        commitCallback_(queue_[pos]); 
    }
    committedHead_++;

    // Bookkeeping for perf counters
    UpdatePerfCounter(AverageCommitTime, elapsedCommit);
}

void OperationQueue::Reset(FABRIC_SEQUENCE_NUMBER startSequence)
{
    if (startSequence <= Constants::InvalidLSN)
    {
        Assert::CodingError("{0}: Reset: startSequence {1} must be strictly positive", ToString(), startSequence);
    }

    // Clean all the existing items
    queue_.clear();
    
    // Set the indexes to the new value
    head_ = startSequence; 
    tail_ = startSequence;
    committedHead_ = startSequence;
    completedHead_ = startSequence;
    totalMemorySize_ = 0;
    operationCount_ = 0;
    completedMemorySize_ = 0;
    completedOperationCount_ = 0;

    capacity_ = initialSize_;
    expandedLast_ = false;
    capacitySum_ = capacity_;
    convergentCapacity_ = 0;
    capacityChangeCount_ = 1;
    mask_ = capacity_ - 1;

    queue_.resize(SafeConvert<size_t>(capacity_));
}

void OperationQueue::DiscardNonCommittedOperations()
{
    DiscardPendingOperations(committedHead_);
    Shrink(tail_ - completedHead_, false /*clearCompletedItems*/);
}

void OperationQueue::DiscardNonCompletedOperations()
{
    DiscardPendingOperations(head_);
    Shrink(tail_ - completedHead_, false /*clearCompletedItems*/);
}

void OperationQueue::DiscardPendingOperations(
    FABRIC_SEQUENCE_NUMBER fromSequenceNumber)
{
    if (fromSequenceNumber == tail_)
    {
        // No operations are pending
        return;
    }

    ASSERT_IF(
        fromSequenceNumber < head_,
        "{0}: Can't discard from {1}, because it includes completed items",
        ToString(),
        fromSequenceNumber);

    OperationQueueEventSource::Events->DiscardPendingOps(
        this->partitionId_,
        this->description_,
        this->completedHead_,
        this->head_,
        this->committedHead_,
        this->tail_,
        fromSequenceNumber,
        this->tail_ - 1);

    // Discard all active (non-completed) operations
    TimeSpan elapsedTime;
    for (FABRIC_SEQUENCE_NUMBER i = fromSequenceNumber; i < tail_; ++i)
    {
        RemoveItem(i, elapsedTime); // Ignore the enqueue time as this operation is being discarded
    }

    tail_ = fromSequenceNumber;

    if (committedHead_ > fromSequenceNumber)
    {
        committedHead_ = fromSequenceNumber;
    }

    CheckQueueInvariants();
}
        
void OperationQueue::RemoveItem(
    FABRIC_SEQUENCE_NUMBER sequenceNumber, 
    __out TimeSpan & elapsedTime)
{
    size_t pos = GetPosition(sequenceNumber);

    if (queue_[pos])
    {
        ULONGLONG dataSize = queue_[pos]->DataSize; 
        operationCount_ -= 1;
        totalMemorySize_ -= dataSize;
        if (sequenceNumber < head_)
        {
            completedMemorySize_ -= dataSize;
            completedOperationCount_ -= 1;
        }

        elapsedTime = queue_[pos]->Cleanup();
        ComOperationCPtr cleanup(move(queue_[pos]));
    }
}

void OperationQueue::UpdatePerfCounter(
    PerfCounterName counterName,
    TimeSpan const & elapsedTime) const
{
    ASSERT_IF(counterName == PerfCounterName::Invalid, "Queue {0}: Invalid call to UpdatePerfCounter", ToString());
    
    if (perfCounters_)
    {
        switch (counterName)
        {
            case PerfCounterName::AverageCommitTime:
                perfCounters_->AverageCommitTimeBase.Increment();
                perfCounters_->AverageCommitTime.IncrementBy(elapsedTime.TotalMilliseconds());
                break;
            case PerfCounterName::AverageCompleteTime:
                perfCounters_->AverageCompleteTimeBase.Increment();
                perfCounters_->AverageCompleteTime.IncrementBy(elapsedTime.TotalMilliseconds());
                break;
            case PerfCounterName::AverageCleanupTime:
                perfCounters_->AverageCleanupTimeBase.Increment();
                perfCounters_->AverageCleanupTime.IncrementBy(elapsedTime.TotalMilliseconds());
                break;
            default:
                break;
        }
    }
}

void OperationQueue::UpdateRatePerfCounters(
    ULONGLONG dataSize) const
{
    if(perfCounters_)
    {
        perfCounters_->EnqueuedOpsPerSecond.Increment();
        perfCounters_->EnqueuedBytesPerSecond.IncrementBy(dataSize);
    }
}

// Update the capacity and keep track of the average values.
void OperationQueue::UpdateCapacity(FABRIC_SEQUENCE_NUMBER newCapacity)
{
    ASSERT_IF(newCapacity <= 1, "Queue {0}: Capacity must be greater than 1", ToString());
    ASSERT_IF(newCapacity == SafeConvert<FABRIC_SEQUENCE_NUMBER>(capacity_), "Queue {0}: Shouldn't update the capacity to the same value", ToString());
    // Resize the queue and move all items to map to the new capacity
    vector<ComOperationCPtr> newQueue;
    newQueue.resize(SafeConvert<size_t>(newCapacity));
    FABRIC_SEQUENCE_NUMBER newMask = newCapacity - 1;
    for (FABRIC_SEQUENCE_NUMBER i = completedHead_; i < tail_; ++i)
    {
        swap(newQueue[GetPosition(i, newMask)], queue_[GetPosition(i)]);
    }

    swap(queue_, newQueue);
    capacity_ = newCapacity;
    mask_ = capacity_ - 1;
    ++capacityChangeCount_;
    capacitySum_ += newCapacity;
    // Invalidate the convergent capacity to force recomputation
    // based on the new values.
    // Lazy compute - do not update to the correct value until it's needed.
    convergentCapacity_ = 0;
}

void OperationQueue::CheckQueueInvariants()
{
    if (completedHead_ > head_ || head_ > committedHead_ || committedHead_ > tail_)
    {
        Assert::CodingError("{0}: Queue doesn't respect invariants", ToString());
    }

    if (cleanOnComplete_ && completedHead_ != head_)
    {
        Assert::CodingError("Queue {0}: CleanOnComplete is active, but queue has completed items", ToString());
    }

    // If the maxSize_ has been configured for the queue, the number of operations in the queue must never go beyond it
    ASSERT_IF(maxSize_ && operationCount_ > maxSize_, "operation count does not respect invariants");
    ASSERT_IFNOT(totalMemorySize_ >= completedMemorySize_, "operations memory does not respect invariants");
    ASSERT_IFNOT(operationCount_ >= completedOperationCount_, "operations count does not respect invariants");
    ASSERT_IF(completedHead_ == head_ && completedMemorySize_ > 0, "completedMemorySize_ should be 0");
    ASSERT_IF(maxCompletedOperationsMemorySize_ !=0 && completedMemorySize_ > maxCompletedOperationsMemorySize_, "completedMemorySize_ should be less than max completed memory size");
    ASSERT_IF(maxCompletedOperationsSize_ !=0 && completedOperationCount_ > maxCompletedOperationsSize_, "completedOperationCount should be less than max completed operations size");
    ASSERT_IF(totalMemorySize_ != 0 && completedMemorySize_ > totalMemorySize_, "completedMemorySize_ should be less than total memory size");
    ASSERT_IF(maxSize_ !=0 && completedOperationCount_ > maxSize_, "completedOperationCount should be less than total operation count");
}

wstring OperationQueue::ToString() const
{
    std::wstring content;
    Common::StringWriter writer(content);
    WriteTo(writer, Common::FormatOptions(0, false, ""));
    return content;
}
        
void OperationQueue::WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const
{
    w << description_ << " [" << completedHead_ << ", " << head_ << ", " << committedHead_ << ", " << tail_ << "] totalMemorySize=" << totalMemorySize_ << "; completedMemorySize=" << completedMemorySize_ << "; completedOperationCount=" << completedOperationCount_ << "; maxMemorySize = " << maxMemorySize_;
}

/*******************************************************
* Helper Methods
*******************************************************/
ULONGLONG OperationQueue::GetCeilingPowerOf2(ULONGLONG n)
{
    if (IsPowerOf2(n))
    {
        return n;
    }

    ULONGLONG x = n - 1;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
    return ++x;
}

bool OperationQueue::IsPowerOf2(ULONGLONG n)
{
    // n is always greater than 1
    return (n & (n - 1)) == 0;
}

} // end namespace ReplicationComponent
} // end namespace Reliability
