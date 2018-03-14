// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "ComTestOperation.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
 
namespace ReplicationUnitTest
{
    using namespace Common;
    using namespace std;
    using namespace Reliability;
    using namespace Reliability::ReplicationComponent;

    static Common::StringLiteral const OperationQueueSource("OperationQueueTest");

    typedef std::shared_ptr<REConfig> REConfigSPtr;
    class OperationQueueWrapper
    {
    public:
        // MaxSecondaryReplication queue sizes are used as the actual size limits of the queue
        // MaxPrimaryReplication queue sizes are used as the completed size limits of the queue
        OperationQueueWrapper(REConfigSPtr const & config, FABRIC_SEQUENCE_NUMBER startSeq, bool cleanOnComplete, bool ignoreCommit = false) 
            :
            queue_(PartitionId, L"OperationQueueTest", config->InitialSecondaryReplicationQueueSize, config->MaxSecondaryReplicationQueueSize, config->MaxSecondaryReplicationQueueMemorySize, config->MaxPrimaryReplicationQueueSize, config->MaxPrimaryReplicationQueueMemorySize, false, cleanOnComplete, ignoreCommit, startSeq, nullptr),
            config_(config),
            cleanOnComplete_(cleanOnComplete),
            ignoreCommit_(ignoreCommit),
            firstAvailableCompletedSN_(startSeq - 1),
            nextToBeCompletedSN_(startSeq),
            lastSN_(startSeq - 1),
            lastCommittedSN_(startSeq - 1),
            dataMap_(),
            closed_(false),
            numberofCompletedOperations_(0)
        {
        }
        
        ~OperationQueueWrapper() { closed_ = true; }

        __declspec(property(get = get_Queue)) OperationQueue const & InnerQueue;
        OperationQueue const & get_Queue() const { return queue_; }

        void Enqueue(vector<FABRIC_SEQUENCE_NUMBER> const & seqNumbers, FABRIC_SEQUENCE_NUMBER lastSN, bool capacityChange, FABRIC_SEQUENCE_NUMBER cleanedCompletedItems, ULONGLONG totalMemorySize=(ULONGLONG)-1, ULONGLONG completedMemorySize=(ULONGLONG)-1, FABRIC_SEQUENCE_NUMBER numberOfDroppedOperation = 0)
        {
            Enqueue(seqNumbers, lastSN, 100, capacityChange, cleanedCompletedItems, totalMemorySize, completedMemorySize, numberOfDroppedOperation);
        }
        void Enqueue(vector<FABRIC_SEQUENCE_NUMBER> const & seqNumbers, FABRIC_SEQUENCE_NUMBER lastSN, int opSize, bool capacityChange, FABRIC_SEQUENCE_NUMBER cleanedCompletedItems, ULONGLONG totalMemorySize=(ULONGLONG)-1, ULONGLONG completedMemorySize=(ULONGLONG)-1, FABRIC_SEQUENCE_NUMBER numberOfDroppedOperation = 0)
        {
            Trace.WriteInfo(OperationQueueSource, "************ Enqueue {0} operations out-of-order", seqNumbers.size());
            FABRIC_SEQUENCE_NUMBER initialCapacityChangeCount = queue_.CapacityChangeCount;
            size_t initialMapSize = dataMap_.size();

            size_t itemsEnqueued = 0;
            for (size_t i = 0; i < seqNumbers.size(); ++i)
            {
                CreateOperationAndEnqueue(seqNumbers[i], opSize, itemsEnqueued);
            }

            lastSN_ -= numberOfDroppedOperation;
            CheckEnqueue(lastSN, capacityChange, initialCapacityChangeCount, static_cast<size_t>(cleanedCompletedItems), itemsEnqueued, initialMapSize - static_cast<size_t>(numberOfDroppedOperation));
            if ((ULONGLONG)-1 != totalMemorySize) VERIFY_ARE_EQUAL(queue_.TotalMemorySize, totalMemorySize);
            if ((ULONGLONG)-1 != completedMemorySize) VERIFY_ARE_EQUAL(queue_.CompletedMemorySize, completedMemorySize);
        }

        void Enqueue(FABRIC_SEQUENCE_NUMBER numberOfOperations, FABRIC_SEQUENCE_NUMBER lastSN, bool capacityChange, FABRIC_SEQUENCE_NUMBER cleanedCompletedItems, ULONGLONG totalMemorySize=(ULONGLONG)-1, ULONGLONG completedMemorySize=(ULONGLONG)-1)
        {
            Enqueue(numberOfOperations, lastSN, 100, capacityChange, cleanedCompletedItems, totalMemorySize, completedMemorySize);
        }
        void Enqueue(FABRIC_SEQUENCE_NUMBER numberOfOperations, FABRIC_SEQUENCE_NUMBER lastSN, int opSize, bool capacityChange, FABRIC_SEQUENCE_NUMBER cleanedCompletedItems, ULONGLONG totalMemorySize=(ULONGLONG)-1, ULONGLONG completedMemorySize=(ULONGLONG)-1)
        {
            Trace.WriteInfo(OperationQueueSource, "************ Enqueue {0} operations in-order", numberOfOperations);

            FABRIC_SEQUENCE_NUMBER endSeq = lastSN_ + 1+ numberOfOperations;
            FABRIC_SEQUENCE_NUMBER initialCapacityChangeCount = queue_.CapacityChangeCount;
            size_t initialMapSize = dataMap_.size();
            size_t itemsEnqueued = 0;
            
            for (FABRIC_SEQUENCE_NUMBER i = lastSN_ + 1; i < endSeq; ++i)
            {
                CreateOperationAndEnqueue(i, opSize, itemsEnqueued);
            }
            
            CheckEnqueue(lastSN, capacityChange, initialCapacityChangeCount, static_cast<size_t>(cleanedCompletedItems), itemsEnqueued, initialMapSize);
            if ((ULONGLONG)-1 != totalMemorySize) VERIFY_ARE_EQUAL(queue_.TotalMemorySize, totalMemorySize);
            if ((ULONGLONG)-1 != completedMemorySize) VERIFY_ARE_EQUAL(queue_.CompletedMemorySize, completedMemorySize);
        }
                
