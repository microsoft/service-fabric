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
    class BatchJobQueueTest
    {
    };
    class TestJobItem
    {};

    class BatchJobRoot : public ComponentRoot
    {
    public:
        BatchJobRoot(int numberOfItems)
            : numberOfItems_(numberOfItems),
            processedItems_(0)
        {
        }

        void AddProcessedItem(int itemSize)
        {
            AcquireWriteLock lock(lock_);

            processedItems_ = processedItems_ + itemSize;
            if (processedItems_ == numberOfItems_) {
                jobProcessingStarted.Set();
            }
        }

        ErrorCode ProcessingCompleted(TimeSpan timespan) {
            return jobProcessingStarted.Wait(timespan);
        }


    private:
        ManualResetEvent jobProcessingStarted;
        RWLOCK(JobQueueTest, lock_);
        int processedItems_;
        int numberOfItems_;
    };
      
    
    //
    // TEST METHODS
    //
    BOOST_FIXTURE_TEST_SUITE(JobQueueTestSuite, BatchJobQueueTest)

   
    BOOST_AUTO_TEST_CASE(SimpleBatchJobQueueTest)
    {
        auto numberOfItemsEnqueued = 10;
        shared_ptr<BatchJobRoot> jobRoot = make_shared<BatchJobRoot>(numberOfItemsEnqueued);

        shared_ptr<BatchJobQueue<TestJobItem, BatchJobRoot>> jobQueue= make_shared<BatchJobQueue
            <TestJobItem, BatchJobRoot>>(
            L"LifoTest",
            [](vector<TestJobItem> & items, BatchJobRoot & root) {
            root.AddProcessedItem(static_cast<int>(items.size()));
        },
            *jobRoot,
            false,
            3);// ThreadCount;
             

            for (int i = 0; i < numberOfItemsEnqueued; i++)
            {
                
                Threadpool::Post([jobQueue]()
                {   
                    TestJobItem item;
                    jobQueue->Enqueue(move(item));
                });
            }
        
        auto errorCode = jobRoot->ProcessingCompleted(TimeSpan::FromSeconds(10));

        VERIFY_IS_TRUE(errorCode.IsSuccess());

        Sleep(2000);

        jobQueue->Close();

    }

    BOOST_AUTO_TEST_SUITE_END()
}
