// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;

namespace Common
{
    class AsyncWorkJobQueueTest
    {
    };

    class AsyncWorkJobRoot : public ComponentRoot
    {
        DENY_COPY(AsyncWorkJobRoot)
    public:
        AsyncWorkJobRoot()
            : timedoutJobs_(0)
            , enqueueFailedJobs_(0)
        {
        }
        void OnEnqueueFailed()
        {
            ++enqueueFailedJobs_;
        }

        long GetEnqueuedFailedJobs()
        {
            return enqueueFailedJobs_.load();
        }

        void OnWorkTimedOut()
        {
            ++timedoutJobs_;
        }

        long GetTimedOutJobs()
        {
            return timedoutJobs_.load();
        }

    private:
        atomic_long timedoutJobs_;
        atomic_long enqueueFailedJobs_;
    };

    class DefaultJobQueueConfigSettingsHolder: public JobQueueConfigSettingsHolder
    {
        DENY_COPY(DefaultJobQueueConfigSettingsHolder)
    public:
        DefaultJobQueueConfigSettingsHolder(int maxThreads, int maxQueueSize, int maxProcessingCount)
            : maxThreads_(maxThreads)
            , maxQueueSize_(maxQueueSize)
            , maxProcessingCount_(maxProcessingCount)
        {
        }

        int get_MaxQueueSize() const override { return maxQueueSize_; }
        int get_MaxParallelPendingWorkCount() const override { return maxProcessingCount_; }
        bool get_TraceProcessingThreads() const override { return true; }
    protected:
        int get_MaxThreadCountValue() const override { return maxThreads_; }
    private:
        int maxThreads_;
        int maxQueueSize_;
        int maxProcessingCount_;
    };

    class TestAsyncWorkJobItem : public AsyncWorkJobItem
    {
        DENY_COPY(TestAsyncWorkJobItem)
    public:
        TestAsyncWorkJobItem(
            std::weak_ptr<AsyncWorkJobRoot> const & owner,
            bool expectedCompletedSync,
            bool useWaitProcessStartSignal = true,
            bool useWaitProcessCallbackSignal = true,
            bool useWaitProcessEndSignal = true,
            bool useWaitCompleteStartSignal = false,
            TimeSpan const & workTimeout = TimeSpan::MaxValue)
            : AsyncWorkJobItem(workTimeout)
            , ExpectedCompletedSync(expectedCompletedSync)
            , owner_(owner)
            , endCalled_()
            , blockStart_(L"BlockStart", useWaitProcessStartSignal)
            , blockCallback_(L"BlockCallback", useWaitProcessCallbackSignal)
            , blockEnd_(L"BlockEnd", useWaitProcessEndSignal)
            , blockCompleteStart_(L"BlockCompleteStart", useWaitCompleteStartSignal)
            , endWorkCalled_(false)
        {
            Trace.WriteInfo("TestAsyncWorkJobItem", "ctor {0}, {1}", this->SequenceNumber, this->State);
        }

        void CheckState(AsyncWorkJobItemState::Enum expectedState)
        {
            for (int i = 0; i < 10; ++i)
            {
                if (this->State == expectedState)
                {
                    return;
                }

                Sleep(100);
            }

            VERIFY_ARE_EQUAL2(this->State, expectedState);
        }

        void SignalAndWaitEnd()
        {
            Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: SignalAndWait, {1}", this->SequenceNumber, this->State);
            SignalProcessStart();
            if (!ExpectedCompletedSync)
            {
                SignalProcessCallback();
            }
            SignalProcessEnd();
            WaitEndDone();
        }

        void SignalProcessStart()
        {
            blockStart_.Set(this->SequenceNumber);
        }

        void SignalProcessCompleteStart()
        {
            blockCompleteStart_.Set(this->SequenceNumber);
        }

        void SignalProcessCallback()
        {
            blockCallback_.Set(this->SequenceNumber);
        }