        void Complete(FABRIC_SEQUENCE_NUMBER numberOfOperations, FABRIC_SEQUENCE_NUMBER nextToBeCompletedSN, bool capacityChange, ULONGLONG totalMemorySize=(ULONGLONG)-1, ULONGLONG completedMemorySize=(ULONGLONG)-1)
        {
            Trace.WriteInfo(OperationQueueSource, "************ Complete {0} operations", numberOfOperations);
            FABRIC_SEQUENCE_NUMBER initialCapacityChangeCount = queue_.CapacityChangeCount;
            size_t expectedSize = dataMap_.size() - static_cast<size_t>(numberOfOperations);

            if (queue_.Complete(nextToBeCompletedSN_ - 1 + numberOfOperations))
            {
                nextToBeCompletedSN_ += numberOfOperations;
                if (cleanOnComplete_)
                {
                    firstAvailableCompletedSN_ += numberOfOperations;
                    CheckDataMapSize(expectedSize);
                }
                else
                {
                    numberofCompletedOperations_ += numberOfOperations;

                    if (config_->MaxPrimaryReplicationQueueSize != 0 && numberofCompletedOperations_ > config_->MaxPrimaryReplicationQueueSize)
                    {
                        firstAvailableCompletedSN_ += (numberofCompletedOperations_ - config_->MaxPrimaryReplicationQueueSize);
                        numberofCompletedOperations_ = config_->MaxPrimaryReplicationQueueSize;
                    }
                }

                if (ignoreCommit_)
                {
                    lastCommittedSN_ += numberOfOperations;
                }
            }
            
            CheckQueue();
            VERIFY_ARE_EQUAL(queue_.NextToBeCompletedSequenceNumber, nextToBeCompletedSN);
            if (cleanOnComplete_)
            {
                CheckCapacityChange(capacityChange, initialCapacityChangeCount);
            }
            if ((ULONGLONG)-1 != totalMemorySize) VERIFY_ARE_EQUAL(queue_.TotalMemorySize, totalMemorySize);
            if ((ULONGLONG)-1 != completedMemorySize) VERIFY_ARE_EQUAL(queue_.CompletedMemorySize, completedMemorySize);
        }

        void Commit(FABRIC_SEQUENCE_NUMBER numberOfOperations, FABRIC_SEQUENCE_NUMBER lastCommittedSN)
        {
            Trace.WriteInfo(OperationQueueSource, "************ Commit {0} operations", numberOfOperations);
            if (queue_.Commit(lastCommittedSN_ + numberOfOperations))
            {
                // Operations should be in order
                lastCommittedSN_ += numberOfOperations;
            }
            
            CheckQueue();
            VERIFY_ARE_EQUAL(queue_.LastCommittedSequenceNumber, lastCommittedSN - 1);
        }

        void Commit(FABRIC_SEQUENCE_NUMBER lastCommittedSN)
        {
            FABRIC_SEQUENCE_NUMBER numberOfOperations = queue_.LastSequenceNumber - queue_.LastCommittedSequenceNumber + 1;
            Trace.WriteInfo(OperationQueueSource, "************ Commit all operations ({0})", numberOfOperations);
            if (queue_.Commit())
            {
                // Operations can be out of order, so not all of them can be committed
                lastCommittedSN_ = queue_.LastCommittedSequenceNumber;
            }
            
            CheckQueue();
            VERIFY_ARE_EQUAL(queue_.LastCommittedSequenceNumber, lastCommittedSN - 1);
        }

        void Complete(bool capacityChange, bool expectSuccess = true)
        {
            FABRIC_SEQUENCE_NUMBER numberOfOperations = queue_.LastSequenceNumber - queue_.NextToBeCompletedSequenceNumber + 1;
            FABRIC_SEQUENCE_NUMBER initialCapacityChangeCount = queue_.CapacityChangeCount;
            
            Trace.WriteInfo(OperationQueueSource, "************ Complete all operations ({0})", numberOfOperations);
            size_t expectedSize = dataMap_.size() - static_cast<size_t>(numberOfOperations);
            if (queue_.Complete())
            {
                // Operations can be out of order, so not all of them can be completed
                nextToBeCompletedSN_ = queue_.NextToBeCompletedSequenceNumber;
                if (cleanOnComplete_)
                {
                    firstAvailableCompletedSN_ = nextToBeCompletedSN_ - 1;
                    CheckDataMapSize(expectedSize);
                    CheckCapacityChange(capacityChange, initialCapacityChangeCount);
                }
                else
                {
                    numberofCompletedOperations_ += numberOfOperations;

                    if (config_->MaxPrimaryReplicationQueueSize != 0 && numberofCompletedOperations_ > config_->MaxPrimaryReplicationQueueSize)
                    {
                        firstAvailableCompletedSN_ += (numberofCompletedOperations_ - config_->MaxPrimaryReplicationQueueSize);
                        numberofCompletedOperations_ = config_->MaxPrimaryReplicationQueueSize;
                    }
                }
            }
            else
            {
                VERIFY_IS_FALSE(expectSuccess, L"Complete should fail");
            }
            
            CheckQueue();
        }

        void DiscardPendingOperations(bool capacityChange)
        {
            Trace.WriteInfo(OperationQueueSource, "************ Discard pending operations");
            ULONGLONG initialCapacityChangeCount = queue_.CapacityChangeCount;
            queue_.DiscardNonCompletedOperations();
            lastSN_ = nextToBeCompletedSN_ - 1;
            lastCommittedSN_ = nextToBeCompletedSN_ - 1;
            CheckQueue();
            if (capacityChange)
            {
                VERIFY_ARE_NOT_EQUAL(initialCapacityChangeCount, queue_.CapacityChangeCount);
            }
        }     

        void ClearCompleted()
        {
            Trace.WriteInfo(OperationQueueSource, "************ Clear completed operations");
            if (!cleanOnComplete_)
            {
                queue_.UpdateCompleteHead(nextToBeCompletedSN_);
                firstAvailableCompletedSN_ = nextToBeCompletedSN_ - 1;
                CheckQueue();
            }
        }

