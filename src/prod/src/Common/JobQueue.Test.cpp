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
    class JobQueueTest
    {
    };

    class JobRoot : public ComponentRoot
    {
    public:
        JobRoot()
            : processedJobs_(0), timedoutJobs_(0), queueFullJobs_(0)
        {
        }

        __declspec(property(get = get_ProcessedItems)) vector<int> const& ProcessedItems;
        vector<int> const& get_ProcessedItems() { return processedItems_; }

        void AddProcessedItem(int item)
        {
            AcquireWriteLock lock(lock_);

            processedItems_.push_back(item);
        }

        class TestJobItem;                                                 
        class TestJobItemWithWait;
        class TestJobItemWithoutOnQueueFullMethod;
        class TestJobItemWithOnQueueFullMethod;
        class TestJobItemWithCloseWait;

        atomic_long processedJobs_;
        atomic_long timedoutJobs_;
        atomic_long queueFullJobs_;

    private:
        RWLOCK(JobQueueTest, lock_);
        vector<int> processedItems_;
    };

    class JobRoot::TestJobItem : public CommonTimedJobItem<JobRoot>
    {
    public:
        TestJobItem(JobRoot &owner, int itemNumber)
            : CommonTimedJobItem(TimeSpan::Zero)
            , owner_(owner)
            , itemNumber_(itemNumber)
        {
        }

        virtual void Process(JobRoot &)
        {
            ++owner_.processedJobs_;
        }

        virtual void OnTimeout(JobRoot &)
        {
            ++owner_.timedoutJobs_;
        }

        virtual void OnQueueFull(JobRoot &, size_t) override
        {
            ++owner_.queueFullJobs_;
        }

        JobRoot &owner_;
        int itemNumber_;
    };

    class JobRoot::TestJobItemWithWait : public CommonTimedJobItem<JobRoot>
    {
    public:
        TestJobItemWithWait(
            JobRoot &owner, 
            int itemNumber, 
            ManualResetEvent &processingStartedEvent, 
            shared_ptr<ManualResetEvent> blockJobProcessingEvent = nullptr)
            : CommonTimedJobItem(TimeSpan::MaxValue)
            , owner_(owner)
            , itemNumber_(itemNumber)
            , processingStartedEvent_(processingStartedEvent)
            , blockJobProcessingEventPtr_(blockJobProcessingEvent)
        {
        }

        virtual void Process(JobRoot &)
        {
            processingStartedEvent_.Set();
            if (blockJobProcessingEventPtr_)
            {
                blockJobProcessingEventPtr_->WaitOne();
            }

            owner_.AddProcessedItem(itemNumber_);
        }

        virtual void OnTimeout(JobRoot &)
        {
            ++owner_.timedoutJobs_;
        }

        virtual void OnQueueFull(JobRoot &, size_t)
        {
            ++owner_.queueFullJobs_;
        }

        JobRoot &owner_;
        int itemNumber_;
        ManualResetEvent &processingStartedEvent_;
        shared_ptr<ManualResetEvent> blockJobProcessingEventPtr_;
    };

    class JobRoot::TestJobItemWithoutOnQueueFullMethod
    {
    public:

        TestJobItemWithoutOnQueueFullMethod(
            std::shared_ptr<JobRoot> owner = nullptr,
            std::shared_ptr<ManualResetEvent> processingStartedEvent = nullptr,
            std::shared_ptr<AutoResetEvent> blockJobProcessingEvent = nullptr)
            : blockJobProcessingEventPtr_(blockJobProcessingEvent)
            , owner_(owner)
            , processingStartedEvent_(processingStartedEvent)
        {
        }

        bool ProcessJob(JobRoot &)
        {
            Process(*owner_);
            return true;
        }

        virtual void Process(JobRoot &)
        {
            processingStartedEvent_->Set();
            if (blockJobProcessingEventPtr_)
            {
                blockJobProcessingEventPtr_->WaitOne();
            }
        }

        std::shared_ptr<ManualResetEvent> processingStartedEvent_;
        std::shared_ptr<AutoResetEvent> blockJobProcessingEventPtr_;
        std::shared_ptr<JobRoot> owner_;
    };

    class JobRoot::TestJobItemWithOnQueueFullMethod
    {
    public:

        TestJobItemWithOnQueueFullMethod(
            std::shared_ptr<JobRoot> owner = nullptr,
            std::shared_ptr<ManualResetEvent> processingStartedEvent = nullptr,
            std::shared_ptr<AutoResetEvent> blockJobProcessingEvent = nullptr)
            : blockJobProcessingEventPtr_(blockJobProcessingEvent)
            , owner_(owner)
            , processingStartedEvent_(processingStartedEvent)
        {
        }

        bool ProcessJob(JobRoot &)
        {
            Process(*owner_);
            return true;
        }

        virtual void Process(JobRoot &)
        {
            processingStartedEvent_->Set();
            if (blockJobProcessingEventPtr_)
            {
                blockJobProcessingEventPtr_->WaitOne();
            }
        }

        virtual void OnQueueFull(JobRoot &, size_t)
        {
            ++owner_->queueFullJobs_;
        }
    private:

        std::shared_ptr<ManualResetEvent> processingStartedEvent_;
        std::shared_ptr<AutoResetEvent> blockJobProcessingEventPtr_;
        std::shared_ptr<JobRoot> owner_;
    };

    class JobRoot::TestJobItemWithCloseWait
    {
    public:
    
        TestJobItemWithCloseWait(
            std::weak_ptr<JobRoot> owner = weak_ptr<JobRoot>(),
            std::shared_ptr<ManualResetEvent> jobItemCloseStarted = nullptr,
            std::shared_ptr<ManualResetEvent> rootClosedEvent = nullptr)
            : owner_(owner)
            , jobItemCloseStarted_(jobItemCloseStarted)
            , rootClosedEvent_(rootClosedEvent)
        {
        }

        bool ProcessJob(JobRoot &)
        {
            Process(*owner_.lock());
            return true;
        }

        virtual void Process(JobRoot &)
        {
        }

        virtual void Close(JobRoot &)
        {
            jobItemCloseStarted_->Set();
            if (rootClosedEvent_)
            {
                rootClosedEvent_->WaitOne();
            }

            //cannot use boost test macro here as it might run after exiting boost test case
            ASSERT_IFNOT(owner_.lock(), "JobRoot::TestJobItemWithCloseWait::Close: owner_.lock() failed");
        }

        std::weak_ptr<JobRoot> owner_;
        std::shared_ptr<ManualResetEvent> jobItemCloseStarted_;
        std::shared_ptr<ManualResetEvent> rootClosedEvent_;
    };

    class IAsyncJobQueue
    {
    public:
        virtual void OnJobItemComplete() = 0;
    };

    class AsyncJobItem
    {
    public:
        AsyncJobItem(
            __in IAsyncJobQueue & jobQueue,
            __in atomic_long & activeJobCounter,
            int delayMillis)
            : jobQueue_(jobQueue)
            , activeJobCounter_(activeJobCounter)
            , delayMillis_(delayMillis)
            , event_(false)
        {
        }

        bool ProcessJob(ComponentRoot &)
        {
            auto count = ++activeJobCounter_;

            Threadpool::Post([this, count]() 
            { 
                Trace.WriteInfo("AsyncJobItem", "+active={0}", count);

                Sleep(delayMillis_);

                auto postCount = --activeJobCounter_; 

                Trace.WriteInfo("AsyncJobItem", "-active={0}", postCount);
                
                jobQueue_.OnJobItemComplete();
                event_.Set();
            });

            return false;
        }

        void WaitForCompletion()
        {
            event_.WaitOne();
        }

    private:
        IAsyncJobQueue & jobQueue_;
        atomic_long & activeJobCounter_;
        int delayMillis_;
        ManualResetEvent event_;
    };

    class AsyncJobQueue 
        : public JobQueue<shared_ptr<AsyncJobItem>, ComponentRoot>
        , public IAsyncJobQueue
    {
    public:
        AsyncJobQueue(ComponentRoot & root, int limit) : JobQueue(
            L"AsyncJobQueue",
            root,
            false, // forceEnqueue
            limit) // maxThreads
        {
            this->SetAsyncJobs(true);
        }

        void OnJobItemComplete() override
        {
            this->CompleteAsyncJob();
        }
    };

    //
    // TEST METHODS
    //
    BOOST_FIXTURE_TEST_SUITE(JobQueueTestSuite,JobQueueTest)

    BOOST_AUTO_TEST_CASE(AsyncJobQueueTest)
    {
        Config cfg;

        int throttleLimit = 10;
        int jobItemDelayMillis = 1500;
        bool isTestComplete = false;
        ManualResetEvent verifyEvent(false);

        auto root = make_shared<ComponentRoot>();
        auto jobQueue = make_shared<AsyncJobQueue>(*root, throttleLimit);
        atomic_long activeJobCounter(0);
        vector<shared_ptr<AsyncJobItem>> jobItems;

        Threadpool::Post([&]()
        {
            while (!isTestComplete)
            {
                auto count = activeJobCounter.load();
                VERIFY_IS_TRUE(count <= throttleLimit);

                Sleep(500);
            }

            verifyEvent.Set();
        });

        for (auto iteration=0; iteration<2; ++iteration)
        {
            Trace.WriteInfo("AsyncJobQueueTest", "*** Test iteration {0}", iteration+1);

            jobItems.clear();

            for (auto ix=0; ix<throttleLimit * 10; ++ix)
            {
                jobItems.push_back(make_shared<AsyncJobItem>(*jobQueue, activeJobCounter, jobItemDelayMillis));
            }

            VERIFY_IS_TRUE(activeJobCounter.load() == 0);

            for (auto const & jobItem : jobItems)
            {
                auto job = jobItem;
                jobQueue->Enqueue(move(job));
            }

            for (auto const & jobItem : jobItems)
            {
                jobItem->WaitForCompletion();
            }

            VERIFY_IS_TRUE(activeJobCounter.load() == 0);

            VERIFY_IS_TRUE(jobQueue->Test_HighestActiveThreads == throttleLimit);

            jobQueue->Test_ResetHighestActiveThreads();
        }

        isTestComplete = true;

        verifyEvent.WaitOne();
    }

    BOOST_AUTO_TEST_CASE(JobItemsTimeoutTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();
        unique_ptr<CommonTimedJobQueue<JobRoot>> jobQueue;

        jobQueue = make_unique<CommonTimedJobQueue<JobRoot>>(
            L"JobQueueTest",
            *jobRoot,
            false,
            0);

        for (int i = 0; i < 1000; ++i)
        {
            jobQueue->Enqueue(make_unique<JobRoot::TestJobItem>(*jobRoot, i));
        }

        Sleep(1000 * 30);
        VERIFY_IS_TRUE(jobRoot->processedJobs_.load() == 0);
        VERIFY_IS_TRUE(jobRoot->timedoutJobs_.load() == 1000);
        jobQueue->Close();
    }

    BOOST_AUTO_TEST_CASE(QueueFullMethodCallTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();
        unique_ptr<JobQueue<JobRoot::TestJobItemWithOnQueueFullMethod, JobRoot>> jobQueueNonEncapsulated;

        jobQueueNonEncapsulated = make_unique<JobQueue<JobRoot::TestJobItemWithOnQueueFullMethod, JobRoot>>(
            L"QueueFullTestNoEncapsulationNoMethod",
            *jobRoot,
            false,
            1,
            nullptr,
            5
            );

        int enqueueFailuresWithMethod = 0;
        std::shared_ptr<ManualResetEvent> jobProcessingStarted = make_shared<ManualResetEvent>();
        std::shared_ptr<AutoResetEvent> event = make_shared<AutoResetEvent>();
        
        VERIFY_IS_TRUE(jobQueueNonEncapsulated->Enqueue(JobRoot::TestJobItemWithOnQueueFullMethod(jobRoot, jobProcessingStarted, event)));
        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted->WaitOne();

        for (int i = 1; i < 10; ++i)
        {
            if (!jobQueueNonEncapsulated->Enqueue(JobRoot::TestJobItemWithOnQueueFullMethod(jobRoot, jobProcessingStarted)))
            {
                ++enqueueFailuresWithMethod;
            }
        }

        VERIFY_IS_TRUE(enqueueFailuresWithMethod == 4);
        VERIFY_IS_TRUE(jobRoot->queueFullJobs_.load() == 4);
        event->Set();
        Sleep(1000 * 10);
        jobQueueNonEncapsulated->Close();
    }