        void SignalProcessEnd()
        {
            blockEnd_.Set(this->SequenceNumber);
        }

        void WaitEndDone()
        {
            VERIFY_IS_TRUE(endCalled_.WaitOne(60 * 1000));
        }

        void SignalEnd()
        {
            endCalled_.Set();
        }

        virtual void OnEnqueueFailed(ErrorCode && error)
        {
            Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: OnEnqueueFailed {1}", this->SequenceNumber, error);
            auto jobRoot = owner_.lock();
            // The root must be kept alive
            VERIFY_IS_TRUE(jobRoot);
            jobRoot->OnEnqueueFailed();
        }

        bool ExpectedCompletedSync;

    protected:
        void StartWork(
            AsyncWorkReadyToCompleteCallback const & completeCallback,
            __out bool & completedSync) override
        {
            completedSync = this->ExpectedCompletedSync;

            blockStart_.Wait(this->SequenceNumber, this->State);

            if (!completedSync)
            {
                Threadpool::Post([this, completeCallback]()
                {
                    blockCallback_.Wait(this->SequenceNumber, this->State);
                    completeCallback(this->SequenceNumber);
                });
            }

            blockCompleteStart_.Wait(this->SequenceNumber, this->State);
            Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: StartWork returns {1}, state {2}", this->SequenceNumber, completedSync, this->State);
        }

        void EndWork() override
        {
            ASSERT_IF(endWorkCalled_, "{0}: {1}: EndWork should only be called once", this->SequenceNumber, this->State);

            // The root must be kept alive
            VERIFY_IS_TRUE(owner_.lock());

            blockEnd_.Wait(this->SequenceNumber, this->State);

            // Done, notify waiters
            this->SignalEnd();
        }

        void OnStartWorkTimedOut() override
        {
            Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: OnStartWorkTimedOut", this->SequenceNumber);
            ASSERT_IF(this->WorkTimeout == TimeSpan::MaxValue, "{0}: OnStartWorkTimedOut, initial timeout is maxvalue", this->SequenceNumber);
            ASSERT_IF(this->RemainingTime > TimeSpan::Zero, "{0}: OnStartWorkTimedOut, but RemainingTime is not 0: {1}", this->RemainingTime);

            auto jobRoot = owner_.lock();
            // The root must be kept alive
            VERIFY_IS_TRUE(jobRoot);
            jobRoot->OnWorkTimedOut();
        }

    private:
        class BlockSignal
        {
            DENY_COPY(BlockSignal)
        public:
            BlockSignal(std::wstring const & name, bool enable)
                : name_(name)
                , event_()
            {
                if (enable)
                {
                    event_ = make_unique<ManualResetEvent>();
                }
            }

            void Wait(uint64 sequenceNumber, AsyncWorkJobItemState::Enum state)
            {
                if (event_)
                {
                    Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: {1} wait for unblock, state {2}, set = {3}", sequenceNumber, name_, state, event_->IsSet());
                    VERIFY_IS_TRUE(event_->WaitOne(3 * 60 * 1000));
                    Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: {1} unblock completed, state {2}", sequenceNumber, name_, state);
                }
            }

            void Set(uint64 sequenceNumber)
            {
                if (event_)
                {
                    Trace.WriteInfo("TestAsyncWorkJobItem", "{0}: set {1}", sequenceNumber, name_);
                    event_->Set();
                }
            }

        private:
            wstring name_;
            unique_ptr<ManualResetEvent> event_;
        };

        std::weak_ptr<AsyncWorkJobRoot> owner_;
        ManualResetEvent endCalled_;
        BlockSignal blockStart_;
        BlockSignal blockCallback_;
        BlockSignal blockEnd_;
        BlockSignal blockCompleteStart_;
        bool endWorkCalled_;
    };

    using TestAsyncWorkJobItemSPtr = std::shared_ptr<TestAsyncWorkJobItem>;