    private:
        class DummyOperation : public ComTestOperation
        {
        public:
            DummyOperation(FABRIC_SEQUENCE_NUMBER sequenceNumber, int opSize, OperationQueueWrapper * parent) : 
                ComTestOperation(0, opSize), 
                sequenceNumber_(sequenceNumber), 
                parent_(parent)
            { 
            }

            virtual ~DummyOperation() 
            {
                parent_->RemoveOperation(sequenceNumber_);
            }

        private:
            FABRIC_SEQUENCE_NUMBER sequenceNumber_;
            OperationQueueWrapper * parent_;
        };

        void CheckDataMapSize(size_t expectedSize)
        {
            // Since the cleanup callback is executed async, wait some time if needed
            Trace.WriteInfo(OperationQueueSource, "Check that dataMap size is {0}", expectedSize);
            VERIFY_ARE_EQUAL(dataMap_.size(), expectedSize);
        }

        void RemoveOperation(FABRIC_SEQUENCE_NUMBER sequenceNumber)
        {
            if (closed_)
            {
                return;
            }

            // Find the data and free it
            auto it = std::find(dataMap_.begin(), dataMap_.end(), sequenceNumber);
            ASSERT_IFNOT(dataMap_.end() != it, "The sequence number {0} should be present in the operations map", sequenceNumber);
            dataMap_.erase(it);
        }
        
        void CheckQueue()
        {
            Trace.WriteInfo(OperationQueueSource, "Queue: {0}. Capacity: {1} -> {2}, {3} changes",
                queue_, queue_.Capacity, queue_.ConvergentCapacity, queue_.CapacityChangeCount);

            VERIFY_ARE_EQUAL(queue_.LastRemovedSequenceNumber, firstAvailableCompletedSN_);
            VERIFY_ARE_EQUAL(queue_.NextToBeCompletedSequenceNumber, nextToBeCompletedSN_);
            VERIFY_ARE_EQUAL(queue_.LastSequenceNumber, lastSN_);
            VERIFY_ARE_EQUAL(queue_.LastCommittedSequenceNumber, lastCommittedSN_);
        }

        void CreateOperationAndEnqueue(FABRIC_SEQUENCE_NUMBER sequenceNumber, int opSize, size_t & itemsEnqueued)
        {
            ComPointer<IFabricOperationData> op = make_com<DummyOperation,IFabricOperationData>(sequenceNumber, opSize, this);
            dataMap_.push_back(sequenceNumber);

            FABRIC_OPERATION_METADATA metadata;
            metadata.Type = FABRIC_OPERATION_TYPE_NORMAL;
            metadata.SequenceNumber = sequenceNumber;
            metadata.Reserved = NULL;
            ComPointer<ComOperation> opPointer = make_com<ComUserDataOperation,ComOperation>(
                move(op), 
                metadata);

            ErrorCode error = queue_.TryEnqueue(move(opPointer));
            if (error.IsSuccess())
            {
                ++itemsEnqueued;
                if (sequenceNumber > lastSN_)
                {
                    lastSN_ = sequenceNumber;
                }

                if (sequenceNumber > (FABRIC_SEQUENCE_NUMBER)(queue_.Capacity) + queue_.LastRemovedSequenceNumber + 1)
                {
                    Trace.WriteInfo(OperationQueueSource, "{0}: Enqueue {1} succeeded: {2} + {3} | {4}. Capacity {5}->{6}",
                        queue_, sequenceNumber, config_->MaxSecondaryReplicationQueueSize, 
                        queue_.LastRemovedSequenceNumber, sequenceNumber,
                        queue_.Capacity, queue_.ConvergentCapacity);
                    VERIFY_FAIL(L"Enqueue succeeded when it should have failed.");
                }
            }
            else
            {
                Trace.WriteInfo(OperationQueueSource, "Enqueued failed for operation with LSN {0}", sequenceNumber);
                // Fails only if the max capacity is reached
                if ((sequenceNumber < config_->MaxSecondaryReplicationQueueSize + queue_.LastRemovedSequenceNumber) &&
                    (queue_.TotalMemorySize + opSize < config_->MaxSecondaryReplicationQueueMemorySize))
                {
                    Trace.WriteInfo(OperationQueueSource, "{0}: Enqueue {1} failed: {2} + {3} < {4}. Capacity {5}->{6}",
                        queue_, sequenceNumber, config_->MaxSecondaryReplicationQueueSize, 
                        queue_.LastRemovedSequenceNumber, sequenceNumber,
                        queue_.Capacity, queue_.ConvergentCapacity);
                    VERIFY_FAIL(L"Enqueue failed when it should have succeeded.");
                }
            }
        }

        void CheckEnqueue(FABRIC_SEQUENCE_NUMBER lastSN, bool capacityChange, FABRIC_SEQUENCE_NUMBER initialCapacityChangeCount, size_t cleanedCompletedItems, size_t itemsAdded, size_t initialMapSize)
        {
            Trace.WriteInfo(OperationQueueSource, "+++++++++++++ Enqueued {0} items successfully.", itemsAdded);

            if (!cleanOnComplete_ && cleanedCompletedItems > 0)
            {
                CheckDataMapSize(initialMapSize + itemsAdded - cleanedCompletedItems);
                firstAvailableCompletedSN_ += cleanedCompletedItems;
            }

            CheckQueue();
            VERIFY_ARE_EQUAL(queue_.LastSequenceNumber, lastSN - 1);
            CheckCapacityChange(capacityChange, initialCapacityChangeCount);
        }

        void CheckCapacityChange(bool capacityChange, ULONGLONG initialCapacityChangeCount)
        {
            if (capacityChange)
            {
                // The capacities are different (smaller/bigger if shrink/expand occurred)
                VERIFY_ARE_NOT_EQUAL(initialCapacityChangeCount, queue_.CapacityChangeCount);
            }
            else
            {
                VERIFY_ARE_EQUAL(initialCapacityChangeCount, queue_.CapacityChangeCount);
            }
        }

