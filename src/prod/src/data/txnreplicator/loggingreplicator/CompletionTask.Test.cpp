// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace std;
    using namespace ktl;
    using namespace Common;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;

    Common::StringLiteral const TraceComponent("CompletionTaskTest");
    #define COMPLETIONTASKTEST_TAG 'moCT'

    class CompletionTaskTest
    {

    protected:

        void EndTest();

        Common::CommonConfig config; // load the config object as its needed for the tracing to work
        ::FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    void CompletionTaskTest::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(CompletionTaskTestSuite, CompletionTaskTest)

    BOOST_AUTO_TEST_CASE(ThreadSafeSPtrCacheTest_VerifyRefCounting)
    {
        // verifies the sptr is released correctly after test case ends as there is memory leak checks by ktl
        TEST_TRACE_BEGIN("ThreadSafeSPtrCacheTest_VerifyRefCounting")

        {
            Data::Utilities::ThreadSafeCSPtrCache<Data::Utilities::SharedException> sptrCache(nullptr);
            Data::Utilities::SharedException::CSPtr ex = Data::Utilities::SharedException::Create(allocator);

            sptrCache.Put(Ktl::Move(ex));
            KArray<KEvent *> events(allocator);
            int threadCount = 10;

            for (int i = 0; i < threadCount; i++)
            {
                KEvent * event = _new(COMPLETIONTASKTEST_TAG, allocator)KEvent();
                events.Append(event);
                Common::Threadpool::Post([&, i]
                {
                    Common::Random r(i);
                    for (int j = 0; j < 100; j++)
                    {
                        NTSTATUS status = sptrCache.Get()->Status();
                        UNREFERENCED_PARAMETER(status);
                        Sleep(1 + (r.Next() % 2));
                    }

                    events[i]->SetEvent();
                });
            }

            for (int i = 0; i < threadCount; i++)
            {
                events[i]->WaitUntilSet();
            }

            for (int i = 0; i < threadCount; i++)
            {
                _delete(events[i]);
            }
        }
    }

    BOOST_AUTO_TEST_CASE(TestToTask)
    {
        TEST_TRACE_BEGIN("TestToTask")

        {
            Awaitable<void> testAwaitable;
            ToTask(testAwaitable);
        }
    }

    BOOST_AUTO_TEST_CASE(Initialization)
    {
        TEST_TRACE_BEGIN("Initialization")

        {
            CompletionTask::SPtr task = CompletionTask::Create(allocator);
            CODING_ERROR_ASSERT(!task->IsCompleted);
        }
    }

    BOOST_AUTO_TEST_CASE(AwaitAfterCompletionWithError_TaskShouldBeFaulted)
    {
        TEST_TRACE_BEGIN("AwaitAfterCompletionWithError_TaskShouldBeFaulted")

        {
            CompletionTask::SPtr task = CompletionTask::Create(allocator);
            task->CompleteAwaiters(E_FAIL);

            NTSTATUS result = SyncAwait(task->AwaitCompletion());
            VERIFY_ARE_EQUAL(result, E_FAIL);
            CODING_ERROR_ASSERT(task->IsCompleted);
        }
    }

    BOOST_AUTO_TEST_CASE(AwaitAfterCompletion_TaskShouldAlreadyBeCompleted)
    {
        TEST_TRACE_BEGIN("AwaitAfterCompletion_TaskShouldAlreadyBeCompleted")

        {
            CompletionTask::SPtr task = CompletionTask::Create(allocator);

            task->CompleteAwaiters(0);

            NTSTATUS result = SyncAwait(task->AwaitCompletion());
            VERIFY_ARE_EQUAL(result, STATUS_SUCCESS);
            CODING_ERROR_ASSERT(task->IsCompleted);
        }
    }

    BOOST_AUTO_TEST_CASE(AwaitBeforeCompletionWithError_TaskShouldGetFaulted)
    {
        TEST_TRACE_BEGIN("AwaitBeforeCompletionWithError_TaskShouldGetFaulted")

        {
            CompletionTask::SPtr task = CompletionTask::Create(allocator);
            auto result = task->AwaitCompletion();
            task->CompleteAwaiters(STATUS_ABANDONED);
            NTSTATUS statusResult = SyncAwait(result);
            VERIFY_ARE_EQUAL(statusResult, STATUS_ABANDONED);
            CODING_ERROR_ASSERT(task->IsCompleted);
        }
    }

    BOOST_AUTO_TEST_CASE(AwaitBeforeCompletion_TaskShouldGetBeCompleted)
    {
        TEST_TRACE_BEGIN("AwaitBeforeCompletion_TaskShouldGetBeCompleted")

        {
            CompletionTask::SPtr task = CompletionTask::Create(allocator);

            auto result = task->AwaitCompletion();
            task->CompleteAwaiters(0);
            NTSTATUS statusResult = SyncAwait(result);
            VERIFY_ARE_EQUAL(statusResult, 0);
            CODING_ERROR_ASSERT(task->IsCompleted);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}