    template <class T>
    static void CheckJobQueueStatistics(
        AsyncWorkJobQueue<T> const & jobQueue,
        int expectedMaxThreads,
        int expectedMaxQueueSize,
        int expectedMaxProcessingCount,
        int expectedActiveThreads,
        size_t expectedItemCount,
        size_t expectedAsyncReadyItemCount,
        size_t expectedPendingProcessItemCount,
        int retryCount = 10)
    {
        int maxThreads;
        int maxQueueSize;
        int maxProcessingCount;
        int activeThreads;
        size_t itemCount;
        size_t asyncReadyItemCount;
        size_t pendingProcessItemCount;

        while (retryCount > 0)
        {
            jobQueue.Test_GetStatistics(
                maxThreads,
                maxQueueSize,
                maxProcessingCount,
                activeThreads,
                itemCount,
                asyncReadyItemCount,
                pendingProcessItemCount);
            bool areEqual =
                (maxThreads == expectedMaxThreads) &&
                (maxQueueSize == expectedMaxQueueSize) &&
                (maxProcessingCount == expectedMaxProcessingCount) &&
                (activeThreads == expectedActiveThreads) &&
                (itemCount == expectedItemCount) &&
                (asyncReadyItemCount == expectedAsyncReadyItemCount) &&
                (pendingProcessItemCount == expectedPendingProcessItemCount);
            wstring msg;
            StringWriter writer(msg);
            writer.Write(
                "configuration: maxThreads={0}/{1},maxQueueSize={2}/{3},maxPending={4}/{5} ",
                maxThreads,
                expectedMaxThreads,
                maxQueueSize,
                expectedMaxQueueSize,
                maxProcessingCount,
                expectedMaxProcessingCount);
            writer.Write(
                "activeThread={0}/{1}, items={2}/{3}, asyncReady={4}/{5}, pending={6}/{7}",
                activeThreads,
                expectedActiveThreads,
                itemCount,
                expectedItemCount,
                asyncReadyItemCount,
                expectedAsyncReadyItemCount,
                pendingProcessItemCount,
                expectedPendingProcessItemCount);
            if (!areEqual)
            {
                --retryCount;
                if (retryCount > 0)
                {
                    Trace.WriteInfo("AsyncWorkJobQueueTest", "CheckJobQueueStatistics failed: actual/expected: {0}", msg);
                }
                else
                {
                    Trace.WriteError("AsyncWorkJobQueueTest", "CheckJobQueueStatistics failed: actual/expected: {0}", msg);
                }

                Sleep(100);
            }
            else
            {
                Trace.WriteInfo("AsyncWorkJobQueueTest", "CheckJobQueueStatistics: {0}", msg);
                return;
            }
        }

        VERIFY_FAIL(L"CheckJobQueueStatistics failed and no retries left");
    }