        static wstring PartitionId;

        OperationQueue queue_;
        REConfigSPtr const & config_;
        bool cleanOnComplete_;
        bool ignoreCommit_;
        FABRIC_SEQUENCE_NUMBER firstAvailableCompletedSN_;
        FABRIC_SEQUENCE_NUMBER nextToBeCompletedSN_;
        FABRIC_SEQUENCE_NUMBER lastSN_;
        FABRIC_SEQUENCE_NUMBER lastCommittedSN_;
        list<FABRIC_SEQUENCE_NUMBER> dataMap_;
        ULONGLONG numberofCompletedOperations_;
        bool closed_;
    };

    wstring OperationQueueWrapper::PartitionId(Common::Guid::NewGuid().ToString());

    class TestOperationQueue
    {
    protected:
        TestOperationQueue() { BOOST_REQUIRE(Setup()); }
        TEST_CLASS_SETUP(Setup);

        void TestInOrderOperations(bool cleanOnComplete);

        void TestOutOfOrderOperations(bool cleanOnComplete);

        static void GenerateOutOfOrderSequenceNumbers(FABRIC_SEQUENCE_NUMBER startSeq, int numberOfItems, int maxOutOfOrder, vector<FABRIC_SEQUENCE_NUMBER> & seqNumbers);

        REConfigSPtr config_;
    };

    BOOST_FIXTURE_TEST_SUITE(TestOperationQueueSuite,TestOperationQueue)

    BOOST_AUTO_TEST_CASE(TestInOrderOperations1)
    {
        TestInOrderOperations(true);
    }

    BOOST_AUTO_TEST_CASE(TestInOrderOperations2)
    {
        TestInOrderOperations(false);
    }

    BOOST_AUTO_TEST_CASE(TestOutOfOrderOperations1)
    {
        TestOutOfOrderOperations(true);
    }

    BOOST_AUTO_TEST_CASE(TestOutOfOrderOperations2)
    {
        TestOutOfOrderOperations(false);
    }
 
