// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"
#include "ReliableConcurrentQueue.h"
#include "ReliableConcurrentQueue.TestBase.h"
#include "../tstore/TStoreTestBase.h"

namespace ReliableConcurrentQueueTests
{
    using namespace ktl;
    using namespace Data::Utilities;
    using namespace Data::Collections;
    using namespace TxnReplicator;

    class ReliableConcurrentQueueBasicOperations : public ReliableConcurrentQueueTestBase<int>
    {
    public:
        Common::CommonConfig config;

    public:
        Awaitable<void> Test_Single_Enqueue_Commit_Dequeue_Commit() noexcept;
        Awaitable<void> Test_Multiple_Enqueue_Commit_Dequeue_Commit() noexcept;
        Awaitable<void> Test_Single_Enqueue_Dequeue_Commit() noexcept;
        Awaitable<void> Test_Multiple_Enqueue_Dequeue_Commit() noexcept;
        Awaitable<void> Test_Dequeue_EmptyQueue() noexcept;
        Awaitable<void> Test_TwoDequeue_EmptyQueue() noexcept;
        Awaitable<void> Test_Enqueue_TwoDequeue_SameTransaction() noexcept;
        Awaitable<void> Test_Enqueue_TwoDequeue_DifferentTransactions() noexcept;
        Awaitable<void> Test_Enqueue_TwoDequeue_AllInDifferentTransactions() noexcept;
        Awaitable<void> Test_Cross_Segments() noexcept;
        Awaitable<void> Test_Enqueue_Abort() noexcept;
        Awaitable<void> Test_Dequeue_Abort() noexcept;
        Awaitable<void> Test_Enqueue_Dequeue_Abort() noexcept;

    public:
        // typedef Awaitable<void> (ReliableConcurrentQueuePerf::*PrintExecutionFunctionType)(int);
        typedef KDelegate<Awaitable<void>(int)> PrintExecutionFunctionType;
    };

    BOOST_FIXTURE_TEST_SUITE(ReliableConcurrentQueueBasicOperationsSuite, ReliableConcurrentQueueBasicOperations)

    BOOST_AUTO_TEST_CASE(Single_Enqueue_Commit_Dequeue_Commit)
    {
        SyncAwait(Test_Single_Enqueue_Commit_Dequeue_Commit());
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_Commit_Dequeue_Commit)
    {
        SyncAwait(Test_Multiple_Enqueue_Commit_Dequeue_Commit());
    }

    BOOST_AUTO_TEST_CASE(Cross_Segments)
    {
        SyncAwait(Test_Cross_Segments());
    }

    BOOST_AUTO_TEST_CASE(Single_Enqueue_Dequeue_Commit)
    {
        // Enable once we have "Read your own write"
        // SyncAwait(Test_Single_Enqueue_Dequeue_Commit());
    }

    BOOST_AUTO_TEST_CASE(Multiple_Enqueue_Dequeue_Commit)
    {
        // Enable once we have "Read your own write"
        // SyncAwait(Test_Multiple_Enqueue_Dequeue_Commit());
    }

    BOOST_AUTO_TEST_CASE(Dequeue_EmptyQueue)
    {
        SyncAwait(Test_Dequeue_EmptyQueue());
        SyncAwait(Test_TwoDequeue_EmptyQueue());
        // Enable once we have "Read your own write"
        // SyncAwait(Test_Enqueue_TwoDequeue_SameTransaction());
        // SyncAwait(Test_Enqueue_TwoDequeue_DifferentTransactions());
        SyncAwait(Test_Enqueue_TwoDequeue_AllInDifferentTransactions());
    }

    BOOST_AUTO_TEST_CASE(Enqueue_Abort)
    {
        SyncAwait(Test_Enqueue_Abort());
    }

    BOOST_AUTO_TEST_CASE(Dequeue_Abort)
    {
        SyncAwait(Test_Dequeue_Abort());
    }
    
    BOOST_AUTO_TEST_CASE(Enqueue_Dequeue_Abort)
    {
        SyncAwait(Test_Enqueue_Dequeue_Abort());
    }

