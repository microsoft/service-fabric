// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{
    template <typename T, typename R>
    class JobQueue
    {
        DENY_COPY(JobQueue);

    public:
        JobQueue(std::wstring const & name, R & root, bool forceEnqueue = false, int maxThreads = 0, JobQueuePerfCountersSPtr perfCounters = nullptr, uint64 maxQueueSize = UINT64_MAX, DequePolicy dequePolicy = DequePolicy::FifoLifo)
            :   name_(name),
                root_(root),
                maxThreads_(maxThreads),
                activeThreads_(0),
                highestActiveThreads_(0),
                completed_(0),
                forceEnqueue_(forceEnqueue),
                extraTracingEnabled_(false),
                isClosed_(false),
                throttled_(false),
                perfCounters_(perfCounters),
                maxQueueSize_(maxQueueSize),
                dequePolicy_(dequePolicy),
                isFifo_(dequePolicy != DequePolicy::Lifo),
                traceProcessingThreads_(false),
                asyncJobs_(false)
        {
            rootSPtr_ = CreateComponentRoot();

            if (maxThreads_ == 0)
            {
                maxThreads_ = Environment::GetNumberOfProcessors();
            }

            Trace.WriteInfo("JobQueue", name_, "Queue created MaxThreads {0}", maxThreads_);
        }

        virtual ~JobQueue()
        {
            ASSERT_IF(activeThreads_ != 0,
                "{0} has {1} active thread during destruction",
                name_, activeThreads_);
            Trace.WriteInfo("JobQueue", name_, "Queue destructed, highest threads={0}", highestActiveThreads_);
        }

        RwLock & GetLockObject()
        {
            return lock_;
        }

        int GetMaxThreads()
        {
            AcquireReadLock lock(lock_);

            return maxThreads_;
        }

        uint64 GetQueueLength()
        {
            AcquireReadLock grab(lock_);
            return queue_.size();
        }

        uint64 GetActiveThreads()
        {
            AcquireReadLock grab(lock_);
            return activeThreads_;
        }

        uint GetCompleted()
        {
            return completed_;
        }

        void SetExtraTracing(bool enable)
        {
            extraTracingEnabled_ = enable;
            traceProcessingThreads_ = enable;
        }

        void SetTraceProcessingThreads(bool enable)
        {
            traceProcessingThreads_ = enable;
        }

        void SetAsyncJobs(bool enable)
        {
            Trace.WriteInfo(
                "JobQueue", 
                name_,
                "SetAsyncJobs={0}",
                enable);

            asyncJobs_ = enable;
        }

        void Test_ResetHighestActiveThreads()
        {
            highestActiveThreads_ = 0;
        }

        void UpdateMaxThreads(int maxThreads)
        {
            AcquireWriteLock grab(lock_);

            Trace.WriteInfo(
                "JobQueue", 
                name_,
                "UpdateMaxThreads: old = {0} new = {1}",
                maxThreads_,
                maxThreads);

            maxThreads_ = maxThreads;
        }

        ComponentRootSPtr CreateComponentRoot()
        {
            __if_exists (R::CreateComponentRoot)
            {
                return root_.CreateComponentRoot();
            }
            __if_not_exists (R::CreateComponentRoot)
            {
                return root_.Root.CreateComponentRoot();
            }
        }

        AsyncOperationSPtr CreateAsyncOperationRoot()
        {
            __if_exists (R::CreateAsyncOperationRoot)
            {
                return root_.CreateAsyncOperationRoot();
            }
            __if_not_exists (R::CreateAsyncOperationRoot)
            {
                return root_.Root.CreateAsyncOperationRoot();
            }
        }


        virtual void Close()
        {
            AcquireWriteLock grab(lock_);

            if (isClosed_)
            {
                return;
            }

            isClosed_ = true;
            if (activeThreads_ == 0)
            {
                CompleteClose(L"Close()");
            }
            else
            {
                Trace.WriteInfo("JobQueue", name_, "Close called, {0} active threads, {1} queued items", activeThreads_, queue_.size());
            }
        }
        
        // Used as a signal to indicate that the job queue is closed and all the items have finished processing
        // This signal is disabled if the forceEnqueue_ flag is set => since more items are allowed to be processed even after closing the queue
        virtual void OnFinishItems()
        {
        }

        bool IsThrottled() const
        {
            return throttled_;
        }

        void SetThrottle(bool enabled)
        {
            AcquireWriteLock grab(lock_);
            throttled_ = enabled;
        }

        bool Enqueue(T && item)
        {
            return Enqueue(item, false);
        }

        bool Reserve(T & item)
        {
            TESTASSERT_IF(
                maxQueueSize_ != UINT64_MAX,
                "Enqueue can fail because of size limit and callers of Reserve() currently dont handle that");

            return Enqueue(item, true);
        }

        void CancelReserve()
        {
            bool completeClose;
            {
                AcquireWriteLock grab(lock_);
                completeClose = OnJobThreadTerminate();
            }

            if (completeClose)
            {
                CompleteClose(L"CancelReserve()");
            }
        }

        void CompleteAsyncJob()
        {
            if (!asyncJobs_)
            {
                TRACE_LEVEL_AND_TESTASSERT(Trace.WriteError, "JobQueue", "{0} CompleteAsyncJob, async jobs not enabled", name_);

                return;
            }

            bool shouldScheduleThread = false;
            bool completeClose = false;
            {
                AcquireWriteLock grab(lock_);

                completed_++;
                Trace.WriteInfo("JobQueue", name_, "CompleteAsyncJob, throttled_={0}, active_={1} size={2}", 
                    throttled_, 
                    activeThreads_, 
                    queue_.size());

                if (!queue_.empty() && activeThreads_ <= maxThreads_ && !throttled_)
                {
                    shouldScheduleThread = true;
                }
                else
                {
                    completeClose = OnJobThreadTerminate();
                }
            }

            if (shouldScheduleThread)
            {
                ScheduleThread();
            }

            if (completeClose)
            {
                CompleteClose(L"CompleteAsyncJob()");
            }
        }

        bool Resume()
        {
            T currentItem;
            bool needThrottle = currentItem.NeedThrottle(root_);
            bool needSchedule = false;
            {
                AcquireWriteLock grab(lock_);

                Trace.WriteInfo("JobQueue", name_, "Resume, needThrottle={0}, throttled_={1}, size={2}", needThrottle, throttled_, queue_.size());

                if (queue_.size() == 0)
                {
                    throttled_ = false;
                }

                if (!throttled_)
                {
                    return false;
                }

                currentItem = move(RemoveFromQueueCallerHoldsLock());

                if (!needThrottle)
                {
                    throttled_ = false;
                    if (queue_.size() > 0)
                    {
                        ++activeThreads_;
                        needSchedule = true;
                    }
                }
            }

            bool isSync = JobTraits<T>::ProcessJob(currentItem, root_);

            if (needSchedule)
            {
                ScheduleThread();
            }

            return !isSync;
        }

        __declspec (property(get=get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() { return name_; }

        __declspec (property(get=get_HighestThreads)) int Test_HighestActiveThreads;
        int get_HighestThreads() { return highestActiveThreads_; }

    protected:
        R & root_;

        bool Enqueue(T & item, bool isReserve)
        {
            bool isRunnable = false;
            bool isQueueFull = false;
            {
                AcquireWriteLock grab(lock_);

                if (extraTracingEnabled_)
                {
                    Trace.WriteInfo(
                        "JobQueue", name_,
                        "Enqueue: activeThreads_ = {0}, maxThreads_ = {1}, throttled_ = {2}, queue_.size() = {3}",
                        activeThreads_, maxThreads_, throttled_, queue_.size());
                }

                if (!forceEnqueue_ && isClosed_)
                {
                    return false;
                }

                isRunnable = (activeThreads_ < maxThreads_) && !throttled_;
                if (isRunnable)
                {
                    ++activeThreads_;
                    if (activeThreads_ > highestActiveThreads_)
                    {
                        highestActiveThreads_ = activeThreads_;
                    }

                    if (isReserve)
                    {
                        return true;
                    }
                }
                
                isQueueFull = (queue_.size() >= maxQueueSize_);
                if (!isQueueFull)
                {
                    auto enqueueSuccess = AddToQueueCallerHoldsLock(std::move(item));
                    TESTASSERT_IFNOT(enqueueSuccess, "Enqueue failed when queue is not full");
                }                
            }

            if (isQueueFull)
            {
                if (dequePolicy_ == DequePolicy::FifoLifo)
                {
                    isFifo_ = false;
                }

                if (perfCounters_)
                {
                    perfCounters_->NumberOfDroppedItems.Increment();
                }

                TESTASSERT_IF(isRunnable == true, "MaxThreads not servicing the queue when queue is full");

                callOnQueueFull(item);

                // If isReserve is true, return true to indicate that the item wasnt queued and so the current
                // thread can be used to process that item if the higher layer wants.
                return isReserve;
            }

            if (isRunnable)
            {
                ScheduleThread();
            }

            return !isReserve;
        }

        void Process()
        {
            int crtThread = GetCurrentThreadId();

            if (traceProcessingThreads_)
            {
                CommonEventSource::Events->JobQueueEnterProcess(name_, crtThread);
            }

            T prevItem;
            T currentItem;
            bool hasCurrentItem = true;
            bool hasPrevItem = false;
            bool completeClose = false;
            bool isCurrentSync = false;
            R & rootRef = root_;
            
            while (hasCurrentItem)
            {
                {
                    AcquireWriteLock grab(lock_);
                    if (hasPrevItem)
                    {
                        JobTraits<T>::SynchronizedProcess(prevItem, rootRef);
                        completed_++;
                    }

                    if (queue_.size() > 0 && (!throttled_ || isCurrentSync))
                    {
                        currentItem = std::move(RemoveFromQueueCallerHoldsLock());
                    }
                    else
                    {
                        hasCurrentItem = false;
                        throttled_ = throttled_ && (queue_.size() > 0);

                        completeClose = OnJobThreadTerminate();
                    }
                }

                // After releasing the lock, if hasCurrentItem is false AND completeClose is also false,
                // no access to any member variable is allowed as the queue
                // could have been deallocated at any time.

                if (hasPrevItem)
                {
                    JobTraits<T>::Close(prevItem, rootRef);
                }

                if (hasCurrentItem)
                {
                    __if_exists(T::NeedThrottle)
                    {
                        bool needThrottle = currentItem.NeedThrottle(root_);
                        if (needThrottle != throttled_)
                        {
                            SetThrottle(needThrottle);
                        }
                    }

                    if (perfCounters_)
                    {
                        JobTraits<T>::UpdatePerfCounter(currentItem, *perfCounters_);
                    }

                    isCurrentSync = JobTraits<T>::ProcessJob(currentItem, root_);

                    if (asyncJobs_)
                    {
                        // ProcessJob() is performing async work when async jobs
                        // are enabled, so do not pump more than one job item from this thread.
                        // The user of JobQueue must call CompleteAsyncJob() when the 
                        // async job completes.
                        //
                        AcquireWriteLock grab(lock_);

                        hasCurrentItem = false;
                    }

                    prevItem = std::move(currentItem);
                    hasPrevItem = true;
                }
            }

            if (traceProcessingThreads_)
            {
                if (asyncJobs_)
                {
                    Trace.WriteInfo("JobQueue", name_, "leaving Process(), thread {0}", crtThread);
                }
                else
                {
                    CommonEventSource::Events->JobQueueLeaveProcess(name_, crtThread);
                }
            }

            if (completeClose)
            {
                CompleteClose(L"Process()");
            }
        }
        
    private:
        bool AddToQueueCallerHoldsLock(T &&item)
        {
            if(queue_.size() < maxQueueSize_)
            {
                queue_.push_back(std::move(item));
                
                if (perfCounters_)
                {
                    perfCounters_->NumberOfItems.Increment();
                    perfCounters_->NumberOfItemsInsertedPerSecond.Increment();
                }
                return true;
            }
            else
            {
                return false;
            }
        }

        T RemoveFromQueueCallerHoldsLock()
        {
            if (isFifo_)
            {
                T currentItem = std::move(queue_.front());
                queue_.pop_front();
                if (perfCounters_) { perfCounters_->NumberOfItems.Decrement(); }
                return move(currentItem);
            }
            else // Lifo
            {
                T currentItem = std::move(queue_.back());
                queue_.pop_back();
                if (dequePolicy_ == DequePolicy::FifoLifo && queue_.empty()) { isFifo_ = true; }
                if (perfCounters_) { perfCounters_->NumberOfItems.Decrement(); }
                return move(currentItem);
            }
        }

        template <typename J> 
        struct JobTraits
        {
            static bool ProcessJob(J & item, R & root)
            {
                return item.ProcessJob(root);
            }

            static void SynchronizedProcess(J & item, R & root)
            {
                __if_exists(J::SynchronizedProcess)
                {
                    item.SynchronizedProcess(root);
                }
                __if_not_exists(J::SynchronizedProcess)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void Close(J & item, R & root)
            {
                __if_exists(J::Close)
                {
                    item.Close(root);
                }
                __if_not_exists(J::Close)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void UpdatePerfCounter(J &item, JobQueuePerfCounters &perfCounter)
            {
                __if_exists(J::UpdatePerfCounter)
                {
                    item.UpdatePerfCounter(perfCounter);
                }
                __if_not_exists(J::UpdatePerfCounter)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(perfCounter);
                }
            }
        };

        template <typename JU>
        struct JobTraits<std::unique_ptr<JU>>
        {
            static bool ProcessJob(std::unique_ptr<JU> & item, R & root)
            {
                return item->ProcessJob(root);
            }

            static void SynchronizedProcess(std::unique_ptr<JU> & item, R & root)
            {
                __if_exists(JU::SynchronizedProcess)
                {
                    item->SynchronizedProcess(root);
                }
                __if_not_exists(JU::SynchronizedProcess)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void Close(std::unique_ptr<JU> & item, R & root)
            {
                __if_exists(JU::Close)
                {
                    item->Close(root);
                }
                __if_not_exists(JU::Close)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void UpdatePerfCounter(std::unique_ptr<JU> &item, JobQueuePerfCounters &perfCounter)
            {
                __if_exists(JU::UpdatePerfCounter)
                {
                    item->UpdatePerfCounter(perfCounter);
                }
                __if_not_exists(JU::UpdatePerfCounter)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(perfCounter);
                }
            }
        };

        template <typename JU>
        struct JobTraits<std::shared_ptr<JU>>
        {
            static bool ProcessJob(std::shared_ptr<JU> & item, R & root)
            {
                return item->ProcessJob(root);
            }

            static void SynchronizedProcess(std::shared_ptr<JU> & item, R & root)
            {
                __if_exists(JU::SynchronizedProcess)
                {
                    item->SynchronizedProcess(root);
                }
                __if_not_exists(JU::SynchronizedProcess)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void Close(std::shared_ptr<JU> & item, R & root)
            {
                __if_exists(JU::Close)
                {
                    item->Close(root);
                }
                __if_not_exists(JU::Close)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(root);
                }
            }

            static void UpdatePerfCounter(std::shared_ptr<JU> &item, JobQueuePerfCounters &perfCounter)
            {
                __if_exists(JU::UpdatePerfCounter)
                {
                    item->UpdatePerfCounter(perfCounter);
                }
                __if_not_exists(JU::UpdatePerfCounter)
                {
                    UNREFERENCED_PARAMETER(item);
                    UNREFERENCED_PARAMETER(perfCounter);
                }
            }
        };

        bool OnJobThreadTerminate()
        {
            return (--activeThreads_ == 0 && isClosed_);
        }

        void ScheduleThread()
        {
            if (traceProcessingThreads_)
            {
                CommonEventSource::Events->JobQueueScheduleThread(name_);
            }

            auto root = CreateComponentRoot();
            Threadpool::Post([this, root]
            {
                this->Process();
            });
        }

        void CompleteClose(wstring const & caller)
        {
            Trace.WriteInfo("JobQueue", name_, "Root reset during {0}", caller);

            // release the root after accessing all the member variables needed in this method as the
            // release could result in the queue destruction 
            auto tempRoot = std::move(rootSPtr_);

            if (!forceEnqueue_)
            {
                OnFinishItems();
            }                         
        }

        ComponentRootSPtr rootSPtr_;
        int maxThreads_;
        int activeThreads_;
        int highestActiveThreads_;
        uint completed_;
        std::deque<T> queue_;
        bool forceEnqueue_;
        bool isClosed_;
        bool throttled_;
        // If enabled, traces information about queue state at the time of enqueue. Disabled by default.
        bool extraTracingEnabled_;
        std::wstring name_;
        JobQueuePerfCountersSPtr perfCounters_;
        uint64 maxQueueSize_;
        RWLOCK(JobQueue, lock_);
        DequePolicy dequePolicy_;
        bool isFifo_;
        // If enabled, traces processing threads. Disabled by default.
        bool traceProcessingThreads_;
        bool asyncJobs_;


        //SFINAE handler to support the optionality of the OnQueueFull method.
        //One of the following 3 calls will be generated depending on the resolution of the return type.
        //If more than one of the following doesn't cause a substitution failure, the generation is 
        //in order of priority. "0 -> int" has priority over "0 -> char" or "..."
        void callOnQueueFull(T & item)
        {
            callOnQueueFull(item, 0);
        }

        //This method will only be generated in this template if the call item.OnQueueFull doesn't cause 
        //a substitution failure. Which means OnQueueFull is directly accessible through item.
        template <typename TU>
        auto callOnQueueFull(TU & item, int) -> decltype(item.OnQueueFull(root_, queue_.size()), void())
        {
            item.OnQueueFull(root_, queue_.size());
        }

        //This method will only be generated in this template if the call item->OnQueueFull doesn't cause 
        //a substitution failure. Which means item is either encapsulated on a smart pointer, its a
        //memory reference or it overrides the "->" operator and has an onQueueFull method visible.
        template <typename TU>
        auto callOnQueueFull(TU & item, char) -> decltype(item->OnQueueFull(root_, queue_.size()), void())
        {
            item->OnQueueFull(root_, queue_.size());
        }

        //This method will be generated if one of the above two are not. This is meant as a fail safe in 
        //order to catch cases where item doesn't have a visible implementation of OnQueueFull.
        template <typename TU>
        auto callOnQueueFull(TU &, ...) -> decltype(void())
        {
        }
    };

    template <typename R>
    class JobItem
    {
    public:
        JobItem() {}
        virtual ~JobItem() {}

        bool ProcessJob(R & root)
        {
            Process(root);
            return true;
        }

        virtual void Process(R &)
        {
        }

        virtual void SynchronizedProcess(R &)
        {
        }

        virtual void OnQueueFull(R &, size_t)
        {
        }
    };

    template <typename R>
    class DefaultJobItem : public Common::JobItem <R>
    {
        DEFAULT_COPY_ASSIGNMENT(DefaultJobItem)

    public:
        DefaultJobItem()
        {
        }

        DefaultJobItem(std::function<void(R &)> const & callback)
            : callback_(callback)
        {
        }

        DefaultJobItem(DefaultJobItem const & other)
            : callback_(other.callback_)
        {
        }

        DefaultJobItem(DefaultJobItem && other)
            : callback_(std::move(other.callback_))
        {
        }

        DefaultJobItem & operator = (DefaultJobItem && other)
        {
            if (&other != this)
            {
                callback_ = std::move(other.callback_);
            }
        
            return *this;
        } 

        void Process(R & r)
        {
            TESTASSERT_IFNOT(callback_, "Valid callback should be set");

            if (callback_)
            {
                callback_(r);
                callback_ = nullptr;
            }            
        }

    private:
        std::function<void(R &)> callback_;
    };    

    template<typename R>
    class CommonTimedJobItem : public Common::JobItem <R>
    {
    public:
        CommonTimedJobItem(TimeSpan const &timeout)
            : timeoutHelper_(timeout)
            , enqueuedTime_()
        {
            enqueuedTime_.Start();
        }

        virtual ~CommonTimedJobItem() {}

        bool ProcessJob(R & root)
        {
            if (timeoutHelper_.GetRemainingTime() > TimeSpan::Zero)
            {
                this->Process(root);
            }
            else
            {
                this->OnTimeout(root);
            }

            return true;
        }

        virtual void OnTimeout(R &)
        {}

        virtual void OnQueueFull(R &, size_t) override
        {
        }

        void UpdatePerfCounter(JobQueuePerfCounters &perfCounter)
        {
            perfCounter.AverageTimeSpentInQueueBase.Increment();
            perfCounter.AverageTimeSpentInQueue.IncrementBy(enqueuedTime_.ElapsedMilliseconds);
        }

        __declspec(property(get=get_RemainingTime)) TimeSpan RemainingTime;
        TimeSpan get_RemainingTime() const { return timeoutHelper_.GetRemainingTime(); }

        __declspec(property(get = get_ElapsedTime)) TimeSpan ElapsedTime;
        TimeSpan get_ElapsedTime() const { return enqueuedTime_.Elapsed; }

    private:
        TimeoutHelper timeoutHelper_;
        Stopwatch enqueuedTime_;
    };

    template <typename R>
    class DefaultTimedJobItem : public Common::CommonTimedJobItem <R>
    {
        DEFAULT_COPY_ASSIGNMENT(DefaultTimedJobItem)

    public:
        DefaultTimedJobItem(TimeSpan const &timeout, std::function<void(R &)> const & callback)
            : CommonTimedJobItem(timeout)
            , callback_(callback)
        {
        }

        DefaultTimedJobItem(TimeSpan const &timeout, std::function<void(R &)> const & callback, std::function<void(R &)> const & timeoutCallback)
            : CommonTimedJobItem<R>(timeout)
            , callback_(callback)
            , timeoutCallback_(timeoutCallback)
        {
        }

        void Process(R & r)
        {
            TESTASSERT_IFNOT(callback_, "Valid callback should be set");

            if(callback_)
            {
                callback_(r);
                callback_ = nullptr;
            }
        }

        void OnTimeout(R & r)
        {
            if(timeoutCallback_)
            {
                timeoutCallback_(r);
                timeoutCallback_ = nullptr;
            }
        }

    private:
        std::function<void(R &)> callback_;
        std::function<void(R &)> timeoutCallback_;
    };

    template <typename R>
    class CommonJobQueue : public JobQueue<std::unique_ptr<JobItem<R>>, R>
    {
    public:
        CommonJobQueue(std::wstring const & name, R & root, bool forceEnqueue = false, int maxThreads = 0, JobQueuePerfCountersSPtr perfCounters = nullptr, uint64 queueSize = UINT64_MAX, DequePolicy dequePolicy = DequePolicy::FifoLifo)
            : JobQueue<std::unique_ptr<JobItem<R>>, R>(name, root, forceEnqueue, maxThreads, perfCounters, queueSize, dequePolicy)
        {
        }
    };

    template <typename R>
    class CommonTimedJobQueue : public JobQueue<std::unique_ptr<CommonTimedJobItem<R>>, R>
    {
    public:
        CommonTimedJobQueue(std::wstring const & name, R & root, bool forceEnqueue = false, int maxThreads = 0, JobQueuePerfCountersSPtr perfCounters = nullptr, uint64 queueSize = UINT64_MAX, DequePolicy dequePolicy = DequePolicy::FifoLifo)
            : JobQueue<std::unique_ptr<CommonTimedJobItem<R>>, R>(name, root, forceEnqueue, maxThreads, perfCounters, queueSize, dequePolicy)
        {
        }
    };

    template <typename R>
    class DefaultJobQueue : public JobQueue<DefaultJobItem<R>, R>
    {
    public:
        DefaultJobQueue(std::wstring const & name, R & root, bool forceEnqueue = false, int maxThreads = 0, JobQueuePerfCountersSPtr perfCounters = nullptr, uint64 queueSize = UINT64_MAX, DequePolicy dequePolicy = DequePolicy::FifoLifo)
            : JobQueue<DefaultJobItem<R>, R>(name, root, forceEnqueue, maxThreads, perfCounters, queueSize, dequePolicy)
        {
        }
    };

    template <typename R>
    class DefaultTimedJobQueue : public JobQueue<std::unique_ptr<DefaultTimedJobItem<R>>, R>
    {
    public:
        DefaultTimedJobQueue(std::wstring const & name, R & root, bool forceEnqueue = false, int maxThreads = 0, JobQueuePerfCountersSPtr perfCounters = nullptr, uint64 queueSize = UINT64_MAX, DequePolicy dequePolicy = DequePolicy::FifoLifo)
            : JobQueue<std::unique_ptr<DefaultTimedJobItem<R>>, R>(name, root, forceEnqueue, maxThreads, perfCounters, queueSize, dequePolicy)
        {
        }
    };
}