    BOOST_AUTO_TEST_CASE(TestOperationNotCleanedupWhenNotNeeded)
    {
        // This test case is for checking if the operation queue cleans up operations even though there is capacity in the queue to allow for more operations to be buffered

        FABRIC_SEQUENCE_NUMBER startSeq = 10;
        config_->InitialSecondaryReplicationQueueSize = 2;
        config_->MaxSecondaryReplicationQueueSize = 8;
        bool cleanOnComplete = false;
        Trace.WriteInfo(OperationQueueSource, "TestOperationNotCleanedupWhenNotNeeded with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}, maxMemorySize {4}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete, 0);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete, false/*ignoreCommit*/);

        // Enqueue 2 items
        // Commit 2 items
        // Complete 2 items
        queue.Enqueue(1, startSeq + 1, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 100 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        queue.Enqueue(1, startSeq + 2, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 200 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        queue.Commit(2, startSeq + 2);
        queue.Complete(2, startSeq + 2, false, 200, 200);

        // This enqueue must cause an expand and should not shrink the queue
        queue.Enqueue(1, startSeq + 3, true /*capacityChange*/, 0 /*cleanedCompletedItems*/, 300 /*totalMemorySize*/, 200 /*completedMemorySize*/);
    }

    BOOST_AUTO_TEST_CASE(TestPowerOf2Computations)
    {
        VERIFY_ARE_EQUAL(OperationQueue::GetCeilingPowerOf2(1 << 3), 1 << 3, L"100 is the closest power of 2 from 100");
        VERIFY_ARE_EQUAL(OperationQueue::GetCeilingPowerOf2((1 << 4) + (1 << 6) + 1), 1 << 7, L"10000000 is the closest ceiling power of 2 from 1010001");
        VERIFY_ARE_EQUAL(OperationQueue::GetCeilingPowerOf2((1 << 10) + (1 << 9) + (1 << 2)), 1 << 11, L"1<<11 is the closest ceiling power of 2 from 1100000100");
    }

    BOOST_AUTO_TEST_CASE(TestEnqueueOperationsWithMemoryLimit)
    {
        FABRIC_SEQUENCE_NUMBER startSeq = 10;//Random().Next(1, 100);
        int initial = 4;
        config_->InitialSecondaryReplicationQueueSize = initial;
        int max = 8;
        config_->MaxSecondaryReplicationQueueSize = max;
        int maxMemory = 600;
        config_->MaxSecondaryReplicationQueueMemorySize = maxMemory;

        bool cleanOnComplete = false;
        Trace.WriteInfo(OperationQueueSource, "TestEnqueueOperationsWithMemoryLimit with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}, maxMemorySize {4}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete, maxMemory);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete, true/*ignoreCommit*/);
        
        // Enqueue operations until queue is full
        // Queue contains |0123 (i.e. |startSeq+0, startSeq++1, startSeq+2, startSeq+3)
        queue.Enqueue(initial, startSeq + 4, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 400 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        // Complete 0, Queue contains 0|123
        queue.Complete(1, startSeq + 1, false /*capacityChange*/, 400 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Try to add one more operation to exceed memory limit, should fail. The queue in unchanged
        // Queue contains 0|123
        queue.Enqueue(1, startSeq + 4, 301, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 400 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Try to add one more operation to exceed size limit, should succeed. 
        // Queue contains 0|1234(200)
        queue.Enqueue(1, startSeq + 5, 100, true /*capacityChange*/, 0 /*cleanedCompletedItems*/, 500 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Complete 1, Queue contains 01(200)|234(200)
        queue.Complete(1, startSeq + 2, false /*capacityChange*/, 500 /*totalMemorySize*/, 200 /*completedMemorySize*/);
        // Try to add one more operation to get the completed operation removed, should succeed.
        // Queue contains 1(100)|234(300)5(198)
        queue.Enqueue(1, startSeq + 6, 198, false /*capacityChange*/, 1 /*cleanedCompletedItems*/, 598 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Try to add 2 more operations to the queue, should succeed.
        // Queue contains 1(100)|234(300)5(198)6(1)7(1)
        queue.Enqueue(2, startSeq + 8, 1, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Complete 234, Queue contains 1234(400)|5(198)6(1)7(1)
        queue.Complete(3, startSeq + 5, false /*capacityChange*/, 600 /*totalMemorySize*/, 400 /*completedMemorySize*/);
        // Try to add 1 1-byte operation to exceed memory limit, should succeed and 1 completed operation should be removed.
        // Queue contains 234(300)|5(198)6(1)7(1)8(1)
        queue.Enqueue(1, startSeq + 9, 1, false /*capacityChange*/, 1 /*cleanedCompletedItems*/, 501 /*totalMemorySize*/, 300 /*completedMemorySize*/);
        // Try to add 1 out-of-order 399-byte operation, should succeed and 3 completed operation should be removed.
        // Queue contains |5(198)6(1)7(1)8(1).A(399)
        vector<FABRIC_SEQUENCE_NUMBER> vect;
        vect.push_back(startSeq + 10);
        queue.Enqueue(vect, startSeq + 11, 399, false /*capacityChange*/, 3 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        // Complete 56, Queue contains 5(198)6(1)|7(1)8(1).A(399)
        queue.Complete(2, startSeq + 7, false /*capacityChange*/, 600/*totalMemorySize*/, 199/*completedMemorySize*/);
        // Try to add one more operation to exceed size limit, should succeed after 2 completed items are removed
        // Queue contains |7(1)8(1).A(399)...E(199)
        vect[0] = startSeq + 14;
        queue.Enqueue(vect, startSeq + 15, 199, false /*capacityChange*/, 2 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        // Try to add one more operation to exceed memory limit, should fail. The queue in unchanged
        // Queue contains |7(1)8(1).A(399)...E(199)
        vect[0] = startSeq + 12;
        queue.Enqueue(vect, startSeq + 15, 200, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        // Try to add one more operation to exceed memory limit, should succeed after dropping 2 out-of-order operation.
        // Queue contains |7(1)8(1)9(200)
        vect[0] = startSeq + 9;
        queue.Enqueue(vect, startSeq + 10, 598, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 0 /*completedMemorySize*/, 5 /*numberOfDroppedOperation*/);
        // Discard pending operations, which shrinks the queue
        queue.DiscardPendingOperations(false);

        // reset the queue memory size
        config_->MaxSecondaryReplicationQueueMemorySize = 0;
    }

    BOOST_AUTO_TEST_CASE(TestEnqueueOperationsWithMemoryLimitNoSizeLimit)
    {
        FABRIC_SEQUENCE_NUMBER startSeq = 10;//Random().Next(1, 100);
        int initial = 4;
        config_->InitialSecondaryReplicationQueueSize = initial;
        int max = 0;
        config_->MaxSecondaryReplicationQueueSize = max;
        int maxMemory = 600;
        config_->MaxSecondaryReplicationQueueMemorySize = maxMemory;

        bool cleanOnComplete = false;
        Trace.WriteInfo(OperationQueueSource, "TestEnqueueOperationsWithMemoryLimitNoSizeLimit with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}, maxMemorySize {4}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete, maxMemory);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete, true/*ignoreCommit*/);
        
        // Enqueue operations until queue is full
        // Queue contains |0123 (i.e. |startSeq+0, startSeq++1, startSeq+2, startSeq+3)
        queue.Enqueue(initial, startSeq + 4, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 400 /*totalMemorySize*/, 0 /*completedMemorySize*/);
        // Complete 012, Queue contains 012|3
        queue.Complete(3, startSeq + 3, false /*capacityChange*/, 400 /*totalMemorySize*/, 300 /*completedMemorySize*/);
        // Try to add one more operation to exceed memory limit, should fail. The queue in unchanged
        // Queue contains 012|3
        queue.Enqueue(1, startSeq + 4, 501, false /*capacityChange*/, 0 /*cleanedCompletedItems*/, 400 /*totalMemorySize*/, 300 /*completedMemorySize*/);
        // Try to add one more operation to exceed memory limit, should succeed. 2 completed operation should be removed
        // Queue contains 2|34
        queue.Enqueue(1, startSeq + 5, 400, false /*capacityChange*/, 2 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 100 /*completedMemorySize*/);
        // Complete 3
        // Queue contains 23|4
        queue.Complete(1, startSeq + 4, false /*capacityChange*/, 600 /*totalMemorySize*/, 200 /*completedMemorySize*/);
        // Try to add one more out-of-order operation to exceed size limit, should succeed. 1 completed operation should be removed for accommodating memory
        // Queue contains 3|4......X
        vector<FABRIC_SEQUENCE_NUMBER> vect;
        vect.push_back(startSeq + 9000);
        queue.Enqueue(vect, startSeq + 9001, 100, true /*capacityChange*/, 1 /*cleanedCompletedItems*/, 600 /*totalMemorySize*/, 100 /*completedMemorySize*/);

        // Discard pending operations, which shrinks the queue
        queue.DiscardPendingOperations(true);

        // reset the queue memory size
        config_->MaxSecondaryReplicationQueueMemorySize = 0;
    }

    BOOST_AUTO_TEST_CASE(TestEnqueueOperations)
    {
        FABRIC_SEQUENCE_NUMBER startSeq = 10;
        int initial = 4;
        config_->InitialSecondaryReplicationQueueSize = initial;
        int max = 8;
        config_->MaxSecondaryReplicationQueueSize = max;
        config_->MaxSecondaryReplicationQueueMemorySize = 0;

        bool cleanOnComplete = false;
        Trace.WriteInfo(OperationQueueSource, "TestEnqueueOperations with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete, true/*ignoreCommit*/);
        
        // Enqueue operations until queue is full
        // |10,11,12,13
        queue.Enqueue(initial, startSeq + initial, false /*capacityChange*/, 0 /*cleanedCompletedItems*/);
        // 10|11,12,13
        queue.Complete(1, startSeq + 1, false /*capacityChange*/);
        // Add one more operation, should succeed. 
        // 10|11,12,13,14
        queue.Enqueue(1, startSeq + 1 + initial, true /*capacityChange*/, 0 /*cleanedCompletedItems*/);

        // Complete next 3 operations
        // 10,11,12,13|14
        queue.Complete(3, startSeq + 4, false /*capacityChange*/);

        // Enqueue one operation out of order, but so it has room if all completed are removed
        // 10,11,12,13|14,gap,16
        vector<FABRIC_SEQUENCE_NUMBER> vect;
        vect.push_back(startSeq + 6);
        queue.Enqueue(vect, startSeq + 7, false /*capacityChange*/, 0 /*cleanedCompletedItems*/); 

        // Enqueue one more operation out of order
        vect.clear();
        vect.push_back(startSeq + 9);
        // 12,13|14,gap,16,gap,gap,19
        queue.Enqueue(vect, startSeq + 10, false/*capacityChange*/, 2 /*cleanedCompletedItems*/); 

        // 12,13,14|gap,16,gap,gap,19
        queue.Complete(1, startSeq + 5, false /*capacityChange*/);
        // Enqueue operations above the queue capacity.
        // Enqueue fails, but the completed items are removed
        // Queue contains gap,16,gap,gap,19
        vect.clear();
        vect.push_back(startSeq + 13);
        queue.Enqueue(vect, startSeq + 10, false /*capacityChange*/, 3 /*cleanedCompletedItems*/); 
        
        // Add the missing operations 
        vect.clear();
        // Queue contains 15,16,gap,gap,19
        vect.push_back(startSeq + 5);
        queue.Enqueue(vect, startSeq + 10, false /*capacityChange*/, 0 /*cleanedCompletedItems*/); 

        // Complete the first 2 operations.
        // Queue contains 15,16|gap,gap,19
        queue.Complete(2, startSeq + 7, false /*capacityChange*/);
        
        // Discard pending operations, which shrinks the queue
        queue.DiscardPendingOperations(true);
    }

    BOOST_AUTO_TEST_CASE(TestCompleteBeforeCommit)
    {
        OperationQueueWrapper queue(config_, 1, false, false);
        queue.Enqueue(1, 2, false, 0 /*cleanedCompletedItems*/);
        queue.Complete(false /*capacityChange*/, false);
        queue.Commit(2);
        queue.Complete(false /*capacityChange*/, true);
        queue.ClearCompleted();
    }

    BOOST_AUTO_TEST_CASE(TestAverageComputationMacro)
    {
        // Case 1: Monotonically increasing average
        {
            DateTime now = DateTime::Now();
            TimeSpan currentAvg = TimeSpan::FromTicks(0);
            LONGLONG numberOfOperations = 0;

            DateTime start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(1));

            start = now - TimeSpan::FromSeconds(3);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(2));

            start = now - TimeSpan::FromSeconds(5);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(3));

            start = now - TimeSpan::FromSeconds(7);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(4));

            start = now - TimeSpan::FromSeconds(9);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(5));
        }

        // Case 2: Monotonically decreasing average
        {
            StopwatchTime now = Stopwatch::Now();
            TimeSpan currentAvg = TimeSpan::FromTicks(0);
            LONGLONG numberOfOperations = 0;

            StopwatchTime start = now - TimeSpan::FromSeconds(5);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(5));

            start = now - TimeSpan::FromSeconds(3);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(4));

            start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(3));

            start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromMilliseconds(2500));

            start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromMilliseconds(2200));
        }

        // Case 3: Increase and Decrease
        {
            StopwatchTime now = Stopwatch::Now();
            TimeSpan currentAvg = TimeSpan::FromTicks(0);
            LONGLONG numberOfOperations = 0;

            StopwatchTime start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(1));

            start = now - TimeSpan::FromSeconds(2);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromMilliseconds(1500));

            start = now - TimeSpan::FromSeconds(1);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg > TimeSpan::FromMilliseconds(1333) && currentAvg <= TimeSpan::FromMilliseconds(1334));

            start = now - TimeSpan::FromSeconds(2);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromMilliseconds(1500));

            start = now - TimeSpan::FromSeconds(0);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromMilliseconds(1200));

            start = now - TimeSpan::FromSeconds(114);
            UpdateAverageTime(now.Ticks - start.Ticks, currentAvg, numberOfOperations);
            VERIFY_IS_TRUE(currentAvg == TimeSpan::FromSeconds(20));
        }

    }