/* Test case disabled due to flakiness from timing issue. TODO: fix timing issue.
    BOOST_AUTO_TEST_CASE(JobItemCloseRootAfterClosed)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();
        unique_ptr<JobQueue<JobRoot::TestJobItemWithCloseWait, JobRoot>> jobQueue;

        jobQueue = make_unique<JobQueue<JobRoot::TestJobItemWithCloseWait, JobRoot>>(
            L"JobQueueWithCloseWaitTest",
            *jobRoot);

        std::shared_ptr<ManualResetEvent> jobItemCloseStarted = make_shared<ManualResetEvent>();
        std::shared_ptr<ManualResetEvent> rootClosed = make_shared<ManualResetEvent>();
        
        VERIFY_IS_TRUE(jobQueue->Enqueue(JobRoot::TestJobItemWithCloseWait(jobRoot, jobItemCloseStarted, rootClosed)));
        //
        // Wait for the job queue thread to call Close, then close the root before it dereferences it
        //
        jobItemCloseStarted->WaitOne();
        jobRoot.reset();
        jobQueue->Close();
        rootClosed->Set();
    }
*/

    BOOST_AUTO_TEST_CASE(QueueFullMethodCallTestNoMethodInJobItem)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();

        unique_ptr<JobQueue<JobRoot::TestJobItemWithoutOnQueueFullMethod, JobRoot>> jobQueueNonEncapsulatedNoMethod;

        jobQueueNonEncapsulatedNoMethod = make_unique<JobQueue<JobRoot::TestJobItemWithoutOnQueueFullMethod, JobRoot>>(
            L"QueueFullTestNoEncapsulationWithMethod",
            *jobRoot,
            false,
            1,
            nullptr,
            5
            );

        int enqueueFailuresNoMethod = 0;
        std::shared_ptr<ManualResetEvent> jobProcessingStarted = make_shared<ManualResetEvent>();
        std::shared_ptr<AutoResetEvent> event = make_shared<AutoResetEvent>();

        VERIFY_IS_TRUE(jobQueueNonEncapsulatedNoMethod->Enqueue(JobRoot::TestJobItemWithoutOnQueueFullMethod(jobRoot, jobProcessingStarted, event)));

        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted->WaitOne();

        for (int i = 1; i < 10; ++i)
        {
            if (!jobQueueNonEncapsulatedNoMethod->Enqueue(JobRoot::TestJobItemWithoutOnQueueFullMethod(jobRoot, jobProcessingStarted)))
            {
                ++enqueueFailuresNoMethod;
            }
        }

        VERIFY_IS_TRUE(enqueueFailuresNoMethod == 4);
        VERIFY_IS_TRUE(jobRoot->queueFullJobs_.load() == 0);
        event->Set();
        Sleep(1000 * 10);
        jobQueueNonEncapsulatedNoMethod->Close();
    }

    BOOST_AUTO_TEST_CASE(QueueFullTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();
        unique_ptr<CommonTimedJobQueue<JobRoot>> jobQueue;

        jobQueue = make_unique<CommonTimedJobQueue<JobRoot>>(
            L"QueueFullTest",
            *jobRoot,
            false,
            1,
            nullptr,
            5);

        int enqueueFailures = 0;
        ManualResetEvent jobProcessingStarted;
        shared_ptr<ManualResetEvent> event = make_shared<ManualResetEvent>();
        VERIFY_IS_TRUE(jobQueue->Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, 0, jobProcessingStarted, event)));
        
        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted.WaitOne();

        for (int i = 1; i < 10; ++i)
        {
            if (!jobQueue->Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, i, jobProcessingStarted)))
            {
                ++enqueueFailures;
            }
        }

        VERIFY_IS_TRUE(enqueueFailures == 4);
        VERIFY_IS_TRUE(jobRoot->queueFullJobs_.load() == 4);
        event->Set();
        Sleep(1000 * 10);
        jobQueue->Close();
    }

    BOOST_AUTO_TEST_CASE(FifoTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();

        CommonTimedJobQueue<JobRoot> jobQueue(
            L"LifoTest",
            *jobRoot,
            false, // ForceEnqueue
            1, // ThreadCount
            nullptr, // PerformanceCounters
            5,  // QueueSize
            Common::DequePolicy::Fifo);

        ManualResetEvent jobProcessingStarted;
        shared_ptr<ManualResetEvent> jobProcessingBlocked = make_shared<ManualResetEvent>();

        int numberOfEnqueuedItems = 0;
        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted.WaitOne();


        for (int i = 0; i < 10; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 2 3 4 5 6)");

        //
        // Enqueue some some items to the job queue. The processing order should remain Fifo
        //
        jobProcessingStarted.Reset();
        jobProcessingBlocked->Reset();

        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        jobProcessingStarted.WaitOne();

        for (int i = 0; i < 3; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 2 3 4 5 6 7 8 9 10)");

        jobQueue.Close();

    }

    BOOST_AUTO_TEST_CASE(LifoTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();

        CommonTimedJobQueue<JobRoot> jobQueue(
            L"LifoTest",
            *jobRoot,
            false, // ForceEnqueue
            1, // ThreadCount
            nullptr, // PerformanceCounters
            5,  // QueueSize
            Common::DequePolicy::Lifo);

        ManualResetEvent jobProcessingStarted;
        shared_ptr<ManualResetEvent> jobProcessingBlocked = make_shared<ManualResetEvent>();

        int numberOfEnqueuedItems = 0;
        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted.WaitOne();


        for (int i = 0; i < 10; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 6 5 4 3 2)");

        //
        // Enqueue some some items to the job queue. The processing order should remain Lifo
        //
        jobProcessingStarted.Reset();
        jobProcessingBlocked->Reset();

        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        jobProcessingStarted.WaitOne();

        for (int i = 0; i < 3; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 6 5 4 3 2 7 10 9 8)");

        jobQueue.Close();

    }

    BOOST_AUTO_TEST_CASE(FifoLifoTest)
    {
        shared_ptr<JobRoot> jobRoot = make_shared<JobRoot>();

        CommonTimedJobQueue<JobRoot> jobQueue(
            L"LifoTest",
            *jobRoot,
            false, // ForceEnqueue
            1, // ThreadCount
            nullptr, // PerformanceCounters
            5,  // QueueSize
            Common::DequePolicy::FifoLifo);

        ManualResetEvent jobProcessingStarted;
        shared_ptr<ManualResetEvent> jobProcessingBlocked = make_shared<ManualResetEvent>();

        int numberOfEnqueuedItems = 0;
        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        //
        // Wait for the job queue thread pickup the first job and block, so that additional items get queued in the jobqueue
        //
        jobProcessingStarted.WaitOne();

        
        for (int i = 0; i < 10; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 6 5 4 3 2)");

        //
        // Enqueue some items to the job queue. The processing order should remain should back to Fifo
        //
        jobProcessingStarted.Reset();
        jobProcessingBlocked->Reset();

        VERIFY_IS_TRUE(jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted, jobProcessingBlocked)));
        numberOfEnqueuedItems++;

        jobProcessingStarted.WaitOne();

        for (int i = 0; i < 3; i++)
        {
            if (jobQueue.Enqueue(make_unique<JobRoot::TestJobItemWithWait>(*jobRoot, numberOfEnqueuedItems + 1, jobProcessingStarted)))
            {
                numberOfEnqueuedItems++;
            }
        }

        jobProcessingBlocked->Set();

        Sleep(1000 * 10);

        VERIFY_ARE_EQUAL(jobRoot->ProcessedItems.size(), numberOfEnqueuedItems);
        VERIFY_ARE_EQUAL(wformatString(jobRoot->ProcessedItems), L"(1 6 5 4 3 2 7 8 9 10)");

        jobQueue.Close();

    }

    BOOST_AUTO_TEST_SUITE_END()
}