    BOOST_AUTO_TEST_SUITE_END()

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Single_Enqueue_Commit_Dequeue_Commit() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None);
            co_await transactionSPtr->CommitAsync();
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            co_await transactionSPtr->CommitAsync();

            CODING_ERROR_ASSERT(value == 10);
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Multiple_Enqueue_Commit_Dequeue_Commit() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 20, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 30, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (transactionSPtr->CommitAsync());
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 20);
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 30);
            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Single_Enqueue_Dequeue_Commit() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));

            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Multiple_Enqueue_Dequeue_Commit() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);

            co_await (rcq->EnqueueAsync(*transactionSPtr, 20, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 20);

            co_await (rcq->EnqueueAsync(*transactionSPtr, 30, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 30);

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Dequeue_EmptyQueue() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);

            co_await transactionSPtr->CommitAsync();
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_TwoDequeue_EmptyQueue() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);
            succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Enqueue_TwoDequeue_SameTransaction() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));

            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);

            bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Enqueue_TwoDequeue_DifferentTransactions() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));

            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);

            co_await (transactionSPtr->CommitAsync());
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Enqueue_TwoDequeue_AllInDifferentTransactions() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (transactionSPtr->CommitAsync());
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(value == 10);

            co_await (transactionSPtr->CommitAsync());
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            bool succeeded = co_await (rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
            CODING_ERROR_ASSERT(!succeeded);

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Cross_Segments() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        // This should create 3 segments, of size QUEUE_SEGMENT_START_SIZE, 2 *, 4 *
        int itemCount = QUEUE_SEGMENT_START_SIZE * 3 + 1;

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            for (int i = 0; i < itemCount; ++i)
            {
                co_await(rcq->EnqueueAsync(*transactionSPtr, i, Common::TimeSpan::MaxValue, CancellationToken::None));
            }

            co_await (transactionSPtr->CommitAsync());
        }

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            for (int i = 0; i < itemCount; ++i)
            {
                int value;
                co_await(rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None));
                CODING_ERROR_ASSERT(value == i);
            }

            co_await (transactionSPtr->CommitAsync());
        }
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Enqueue_Abort() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 20, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await transactionSPtr->CommitAsync();
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 30, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 40, Common::TimeSpan::MaxValue, CancellationToken::None));
            transactionSPtr->Abort();
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 10);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 20);

            bool succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(!succeeded);

            co_await transactionSPtr->CommitAsync();
        }            
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Dequeue_Abort() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 20, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 30, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await transactionSPtr->CommitAsync();
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 10);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 20);

            transactionSPtr->Abort();                
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 30);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 10);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 20);

            bool succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(!succeeded);

            co_await transactionSPtr->CommitAsync();              
        }      
    }

    Awaitable<void> ReliableConcurrentQueueBasicOperations::Test_Enqueue_Dequeue_Abort() noexcept
    {
        KSharedPtr<Data::Collections::ReliableConcurrentQueue<int>> rcq = this->get_RCQ();

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 10, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 20, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await (rcq->EnqueueAsync(*transactionSPtr, 30, Common::TimeSpan::MaxValue, CancellationToken::None));
            co_await transactionSPtr->CommitAsync();
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            co_await (rcq->EnqueueAsync(*transactionSPtr, 40, Common::TimeSpan::MaxValue, CancellationToken::None));

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 10);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 20);

            co_await (rcq->EnqueueAsync(*transactionSPtr, 50, Common::TimeSpan::MaxValue, CancellationToken::None));

            transactionSPtr->Abort();                
        }            

        {
            KSharedPtr<TxnReplicator::Transaction> transactionSPtr = CreateReplicatorTransaction();
            KFinally([&] { transactionSPtr->Dispose(); });

            int value = 0;
            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 30);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 10);

            co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(value == 20);

            bool succeeded = co_await rcq->TryDequeueAsync(*transactionSPtr, value, Common::TimeSpan::MaxValue, CancellationToken::None);
            CODING_ERROR_ASSERT(!succeeded);

            co_await transactionSPtr->CommitAsync();              
        }      
    }
}