    BOOST_AUTO_TEST_CASE(TestQueryResult)
    {
        // Use Large LSN's to ensure no round off errors or overflows during percentage calculations
        Common::ScopedHeap heap;

        Trace.WriteInfo(OperationQueueSource, "Test 1 - 50% queue full on primary");
        // 50% queue usage in primary scenario
        {
            config_->MaxSecondaryReplicationQueueSize = 64*1024;
            config_->InitialSecondaryReplicationQueueSize = 64*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;
            ULONGLONG startLSN = 12345789999;

            OperationQueueWrapper queue(config_, startLSN, true/*cleanoncomplete*/, false);

            queue.Enqueue(32*1024, startLSN + (32*1024), false, 0 /*cleanedCompletedItems*/);
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 50;
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }
        
        Trace.WriteInfo(OperationQueueSource, "Test 2 - 12% queue full on secondary");
        // 12.5% queue usage in secondary scenario - rounded down to 12%
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 512*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;
            ULONGLONG startLSN = 999999999999999;

            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(128*1024, startLSN + (128*1024), false, 0 /*cleanedCompletedItems*/);
            queue.Commit(startLSN + (1024*128));
            queue.Complete(false);
            queue.Enqueue(128*1024, startLSN + (256*1024), false, 0);
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 12;
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }
        
