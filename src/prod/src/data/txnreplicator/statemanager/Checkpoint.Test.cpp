// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class CheckpointTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        CheckpointTest()
        {
        }

        ~CheckpointTest()
        {
        }

    public:
        Awaitable<void> Test_Primary_Empty_Success() noexcept;
        Awaitable<void> Test_Primary_One_AfterPrepare_Success() noexcept;
        Awaitable<void> Test_Primary_Multiple_MultipleCheckpoint_Success(
            __in ULONG numberOfCheckpoints,
            __in ULONG stateProviderCountPerCheckpoint) noexcept;
        Awaitable<void> Test_Primary_RemoveSPAfterPrepareCheckpoint_SPNotCheckpointed() noexcept;
        Awaitable<void> Test_RecoverCheckpoint_SameName_NotAssert() noexcept;
        Awaitable<void> Test_StateProvider_Checkpoint_Recover() noexcept;

    private:
        void ExpectCommit(__in KArray<KUri::CSPtr> const & nameList, __in LONG64 commitLsn);
        void Verify(__in KArray<KUri::CSPtr> const & nameList);
    };

    Awaitable<void> CheckpointTest::Test_Primary_Empty_Success() noexcept
    {
        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        // Test
        co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

        // Clean up
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> CheckpointTest::Test_Primary_One_AfterPrepare_Success() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();

            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] {txnSPtr->Dispose(); });

                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            IStateProvider2::SPtr stateProvider2;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider2);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider2->GetName());

            // Test
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            IStateProvider2::SPtr stateProvider2;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider2);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider2);

            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> CheckpointTest::Test_Primary_Multiple_MultipleCheckpoint_Success(
        __in ULONG numberOfCheckpoints,
        __in ULONG stateProviderCountPerCheckpoint) noexcept
    {
        // Setting up expectations.
        KArray<KArray<KUri::CSPtr>> expectedStateProviderList(GetAllocator(), 16);
        for (ULONG i = 0; i < numberOfCheckpoints; i++)
        {
            KArray<KUri::CSPtr> perCheckpointList(GetAllocator(), stateProviderCountPerCheckpoint);
            for (ULONG spIndex = 0; spIndex < stateProviderCountPerCheckpoint; spIndex++)
            {
                NTSTATUS status = perCheckpointList.Append(GetStateProviderName(NameType::Random));
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            NTSTATUS status = expectedStateProviderList.Append(perCheckpointList);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }
        
        // Test Checkpointing.
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            for (ULONG i = 0; i < numberOfCheckpoints; i++)
            {
                co_await PopulateAsync(expectedStateProviderList[i]);

                LONG64 checkpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

                for (ULONG index = 0; index <= i; index++)
                {
                    ExpectCommit(expectedStateProviderList[index], checkpointLSN);
                    Verify(expectedStateProviderList[index]);
                }
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test Recovery
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            for (ULONG i = 0; i < numberOfCheckpoints; i++)
            {
                VerifyExist(expectedStateProviderList[i], true);
            }

            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> CheckpointTest::Test_Primary_RemoveSPAfterPrepareCheckpoint_SPNotCheckpointed() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] {txnSPtr->Dispose(); });

                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            IStateProvider2::SPtr stateProvider1;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider1);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider1);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider1->GetName());

            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();

            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] {txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            IStateProvider2::SPtr stateProvider2;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider2);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider2);

            // Test
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

            // We expect prepare, perform and complete checkpoint finished, and now we verify they are called on state provider
            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider1);
            testStateProvider.ExpectCheckpoint(2);
            testStateProvider.Verify(true);

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            IStateProvider2::SPtr stateProvider2;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider2);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider2);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider2->GetName());

            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    //
    // Goal: Simulate the situation that the checkpoint file has two metadatas with the same name, and when
    //       we recover from the checkpoint file, it should not assert on the part of checking whether 
    //       the metadata list are in the descending order.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider, remove it and add it back again
    //    3. Take checkpoint on the current state
    //    4. Close the replica and open again
    //    5. Verify the state provider is there and no asserts
    //    6. Clean up and shut down the replica
    //
    Awaitable<void> CheckpointTest::Test_RecoverCheckpoint_SameName_NotAssert() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        {
            // Setup: Bring up the primary replica
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // Setup: Add a state provider, remove it and add it back again
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            IStateProvider2::SPtr stateProvider;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider);

            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

            // Take checkpoint on the current state
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Close the replica
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            // Open the replica, it will recover from the checkpoint file
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // Verify the state provide exists
            IStateProvider2::SPtr stateProvider;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

            // Clean up and shut down
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> CheckpointTest::Test_StateProvider_Checkpoint_Recover() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);
        KArray<LONG64> expectedStates(GetAllocator());

        {
            // Setup: Bring up the primary replica
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // Setup: Add a state provider,
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            IStateProvider2::SPtr stateProvider;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

            // Take checkpoint on the current state
            LONG64 checkpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            status = expectedStates.Append(checkpointLSN);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Close the replica
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            // Open the replica, it will recover from the checkpoint file
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // Verify the state provide exists
            IStateProvider2::SPtr stateProvider;
            status = stateManagerSPtr->Get(*stateProviderName, stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());

            // Verfiy the state provider recovery
            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
            testStateProvider.VerifyRecoveredStates(expectedStates);

            // Clean up and shut down
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    void CheckpointTest::ExpectCommit(
        __in KArray<KUri::CSPtr> const & nameList, 
        __in LONG64 checkpointLSN)
    {
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        for (ULONG i = 0; i < nameList.Count(); i++)
        {
            IStateProvider2::SPtr stateProvider;
            NTSTATUS status = stateManagerSPtr->Get(*nameList[i], stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*nameList[i], stateProvider->GetName());
            
            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
            testStateProvider.ExpectCheckpoint(checkpointLSN);
        }
    }

    void CheckpointTest::Verify(__in KArray<KUri::CSPtr> const& nameList)
    {
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        for (ULONG i = 0; i < nameList.Count(); i++)
        {
            IStateProvider2::SPtr stateProvider;
            NTSTATUS status = stateManagerSPtr->Get(*nameList[i], stateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(stateProvider);
            VERIFY_ARE_EQUAL(*nameList[i], stateProvider->GetName());

            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*stateProvider);
            testStateProvider.Verify(true);
        }
    }

    BOOST_FIXTURE_TEST_SUITE(CheckpointTestSuite, CheckpointTest)

    BOOST_AUTO_TEST_CASE(Primary_Empty_Success)
    {
        SyncAwait(this->Test_Primary_Empty_Success());
    }

    BOOST_AUTO_TEST_CASE(Primary_One_Success)
    {
        SyncAwait(this->Test_Primary_Multiple_MultipleCheckpoint_Success(1, 1));
    }

    BOOST_AUTO_TEST_CASE(Primary_One_AfterPrepare_Success)
    {
        SyncAwait(this->Test_Primary_One_AfterPrepare_Success());
    }

    BOOST_AUTO_TEST_CASE(Primary_TwoCheckpointsWithTwoSPs_Success)
    {
        SyncAwait(this->Test_Primary_Multiple_MultipleCheckpoint_Success(2, 2));
    }

    BOOST_AUTO_TEST_CASE(Primary_ManyCheckpointsWithManySPs_Success)
    {
        // Reduce the checkpoint count and stateProviderCountPerCheckpoint from 32 to 16,
        // since after adding state provider checkpoint, this test takes very long time.
        SyncAwait(this->Test_Primary_Multiple_MultipleCheckpoint_Success(16, 16));
    }

    //
    // Scenario:        Add a state provider, then remove the state provider after the PrepareCheckpoint and before
    //                  PerformCheckpoint.
    // Expected Result: When it recovers from the checkpoint file, the state provider should still be there
    // 
    // Note: This test case added with the PR description: SM does not call complete checkpoint on SPs that are deleted if delete operation is applied after prepare.
    BOOST_AUTO_TEST_CASE(Primary_RemoveSPAfterPrepareCheckpoint_SPNotCheckpointed)
    {
        SyncAwait(this->Test_Primary_RemoveSPAfterPrepareCheckpoint_SPNotCheckpointed());
    }

    //
    // Scenario:        Add a state provider, remove it and then add it back again. Take checkpoint on the 
    //                  current state, then recover the state from the checkpoint file.
    // Expected Result: In this situation, the checkpoint file will have two metadatas with the same name, when
    //                  we recover from the checkpoint file, it should not assert on the part of checking whether 
    //                  the metadata list are in the descending order.
    // 
    BOOST_AUTO_TEST_CASE(RecoverCheckpoint_SameName_NotAssert)
    {
        SyncAwait(this->Test_RecoverCheckpoint_SameName_NotAssert());
    }

    //
    // Scenario:        StateProvider checkpoint states, recover checkpoint
    // Expected Result: The StateProvider checkpoint saves current states to the file and 
    //                  Recover should return saved states.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_States_Checkpointed_Recover)
    {
        SyncAwait(this->Test_StateProvider_Checkpoint_Recover());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