    //
    // TEST METHODS
    //
    BOOST_FIXTURE_TEST_SUITE(AsyncWorkJobQueueTestSuite,AsyncWorkJobQueueTest)

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobItemStateTransitions)
    {
        // Tests state transitions for the simplest async work job item
        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        //
        // Completed sync
        //
        {
            TestAsyncWorkJobItem syncJobItem(jobRoot, true, false, false, false);
            syncJobItem.CheckState(AsyncWorkJobItemState::NotStarted);

            auto state = syncJobItem.ProcessJob([](uint64){ VERIFY_FAIL(L"The sync jobs should not call the callback"); });
            VERIFY_ARE_EQUAL2(state, AsyncWorkJobItemState::CompletedSync);
            syncJobItem.CheckState(AsyncWorkJobItemState::CompletedSync);
        }

        //
        // Completed async
        //
        {
            TestAsyncWorkJobItem asyncJobItem(jobRoot, false, false, false, false);
            asyncJobItem.CheckState(AsyncWorkJobItemState::NotStarted);
            auto state = asyncJobItem.ProcessJob(
                [&asyncJobItem, jobRoot](uint64 sequenceNumber)
                {
                    Trace.WriteInfo("TestAsyncWorkJobItemStateTransitions", "{0}: callback invoked, call ProcessJob, state {1}", sequenceNumber, asyncJobItem.State);
                    asyncJobItem.ProcessJob(nullptr);
                });
            VERIFY_ARE_EQUAL2(state, AsyncWorkJobItemState::AsyncPending);

            asyncJobItem.WaitEndDone();
            asyncJobItem.CheckState(AsyncWorkJobItemState::CompletedAsync);
        }
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueStatistics)
    {
        // Add job items to the queue and checks the expected counts in all the inner structures
        int maxThreads = 2;
        int maxQueueSize = 4;
        int maxProcessingCount = maxQueueSize;

        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueStatistics",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Queue is empty
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);

        //
        // Enqueue item that completes sync
        //
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            syncJobItem->CheckState(AsyncWorkJobItemState::NotStarted);
            auto syncJobItemCopy = syncJobItem;

            VERIFY_IS_TRUE(jobQueue.Enqueue(move(syncJobItemCopy)).IsSuccess());

            // job item is placed in items_, a job is created to pick it up and then it's moved to pending items
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 1, 0, 0, 1);

            // Start is blocked
            syncJobItem->CheckState(AsyncWorkJobItemState::NotStarted);

            // Allow start to execute and the ji should complete sync
            syncJobItem->SignalProcessStart();

            // End is blocked, so thread not done yet
            syncJobItem->CheckState(AsyncWorkJobItemState::NotStarted);

            syncJobItem->SignalProcessEnd();

            syncJobItem->WaitEndDone();

            syncJobItem->CheckState(AsyncWorkJobItemState::CompletedSync);

            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);
        }

        //
        // Enqueue item that completes async
        //
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, false);
            asyncJobItem->CheckState(AsyncWorkJobItemState::NotStarted);

            auto asyncJobItemCopy = asyncJobItem;
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItemCopy)).IsSuccess());

            // job item is placed in items and a thread is created to pick it up, at which point is moved to pending items
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, 1);
            asyncJobItem->CheckState(AsyncWorkJobItemState::NotStarted);

            // Allow start to execute
            Trace.WriteInfo("TestAsyncWorkJobQueueStatistics", "Async-{0}: Unblock start", asyncJobItem->SequenceNumber);
            asyncJobItem->SignalProcessStart();

            asyncJobItem->CheckState(AsyncWorkJobItemState::AsyncPending);
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 1);

            // Signal callback, which calls into job queue OnAsyncWorkReadyToComplete.
            // This moves the work to async ready and adds a thread.
            asyncJobItem->SignalProcessCallback();
            // The thread removes the item from active and starts processing.
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, 0);

            asyncJobItem->SignalProcessEnd();
            asyncJobItem->WaitEndDone();

            asyncJobItem->CheckState(AsyncWorkJobItemState::CompletedAsync);
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);
        }

        jobQueue.Close();
        VERIFY_ARE_EQUAL(jobRoot->GetTimedOutJobs(), 0);
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueWithSyncWorkAndQueueFull)
    {
        // Add job items to the queue and checks the expected counts in all the inner structures
        Config cfg;

        int maxThreads = 2;
        int maxQueueSize = 5;
        int maxProcessingCount = maxQueueSize;

        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueWithSyncWorkAndQueueFull",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Queue is empty
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);

        vector<TestAsyncWorkJobItemSPtr> jobs;

        // Because the items that are executed are counted until they complete, the max number of items that
        // can be queued is maxQueueSize.
        for (int i = 0; i < maxQueueSize; ++i)
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            jobs.push_back(syncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(syncJobItem)).IsSuccess());
        }

        // maxThreads are executed, the rest up to maxQueueSize are queued in items_
        // and maxThreads items are in pending.
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads, 0, maxThreads);

        // Since items are blocked, no progress is made and the queue can't accept anymore items
        long rejectCount = 3;
        for (int i = 0; i < rejectCount; ++i)
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(syncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
        }

        VERIFY_ARE_EQUAL(rejectCount, jobRoot->GetEnqueuedFailedJobs());

        // Thread1 execute item [0] and Thread2 item [1].
        // Unblock the first item
        jobs[0]->SignalAndWaitEnd();
        // The thread that completed the first job picks up another
        // Since the queue is full, the policy is Fifo, so the queue chooses last item queued (at index 4)
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads - 1, 0, maxThreads);

        // One work is accepted
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            jobs.push_back(syncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(syncJobItem)).IsSuccess());
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads, 0, maxThreads);
        }

        // Next work is rejected
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(syncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
            CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads, 0, maxThreads);
        }
        VERIFY_ARE_EQUAL(rejectCount + 1, jobRoot->GetEnqueuedFailedJobs());

        // Unblock item on Thread2.
        // Once done, Thread2 pick up last added item, at index 5.
        jobs[1]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads - 1, 0, maxThreads);

        // Unblock the item Thread1, the one at index 4.
        // Thread1 will pick up next item, at index [3]
        jobs[maxQueueSize - 1]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, maxQueueSize - maxThreads - 2, 0, maxThreads);

        // Signal item [5] on Thread2. Thread2 picks up last item, [2]
        jobs[jobs.size() - 1]->SignalAndWaitEnd();

        jobs[2]->SignalAndWaitEnd();

        // One more item in the queue, blocked. One thread is done.
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, 1);

        // Signal the last item
        jobs[3]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);

        // Since the queue is empty, the deque policy is again Fifo
        // Queue more items but do not hit queue full and check the items are picked up using Fifo.
        // Thread1 picks [first] and Thread2 [first+1].
        size_t first = jobs.size();
        int count = maxQueueSize - 1;
        for (int i= 0; i < count; ++i)
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            jobs.push_back(syncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(syncJobItem)).IsSuccess());
        }
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, count - maxThreads, 0, maxThreads);

        // Signal job on Thread1, which will pick up [first+2] (Fifo)
        jobs[first]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, count - maxThreads - 1, 0, maxThreads);

        // Insert one more item, [first+4]
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            jobs.push_back(syncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(syncJobItem)).IsSuccess());
        }
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, count - maxThreads, 0, maxThreads);

        // Signal [first+2], Thread1 picks [first+3], not [first+4]
        jobs[first + 2]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 1, 0, maxThreads);

        jobs[first + 1]->SignalAndWaitEnd();
        jobs[first + 4]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, 1);

        jobs[first + 3]->SignalAndWaitEnd();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);

        jobQueue.Close();
        VERIFY_ARE_EQUAL(jobRoot->GetTimedOutJobs(), 0);

        // Insert after close fails, but enqueuedfailed is not called
        {
            auto syncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(syncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));
        }

        VERIFY_ARE_EQUAL(rejectCount + 1, jobRoot->GetEnqueuedFailedJobs());
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueWithAsyncWorkAndQueueFull)
    {
        // Add job items to the queue and checks the expected counts in all the inner structures
        Config cfg;

        int maxThreads = 2;
        int maxQueueSize = 5;
        int maxProcessingCount = maxQueueSize;

        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueWithAsyncWorkAndQueueFull",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false,
            DequePolicy::Fifo);

        // Queue is empty
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);

        vector<TestAsyncWorkJobItemSPtr> jobs;

        for (int i = 0; i < maxQueueSize; ++i)
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, false);
            jobs.push_back(asyncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItem)).IsSuccess());
        }

        // maxThreads are executed, the rest up to maxQueueSize are queued in items_
        // and maxThreads items are in pending.
        // items = maxQueueSize - maxThreads = 3
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 3, 0, maxThreads);

        // Since items are blocked, no progress is made and the queue can't accept anymore items
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(asyncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
        }

        VERIFY_ARE_EQUAL(1, jobRoot->GetEnqueuedFailedJobs());

        // Thread1 executes item [0] and Thread2 item [1].
        // Unblock Begin for the first item; it transitions to PendingAsync.
        // It's still counted in pending items until callback is called.
        // and the thread can pick up the next item, [2]
        jobs[0]->SignalProcessStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 2, 0, maxThreads + 1);

        // items + pending = max size, reject items
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(asyncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
        }
        VERIFY_ARE_EQUAL(2, jobRoot->GetEnqueuedFailedJobs());

        // Signal begin for second item, Thread2 selects [3]
        jobs[1]->SignalProcessStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 1, 0, maxThreads + 2);

        // Signal callback for [0]. It moves to ready to complete.
        // Since threads are stuck waiting for begin, the async work is not yet scheduled.
        jobs[0]->SignalProcessCallback();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 1, 1, maxThreads + 1);
        // items + pending + async = max size, reject items
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(asyncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
        }
        VERIFY_ARE_EQUAL(3, jobRoot->GetEnqueuedFailedJobs());

        // Signal begin for [3], so Thread2 completes begin [3] and starts processing [0], which is ready async
        jobs[3]->SignalProcessStart();

        jobs[0]->SignalProcessEnd();
        jobs[0]->WaitEndDone();
        // After [0] completes, Thread2 picks up [4]
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 0, 0, maxThreads + 2);

        // 4 items stuck waiting, one more is accepted, [5]
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, false);
            jobs.push_back(asyncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItem)).IsSuccess());
        }

        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 1, 0, maxThreads + 2);
        // queue full again, reject
        {
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(jobRoot, true);
            auto error = jobQueue.Enqueue(move(asyncJobItem));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::JobQueueFull));
        }
        VERIFY_ARE_EQUAL(4, jobRoot->GetEnqueuedFailedJobs());

        // Thread1 is stuck with [2], signal it and it picks up [5]
        jobs[2]->SignalProcessStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 0, 0, maxThreads + 3);

        // Signal Start [5], no items ready so Thread1 is done.
        jobs[5]->SignalProcessStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, maxThreads + 3);

        // Signal Start [4], no items left so Thread2 is done.
        jobs[4]->SignalProcessStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, maxThreads + 3);

        // Make another item ready. Thread is available, so it will start processing,
        // until it gets stuck waiting for end.
        jobs[1]->SignalProcessCallback();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 1, 0, 0, maxThreads + 2);
        jobs[1]->SignalProcessEnd();
        jobs[1]->WaitEndDone();
        // The thread is release, no more ready items
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, maxThreads + 2);

        // Signal all remaining items as being ready.
        // 2 items are picked up and they block on end
        for (int i = 2; i < jobs.size(); i++)
        {
            jobs[i]->SignalProcessCallback();
        }

        // 2 items are executing, picked up from ready items, but not processed as they are blocked on End
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, maxThreads, 0, 2, 0);

        // Timing issue makes it hard to determine which items are picked up first for execution.
        // Signal all of them.
        // The 2 that are waiting will complete and then the threads will pick up the last items.
        for (int i = 2; i < jobs.size(); i++)
        {
            jobs[i]->SignalProcessEnd();
        }

        // Since we don't know which 2 items are done, signal again all items, to reach the non-signaled ones.
        // All should eventually complete
        for (int i = 2; i < jobs.size(); i++)
        {
            jobs[i]->SignalProcessEnd();
            jobs[i]->WaitEndDone();
        }

        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, 0);
        jobQueue.Close();
        VERIFY_ARE_EQUAL(jobRoot->GetTimedOutJobs(), 0);
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueThrottlePendingMaxCount)
    {
        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        int maxQueueSize = 10;
        int maxThreads = 2;
        int maxProcessingCount = 6;

        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueThrottlePendingMaxCount",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Fill the queue.
        vector<TestAsyncWorkJobItemSPtr> jobs;
        for (int i = 0; i < maxQueueSize; ++i)
        {
            // Do not block start and end, just block the callback
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(
                jobRoot,
                false,
                false /*useWaitProcessStartSignal*/,
                true /*useWaitProcessCallbackSignal*/,
                false /*useWaitProcessEndSignal*/,
                false /*useWaitProcessCompleteStartSignal*/);
            jobs.push_back(asyncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItem)).IsSuccess());
        }

        // Since start is not blocked, Thread1 and Thread2 pick up items to execute
        // until the throttle limit is reached.
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, maxQueueSize - maxProcessingCount, 0, maxProcessingCount);

        // Signal callback for the processing threads, they will become ready and complete
        for (int i = 0; i < maxProcessingCount; ++i)
        {
            jobs[i]->SignalProcessCallback();
        }

        for (int i = 0; i < maxProcessingCount; ++i)
        {
            jobs[i]->WaitEndDone();
        }

        // Thread1 and Thread2 complete the async work then pick up the rest of the pending items
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 0, 0, 0, maxQueueSize - maxProcessingCount);

        // Signal the rest of the items, they should complete
        for (int i = maxProcessingCount; i < maxQueueSize; ++i)
        {
            jobs[i]->SignalProcessCallback();
        }

        for (int i = maxProcessingCount; i < maxQueueSize; ++i)
        {
            jobs[i]->WaitEndDone();
        }

        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 0, 0, 0, 0);

        jobQueue.Close();
        VERIFY_ARE_EQUAL(jobRoot->GetTimedOutJobs(), 0);
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueTimedOutJobs)
    {
        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        int maxQueueSize = 10;
        int maxThreads = 2;
        int maxProcessingCount = 6;

        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueTimedOutJobs",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Fill the queue.
        vector<TestAsyncWorkJobItemSPtr> jobs;
        for (int i = 0; i < maxQueueSize; ++i)
        {
            // Block start only for first items, that are picked up by threads
            bool blockStart = (i < maxThreads);
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(
                jobRoot,
                false,
                blockStart /*useWaitProcessStartSignal*/,
                false /*useWaitProcessCallbackSignal*/,
                false /*useWaitProcessEndSignal*/,
                false /*useWaitProcessCompleteStartSignal*/,
                TimeSpan::FromSeconds(1));
            jobs.push_back(asyncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItem)).IsSuccess());
        }

        // Since start is blocked, Thread1 and Thread2 pick up first 2 items and wait until start is invoked
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, maxThreads, maxQueueSize - maxThreads, 0, maxThreads);

        // Wait for the timeout, so items expire
        Sleep(1000);

        // Signal start for the first items. They will execute and then threads will execute all from queue
        for (int i = 0; i < maxThreads; ++i)
        {
            jobs[i]->SignalProcessStart();
        }

        // All items complete
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 0, 0, 0, 0);

        // All items timed out except for the 2 items picked up by the threads originally
        VERIFY_ARE_EQUAL(jobRoot->GetTimedOutJobs(), maxQueueSize - maxThreads);

        jobQueue.Close();
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueParallelStartAndReadyToComplete)
    {
        auto jobRoot = make_shared<AsyncWorkJobRoot>();

        int maxQueueSize = 10;
        int maxThreads = 5;
        int maxProcessingCount = 10;
        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobQueueParallelStartAndReadyToComplete",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Add one async work, where complete start is blocked,
        // to test that async ready to complete is executed in parallel with Process
        auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(
            jobRoot,
            false,
            false /*useWaitProcessStartSignal*/,
            false /*useWaitProcessCallbackSignal*/,
            false /*useWaitProcessEndSignal*/,
            true /*useWaitProcessCompleteStartSignal*/);
        auto jobCopy = asyncJobItem;
        VERIFY_IS_TRUE(jobQueue.Enqueue(move(jobCopy)).IsSuccess());

        // Thread 1 starts the job item, callback is started and then async ready to complete waits for start to be done
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 1, 0, 0, 1);

        // Signal end start and the work will complete
        asyncJobItem->SignalProcessCompleteStart();
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize, maxProcessingCount, 0, 0, 0, 0);

        jobQueue.Close();
    }

    BOOST_AUTO_TEST_CASE(TestAsyncWorkJobQueueClose)
    {
        auto jobRoot = make_shared<AsyncWorkJobRoot>();
        weak_ptr<AsyncWorkJobRoot> jobRootWeak(jobRoot);

        int maxQueueSize = 10;
        int maxThreads = 5;
        int maxProcessingCount = maxQueueSize;
        auto jobRootCopy = jobRoot;
        AsyncWorkJobQueue<TestAsyncWorkJobItemSPtr> jobQueue(
            L"TestAsyncWorkJobItemCloseRootAfterClosed",
            L"",
            move(jobRootCopy),
            make_unique<DefaultJobQueueConfigSettingsHolder>(maxThreads, maxQueueSize, maxProcessingCount),
            false);

        // Add a couple of async work
        int workCount = maxThreads;
        vector<TestAsyncWorkJobItemSPtr> jobs;
        for (int i = 0; i < workCount; ++i)
        {
            // Do not block start and end, just block the callback
            auto asyncJobItem = make_shared<TestAsyncWorkJobItem>(
                jobRoot,
                false,
                false /*useWaitProcessStartSignal*/,
                true /*useWaitProcessCallbackSignal*/,
                false /*useWaitProcessEndSignal*/,
                false /*useWaitProcessCompleteStartSignal*/);
            jobs.push_back(asyncJobItem);
            VERIFY_IS_TRUE(jobQueue.Enqueue(move(asyncJobItem)).IsSuccess());
        }

        // Create more job items, but do not add them yet to the queue
        vector<TestAsyncWorkJobItemSPtr> rejectedJobs;
        int rejectWorkCount = 3;
        for (int i = 0; i < rejectWorkCount; ++i)
        {
            // Do not block start and end, just block the callback
            rejectedJobs.push_back(make_shared<TestAsyncWorkJobItem>(jobRoot, false, false, false, false));
        }

        // Reset the job root, it must be kept alive by the job queue.
        jobRoot.reset();

        // Close job queue, then try to enqueue more items - fails
        jobQueue.Close();

        for (int i = 0; i < rejectWorkCount; ++i)
        {
            auto error = jobQueue.Enqueue(move(rejectedJobs[i]));
            VERIFY_IS_TRUE(error.IsError(ErrorCodeValue::ObjectClosed));
            rejectedJobs[i].reset();
        }

        // Since start is not blocked, all work items are started and are pending callback
        CheckJobQueueStatistics(jobQueue, maxThreads, maxQueueSize,  maxProcessingCount, 0, 0, 0, workCount);

        // Since there are pending items in the queue, the root must not be reset yet.
        for (int i = 0; i < workCount; ++i)
        {
            jobs[i]->SignalProcessCallback();
        }

        // Wait for all items to complete
        for (int i = 0; i < workCount; ++i)
        {
            jobs[i]->WaitEndDone();
        }

        jobs.clear();

        int retryCount = 10;
        while (retryCount > 0)
        {
            auto root = jobRootWeak.lock();
            if (!root)
            {
                // Verified that root is not leaked
                break;
            }

            --retryCount;
            if (retryCount == 0)
            {
                Trace.WriteError("AsyncWorkJobQueueTest", "TestAsyncWorkJobQueueClose: root not reset after close, retry exhausted. Use count={0}", root.use_count());
                VERIFY_FAIL(L"Root not reset after close");
            }

            Trace.WriteInfo("AsyncWorkJobQueueTest", "TestAsyncWorkJobQueueClose: root not reset after close, retry");
            Sleep(1000);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
