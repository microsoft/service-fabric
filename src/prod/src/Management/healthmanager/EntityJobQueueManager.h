// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Management
{
    namespace HealthManager
    {
        // Job queue manager that queues and schedules work for entities.
        // It supports job items that can complete sync or async.
        // Work is enqueued by the entity cache manager, and has an identifier used as key into a map.
        // The identifier is either based on entity id or the cache id depending on the type of job item.
        // When added to job queue manager, work items are given an increasing sequence number.
        // Job items have priority (Low, Normal, High, Critical etc), and the queue manager ensures order based on priority then sequence numbers.
        // When an entity needs to execute lengthy work, it should perform it async; when done, job is considered read-only and have precendence over read-write jobs.
        // The store can notify the job queue manager when throttled, in which case the read-write job items
        // are blocked, while the read-only ones are allowed to be processed.
        // When throttled, read only jobs that came later than read-write jobs can be processed first.
        // The read-write ones must be blocked when the queue manager is in throttled way.
        
        // Provides:
        // 1. Work is processed based on priority and the order in which it arrives
        //    - higher priority jobs are started before the lower priority work
        //    - all jobs with the same priority are processed in the order in which they arrived with respect to each other
        //    - all read-only jobs are processed in the order in which they arrived with respect to each other
        //    - read-only jobs have precedence over read-write jobs, since they have completed the bulk of work and should be quickly completed.
        // 2. Two units of work for the same entity will not be processed in parallel
        // 3. Two units of work for different entities will be processed in parallel
        // 4. The maximum number of parallel entities being processed at a time is limited
        // 5. While an entity is performing replicated store work or other asynchronous work, work from other entities can proceed
        // 6. When an entity async commit completes, it will be executed in the order in which it was originally enqueued and has priority over read-write work.
        //    The job item will be executed even if the store is throttled, because the read-write part of the work is done.
        // 
        // Contains:
        // 1. A map of pending work for each entity. The work is sorted by priority and sequence number.
        // 2. A ready queue with job items to be processed next, sorted by priority and sequence number.
        class EntityJobQueueManager
            : public Store::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::HealthManager>
        {
        private:
            DENY_COPY(EntityJobQueueManager);
            
            // Sorted job items associated with a position in the ready queue. -1 means they are not in the ready queue.
            struct SortedJobItems
            {
                DENY_COPY(SortedJobItems)
            public:
                SortedJobItems(IHealthJobItemSPtr && item) : JobItems(), Position(-1)
                {
                    JobItems.push(std::move(item));
                }

                struct JobItemComparator
                {
                    inline bool operator()(IHealthJobItemSPtr const & lhs, IHealthJobItemSPtr const & rhs) { return *lhs > *rhs; }
                };
                
                std::priority_queue<IHealthJobItemSPtr, std::vector<IHealthJobItemSPtr>, JobItemComparator> JobItems;
                int Position;
            };

            struct SortedJobItems;
            using SortedJobItemsRPtr = SortedJobItems*;
            using SortedJobItemsUPtr = std::unique_ptr<SortedJobItems>;

            // Mutable priority queue, contains pointers to sorted list of items.
            // When new items are added, the sorted list max priority can change, so the ReadyQueue must re-arrange objects to maintain the heap order.
            class ReadyQueue
            {
                DENY_COPY(ReadyQueue)
            public:
                ReadyQueue();
                ~ReadyQueue();

                bool Empty() const { return items_.empty(); }
                size_t Size() const { return items_.size(); }
                SortedJobItemsRPtr const Top() const { return items_.front(); }

                void Push(SortedJobItemsRPtr && item);
                void Pop();

                // Update order based on new priority. The items are shifter up, starting with 
                // the item at the position of the prev.
                // The rest of the items are not touched.
                void IncreasePriority(__in SortedJobItems & prev);
                
            private:
                int GetLeft(int i) const { return 2 * i + 1; }
                int GetRight(int i) const { return 2 * i + 2; }
                int GetParent(int i) const { return (i - 1) / 2; }

                // Swap the items and keep track of position
                void Swap(int i, int j);

                // Moves items up to repair the heap, starting with lastPos. Items after lastPos as not checked.
                void ShiftUp(int lastPos);
                // Moves items down to repair the heap, starting with firstPos. Items before firstPos are not checked.
                void ShiftDown(int firstPos);

                void CheckPos(int i) const;
                void CheckHeapProperty() const;

            private:
                struct Comparator
                {
                    inline bool operator()(SortedJobItemsRPtr const lhs, SortedJobItemsRPtr const rhs) const
                    {
                        return (*lhs->JobItems.top()) > (*rhs->JobItems.top());
                    }
                };

                std::vector<SortedJobItemsRPtr> items_;
                Comparator comp_;
            };
                        
        public:
            EntityJobQueueManager(
                Store::PartitionedReplicaId const & partitionedReplicaId,
                __in HealthManagerReplica & healthManagerReplica,
                int maxThreadCount);

            virtual ~EntityJobQueueManager();
            
            bool HasWork(std::wstring const & entityId) const;

            void AddWork(IHealthJobItemSPtr && jobItem);

            void OnWorkComplete(
                __in IHealthJobItem & jobItem,
                Common::ErrorCode const & error);
            
            void OnAsyncWorkStarted(__in IHealthJobItem & jobItem);

            void OnAsyncWorkCompleted(
                IHealthJobItemSPtr const & jobItem,
                Common::AsyncOperationSPtr const & operation);
            
            void Close();

            // Test hooks
            void Test_SetThrottle(bool shouldThrottle);
            
        private:
            
            void SetThrottle(bool shouldThrottle);

            void AssertInvariants() const;
                        
            bool HasThreadsForProcessing() const;
            
            void TryEnqueueWork();

            void MoveToReadyQueueAndTryEnqueueWork(
                SortedJobItemsUPtr const & mapEntry);

            void EnqueueFromWorkList(
                __inout SortedJobItemsRPtr pendingWorkList);

            IHealthJobItemSPtr GetTopToExecute(SortedJobItems const & list);

            using EntityWorkMap = std::map<std::wstring, SortedJobItemsUPtr>;
            EntityWorkMap workMap_;

            ReadyQueue readyQueue_;
            
            HealthManagerReplica & healthManagerReplica_;
            
            int currentThreadCount_;
            int maxThreadCount_;
                        
            // Increasing sequence number associated with a job item
            // and used to execute job items in the order they were enqueued
            uint64 sequenceNumber_;

            MUTABLE_RWLOCK(HM.JobQueue, lock_);

            bool closed_;

            bool shouldThrottle_;
        };
    }
}
