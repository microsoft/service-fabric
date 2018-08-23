// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    //
    // Job queue for async work items, that can start execution on one thread
    // and complete it on another thread.
    // The work can be long running - eg. async operations that send messages or have long running steps.
    //
    // Purpose: throttle the number of threads that can execute work and the number of async work that can be executed.
    // The job queue ensures that:
    // - items are queued if there is room in the queue, otherwise they are rejected with JobQueueFull error.
    // - work is started if there are enough threads to execute it.
    //   No more than max threads configured can be active at one time.
    // - if start work completes sync, work is considered completed and removed from the queue.
    // - if start work completes async, the work continues to be counted.
    //   When the work is ready to complete async, it notifies the job queue. When threads are available,
    //   the async work is completed, and the job item is considered completed.
    //   Async work ready to complete has priority over non-started items.
    // 
    // The job items has a sequence number that is guaranteed to increase.
    // The sequence number is used to retrieve the work item from the pending structure when async work is ready to complete.
    // The job items have state to keep track of their progress and their completion mode (sync or async).
    //
    // The job queue can dequeue items fifo or lifo, based on configuration and on its state (empty, full or in between).
    //
    template <typename T>
    class AsyncWorkJobQueue
    {
        DENY_COPY(AsyncWorkJobQueue);
    public:
        AsyncWorkJobQueue(
            std::wstring const & ownerName,
            std::wstring const & ownerInstance,
            ComponentRootSPtr && rootSPtr,
            JobQueueConfigSettingsHolderUPtr && settingsHolder,
            bool createPerfCounters = true,
            DequePolicy dequePolicy = DequePolicy::FifoLifo)
            : name_(ownerName + ownerInstance)
            , rootSPtr_(std::move(rootSPtr))
            , settingsHolder_(std::move(settingsHolder))
            , perfCounters_(createPerfCounters ? AsyncWorkJobQueuePerfCounters::CreateInstance(ownerName, ownerInstance) : nullptr)
            , dequePolicyHelper_(dequePolicy)
            , activeThreads_(0)
            , isClosed_(false)
            , items_()
            , asyncReadyItems_()
            , pendingProcessItems_()
            , lock_()
        {
            ASSERT_IFNOT(settingsHolder_, "{0}: settings holder can't be null", name_);
            CommonEventSource::Events->JobQueueCtor(
                name_,
                settingsHolder_->MaxThreadCount,
                settingsHolder_->MaxQueueSize,
                settingsHolder_->MaxParallelPendingWorkCount);
        }

        virtual ~AsyncWorkJobQueue()
        {
            ASSERT_IF(
                activeThreads_ != 0,
                "{0} has active thread during destruction: {1}",
                name_,
                *this);
            ASSERT_IFNOT(
                pendingProcessItems_.empty() && asyncReadyItems_.empty(),
                "{0} has pending async items during destruction: {1}",
                name_,
                *this);
            ASSERT_IF(rootSPtr_, "{0} has root set during destruction: {1}", name_, *this);
            Trace.WriteInfo("AsyncWorkJobQueue", name_, ".dtor");
        }

        __declspec (property(get = get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() { return name_; }

        virtual void Close()
        {
            bool completeClose = false;
            { // lock
                AcquireExclusiveLock grab(lock_);
                isClosed_ = true;
                completeClose = CompleteCloseIfNeededCallerHoldsLock(L"Close()");
                if (!completeClose)
                {
                    Trace.WriteInfo("AsyncWorkJobQueue", name_, "Close called: {0}", *this);
                }
            } // endlock

            if (completeClose)
            {
                // Call OnFinishItem outside the lock
                OnFinishItems();
            }
        }
        
        // Used as a signal to indicate that the job queue is closed and all the items have finished processing
        virtual void OnFinishItems()
        {
            // NOP, derived classes can use it for custom logic
        }

        Common::ErrorCode Enqueue(T && item)
        {
            ComponentRootSPtr root;
            { // lock
                AcquireExclusiveLock grab(lock_);
                if (isClosed_)
                {
                    return ErrorCode(ErrorCodeValue::ObjectClosed);
                }

                auto error = AddBeginWorkCallerHoldsLock(std::move(item));
                if (!error.IsSuccess())
                {
                    return error;
                }

                if (AddThreadIfRunnableCallerHoldsLock())
                {
                    root = rootSPtr_;
                }
            } // endlock

            ScheduleThreadIfNeeded(root);
            return ErrorCode::Success();
        }

        // The job item async processing came back on a different thread. Enqueue the work and schedule threads if possible.
        void OnAsyncWorkReadyToComplete(uint64 sequenceNumber)
        {
            // The job must be in the map of pending items, so this can't cause queue full.
            ComponentRootSPtr root;

            { // lock
                AcquireExclusiveLock grab(lock_);
                AddEndWorkCallerHoldsLock(sequenceNumber);
                if (AddThreadIfRunnableCallerHoldsLock())
                {
                    root = rootSPtr_;
                }
            } // endlock

            ScheduleThreadIfNeeded(root);
        }

    private:
        void Process(ComponentRootSPtr const & root)
        {
            ASSERT_IFNOT(root, "{0}: ProcessJob: root should be set inside Process(): {1}", name_, *this);
            int crtThread = GetCurrentThreadId();

            if (settingsHolder_->TraceProcessingThreads)
            {
                CommonEventSource::Events->JobQueueEnterProcess(name_, crtThread);
            }

            bool completeClose = false;
            bool hasItem = true;
            bool hasAsyncReadyItem = false;
            T asyncReadyItem;
            auto jobItemState = AsyncWorkJobItemState::NotStarted;
            auto crtItemIt = pendingProcessItems_.end();

            while (hasItem || hasAsyncReadyItem)
            {
                { // lock
                    AcquireExclusiveLock grab(lock_);

                    if (jobItemState == AsyncWorkJobItemState::CompletedSync)
                    {
                        // Remove the item from the map, processing is done.
                        ASSERT_IF(crtItemIt == pendingProcessItems_.end(), "{0}: error processing item, iterator not set", name_);
                        pendingProcessItems_.erase(crtItemIt);
                    }

                    crtItemIt = pendingProcessItems_.end();
                    jobItemState = AsyncWorkJobItemState::NotStarted;
                    hasAsyncReadyItem = false;
                    hasItem = false;

                    // First look for async items ready to complete, since they complete work already partially processed
                    if (!asyncReadyItems_.empty())
                    {
                        asyncReadyItem = GetAsyncItemToExecuteCallerHoldsLock();
                        hasAsyncReadyItem = true;
                    }
                    else
                    {
                        // Pick up the items from the input list and put it in pending processing map
                        uint64 sequenceNumber;
                        if (TryGetItemToExecuteCallerHoldsLock(sequenceNumber))
                        {
                            crtItemIt = pendingProcessItems_.find(sequenceNumber);
                            hasItem = true;
                        }
                    }

                    if (!hasAsyncReadyItem && !hasItem)
                    {
                        // No items to process currently, terminate thread
                        TerminateThreadCallerHoldsLock();
                        if (isClosed_)
                        {
                            completeClose = CompleteCloseIfNeededCallerHoldsLock(L"Process()");
                        }
                    }                    
                } // endlock

                if (hasAsyncReadyItem)
                {
                    // Async items must complete sync, so pass no callback
                    jobItemState = JobTraits<T>::ProcessJob(asyncReadyItem, nullptr);
                    ASSERT_IFNOT(jobItemState == AsyncWorkJobItemState::CompletedAsync, "{0}: ProcessJob of async ready item shouldn't return {1} state, expected CompletedAsync: {2}", name_, jobItemState, *this);
                    if (perfCounters_)
                    {
                        perfCounters_->OnItemCompleted(
                            true /*completedSync*/,
                            JobTraits<T>::GetEnqueuedTime(asyncReadyItem));
                    }
                }
                else if (hasItem)
                {
                    ASSERT_IF(crtItemIt == pendingProcessItems_.end(), "{0}: error processing item, iterator not set: {1}", name_, *this);
                    StopwatchTime enqueueTime = JobTraits<T>::GetEnqueuedTime(crtItemIt->second);
                    if (perfCounters_)
                    {
                        perfCounters_->OnItemStartWork(enqueueTime);
                    }

                    jobItemState = JobTraits<T>::ProcessJob(
                        crtItemIt->second,
                        [this, root](uint64 sequenceNumber) { this->OnAsyncWorkReadyToComplete(sequenceNumber); });

                    ASSERT_IFNOT(jobItemState == AsyncWorkJobItemState::CompletedSync || jobItemState == AsyncWorkJobItemState::AsyncPending, "{0}: ProcessJob: item shouldn't return {1} state: {2}", name_, jobItemState, *this);
                    if (perfCounters_ && jobItemState == AsyncWorkJobItemState::CompletedSync)
                    {
                        perfCounters_->OnItemCompleted(false /*completedAsync*/, enqueueTime);
                    }
                }
            }

            if (settingsHolder_->TraceProcessingThreads)
            {
                CommonEventSource::Events->JobQueueLeaveProcess(name_, crtThread);
            }

            if (completeClose)
            {
                OnFinishItems();
            }
        }

    public:
        // *Must* be called under the lock
        void WriteTo(Common::TextWriter & writer, Common::FormatOptions const &) const
        {
            writer.WriteLine("configuration: maxThreads={0},maxSize={1},maxPendingCount={2}, activeThreads={3}, count:items={4},pending={5},asyncReady={6}",
                settingsHolder_->MaxThreadCount,
                settingsHolder_->MaxQueueSize,
                settingsHolder_->MaxParallelPendingWorkCount,
                activeThreads_,
                items_.size(),
                pendingProcessItems_.size(),
                asyncReadyItems_.size());
        }

        void Test_GetStatistics(
            __out int & maxThreads,
            __out int & maxQueueSize,
            __out int & maxParallelPendingCount,
            __out int & activeThreads,
            __out size_t & itemCount,
            __out size_t & asyncReadyItemCount,
            __out size_t & pendingProcessItemCount) const
        {
            AcquireExclusiveLock grab(lock_);
            maxThreads = settingsHolder_->MaxThreadCount;
            maxQueueSize = settingsHolder_->MaxQueueSize;
            maxParallelPendingCount = settingsHolder_->MaxParallelPendingWorkCount;
            activeThreads = activeThreads_;
            itemCount = items_.size();
            asyncReadyItemCount = asyncReadyItems_.size();
            pendingProcessItemCount = pendingProcessItems_.size();
        }

    private:
        size_t GetCurrentSizeCallerHoldsLock()
        {
            return items_.size() + pendingProcessItems_.size() + asyncReadyItems_.size();
        }

        void CheckQueueInvariantsCallerHoldsLock()
        {
#ifdef DBG
            // Note: the settings can be dynamic and can change between the calls.
            // The the conditions below may not hold, which is ok.
            ASSERT_IF(
                activeThreads_ > settingsHolder_->MaxThreadCount,
                "{0}: Invariant not respected: activeThreads <= maxThreads: {1}",
                name_,
                *this)

            size_t crtSize = GetCurrentSizeCallerHoldsLock();
            ASSERT_IF(
                crtSize > settingsHolder_->MaxQueueSize,
                "{0}: Invariant not respected: crtSize {1} <= maxQueueSize: {2}",
                name_,
                crtSize,
                *this);

            ASSERT_IF(
                static_cast<int>(pendingProcessItems_.size()) > settingsHolder_->MaxParallelPendingWorkCount,
                "{0}: Invariant not respected: pending items <= maxParallelPendingWork: {1}",
                name_,
                *this);
#endif
        }

        ErrorCode AddBeginWorkCallerHoldsLock(T && item)
        {
            CheckQueueInvariantsCallerHoldsLock();
            auto sequenceNumber = JobTraits<T>::GetSequenceNumber(item);
            size_t crtSize = GetCurrentSizeCallerHoldsLock();
            if (crtSize >= settingsHolder_->MaxQueueSize)
            {
                if (perfCounters_)
                {
                    perfCounters_->OnItemRejected();
                }

                dequePolicyHelper_.UpdateOnQueueFull();

                ErrorCode error(ErrorCodeValue::JobQueueFull);
                CommonEventSource::Events->JobQueueEnqueueFailed(
                    name_,
                    sequenceNumber,
                    error,
                    settingsHolder_->MaxThreadCount,
                    settingsHolder_->MaxQueueSize,
                    settingsHolder_->MaxParallelPendingWorkCount,
                    activeThreads_,
                    items_.size(),
                    pendingProcessItems_.size(),
                    asyncReadyItems_.size());

                JobTraits<T>::OnEnqueueFailed(item, move(error));
                return error;
            }

            items_.push_back(std::move(item));
            Trace.WriteNoise("JobQueue", name_, "Enqueued item {0}: {1}", sequenceNumber, *this);

            if (perfCounters_)
            {
                perfCounters_->OnItemAdded();
            }

            return ErrorCode::Success();
        }

        T GetAsyncItemToExecuteCallerHoldsLock()
        {
            TESTASSERT_IF(asyncReadyItems_.empty(), "{0}: get async item failed because there are no items: {1}", name_, *this);
            CheckQueueInvariantsCallerHoldsLock();
            
            // We can remove items from the front or back, doesequenceNumber't really matter.
            T asyncReadyItem = std::move(asyncReadyItems_.back());
            asyncReadyItems_.pop_back();

            Trace.WriteNoise("JobQueue", name_, "Select async ready item {0}: {1}", JobTraits<T>::GetSequenceNumber(asyncReadyItem), *this);
            return std::move(asyncReadyItem);
        }

        bool TryGetItemToExecuteCallerHoldsLock(__inout uint64 & sequenceNumber)
        {
            if (items_.empty())
            {
                return false;
            }

            CheckQueueInvariantsCallerHoldsLock();
            ASSERT_IFNOT(asyncReadyItems_.empty(), "{0}: GetItemToExecute: asyncReadyItem expected empty: {1}", name_, *this);

            if (pendingProcessItems_.size() >= static_cast<size_t>(settingsHolder_->MaxParallelPendingWorkCount))
            {
                Trace.WriteNoise("JobQueue", name_, "Can't select item as max pending items is reached: {0}", *this);
                return false;
            }

            // Pick up a job item from the items_ using the dequeue policy helper
            bool fifo = dequePolicyHelper_.ShouldTakeFistItem();
            
            T item;
            if (fifo)
            {
                item = std::move(items_.front());
                items_.pop_front();
            }
            else
            {
                item = std::move(items_.back());
                items_.pop_back();
            }

            if (items_.empty())
            {
                dequePolicyHelper_.UpdateOnQueueEmpty();
            }

            sequenceNumber = JobTraits<T>::GetSequenceNumber(item);
            auto result = pendingProcessItems_.insert(make_pair(sequenceNumber, std::move(item)));
            ASSERT_IFNOT(result.second, "{0}: error inserting job item {1} in the pending items", name_, sequenceNumber);
            Trace.WriteNoise("JobQueue", name_, "Select item {0} (fifo={1}): {2}", sequenceNumber, fifo, *this);

            return true;
        }

        void AddEndWorkCallerHoldsLock(uint64 sequenceNumber)
        {
            CheckQueueInvariantsCallerHoldsLock();

            // Find the item in map and move it to ready map
            auto it = pendingProcessItems_.find(sequenceNumber);
            ASSERT_IF(it == pendingProcessItems_.end(), "{0}: OnAsyncWorkReadyToComplete: sequenceNumber {1} is not found in map: {2}", name_, sequenceNumber, *this);

            if (perfCounters_)
            {
                perfCounters_->OnItemAsyncWorkReady(JobTraits<T>::GetEnqueuedTime(it->second));
            }

            asyncReadyItems_.push_back(std::move(it->second));
            pendingProcessItems_.erase(it);

            Trace.WriteNoise("JobQueue", name_, "OnAsyncWorkReadyToComplete {0}: {1}", sequenceNumber, *this);
        }

        bool AddThreadIfRunnableCallerHoldsLock()
        {
            if (activeThreads_ < settingsHolder_->MaxThreadCount)
            {
                ++activeThreads_;
                return true;
            }
            
            return false;
        }
        
        void ScheduleThreadIfNeeded(ComponentRootSPtr const & root)
        {
            if (!root)
            {
                return;
            }

            if (settingsHolder_->TraceProcessingThreads)
            {
                CommonEventSource::Events->JobQueueScheduleThread(name_);
            }

            Threadpool::Post([this, root]
            {
                this->Process(root);
            });
        }

        void TerminateThreadCallerHoldsLock()
        {
            ASSERT_IF(activeThreads_ <= 0, "{0}: TerminateThreadCallerHoldsLock: activeThreads should be positive: {1}", name_, *this);
            --activeThreads_;
        }

        bool CompleteCloseIfNeededCallerHoldsLock(std::wstring const & caller)
        {
            if (activeThreads_ == 0 &&
                isClosed_ &&
                pendingProcessItems_.empty() &&
                asyncReadyItems_.empty())
            {
                Trace.WriteInfo("AsyncWorkJobQueue", name_, "Root reset during {0}: {1}", caller, *this);
                rootSPtr_.reset();
                return true;
            }

            return false;
        }
        
    private:
        template <typename J>
        struct JobTraits
        {
            static AsyncWorkJobItemState::Enum ProcessJob(J & item, AsyncWorkReadyToCompleteCallback const & callback)
            {
                return item.ProcessJob(callback);
            }
            
            static void OnEnqueueFailed(J & item, ErrorCode && error)
            {
                return item.OnEnqueueFailed(std::move(error));
            }

            static uint64 GetSequenceNumber(J const & item)
            {
                return item.SequenceNumber;
            }

            static StopwatchTime GetEnqueuedTime(J const & item)
            {
                return item.EnqueuedTime;
            }
        };

        template <typename JU>
        struct JobTraits<std::unique_ptr<JU>>
        {
            static AsyncWorkJobItemState::Enum ProcessJob(std::unique_ptr<JU> & item, AsyncWorkReadyToCompleteCallback const & callback)
            {
                ASSERT_IFNOT(item, "ProcessJob item not set");
                return item->ProcessJob(callback);
            }

            static void OnEnqueueFailed(std::unique_ptr<JU> & item, ErrorCode && error)
            {
                return item->OnEnqueueFailed(std::move(error));
            }

            static uint64 GetSequenceNumber(std::unique_ptr<JU> const & item)
            {
                ASSERT_IFNOT(item, "GetSequenceNumber item not set");
                return item->SequenceNumber;
            }

            static StopwatchTime GetEnqueuedTime(std::unique_ptr<JU> const & item)
            {
                return item->EnqueuedTime;
            }
        };

        template <typename JS>
        struct JobTraits<std::shared_ptr<JS>>
        {
            static AsyncWorkJobItemState::Enum ProcessJob(std::shared_ptr<JS> & item, AsyncWorkReadyToCompleteCallback const & callback)
            {
                ASSERT_IFNOT(item, "ProcessJob item not set");
                return item->ProcessJob(callback);
            }

            static void OnEnqueueFailed(std::shared_ptr<JS> & item, ErrorCode && error)
            {
                return item->OnEnqueueFailed(std::move(error));
            }

            static uint64 GetSequenceNumber(std::shared_ptr<JS> const & item)
            {
                ASSERT_IFNOT(item, "GetSequenceNumber item not set");
                return item->SequenceNumber;
            }

            static StopwatchTime GetEnqueuedTime(std::shared_ptr<JS> const & item)
            {
                return item->EnqueuedTime;
            }
        };

        std::wstring name_;
        ComponentRootSPtr rootSPtr_;
        JobQueueConfigSettingsHolderUPtr settingsHolder_;
        AsyncWorkJobQueuePerfCountersSPtr perfCounters_;
        JobQueueDequePolicyHelper dequePolicyHelper_;
        int activeThreads_;
        bool isClosed_;
        
        // All job items must be kept in one of the structures below, as follows:
        // - First, job items are put in items_, which contain non-started work
        // - When selected for processing, a job item is moved to pendingProcessItems_, 
        //   which contains items that in course of completion, either sync or async.
        //   These are kept in a map with sequence number as the key for easy retrieval.
        // - When the items complete sync, their processing is done
        // - When the items are completed async, they are moved from pendingProcessItems_ to asyncReadyItems_,
        //   which contains items for which the callback was triggered, so async work is ready to complete.
        //   This items must be picked up for processing again, so EndWork can be done.
        //   When end work is done, the job item processing is complete and the item can be removed from the queue.
        //
        // To close the queue, the items pending processing and the async ready ones must all be completed.

        std::deque<T> items_;
        std::vector<T> asyncReadyItems_;

        using PendingProcessItemMap = std::unordered_map<uint64, T>;
        PendingProcessItemMap pendingProcessItems_;

        MUTABLE_RWLOCK(AsyncWorkJobQueue, lock_);
    };

    using AsyncWorkJobItemUPtrQueue = AsyncWorkJobQueue<AsyncWorkJobItemUPtr>;
    using AsyncOperationWorkJobQueue = AsyncWorkJobQueue<AsyncOperationWorkJobItem>;
}
