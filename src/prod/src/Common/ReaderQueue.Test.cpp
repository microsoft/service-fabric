// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace Common
{
    using namespace std;


    typedef shared_ptr<AsyncOperation> AsyncOperationSPtr;
    typedef shared_ptr<ReaderQueue<int>> ReaderQueueSPtr;

    static ManualResetEvent readerDoneEvent(false);

    //
    // Basic enqueue/dequeue sequence
    //
    BOOST_AUTO_TEST_SUITE(TestReaderQueue)

    BOOST_AUTO_TEST_CASE(BasicEnqueueDequeue)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();
        unique_ptr<int> resultSPtr;

        bool success = queue->EnqueueAndDispatch(make_unique<int>(142));
        BOOST_TEST_MESSAGE(formatString("Enqueue result = '{0}'", success));
        VERIFY_IS_TRUE(success);

        readerDoneEvent.Reset();

        AsyncOperationSPtr contextSPtr = queue->BeginDequeue(
            TimeSpan::MaxValue, 
            [&queue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void
            {
                ErrorCode err = queue->EndDequeue(operationSPtr, resultSPtr);
                    
                BOOST_TEST_MESSAGE(formatString("Error code = '{0}'", static_cast<int>(err.ReadValue())));
                VERIFY_IS_TRUE(err.IsError(ErrorCodeValue::Success));

                readerDoneEvent.Set();
            }, 
            AsyncOperationSPtr());

        readerDoneEvent.WaitOne(TimeSpan::MaxValue);

        BOOST_TEST_MESSAGE(formatString("BeginDequeue use_count = '{0}'", contextSPtr.use_count()));
        VERIFY_IS_TRUE(1 == contextSPtr.use_count());

        // Abort before accessing result
        queue->Abort();

        BOOST_TEST_MESSAGE(formatString("Dequeue result = '{0}'", *resultSPtr.get()));
        VERIFY_IS_TRUE(142 == *resultSPtr.get());
    }
        
    //
    // Basic dequeue/enqueue - dequeue completion should wait for enqueue
    //
    BOOST_AUTO_TEST_CASE(BasicDequeueEnqueue)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();
            
        queue->BeginDequeue(
            TimeSpan::MaxValue,
            [&queue] (AsyncOperationSPtr contextSPtr)
            -> void
            {
                unique_ptr<int> resultSPtr;
                ErrorCode err = queue->EndDequeue(contextSPtr, resultSPtr);

                BOOST_TEST_MESSAGE(formatString("Dequeue result = '{0}'", *resultSPtr.get()));
                VERIFY_IS_TRUE(242 == *resultSPtr.get());

                BOOST_TEST_MESSAGE(formatString(
                    "Error code = '{0}'", 
                    static_cast<int>(err.ReadValue())));
                VERIFY_IS_TRUE(err.IsError(ErrorCodeValue::Success));
            }, 
            AsyncOperationSPtr());

        // Don't need waitHandle in this testcase - the following call should dispatch in the same thread
        bool success = queue->EnqueueAndDispatch(make_unique<int>(242));

        BOOST_TEST_MESSAGE(formatString("Enqueue result = '{0}'", success));
        VERIFY_IS_TRUE(success);

        queue->Abort();
    }
        
    //
    // Basic abort behavior - blocks enqueues and dequeues
    //
    BOOST_AUTO_TEST_CASE(BasicAbort)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();

        for (int ix = 0; ix < 3; ++ix)
        {
            bool success = queue->EnqueueAndDispatch(make_unique<int>(3421 + ix));
            BOOST_TEST_MESSAGE(formatString("Enqueue result = '{0}'", success));
            VERIFY_IS_TRUE(success);
        }

        BOOST_TEST_MESSAGE(formatString("Item Count = '{0}'", queue->ItemCount));
        VERIFY_IS_TRUE(3 == queue->ItemCount);

        queue->Abort();

        ManualResetEvent queueDrainedEvent;
        Stopwatch stopwatch;

        stopwatch.Start();

        queue->BeginWaitForQueueToDrain(
            [&queue, &queueDrainedEvent, &stopwatch](AsyncOperationSPtr const & asyncOperation) -> void
            {
                stopwatch.Stop();

                ErrorCode err = queue->EndWaitForQueueToDrain(asyncOperation);
                VERIFY_IS_TRUE(
                    err.IsSuccess(),
                    wformatString(
                        "Scheduled WaitForQueueToDrain returned error code = '{0}'", 
                        static_cast<int>(err.ReadValue())).c_str());
                queueDrainedEvent.Set();

                VERIFY_IS_TRUE(queue->ItemCount == 0, L"Queue has 0 items after WaitForQueueToDrain returns");
            },
            AsyncOperationSPtr());

        // Since the items are cleared on abort,
        // this should return immediately
        queueDrainedEvent.WaitOne();
        TimeSpan accuracyMargin = TimeSpan::FromMilliseconds(1000);
        BOOST_TEST_MESSAGE(formatString(
            "Delay WaitForQueueToDrain = '{0}'. Acceptable delay = '{1}'", 
            stopwatch.ElapsedMilliseconds,
            accuracyMargin));
        VERIFY_IS_TRUE(stopwatch.Elapsed <= accuracyMargin, L"After Abort, WaitForQueueToDrain should return immediately");
                      
        for (int ix = 0; ix < 2; ++ix)
        {
            bool success = queue->EnqueueAndDispatch(make_unique<int>(3424 + ix));
            BOOST_TEST_MESSAGE(formatString("Enqueue result = '{0}'", success));
            VERIFY_IS_FALSE(success);

            unique_ptr<int> resultSPtr;
            readerDoneEvent.Reset();
                
            queue->BeginDequeue(
                TimeSpan::MaxValue, 
                [&queue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    ErrorCode err = queue->EndDequeue(operationSPtr, resultSPtr);
        
                    BOOST_TEST_MESSAGE(formatString(
                        "Error code = '{0}'", 
                        static_cast<int>(err.ReadValue())));
                    VERIFY_IS_TRUE(err.IsError(ErrorCodeValue::Success));

                    readerDoneEvent.Set();
                }, 
                AsyncOperationSPtr());
                
            readerDoneEvent.WaitOne(TimeSpan::MaxValue);

            BOOST_TEST_MESSAGE(formatString(
                "Dequeue result = '{0}'", 
                (resultSPtr.get() == nullptr ? 0 : *resultSPtr.get())));
            VERIFY_IS_TRUE(nullptr == resultSPtr.get());
        }

        BOOST_TEST_MESSAGE(formatString("Item Count = '{0}'", queue->ItemCount));
        VERIFY_IS_TRUE(0 == queue->ItemCount);
    }

    //
    // Basic close behavior - blocks Enqueue(), allows Dequeue()
    // Basic abort behavior - blocks both operations
    //
    BOOST_AUTO_TEST_CASE(BasicCloseAbort)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();

        for (int ix = 0; ix < 3; ++ix)
        {
            bool success = queue->EnqueueAndDispatch(make_unique<int>(3421 + ix));
            VERIFY_IS_TRUE(
                success, 
                wformatString("Enqueue result = '{0}'", success).c_str());
        }

        queue->Close();

        DateTime queueDrainedTime;
        ManualResetEvent queueDrainedEvent;
        queue->BeginWaitForQueueToDrain(
            [&queue, &queueDrainedTime, &queueDrainedEvent](AsyncOperationSPtr const & asyncOperation) -> void
            {
                ErrorCode err = queue->EndWaitForQueueToDrain(asyncOperation);
                VERIFY_IS_TRUE(
                    err.IsSuccess(),
                    wformatString(
                        "Scheduled WaitForQueueToDrain returned error code = '{0}'", 
                        static_cast<int>(err.ReadValue())).c_str());
                VERIFY_IS_TRUE(queue->ItemCount == 0, L"Queue has 0 items after WaitForQueueToDrain returns");
                queueDrainedTime = DateTime::Now();
                queueDrainedEvent.Set();
            },
            AsyncOperationSPtr());

        for (int ix = 0; ix < 2; ++ix)
        {
            bool success = queue->EnqueueAndDispatch(make_unique<int>(3424 + ix));
            VERIFY_IS_FALSE(success, wformatString("Enqueue result = '{0}'", success).c_str());
        }

        for (int ix = 0; ix < 5; ++ix)
        {
            unique_ptr<int> resultSPtr;
                
            readerDoneEvent.Reset();

            queue->BeginDequeue(
                TimeSpan::MaxValue, 
                [&queue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void 
                {
                    ErrorCode err = queue->EndDequeue(operationSPtr, resultSPtr);
                    VERIFY_IS_TRUE(
                        err.IsError(ErrorCodeValue::Success), 
                        wformatString(
                            "Error code = '{0}'", 
                            static_cast<int>(err.ReadValue())).c_str());

                    readerDoneEvent.Set();
                }, 
                AsyncOperationSPtr());

            readerDoneEvent.WaitOne(TimeSpan::MaxValue);

            BOOST_TEST_MESSAGE(formatString(
                "Dequeue result = '{0}'", 
                (resultSPtr.get() == nullptr ? 0 : *resultSPtr.get())));

            if (ix < 3)
            {
                VERIFY_IS_TRUE((3421 + ix) == *resultSPtr.get(), L"Dequeue returned correct number");
            }
            else
            {
                VERIFY_IS_TRUE(nullptr == resultSPtr.get(), L"Dequeue returned expected null");
            }
        }

        queueDrainedEvent.WaitOne();
        VERIFY_IS_TRUE(queueDrainedTime <= DateTime::Now(), L"Queue should be drained when all items are dequeued");
        
        queue->Abort();

        for (int ix = 0; ix < 2; ++ix)
        {
            bool success = queue->EnqueueAndDispatch(make_unique<int>(3424 + ix));
            VERIFY_IS_FALSE(
                success, 
                wformatString("Enqueue result = '{0}'", success).c_str());

            unique_ptr<int> resultSPtr;
                
            readerDoneEvent.Reset();

            queue->BeginDequeue(
                TimeSpan::MaxValue,
                [&queue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void
                {
                    ErrorCode err = queue->EndDequeue(operationSPtr, resultSPtr);

                    VERIFY_IS_TRUE(
                        err.IsError(ErrorCodeValue::Success), 
                        wformatString("Error code = '{0}'", static_cast<int>(err.ReadValue())).c_str());
                        
                    readerDoneEvent.Set();
                },
                AsyncOperationSPtr());

            readerDoneEvent.WaitOne();
                
            VERIFY_IS_TRUE(
                nullptr == resultSPtr.get(), 
                wformatString("Dequeue result = '{0}'", (resultSPtr.get() == nullptr ? 0 : *resultSPtr.get())).c_str());
        }
    }

    //
    // Asynchronous Timeout
    //
    BOOST_AUTO_TEST_CASE(AsynchronousTimeout)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();

        ManualResetEvent waitHandle(false);

        Stopwatch stopwatch;

        stopwatch.Start();

        queue->BeginDequeue(
            TimeSpan::FromSeconds(5),
            [&waitHandle, &queue, &stopwatch](AsyncOperationSPtr contextSPtr)
            -> void
            {
                stopwatch.Stop();

                unique_ptr<int> resultSPtr;
                ErrorCode err = queue->EndDequeue(contextSPtr, resultSPtr);
                BOOST_TEST_MESSAGE(formatString("Dequeue result = '{0}'", (resultSPtr.get() == nullptr ? 0 : *resultSPtr.get())));
                BOOST_TEST_MESSAGE(formatString("Error code = '{0}'", static_cast<int>(err.ReadValue())));

                VERIFY_IS_TRUE(nullptr == resultSPtr.get());
                VERIFY_IS_TRUE(err.IsError(ErrorCodeValue::Timeout));
                waitHandle.Set();
            },
            AsyncOperationSPtr());

        waitHandle.WaitOne(TimeSpan::MaxValue);

        BOOST_TEST_MESSAGE(formatString("Delay = '{0}'", stopwatch.ElapsedMilliseconds));
        TimeSpan accuracyMargin = TimeSpan::FromMilliseconds(1000);
        VERIFY_IS_TRUE((stopwatch.Elapsed + accuracyMargin) >= TimeSpan::FromSeconds(5));

        BOOST_TEST_MESSAGE(formatString("Pending readers = '{0}'", queue->PendingReaderCount));
        VERIFY_IS_TRUE(0 == queue->PendingReaderCount);

        BOOST_TEST_MESSAGE(formatString("Pending dispatch = '{0}'", queue->PendingDispatchCount));
        VERIFY_IS_TRUE(0 == queue->PendingDispatchCount);
    }

    //
    // Basic enqueue but dispatch later
    //
    BOOST_AUTO_TEST_CASE(BasicEnqueueWithoutDispatch)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();

        queue->BeginDequeue(
            TimeSpan::MaxValue,
            [&queue] (AsyncOperationSPtr contextSPtr)
            -> void
            {
                unique_ptr<int> resultSPtr;
                ErrorCode err = queue->EndDequeue(contextSPtr, resultSPtr);

                BOOST_TEST_MESSAGE(formatString("Error code = '{0}'", static_cast<int>(err.ReadValue())));
                VERIFY_IS_TRUE(err.IsError(ErrorCodeValue::Success));

                BOOST_TEST_MESSAGE(formatString("Dequeue result = '{0}'", *resultSPtr.get()));
                VERIFY_IS_TRUE(442 == *resultSPtr.get());

            }, 
            AsyncOperationSPtr());

        BOOST_TEST_MESSAGE(formatString("Pending readers = '{0}'", queue->PendingReaderCount));
        VERIFY_IS_TRUE(1 == queue->PendingReaderCount);

        BOOST_TEST_MESSAGE(formatString("Pending dispatch = '{0}'", queue->PendingDispatchCount));
        VERIFY_IS_TRUE(0 == queue->PendingDispatchCount);

        bool success = queue->EnqueueWithoutDispatch(make_unique<int>(442));
        BOOST_TEST_MESSAGE(formatString("Enqueue result = '{0}'", success));
        VERIFY_IS_TRUE(success);

        BOOST_TEST_MESSAGE(formatString("Pending readers = '{0}'", queue->PendingReaderCount));
        VERIFY_IS_TRUE(0 == queue->PendingReaderCount);

        BOOST_TEST_MESSAGE(formatString("Pending dispatch = '{0}'", queue->PendingDispatchCount));
        VERIFY_IS_TRUE(1 == queue->PendingDispatchCount);

        queue->Dispatch();

        BOOST_TEST_MESSAGE(formatString("Pending readers = '{0}'", queue->PendingReaderCount));
        VERIFY_IS_TRUE(0 == queue->PendingReaderCount);

        BOOST_TEST_MESSAGE(formatString("Pending dispatch = '{0}'", queue->PendingDispatchCount));
        VERIFY_IS_TRUE(0 == queue->PendingDispatchCount);

        queue->Abort();
    }

    //
    // Basic multi-threaded test
    //
    BOOST_AUTO_TEST_CASE(BasicMultiThreaded)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();
        ManualResetEvent waitHandle(false);
        const int numItems = 100;

        Threadpool::Post([&queue,&waitHandle, &numItems]() -> void
            {
                for (int ix = 0; ix < numItems; ++ix)
                {
                    queue->EnqueueAndDispatch(make_unique<int>(ix));
                }
            });

        int results[numItems];
        Threadpool::Post([&queue,&waitHandle,&results,&numItems]() -> void
            {
                int count = 0;
                unique_ptr<int> resultSPtr;
                ReaderQueueSPtr &innerQueue = queue;

                do
                {
                    readerDoneEvent.Reset();

                    innerQueue->BeginDequeue(
                        TimeSpan::MaxValue,
                        [&innerQueue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            ErrorCode err = innerQueue->EndDequeue(operationSPtr, resultSPtr);
                            readerDoneEvent.Set();
                            if(!err.IsSuccess()) 
                            {
                                VERIFY_FAIL(L"EndDequeue returned error");
                            }
                        },
                        AsyncOperationSPtr());

                    readerDoneEvent.WaitOne();

                    const int index = *resultSPtr.get();
                    results[index] = index;

                    ++count;
                } while (count < numItems);

                waitHandle.Set();
            });

        BOOST_TEST_MESSAGE(formatString("Waiting for queue to drain ..."));
        waitHandle.WaitOne();

        BOOST_TEST_MESSAGE(formatString("Verifying item values ..."));
        for (int ix = 0; ix < numItems; ++ix)
        {
            if(ix != results[ix])
            {
                BOOST_FAIL(formatString("Error: {0} != {1}", ix, results[ix]));
            }
        }
        BOOST_TEST_MESSAGE(formatString("Queue drained. Item values verified."));

        queue->Abort();
    }
        
    //
    // Slow readers multi-threaded test
    // Readers should continue to drain queue after writers are done
    //
    BOOST_AUTO_TEST_CASE(SlowReadersMultiThreaded)
    {
        ReaderQueueSPtr queue = ReaderQueue<int>::Create();
        ManualResetEvent waitHandle(false);
        const int numItems = 100;

        Threadpool::Post([&queue,&waitHandle, &numItems]() -> void
            {
                for (int ix = 0; ix < numItems; ++ix)
                {
                    queue->EnqueueAndDispatch(make_unique<int>(ix));
                }

                queue->Close();
            });

        int results[numItems];
        Threadpool::Post([&queue,&waitHandle,&results,&numItems]() -> void
            {
                int count = 0;
                unique_ptr<int> resultSPtr;
                ReaderQueueSPtr &innerQueue = queue;

                do
                {
                    readerDoneEvent.Reset();
                    innerQueue->BeginDequeue(
                        TimeSpan::MaxValue,
                        [&innerQueue, &resultSPtr](AsyncOperationSPtr const & operationSPtr) -> void
                        {
                            ErrorCode err = innerQueue->EndDequeue(operationSPtr, resultSPtr);
                            readerDoneEvent.Set();
                            if(!err.IsSuccess()) 
                            {
                                VERIFY_FAIL(L"EndDequeue returned error");
                            }
                        },
                        AsyncOperationSPtr());

                    readerDoneEvent.WaitOne(TimeSpan::MaxValue);

                    const int index = *resultSPtr.get();
                    results[index] = index;

                    ++count;
                    Sleep(20);
                } while (count < numItems);

                waitHandle.Set();
            });

        BOOST_TEST_MESSAGE(formatString("Waiting for queue to drain ..."));
        waitHandle.WaitOne();

        BOOST_TEST_MESSAGE(formatString("Verifying item values ..."));
        for (int ix = 0; ix < numItems; ++ix)
        {
            if(ix != results[ix])
            {
                BOOST_FAIL(formatString("Error: {0} != {1}", ix, results[ix]));
            }
        }
        BOOST_TEST_MESSAGE(formatString("Queue drained. Item values verified."));

        queue->Abort();
    }

    BOOST_AUTO_TEST_SUITE_END()
}
