// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Common
{

    template <typename T, typename R>
    class BatchJobQueue
    {
        DENY_COPY(BatchJobQueue);

    public:
        BatchJobQueue(std::wstring const & name, std::function<void(vector<T> &, R &)> const & callback, R & root, bool forceEnqueue = false, int maxThreads = 0, uint64 maxQueueSize = UINT64_MAX)
            : name_(name),
            processCallBack_(callback),
            root_(root),
            maxThreads_(maxThreads),
            activeThreads_(0),
            highestActiveThreads_(0),
            forceEnqueue_(forceEnqueue),
            isClosed_(false),
            maxQueueSize_(maxQueueSize)
        {
            rootSPtr_ = CreateComponentRoot();

            if (maxThreads_ == 0)
            {
                maxThreads_ = Environment::GetNumberOfProcessors();
            }

            Trace.WriteInfo("BatchJobQueue", name_, "BatchJobQueue created MaxThreads {0}", maxThreads_);
        }

        virtual ~BatchJobQueue()
        {
            ASSERT_IF(activeThreads_ != 0,
                "{0} has {1} active thread during destruction",
                name_, activeThreads_);
            Trace.WriteInfo("BatchJobQueue", name_, "Queue destructed, highest threads={0}", highestActiveThreads_);
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
                Trace.WriteInfo("BatchJobQueue", name_, "Close called, {0} active threads, {1} queued items", activeThreads_, queue_.size());
            }
        }

        bool Enqueue(T && item)
        {
            return Enqueue(item);
        }

        __declspec (property(get = get_Name)) std::wstring const & Name;
        std::wstring const & get_Name() { return name_; }

        __declspec (property(get = get_HighestThreads)) int Test_HighestActiveThreads;
        int get_HighestThreads() { return highestActiveThreads_; }

    protected:
        R & root_;

        bool Enqueue(T & item)
        {
            bool isRunnable = false;
            bool isQueueFull = false;
            {
                AcquireWriteLock grab(lock_);

                if (!forceEnqueue_ && isClosed_)
                {
                    return false;
                }

                isRunnable = (activeThreads_ < maxThreads_);
                if (isRunnable)
                {
                    ++activeThreads_;
                    if (activeThreads_ > highestActiveThreads_)
                    {
                        highestActiveThreads_ = activeThreads_;
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

                TESTASSERT_IF(isRunnable == true, "MaxThreads not servicing the queue when queue is full");

                return false;
            }

            if (isRunnable)
            {
                ScheduleThread();
            }

            return true;
        }

        void Process()
        {
            vector<T> currentItems;
            bool hasCurrentItem = true;
            bool completeClose = false;

            while (hasCurrentItem)
            {
                {
                    AcquireWriteLock grab(lock_);

                    if (queue_.size() > 0)
                    {
                        currentItems = GetAllItemsFromQueueCallerHoldsLock();
                        queue_.clear();
                    }
                    else
                    {
                        hasCurrentItem = false;
                        completeClose = OnJobThreadTerminate();
                    }
                }

                if (hasCurrentItem)
                {
                  processCallBack_(currentItems, root_);
                }
            }

            if (completeClose)
            {
                CompleteClose(L"Process()");
            }
          
		}

    private:

        std::function<void(vector<T> & items, R &)> processCallBack_;
        ComponentRootSPtr rootSPtr_;
        int maxThreads_;
        int activeThreads_;
        int highestActiveThreads_;
        std::deque<T> queue_;
        bool forceEnqueue_;
        bool isClosed_;
        std::wstring name_;
        uint64 maxQueueSize_;
        RWLOCK(BatchJobQueue, lock_);


        bool AddToQueueCallerHoldsLock(T &&item)
        {
            if (queue_.size() < maxQueueSize_)
            {
                queue_.push_back(std::move(item));

                return true;
            }
            else
            {
                return false;
            }
        }

        vector<T> GetAllItemsFromQueueCallerHoldsLock()
        {
            vector<T> items;
            items.reserve(queue_.size());

            for (auto& item : queue_)
            {
                items.push_back(std::move(item));
            }
            return items;
        }

        void CompleteClose(wstring const & caller)
        {
            Trace.WriteInfo("BatchJobQueue", name_, "Root reset during {0}", caller);

            // release the root after accessing all the member variables needed in this method as the
            // release could result in the queue destruction 
            auto tempRoot = std::move(rootSPtr_);
        }

        void ScheduleThread()
        {
            auto root = CreateComponentRoot();
            Threadpool::Post([this, root]
            {
                this->Process();
            });
        }

        bool OnJobThreadTerminate()
        {
            return (--activeThreads_ == 0 && isClosed_);
        }

    };
}
    
