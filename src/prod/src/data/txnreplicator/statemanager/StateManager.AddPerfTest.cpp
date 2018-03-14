// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class StateManagerAddPerfTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerAddPerfTest()
        {
        }

        ~StateManagerAddPerfTest()
        {
        }

    public:
        enum AddMode
        {
            AddAsync = 0,
            GetOrAddAsync = 1
        };

    public:
        // Add Performance Test 
        Awaitable<void> Test_ConcurrentlyAdd_Performance_Test(
            __in ULONG concurrentTxn, 
            __in AddMode addMode) noexcept;

    private:
        Awaitable<void> ConcurrentlyAdd_Test(
            __in ULONG concurrentTxn,
            __in ULONG opersPerTxn,
            __in AddMode addMode) noexcept;

        Awaitable<void> ConcurrentAddTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KSharedArray<KUri::CSPtr> const & nameArray,
            __in AddMode addMode) noexcept;

    private:
        // Todo: Adding 1024 SPS instead of 131072 to avoid statemanager test taking too long
        // Move the Perf Tests to statemanager longhaul when all the tests finished.
        const ULONG itemsCount = 1024;
    };

    //
    // Goal: Add State Providers Performance Test. The time interval for adding SPs will be printed out 
    //       Takes in the concurrentTxn, and run the test with different operations per txn
    //       Do this way to avoid putting all cases in one test which generates too much files in the system and got stuck
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Set the Operations Per Task and call the ConcurrentlyAdd_Test
    //    3. Clean up and shut down the replica
    //
    Awaitable<void> StateManagerAddPerfTest::Test_ConcurrentlyAdd_Performance_Test(
        __in ULONG concurrentTxn,
        __in AddMode addMode) noexcept
    {
        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        for (ULONG operationsPerTxn = 1; operationsPerTxn <= 1024; operationsPerTxn = operationsPerTxn * 4) 
        {
            co_await this->ConcurrentlyAdd_Test(concurrentTxn, operationsPerTxn, addMode);
        }

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
    }

    //
    // Algorithm:
    //    1. Populate the state providers name list
    //    2. Dispatch the tasks and run all the tasks concurrently 
    //    3. Verify all the state providers exist
    //
    Awaitable<void> StateManagerAddPerfTest::ConcurrentlyAdd_Test(
        __in ULONG concurrentTxn,
        __in ULONG opersPerTxn,
        __in AddMode addMode) noexcept
    {
        if (concurrentTxn * opersPerTxn > itemsCount) {
            co_return;
        }

        NTSTATUS status = STATUS_UNSUCCESSFUL;
        Common::Stopwatch stopwatch;

        // Setup: Populate the state providers name list
        KArray<KUri::CSPtr> nameList(GetAllocator(), itemsCount);
        for (ULONG index = 0; index < itemsCount; index++)
        {
            status = nameList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        for (ULONG round = 0; round < itemsCount / (concurrentTxn * opersPerTxn); round++)
        {
            KArray<Awaitable<void>> taskArray(GetAllocator());
            AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            for (ULONG i = 0; i < concurrentTxn; i++)
            {
                KSharedArray<KUri::CSPtr>::SPtr nameArray = TestHelper::CreateStateProviderNameArray(GetAllocator());

                for (ULONG j = 0; j < opersPerTxn; j++)
                {
                    auto index = round * (concurrentTxn * opersPerTxn) + i * opersPerTxn + j;
                    status = nameArray->Append(nameList[index]);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }

                status = taskArray.Append(ConcurrentAddTask(*signalCompletion, *nameArray, addMode));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            stopwatch.Start();

            // Test
            signalCompletion->SetResult(true);
            co_await TaskUtilities<void>::WhenAll(taskArray);

            stopwatch.Stop();
        }

        Trace.WriteInfo(
            BoostTestTrace,
            "ConcurrentlyAdd_Test with {0} ConcurrentTxn and each Txn has {1} operations completed in {2} ms.",
            concurrentTxn,
            opersPerTxn,
            stopwatch.ElapsedMilliseconds);

        // Verification
        VerifyExist(nameList, true);
    }

    // Task used to concurrently add state providers to state manager
    Awaitable<void> StateManagerAddPerfTest::ConcurrentAddTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KSharedArray<KUri::CSPtr> const & nameArray,
        __in AddMode addMode) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        KSharedArray<KUri::CSPtr>::CSPtr nameArraySPtr = &nameArray;

        co_await tempCompletion->GetAwaitable();

        {
            Transaction::SPtr txnSPtr = nullptr;
            status = testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            KFinally([&] { txnSPtr->Dispose(); });
            for (KUri::CSPtr nameSPtr : *nameArraySPtr)
            {
                if (addMode == AddMode::AddAsync)
                {
                    status = co_await stateManagerSPtr->AddAsync(
                        *txnSPtr,
                        *nameSPtr,
                        TestStateProvider::TypeName);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                else
                {
                    CODING_ERROR_ASSERT(addMode == AddMode::GetOrAddAsync);
                    IStateProvider2::SPtr outStateProvider;
                    bool alreadyExist = false;
                    status = co_await stateManagerSPtr->GetOrAddAsync(
                        *txnSPtr,
                        *nameSPtr,
                        TestStateProvider::TypeName,
                        outStateProvider,
                        alreadyExist);

                    // GetOrAddAsync must be the add path.
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                    CODING_ERROR_ASSERT(alreadyExist == false);
                    CODING_ERROR_ASSERT(outStateProvider != nullptr);
                    CODING_ERROR_ASSERT(*nameSPtr == outStateProvider->GetName());
                }
            }

            co_await txnSPtr->CommitAsync();
        }
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerAddPerfTestSuite, StateManagerAddPerfTest)

    //
    // Scenario:        Add State Provider Performance Test with AddAsync, 
    //                  Concurrently add 131,072 SPs
    // Expected Result: The time interval for adding SPs will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_1)
    {
        const ULONG concurrentTxn = 1;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_4)
    {
        const ULONG concurrentTxn = 4;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_16)
    {
        const ULONG concurrentTxn = 16;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_64)
    {
        const ULONG concurrentTxn = 64;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_256)
    {
        const ULONG concurrentTxn = 256;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    BOOST_AUTO_TEST_CASE(AddAsync_Performance_Test_ConcurrentTxn_1024)
    {
        const ULONG concurrentTxn = 1024;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::AddAsync));
    }

    //
    // Scenario:        Add State Provider Performance Test with GetOrAddAsync, 
    //                  Concurrently add 131,072 SPs
    // Expected Result: The time interval for adding SPs will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_1)
    {
        const ULONG concurrentTxn = 1;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_4)
    {
        const ULONG concurrentTxn = 4;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_16)
    {
        const ULONG concurrentTxn = 16;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_64)
    {
        const ULONG concurrentTxn = 64;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_256)
    {
        const ULONG concurrentTxn = 256;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_CASE(GetOrAddAsync_Performance_Test_ConcurrentTxn_1024)
    {
        const ULONG concurrentTxn = 1024;
        SyncAwait(this->Test_ConcurrentlyAdd_Performance_Test(concurrentTxn, AddMode::GetOrAddAsync));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
