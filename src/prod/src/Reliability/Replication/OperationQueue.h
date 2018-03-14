// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Reliability
{
    namespace ReplicationComponent
    {
        typedef enum PerfCounterName
        {
            Invalid,
            AverageCommitTime,
            AverageCompleteTime,
            AverageCleanupTime

        } PerfCounterName;

        // Queue used for ordering operations that arrive out-of-order.
        // Implemented using a cyclic buffer that stores operations based on their
        // sequence number.
        // The queue has indexes to keep track of the sequence number
        // of the last in-order, last out-of-order, first completed and committed operation.
        // The size of the queue is variable, and it can grow between 
        // configured start and max sizes.
        // The queue does not provide any threading protection.  It is
        // the caller's responsibility to do synchronization.
        class OperationQueue
        {
            DENY_COPY(OperationQueue);
        public:
            OperationQueue(
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
                REPerformanceCountersSPtr const & perfCounters);

            OperationQueue(
                std::wstring const & newDescription,
                OperationQueue && other,
                ULONGLONG initialSize,
                ULONGLONG maxSize,
                ULONGLONG maxMemorySize,
                ULONGLONG maxCompletedOperationsCount,
                ULONGLONG maxCompletedOperationsMemorySize,
                bool cleanOnComplete,
                REPerformanceCountersSPtr const & perfCounters);
            
            ~OperationQueue();

            // The sequence number of the last operation placed in the queue
            __declspec (property(get=get_LastSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastSequenceNumber() const { return tail_ - 1; }

            // The sequence number of the next operation that is to be completed
            __declspec (property(get=get_NextToBeCompletedSequenceNumber)) FABRIC_SEQUENCE_NUMBER NextToBeCompletedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_NextToBeCompletedSequenceNumber() const { return head_; }

            // The sequence number of the last completed operation
            __declspec (property(get=get_LastCompletedSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastCompletedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastCompletedSequenceNumber() const { return head_ - 1; }

            __declspec (property(get=get_LastCommittedSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastCommittedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastCommittedSequenceNumber() const { return committedHead_ - 1; }
            
            // Return the first available completed sequence number;
            // this operation was completed but kept in the list.
            // If completed operations are not kept, returns 0.
            // If all completed operations were removed, completedHead_ = head_
            __declspec (property(get=get_FirstAvailableCompletedSequenceNumber)) FABRIC_SEQUENCE_NUMBER FirstAvailableCompletedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_FirstAvailableCompletedSequenceNumber() const { return (completedHead_ == head_) ? Constants::InvalidLSN : completedHead_; }

            __declspec (property(get=get_FirstOperationInReplicationQueueEnqueuedSince)) Common::TimeSpan FirstOperationInReplicationQueueEnqueuedSince;
            Common::TimeSpan get_FirstOperationInReplicationQueueEnqueuedSince() const;

            __declspec (property(get=get_FirstCommittedSequenceNumber)) FABRIC_SEQUENCE_NUMBER FirstCommittedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_FirstCommittedSequenceNumber() const { return (head_ == committedHead_) ? Constants::InvalidLSN : head_; }

            __declspec (property(get=get_LastRemovedSequenceNumber)) FABRIC_SEQUENCE_NUMBER LastRemovedSequenceNumber;
            FABRIC_SEQUENCE_NUMBER get_LastRemovedSequenceNumber() const { return completedHead_ - 1; }

            __declspec (property(get=get_Capacity))ULONGLONG Capacity;
            ULONGLONG get_Capacity() const { return capacity_; }

            __declspec (property(get=get_CapacityChangeCount)) ULONGLONG CapacityChangeCount;
            ULONGLONG get_CapacityChangeCount() const { return capacityChangeCount_; }

            __declspec (property(get=get_TotalMemorySize)) ULONGLONG TotalMemorySize;
            ULONGLONG get_TotalMemorySize() const { return totalMemorySize_; }

            __declspec (property(get=get_CompletedMemorySize)) ULONGLONG CompletedMemorySize;
            ULONGLONG get_CompletedMemorySize() const { return completedMemorySize_; }

            __declspec (property(get=get_MaxSize)) ULONGLONG MaxSize;
            ULONGLONG get_MaxSize() const { return maxSize_; }

            __declspec (property(get=get_MaxMemorySize)) ULONGLONG MaxMemorySize;
            ULONGLONG get_MaxMemorySize() const { return maxMemorySize_; }

            __declspec (property(get=get_ConvergentCapacity)) ULONGLONG ConvergentCapacity;
            ULONGLONG get_ConvergentCapacity() const;

            __declspec (property(get=get_Description, put=set_Description)) std::wstring const & Description;
            std::wstring const & get_Description() const { return description_; } 
            void set_Description(std::wstring const & value) { description_ = value; }

            __declspec (property(get=get_CleanOnComplete, put=set_CleanOnComplete)) bool CleanOnComplete;
            bool get_CleanOnComplete() const { return cleanOnComplete_; }
            void set_CleanOnComplete(bool value) { cleanOnComplete_ = value; }

            __declspec (property(get=get_Mask)) ULONGLONG const & Mask;
            ULONGLONG const & get_Mask() const { return mask_; }

            __declspec (property(get=get_IgnoreCommit, put=set_IgnoreCommit)) bool IgnoreCommit;
            bool get_IgnoreCommit() const { return ignoreCommit_; }
            void set_IgnoreCommit(bool value);

            __declspec (property(get=get_OperationCount)) ULONGLONG OperationCount ;
            size_t get_OperationCount() const { return operationCount_; }

            void SetCommitCallback(OperationCallback const & callback);

            // Returns true if there are any non-completed operations
            bool HasPendingOperations() const { return head_ != tail_; }
            
            // Returns true if there are any committed operations
            // not yet completed;
            // ignores any operations not committed yet.
            bool HasCommittedOperations() const { return head_ != committedHead_; }

            Common::ErrorCode TryEnqueue(ComOperationCPtr const & operation);

            // Complete all operations up to a provided sequence number
            bool Complete(FABRIC_SEQUENCE_NUMBER sequenceNumber);

            // Complete all in-order pending operations in the queue
            bool Complete();

            // Update the last completed head to the new value. 
            // The new value can be either less than or greater than the old one.
            // If less, it can't move below the first available completed value.
            bool UpdateLastCompletedHead(FABRIC_SEQUENCE_NUMBER sequenceNumber);

            // Commit all operations up to a provided sequence number
            bool Commit(FABRIC_SEQUENCE_NUMBER sequenceNumber);

            // Commit all in-order pending operations in the queue,
            // until the first gap or last enqueued is reached 
            bool Commit();

            // Update the commit head to the new value. 
            // The new value can be either less than or greater than the old one.
            bool UpdateCommitHead(FABRIC_SEQUENCE_NUMBER sequenceNumber);

            // If needed, shrink the queue capacity
            // but keep completed items up to the provided value (exclusive).
            bool UpdateCompleteHead(FABRIC_SEQUENCE_NUMBER sequenceNumber);

            void ResetCompleted();

            // Discards all operations except the completed ones
            void DiscardNonCompletedOperations();

            // Discard all operations between committed head and tail
            void DiscardNonCommittedOperations();

            // Clears everything in the current queue
            // and sets the starting sequence to the specified value.
            void Reset(FABRIC_SEQUENCE_NUMBER startSequence);
                        
            // Get in-order operations between start and tail
            // Returns false if any operation is missing
            bool GetOperations(
                FABRIC_SEQUENCE_NUMBER start,
                __out ComOperationRawPtrVector & operations) const;

            ComOperationRawPtr GetOperation(
                FABRIC_SEQUENCE_NUMBER sequenceNumber) const;

            std::wstring ToString() const;
            
            void WriteTo(__in Common::TextWriter & w, Common::FormatOptions const &) const;
        
            static ULONGLONG GetCeilingPowerOf2(ULONGLONG n);

        private:

            static bool IsPowerOf2(ULONGLONG n);

            template <typename T>
            static T SafeConvert(FABRIC_SEQUENCE_NUMBER number)
            {
                T converted = static_cast<T>(number);
                ASSERT_IF(static_cast<FABRIC_SEQUENCE_NUMBER>(converted) != number, 
                    "SafeConvert: Conversion lost precision ({0} -> {1})", number, converted);
                return converted;
            }

            // Returns the position in the vector
            static size_t GetPosition(FABRIC_SEQUENCE_NUMBER sequenceNumber, FABRIC_SEQUENCE_NUMBER mask);
            inline size_t GetPosition(FABRIC_SEQUENCE_NUMBER sequenceNumber) const
            {
                return GetPosition(sequenceNumber, mask_);
            }

            void CommitItem(size_t pos);
            void CompleteItem(size_t pos);

            // Changes the capacity and moves the elements in the vector
            // to match the new capacity.
            void UpdateCapacity(FABRIC_SEQUENCE_NUMBER newCapacity);

            // Remove previously completed items.
            void ClearCompleted(FABRIC_SEQUENCE_NUMBER sequenceNumber);
            void ClearCompleted()
            {
                return ClearCompleted(head_);
            }

            void TrimCompletedOperations();

            bool ShouldTrimCompletedOperations();

            void DiscardPendingOperations(FABRIC_SEQUENCE_NUMBER fromSequenceNumber);
            
            // Remove the item with specified sequence number;
            // the item must be completed if force is false.
            void RemoveItem(FABRIC_SEQUENCE_NUMBER sequenceNumber, __out Common::TimeSpan & elapsedTime);

            // Increase the capacity of the queue 
            // so the specified number of items can fit
            bool Expand(ULONGLONG activeItemsCount);

            // Shrink the capacity of the queue 
            // as long as the specified number or items can fit.
            bool Shrink(ULONGLONG activeItemsCount, bool clearCompleted = true);

            Common::ErrorCode MakeRoomForOperations(
                FABRIC_SEQUENCE_NUMBER lastSequenceNumber,
                ULONGLONG totalDataSize);

            void CheckQueueInvariants();

            void UpdatePerfCounter(
                PerfCounterName counterName,
                Common::TimeSpan const & elapsedTime) const;

            void UpdateRatePerfCounters(
                ULONGLONG dataSize) const;

            ULONGLONG initialSize_;
            ULONGLONG maxSize_;
            ULONGLONG maxMemorySize_;

            // The queue that holds the operations
            std::vector<ComOperationCPtr> queue_;
            
            // ID used for tracing
            Common::Guid const partitionId_;

            std::wstring description_;
            
            bool requireServiceAck_;

            OperationCallback commitCallback_;
            
            // Remove the items from the queue as soon as they are completed
            bool cleanOnComplete_;
            // Ignore the commit state, allow complete of the items
            // even if they are not committed
            bool ignoreCommit_;
            
            // The capcity of the queue, which must be a power of 2
            // between start and max capacity.
            ULONGLONG capacity_;
        
            // Cached sequence numbers to keep track of the completed, committed 
            // and first non-completed/last in-order sequence number.
            // The relationship between these sequence numbers is:
            // completedHead_ <= head_ <= committedHead_ <= tail
            //-----------|---------------------|---------------------------|-------------------|--------
            //       completedHead            head                   committedHead            tail
            //-----------|--completed items----|--non-completed,committed--|---non-committed---|-------

            // The LSN of last in-order operation received + 1.
            FABRIC_SEQUENCE_NUMBER head_;

            // The LSN of the first completed operation + 1
            FABRIC_SEQUENCE_NUMBER completedHead_;

            // The LSN of the first committed operation + 1
            FABRIC_SEQUENCE_NUMBER committedHead_;
        
            // The LSN of the most advanced operation received
            // in the ordering queue + 1
            FABRIC_SEQUENCE_NUMBER tail_;

            // Whether the capacity was changed last at expand or shrink.
            bool expandedLast_;

            // The sum of all capacities tried so far.
            // The number is not necessarily a power of 2;
            // we will compute the closest power of 2 when we want to change the capacity.
            FABRIC_SEQUENCE_NUMBER capacitySum_;
            // Convergent capacity used to determine when and to what size to shrink the array;
            // It's computed as the the ceiling power of 2 from the average capacity.
            mutable ULONGLONG convergentCapacity_;
            ULONGLONG capacityChangeCount_;
            ULONGLONG mask_;
            
            // Number of user operations that are currently stored in the queue (Includes completed,committed and pending)
            ULONGLONG operationCount_; 

            ULONGLONG totalMemorySize_;
            ULONGLONG completedMemorySize_;
            ULONGLONG completedOperationCount_;

            // Maximum number of completed user operations that can exist in the queue.
            ULONGLONG maxCompletedOperationsSize_;

            // Maximum memory consumption of completed user operations that can exist in the queue.
            ULONGLONG maxCompletedOperationsMemorySize_;

            REPerformanceCountersSPtr perfCounters_;
        }; // end class OperationQueue

    } // end namespace ReplicationComponent
} // end namespace Reliability
