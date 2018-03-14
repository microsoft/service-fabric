// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
namespace Reliability
{
    namespace ReconfigurationAgentComponent
    {
        namespace ReliabilityUnitTest
        {
            class ThreadpoolStub : public Infrastructure::IThreadpool
            {
                DENY_COPY(ThreadpoolStub);

            public:

                template<typename T>
                class Queue
                {
                public:
                    Queue() : totalEnqueuedCount_(0) {}

                    __declspec(property(get = get_IsEmpty)) bool IsEmpty;
                    bool get_IsEmpty() const { return items_.empty(); }

                    __declspec(property(get = get_Items)) std::vector<T> const & Items;
                    std::vector<T> const & get_Items() const { return items_; }

                    __declspec(property(get = get_Count)) size_t Count;
                    size_t get_Count() const { return items_.size(); }

                    __declspec(property(get = get_TotalEnqueuedCount)) int TotalEnqueuedCount;
                    int get_TotalEnqueuedCount() const { return totalEnqueuedCount_; }

                    void Add(T && item)
                    {
                        items_.push_back(std::move(item));
                        totalEnqueuedCount_++;
                    }

                    void Add(T const & item)
                    {
                        items_.push_back(item);
                        totalEnqueuedCount_++;
                    }

                    void Drain(ReconfigurationAgent & ra)
                    {
                        while (!IsEmpty)
                        {
                            ExecuteFirstAndPop(ra);
                        }
                    }

                    void ExecuteLastAndPop(ReconfigurationAgent & ra)
                    {
                        auto item = std::move(items_[items_.size() - 1]);

                        items_.pop_back();

                        ThreadpoolStub::ProcessQueueItem<T>(ra, item);
                    }

                    void ExecuteFirstAndPop(ReconfigurationAgent & ra)
                    {
                        auto item = std::move(items_[0]);

                        items_.erase(items_.begin());

                        ThreadpoolStub::ProcessQueueItem<T>(ra, item);
                    }

                    void Clear()
                    {
                        items_.clear();
                    }

                private:
                    int totalEnqueuedCount_;
                    std::vector<T> items_;
                };

                ThreadpoolStub(ReconfigurationAgent & ra, bool drainOnClose) : ra_(ra), drainOnClose_(drainOnClose), failEnqueueIntoMessageQueue_(false)
                {
                }

                __declspec(property(get = get_FailEnqueueIntoMessageQueue, put=set_FailEnqueueIntoMessageQueue)) bool FailEnqueueIntoMessageQueue;
                bool get_FailEnqueueIntoMessageQueue() const { return failEnqueueIntoMessageQueue_; }
                void set_FailEnqueueIntoMessageQueue(bool const& value) { failEnqueueIntoMessageQueue_ = value; }

                __declspec(property(get = get_AreAllQueuesDrained)) bool AreAllQueuesDrained;
                bool get_AreAllQueuesDrained() const { return ThreadpoolCallbacks.IsEmpty && MessageQueue.IsEmpty && FTQueue.IsEmpty; }

                virtual void ExecuteOnThreadpool(Common::ThreadpoolCallback callback)
                {
                    ThreadpoolCallbacks.Add(callback);

                    RaiseItemEnqueuedEvent();
                }

                virtual bool EnqueueIntoMessageQueue(MessageProcessingJobItem<ReconfigurationAgent> & ji)
                {
                    if (failEnqueueIntoMessageQueue_)
                    {
                        return false;
                    }

                    MessageQueue.Add(std::move(ji));
                    RaiseItemEnqueuedEvent();
                    return true;
                }

                Common::AsyncOperationSPtr BeginScheduleEntity(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent)
                {
                    auto op = Common::AsyncOperation::CreateAndStart<AsyncApiStub<void*, void*>::AsyncOperationWithHolder>(
                        false,
                        nullptr,
                        nullptr,
                        Common::ErrorCodeValue::Success,
                        callback,
                        parent);

                    FTQueue.Add(op);

                    RaiseItemEnqueuedEvent();

                    return op;
                }

                Common::ErrorCode EndScheduleEntity(
                    Common::AsyncOperationSPtr const & op)
                {
                    return Common::AsyncOperation::End<AsyncApiStub<void*, void*>::AsyncOperationWithHolder>(op)->Error;
                }

                Common::AsyncOperationSPtr BeginScheduleCommitCallback(
                    Common::AsyncCallback const & callback,
                    Common::AsyncOperationSPtr const & parent)
                {
                    auto op = Common::AsyncOperation::CreateAndStart<AsyncApiStub<void*, void*>::AsyncOperationWithHolder>(
                        true,
                        nullptr,
                        nullptr,
                        Common::ErrorCodeValue::Success,
                        callback,
                        parent);

                    RaiseItemEnqueuedEvent();

                    return op;
                }

                Common::ErrorCode EndScheduleCommitCallback(
                    Common::AsyncOperationSPtr const & op)
                {
                    return Common::AsyncOperation::End<AsyncApiStub<void*, void*>::AsyncOperationWithHolder>(op)->Error;
                }

                void Close()
                {
                    auto msg = Common::wformatString(
                        "Threadpool: {0}. Message: {1}. FTQueue: {2}. Commit: {3}",
                        ThreadpoolCallbacks.Count,
                        MessageQueue.Count,
                        FTQueue.Count,
                        CommitQueue.Count);

                    if (!AreAllQueuesDrained)
                    {
                        TestLog::WriteInfo(msg);
                    }

                    if (drainOnClose_)
                    {
                        DrainAll(ra_);
                    }
                }

                void DrainAll(ReconfigurationAgent & ra)
                {
                    while (!AreAllQueuesDrained)
                    {
                        ThreadpoolCallbacks.Drain(ra);
                        MessageQueue.Drain(ra);
                        FTQueue.Drain(ra);
                        CommitQueue.Drain(ra);
                    }
                }

                void Reset()
                {
                    ThreadpoolCallbacks.Clear();
                    MessageQueue.Clear();
                    FTQueue.Clear();
                }

                void RaiseItemEnqueuedEvent()
                {
                    if (ItemEnqueuedCallback != nullptr)
                    {
                        ItemEnqueuedCallback();
                    }
                }

                Queue<Common::ThreadpoolCallback> ThreadpoolCallbacks;
                Queue<MessageProcessingJobItem<ReconfigurationAgent>> MessageQueue;
                Queue<Common::AsyncOperationSPtr> FTQueue;
                Queue<Common::AsyncOperationSPtr> CommitQueue;
                std::function<void()> ItemEnqueuedCallback;
            private:

                template<typename T>
                static void ProcessQueueItem(ReconfigurationAgent &, T &)
                {
                    static_assert(false, "use specialization");
                }

                template<>
                static void ProcessQueueItem<MessageProcessingJobItem<ReconfigurationAgent>>(ReconfigurationAgent & ra, MessageProcessingJobItem<ReconfigurationAgent> & item)
                {
                    item.Process(ra);
                }

                template<>
                static void ProcessQueueItem<Common::AsyncOperationSPtr>(ReconfigurationAgent &, Common::AsyncOperationSPtr & op)
                {
                    op->TryComplete(op);
                }

                template<>
                static void ProcessQueueItem<Common::ThreadpoolCallback>(ReconfigurationAgent &, Common::ThreadpoolCallback & item)
                {
                    item();
                }

                bool drainOnClose_;
                ReconfigurationAgent & ra_;

                bool failEnqueueIntoMessageQueue_;
            };
        }
    }
}
