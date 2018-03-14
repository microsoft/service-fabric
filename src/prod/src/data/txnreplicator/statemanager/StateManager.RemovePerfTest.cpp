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
    class StateManagerRemovePerfTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerRemovePerfTest()
        {
        }

        ~StateManagerRemovePerfTest()
        {
        }

    public:
        // Remove Performance Test 
        Awaitable<void> Test_ConcurrentlyRemove_Performance_Test(__in ULONG concurrentTxn) noexcept;

    private:
        Awaitable<void> ConcurrentlyRemove_Test(
            __in ULONG concurrentTxn,
            __in ULONG opersPerTxn) noexcept;

        Awaitable<void> ConcurrentRemoveTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KSharedArray<KUri::CSPtr> const & nameArray) noexcept;

    private:
        const ULONG itemsCount = 1024;
    };

    //
    // Goal: Remove State Providers Performance Test. The time interval for removing SPs will be printed out 
    //       Takes in the concurrentTxn, and run the test with different operations per txn
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Set the Operations Per Task and call the ConcurrentlyRemove_Test
    //    3. Clean up and shut down the replica
    //
    Awaitable<void> StateManagerRemovePerfTest::Test_ConcurrentlyRemove_Performance_Test(__in ULONG concurrentTxn) noexcept
    {
        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        for (ULONG operationsPerTxn = 1; operationsPerTxn <= 1024; operationsPerTxn = operationsPerTxn * 4) 
        {
            co_await this->ConcurrentlyRemove_Test(concurrentTxn, operationsPerTxn);
        }

        // Clean up and shut down the replica
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
    }

    //
    // Algorithm:
    //    1. Populate the state providers
    //    2. Verify all the state providers exist
    //    3. Dispatch the tasks and run all the tasks concurrently 
    //    4. Verify all the state providers removed
    //
    Awaitable<void> StateManagerRemovePerfTest::ConcurrentlyRemove_Test(
        __in ULONG concurrentTxn,
        __in ULONG opersPerTxn) noexcept
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

        // Add all the state providers in nameList
        co_await PopulateAsync(nameList);

        // Verify all the state provider are added
        VerifyExist(nameList, true);

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

                status = taskArray.Append(ConcurrentRemoveTask(*signalCompletion, *nameArray));
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
            "ConcurrentlyRemove_Test with {0} ConcurrentTxn and each Txn has {1} operations completed in {2} ms.",
            concurrentTxn,
            opersPerTxn,
            stopwatch.ElapsedMilliseconds);

        // Verification
        VerifyExist(nameList, false);
    }

    // Task used to concurrently remove state providers from state manager
    Awaitable<void> StateManagerRemovePerfTest::ConcurrentRemoveTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KSharedArray<KUri::CSPtr> const & nameArray) noexcept
    {
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        KSharedArray<KUri::CSPtr>::CSPtr nameArraySPtr = &nameArray;

        co_await tempCompletion->GetAwaitable();

        {
            Transaction::SPtr txnSPtr = testTransactionalReplicatorSPtr_->CreateTransaction();
            KFinally([&] { txnSPtr->Dispose(); });
            for (KUri::CSPtr nameSPtr : *nameArraySPtr)
            {
                NTSTATUS status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *nameSPtr);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            co_await txnSPtr->CommitAsync();
        }
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerRemovePerfTestSuite, StateManagerRemovePerfTest)

    //
    // Scenario:        Remove State Provider Performance Test, 
    //                  Concurrently remove 131,072 SPs
    // Expected Result: The time interval for removing SPs will be printed out 
    // 
    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_1)
    {
        const ULONG concurrentTxn = 1;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_4)
    {
        const ULONG concurrentTxn = 4;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_16)
    {
        const ULONG concurrentTxn = 16;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_64)
    {
        const ULONG concurrentTxn = 64;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_256)
    {
        const ULONG concurrentTxn = 256;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_CASE(Remove_Performance_Test_ConcurrentTxn_1024)
    {
        const ULONG concurrentTxn = 1024;
        SyncAwait(this->Test_ConcurrentlyRemove_Performance_Test(concurrentTxn));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