        Trace.WriteInfo(OperationQueueSource, "Test 3 - 100% queue full on primary");
        // Queue full on primary
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, true/*cleanoncomplete*/, false);

            queue.Enqueue(1024*1024, startLSN + (1024*1024), false, 0 /*cleanedCompletedItems*/);
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 100;
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }

        Trace.WriteInfo(OperationQueueSource, "Test 4 - 100% queue full on secondary");
        // queue full on secondary
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(1024*1024, startLSN + (1024*1024), false, 0 /*cleanedCompletedItems*/);
            queue.Commit(startLSN + (1024*1024));
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 100;
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }
        
        Trace.WriteInfo(OperationQueueSource, "Test 5 - 0% queue full on secondary with only completed items");
        // empty queue on secondary with only completed items
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(1024*1024, startLSN + (1024*1024), false, 0 /*cleanedCompletedItems*/);
            queue.Commit(startLSN + (1024*1024));
            queue.Complete(false);
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 0;
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }

        Trace.WriteInfo(OperationQueueSource, "Test 6 - queue memory vs operation count 50% on primary");
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 1024*1024;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, true/*cleanoncomplete*/, false);
            
            queue.Enqueue(256 * 1024, startLSN + (256 * 1024), 2 /*operationsize*/, false, true);
            queue.Commit(startLSN + (256 * 1024));
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 50; // even though number of operations is only 25% of queue full size, the memory consumes 50%
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }

        Trace.WriteInfo(OperationQueueSource, "Test 7 - 75% queue full on secondary due to memory and not number of ops");
        {
            config_->MaxSecondaryReplicationQueueSize = 1024*1024;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 1024*1024;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(1, startLSN + 1, 1024 * 768, false, false);
            queue.Commit(startLSN + 1);
            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 75; // even though there is only 1 operation, memory consumption has reached 75%
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }

        Trace.WriteInfo(OperationQueueSource, "Test 8 - 37% queue full on secondary due to memory and not number of ops when there are completed items remaining in queue");
        {
            config_->MaxSecondaryReplicationQueueSize = 0;
            config_->InitialSecondaryReplicationQueueSize = 1024*1024;
            config_->MaxSecondaryReplicationQueueMemorySize = 2*1024*1024;
            ULONGLONG startLSN = 999999999999999999;

            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(1, startLSN + 1, 1024 * 768, false, false);
            queue.Commit(startLSN + 1);
            queue.Complete(false);

            queue.Enqueue(1, startLSN + 1 + 1, 1024 * 768, false, false);
            queue.Commit(startLSN + 1 + 1);

            ServiceModel::ReplicatorQueueStatus result = Replicator::GetReplicatorQueueStatusForQuery(queue.InnerQueue);
            FABRIC_REPLICATOR_QUEUE_STATUS publicResult;
            result.ToPublicApi(heap, publicResult);
            DWORD expected = 37; // even though there is only 1 operation, memory consumption has reached 37%
            VERIFY_ARE_EQUAL(expected, publicResult.QueueUtilizationPercentage);
        }
    }

    BOOST_AUTO_TEST_CASE(TestSecondaryQueueTrim)
    {
        Trace.WriteInfo(OperationQueueSource, "Test 1 - Trim items based on operation count");
        {
            config_->MaxSecondaryReplicationQueueSize = 8;
            config_->InitialSecondaryReplicationQueueSize = 2;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;

            config_->MaxPrimaryReplicationQueueMemorySize = 0;
            config_->MaxPrimaryReplicationQueueSize = 4;
            config_->InitialReplicationQueueSize = 2;

            ULONGLONG startLSN = 1;
            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(8, startLSN + 8, 1024, true/*capacityChange*/, false);
            queue.Commit(8, startLSN + 8);
            
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 0);

            queue.Complete(1, 2, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(3, 5, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(1, 6, false /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 2);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(true /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 5);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            config_->MaxPrimaryReplicationQueueSize = 0;
            config_->InitialReplicationQueueSize = 0;
        }

        Trace.WriteInfo(OperationQueueSource, "Test 2 - Secondary queue smaller than primary. Nothing to trim");
        {
            config_->MaxSecondaryReplicationQueueSize = 8;
            config_->InitialSecondaryReplicationQueueSize = 2;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;

            config_->MaxPrimaryReplicationQueueMemorySize = 0;
            config_->MaxPrimaryReplicationQueueSize = 16;
            config_->InitialReplicationQueueSize = 2;

            ULONGLONG startLSN = 1;
            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(8, startLSN + 8, 1024, true/*capacityChange*/, false);
            queue.Commit(8, startLSN + 8);
            
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 0);

            queue.Complete(1, 2, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(3, 5, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(1, 6, false /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(true /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            config_->MaxPrimaryReplicationQueueSize = 0;
            config_->InitialReplicationQueueSize = 0;
        }
        
        Trace.WriteInfo(OperationQueueSource, "Test 3 - Secondary queue equal to primary queue. Nothing to trim");
        {
            config_->MaxSecondaryReplicationQueueSize = 8;
            config_->InitialSecondaryReplicationQueueSize = 2;
            config_->MaxSecondaryReplicationQueueMemorySize = 0;

            config_->MaxPrimaryReplicationQueueMemorySize = 0;
            config_->MaxPrimaryReplicationQueueSize = 8;
            config_->InitialReplicationQueueSize = 2;

            ULONGLONG startLSN = 1;
            OperationQueueWrapper queue(config_, startLSN, false/*cleanoncomplete*/, false);

            queue.Enqueue(8, startLSN + 8, 1024, true/*capacityChange*/, false);
            queue.Commit(8, startLSN + 8);
            
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 0);

            queue.Complete(1, 2, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(3, 5, false/*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(1, 6, false /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            queue.Complete(true /*capacityChange*/);
            VERIFY_ARE_EQUAL(queue.InnerQueue.FirstAvailableCompletedSequenceNumber, 1);
            VERIFY_ARE_EQUAL(queue.InnerQueue.LastSequenceNumber, 8);

            config_->MaxPrimaryReplicationQueueSize = 0;
            config_->InitialReplicationQueueSize = 0;
        }
    }

    BOOST_AUTO_TEST_SUITE_END()

    bool TestOperationQueue::Setup()
    {
        config_ = std::make_shared<REConfig>();
        config_->MaxPrimaryReplicationQueueSize = 0;
        config_->MaxPrimaryReplicationQueueMemorySize = 0;
        config_->QueueHealthMonitoringInterval = TimeSpan::Zero;
        return TRUE;
    }

    void TestOperationQueue::TestInOrderOperations(bool cleanOnComplete)
    {
        FABRIC_SEQUENCE_NUMBER startSeq = Random().Next(1, 100);
        int initial = 16;
        config_->InitialSecondaryReplicationQueueSize = initial;
        int max = 512;
        config_->MaxSecondaryReplicationQueueSize = max;
        config_->MaxSecondaryReplicationQueueMemorySize = 0;

        Trace.WriteInfo(OperationQueueSource, "TestInOrderOperations with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete);
        
        // Add more operations than the queue capacity
        // and make sure that the queue expands until max capacity is reached.
        queue.Enqueue(max + 4 /* 516 */, startSeq + max, true, 0 /*cleanedCompletedItems*/);

        FABRIC_SEQUENCE_NUMBER commitCount = (max >> 1) + initial; 
        queue.Commit(commitCount, startSeq + commitCount);

        // Complete some items and try to add more items.
        FABRIC_SEQUENCE_NUMBER completeCount = (max >> 1) - initial; /* 248 */
        // Because we complete less than max * 2, shrink will not take place.
        queue.Complete(completeCount, startSeq + completeCount, false /*capacityChange*/);

        // Add items triggers removing the completed items;
        // Capacity will increase if necessary (not so in our case,
        // since we are at max capacity already).
        FABRIC_SEQUENCE_NUMBER enqueueCount = max >> 1; /* 256, only 248 will succeed */
        queue.Enqueue(enqueueCount, startSeq + max + completeCount, false, completeCount /*cleanedCompletedItems*/);

        // Commit all pending items so far in the queue.
        queue.Commit(startSeq + max + completeCount);

        // Complete more than half of the queue.
        FABRIC_SEQUENCE_NUMBER completeCount2 = (max >> 1) + initial; /* 264 */
        queue.Complete(completeCount2, startSeq + completeCount + completeCount2, true /*capacityChange*/);

        FABRIC_SEQUENCE_NUMBER tail = startSeq + max + completeCount + 1;
        queue.Enqueue(1, tail, false/*capacityChange*/, 1 /*cleanedCompletedItems*/);

        // Commit all operations.
        queue.Commit(tail);

        // Complete all operations.
        queue.Complete(false /*capacityChange*/);

        // Before the queue is destructed, we need to remove all completed items
        // Otherwise, when the operation destructors run, 
        // the test map doesn't exit.
        queue.ClearCompleted();
    }    

    void TestOperationQueue::TestOutOfOrderOperations(bool cleanOnComplete)
    {
        FABRIC_SEQUENCE_NUMBER startSeq = Random().Next(1, 100);
        int initial = 16;
        config_->InitialSecondaryReplicationQueueSize = initial;
        int max = 1024;
        config_->MaxSecondaryReplicationQueueSize = max;
        config_->MaxSecondaryReplicationQueueMemorySize = 0;

        Trace.WriteInfo(OperationQueueSource, "TestOutOfOrderOperations with start sequence {0}, capacity [{1}, {2}], cleanOnComplete {3}",
            startSeq, config_->InitialSecondaryReplicationQueueSize, config_->MaxSecondaryReplicationQueueSize, cleanOnComplete);

        // Create an array with items between startSeq and startSeq + max.
        // Shuffle the array so items are out of order and enqueue them.
        vector<FABRIC_SEQUENCE_NUMBER> seqNumbers;
        GenerateOutOfOrderSequenceNumbers(startSeq, max + 4, 64, seqNumbers);

        OperationQueueWrapper queue(config_, startSeq, cleanOnComplete);
        
        // Enqueue all items in the queue.
        // The capacity should grow to allow all items to be enqueued 
        // but the 4 that are bigger than max.
        queue.Enqueue(seqNumbers, startSeq + max, true /*capacityChange*/, 0 /*cleanedCompletedItems*/);

        queue.Commit(startSeq + max);

        // Complete more than half of the items
        FABRIC_SEQUENCE_NUMBER completeCount = (max >> 1) + initial; 
        queue.Complete(completeCount, startSeq + completeCount, true /*capacityChange*/);

        // Enqueue more items out-of-order
        vector<FABRIC_SEQUENCE_NUMBER> vect;
        FABRIC_SEQUENCE_NUMBER seq1 = startSeq + max + initial;
        FABRIC_SEQUENCE_NUMBER seq2 = seq1 + (max >> 1) - 1;
        FABRIC_SEQUENCE_NUMBER seq3 = seq1 + max;
        vect.push_back(seq1);
        vect.push_back(seq2);
        vect.push_back(seq3);
        vect.push_back(startSeq + max);
        vect.push_back(startSeq + max + 1);
        queue.Enqueue(vect, seq2 + 1, cleanOnComplete /*capacityChange*/, completeCount /*cleanedCompletedItems*/);

        // Try to commit all items; because only in order items are touched, none of them should be committed
        queue.Commit(startSeq + max + 2);
        queue.DiscardPendingOperations(false);
        queue.ClearCompleted();
    }


    void TestOperationQueue::GenerateOutOfOrderSequenceNumbers(FABRIC_SEQUENCE_NUMBER startSeq, int numberOfItems, int maxOutOfOrder, vector<FABRIC_SEQUENCE_NUMBER> & seqNumbers)
    {
        seqNumbers.resize(numberOfItems);
        FABRIC_SEQUENCE_NUMBER next = startSeq - 1;
        generate_n(seqNumbers.begin(), numberOfItems, [&next] { return ++next; });

        // Shuffle the vector
        auto it = seqNumbers.begin();
        for (int i = 0; i < numberOfItems - 1;)
        {
            int step = (maxOutOfOrder + i > numberOfItems) ? numberOfItems - i : maxOutOfOrder;
            random_shuffle(it, it + step);
            it += step;
            i += step;
        }
    }
}
