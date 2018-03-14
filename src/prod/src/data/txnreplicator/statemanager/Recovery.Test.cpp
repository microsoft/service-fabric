// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;
using namespace ktl;

namespace StateManagerTests
{
    // This set focus on log replay recovery (Apply and Unlock with RecoveryRedo).
    // Main cases covered
    // 1. Basic RecoveryRedo with Add and Remove State Provider.
    // 2. Parallel RecoveryRedo with Adds and Removes
    // 3. Out of order Unlocks: Operations belonging to different group commits will be applied in order but 
    //      may not be unlocked in order.
    // TODO: Some additional test that should be added later.
    // 1. Dispatching of Apply operations in recovery to StateProviders.
    // 2. Duplicate operations. Note that for this we need to make sure state provider ids match.
    //      a) Replay of existing items b) replay of not existing items.
    class RecoveryTest : StateManagerTestBase
    {
        Common::CommonConfig config; // load the config object as its needed for the tracing to work

    public:
        ktl::Awaitable<void> Test_RecoveryRedo_Checkpointless_AllItemsAdded(
            __in ULONG32 numberOfAdds, 
            __in ULONG32 numberOfRemoves,
            __in_opt bool isInitParameterNull = true) noexcept;
        ktl::Awaitable<void> Test_RecoveryRedo_Checkpointless_DelayedOutOfOrderUnlock_AllItemsAdded(
            __in ULONG32 numberOfAdds,
            __in ULONG32 numberOfRemoves) noexcept;

        ktl::Awaitable<void> RecoveryTest::Test_RecoveryRedo_DeleteCheckpointedStateProviders_AllItemsAdded(
            __in ULONG32 checkpointedStateProviderCount,
            __in ULONG32 recoveryAddedProviderCount) noexcept;

        // Verifies that unreferenced state provider folders are deleted.
        // There are four types of state providers we care about in this scenario
        //      1. Active & !Checkpointed: Must be deleted.   
        //      2. Active & Checkpointed: Must not be deleted
        //      3. Deleted & Checkpointed as stale: Must not be deleted.
        //      4. Deleted & passed Safe LSN: Must be deleted.
        ktl::Awaitable<void> RecoveryTest::Test_RemoveUnreferencedStateProviders_AllTypes_CorrectFoldersAreDeleted(
            __in ULONG32 stateProviderCountOfEachType) noexcept;
    };

    ktl::Awaitable<void> RecoveryTest::Test_RecoveryRedo_Checkpointless_AllItemsAdded(
        __in ULONG32 numberOfAdds, 
        __in ULONG32 numberOfRemoves,
        __in_opt bool isInitParameterNull) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfAdds);
        KArray<KUri::CSPtr> expectedNameList(GetAllocator(), numberOfAdds - numberOfRemoves);
        KArray<KUri::CSPtr> unexpectedNameList(GetAllocator(), numberOfRemoves);
        for (ULONG index = 0; index < numberOfAdds; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (index >= numberOfRemoves)
            {
                status = expectedNameList.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
            else
            {
                status = unexpectedNameList.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        for (LONG64 i = 0; i < numberOfAdds; i++)
        {
            const LONG64 transactionId = i + 1;
            const LONG64 stateProviderId = i + 1;

            auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
            auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *nameList[static_cast<ULONG>(i)], ApplyType::Insert);

            OperationData::SPtr initParameters = nullptr;
            RedoOperationData::CSPtr redoOperationData = nullptr;
            if (!isInitParameterNull)
            {
                initParameters = TestHelper::CreateInitParameters(GetAllocator());
            }

            redoOperationData = GenerateRedoOperationData(TestStateProvider::TypeName, initParameters.RawPtr());

            TxnReplicator::OperationContext::CSPtr operationContext;
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                i,
                *txn,
                ApplyContext::RecoveryRedo,
                namedMetadataOperationData.RawPtr(),
                redoOperationData.RawPtr(),
                operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            VERIFY_IS_TRUE(operationContext != nullptr);

            status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        for (LONG64 i = 0; i < numberOfRemoves; i++)
        {
            const LONG64 transactionId = i + 1;
            const LONG64 stateProviderId = i + 1;

            auto txn = TransactionBase::CreateTransaction(transactionId, false, GetAllocator());
            auto namedMetadataOperationData = GenerateNamedMetadataOperationData(stateProviderId, *nameList[static_cast<ULONG>(i)], ApplyType::Delete);

            TxnReplicator::OperationContext::CSPtr operationContext;
            status = co_await testTransactionalReplicatorSPtr_->StateManager->ApplyAsync(
                i,
                *txn,
                ApplyContext::RecoveryRedo,
                namedMetadataOperationData.RawPtr(),
                nullptr,
                operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
            VERIFY_IS_TRUE(operationContext != nullptr);

            status = testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        VerifyExist(expectedNameList, true, nullptr, isInitParameterNull);
        VerifyExist(unexpectedNameList, false, nullptr, isInitParameterNull);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    ktl::Awaitable<void> RecoveryTest::Test_RecoveryRedo_Checkpointless_DelayedOutOfOrderUnlock_AllItemsAdded(
        __in ULONG32 numberOfAdds,
        __in ULONG32 numberOfRemoves) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfAdds);
        KArray<KUri::CSPtr> expectedNameList(GetAllocator(), numberOfAdds - numberOfRemoves);
        KArray<KUri::CSPtr> unexpectedNameList(GetAllocator(), numberOfRemoves);
        for (ULONG index = 0; index < numberOfAdds; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (index >= numberOfRemoves)
            {
                status = expectedNameList.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
            else
            {
                status = unexpectedNameList.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        // First do all the adds. Since each is a distict state provider, we know they cannot be conflicting.
        // So potentially they could have been in the same group commit.
        KSharedArray<OperationContext::CSPtr>::SPtr addContextList = co_await ApplyAddAsync(
            nameList, 
            0, 
            numberOfAdds, 
            1, 
            1, 
            1, 
            ApplyContext::RecoveryRedo);

        auto parallelRemoveAwaitable = ApplyRemoveAsync(
            nameList,
            0,
            numberOfRemoves,
            1 + numberOfAdds,   // starting transaction id
            1 + numberOfAdds,   // starting LSN
            1,                  // starting state provider id.
            ApplyContext::RecoveryRedo);

        // Unlock all the add operations in reverse.
        for (LONG32 i = addContextList->Count() - 1; i >= 0; i--)
        {
            auto operationContext = (*addContextList)[i];
            VERIFY_IS_NOT_NULL(operationContext);
            testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
        }

        // Wait for all the remove operations to complete.
        KSharedArray<OperationContext::CSPtr>::SPtr removeContextList = co_await parallelRemoveAwaitable;

        // Unlock all the remove operations in reverse.
        for (LONG32 i = removeContextList->Count() - 1; i >= 0; i--)
        {
            auto operationContext = (*removeContextList)[i];
            VERIFY_IS_NOT_NULL(operationContext);
            testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
        }

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        VerifyExist(expectedNameList, true);
        VerifyExist(unexpectedNameList, false);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    ktl::Awaitable<void> RecoveryTest::Test_RecoveryRedo_DeleteCheckpointedStateProviders_AllItemsAdded(
        __in ULONG32 checkpointedStateProviderCount,
        __in ULONG32 recoveryAddedProviderCount) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> checkpointNameList(GetAllocator(), checkpointedStateProviderCount);
        for (ULONG index = 0; index < checkpointedStateProviderCount; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = checkpointNameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> recoveryAddedNameList(GetAllocator(), recoveryAddedProviderCount);
        for (ULONG index = 0; index < recoveryAddedProviderCount; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = recoveryAddedNameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        LONG64 prepareCheckpointLSN;
        // Checkpoint
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            co_await PopulateAsync(checkpointNameList);

            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

            KSharedArray<OperationContext::CSPtr>::SPtr removeContextList = co_await ApplyRemoveAsync(
                checkpointNameList,
                0,
                checkpointedStateProviderCount,
                1 + prepareCheckpointLSN,   // starting transaction id
                1 + prepareCheckpointLSN,   // starting LSN
                1,                          // starting state provider id.
                ApplyContext::RecoveryRedo);

            // Unlock all the remove operations in reverse.
            for (LONG32 i = removeContextList->Count() - 1; i >= 0; i--)
            {
                auto operationContext = (*removeContextList)[i];
                VERIFY_IS_NOT_NULL(operationContext);
                testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            }

            KSharedArray<OperationContext::CSPtr>::SPtr addContextList = co_await ApplyAddAsync(
                recoveryAddedNameList,
                0,
                recoveryAddedProviderCount,
                1 + prepareCheckpointLSN + checkpointedStateProviderCount,
                1 + prepareCheckpointLSN + checkpointedStateProviderCount,
                1 + checkpointedStateProviderCount,
                ApplyContext::RecoveryRedo);

            // Unlock all the remove operations in reverse.
            for (LONG32 i = addContextList->Count() - 1; i >= 0; i--)
            {
                auto operationContext = (*addContextList)[i];
                VERIFY_IS_NOT_NULL(operationContext);
                testTransactionalReplicatorSPtr_->StateManager->Unlock(*operationContext);
            }

            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            VerifyExist(checkpointNameList, false);
            VerifyExist(recoveryAddedNameList, true);

            // Shutdown
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    // Verifies that unreferenced state provider folders are deleted.
    // There are four types of state providers we care about in this scenario
    //      1. Active & !Checkpointed: Must be deleted.   
    //      2. Active & Checkpointed: Must not be deleted
    //      3. Deleted & Checkpointed as stale: Must not be deleted.
    //      4. Deleted & passed Safe LSN: Must be deleted.
    // Note that due to removing folder as part of cleanup in complete, 4th is not tested here.
    ktl::Awaitable<void> RecoveryTest::Test_RemoveUnreferencedStateProviders_AllTypes_CorrectFoldersAreDeleted(
        __in ULONG32 stateProviderCountOfEachType) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> activeNotCheckpointedNames(GetAllocator(), stateProviderCountOfEachType);
        for (ULONG index = 0; index < stateProviderCountOfEachType; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = activeNotCheckpointedNames.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> activeCheckpointedNames(GetAllocator(), stateProviderCountOfEachType);
        for (ULONG index = 0; index < stateProviderCountOfEachType; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = activeCheckpointedNames.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> staleCheckpointedNames(GetAllocator(), stateProviderCountOfEachType);
        for (ULONG index = 0; index < stateProviderCountOfEachType; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = staleCheckpointedNames.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> safeNames(GetAllocator(), stateProviderCountOfEachType);
        for (ULONG index = 0; index < stateProviderCountOfEachType; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = safeNames.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN = FABRIC_INVALID_SEQUENCE_NUMBER;
        KArray<LONG64> activeCheckpointedIds(GetAllocator());
        KArray<LONG64> activeNotCheckpointedIds(GetAllocator());
        KArray<LONG64> staleIds(GetAllocator());
        KArray<LONG64> safeNameIds(GetAllocator());
        // Checkpoint
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Active Checkpointed Names
            co_await PopulateAsync(activeCheckpointedNames);
            activeCheckpointedIds = PopulateStateProviderIdArray(activeCheckpointedNames);

            // Safe Names.
            co_await PopulateAsync(safeNames);
            safeNameIds = PopulateStateProviderIdArray(safeNames);
            co_await DepopulateAsync(safeNames);

            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            testTransactionalReplicatorSPtr_->SetSafeLSN(prepareCheckpointLSN);

            // Stale Names
            co_await PopulateAsync(staleCheckpointedNames);
            staleIds = PopulateStateProviderIdArray(staleCheckpointedNames);
            co_await DepopulateAsync(staleCheckpointedNames);

            // Note if we did not depend on folder removal at tombstone removal, we would need one more checkpoint for this test.
            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Must not checkpoint these names;
            co_await PopulateAsync(activeNotCheckpointedNames);
            activeNotCheckpointedIds = PopulateStateProviderIdArray(activeNotCheckpointedNames);

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
            testTransactionalReplicatorSPtr_.Reset();
        }

        // Test starts here.
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            IStateManager::SPtr stateManagerInterface = testTransactionalReplicatorSPtr_->StateManager;
            StateManager & stateManager = dynamic_cast<StateManager &>(*stateManagerInterface);

            for (ULONG32 index = 0; index < activeNotCheckpointedNames.Count(); index++)
            {
                KString::CSPtr directory = stateManager.GetStateProviderWorkDirectory(activeNotCheckpointedNames[index]);
                bool directoryExists = Common::Directory::Exists(wstring(*directory));
                VERIFY_IS_FALSE(directoryExists);
            }

            for (ULONG32 index = 0; index < activeCheckpointedIds.Count(); index++)
            {
                KString::CSPtr directory = stateManager.GetStateProviderWorkDirectory(activeCheckpointedIds[index]);
                bool directoryExists = Common::Directory::Exists(wstring(*directory));
                VERIFY_IS_TRUE(directoryExists);
            }

            for (ULONG32 index = 0; index < staleCheckpointedNames.Count(); index++)
            {
                KString::CSPtr directory = stateManager.GetStateProviderWorkDirectory(staleIds[index]);
                bool directoryExists = Common::Directory::Exists(wstring(*directory));
                VERIFY_IS_TRUE(directoryExists);
            }

            for (ULONG32 index = 0; index < safeNames.Count(); index++)
            {
                KString::CSPtr directory = stateManager.GetStateProviderWorkDirectory(safeNames[index]);
                bool directoryExists = Common::Directory::Exists(wstring(*directory));
                VERIFY_IS_FALSE(directoryExists);
            }

            // Shutdown
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(RecoveryTestSuite, RecoveryTest)

    // Recover redo operation which inserts a StateProvider with null initialization parameter
    // The StateProvider is added and the initialization parameter is null 
    BOOST_AUTO_TEST_CASE(ApplyRedo_Insert_NullInitParameter_ItemAdded)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(1, 0));
    }

    // Recover redo operation which inserts a StateProvider with initialization parameter
    // The StateProvider is added and the initialization parameter is as expected 
    BOOST_AUTO_TEST_CASE(ApplyRedo_Insert_WithInitParameter__ItemAdded)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(1, 0, false));
    }

    BOOST_AUTO_TEST_CASE(ApplyRedo_Insert_Multiple_ItemsAdded)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(16, 0));
    }

    BOOST_AUTO_TEST_CASE(ApplyRedo_InsertAndRemove_Empty)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(1, 1));
    }

    BOOST_AUTO_TEST_CASE(ApplyRedo_InsertAndRemove_Multiple_Empty)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(16, 16));
    }

    // Recover redo operation which inserts and removes multiple StateProviders with null initialization parameter
    // In this test, inserts 64 StateProviders and removes 32 StateProviders
    // The added StateProvider should have the null initialization parameter
    BOOST_AUTO_TEST_CASE(ApplyRedo_InsertAndRemove_Multiple_InBalanced_AllNullInitParameter_OnlyUnremovedItemsExist)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(64, 32));
    }

    // Recover redo operation which inserts and removes multiple StateProviders with initialization parameter
    // In this test, inserts 64 StateProviders and removes 32 StateProviders
    // The added StateProvider should have the initialization parameter as expected 
    BOOST_AUTO_TEST_CASE(ApplyRedo_InsertAndRemove_Multiple_InBalanced_WithInitParameter_OnlyUnremovedItemsExist)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_AllItemsAdded(64, 32, false));
    }

    BOOST_AUTO_TEST_CASE(ApplyRedo_InsertAndRemove_Multiple_InBalanced_DelayedUnorderedUnlock_OnlyUnremovedItemsExist)
    {
        SyncAwait(this->Test_RecoveryRedo_Checkpointless_DelayedOutOfOrderUnlock_AllItemsAdded(1024, 512));
    }

    BOOST_AUTO_TEST_CASE(ApplyRedo_RecoveredCheckpoint_RemoveAllCheckpointedAndAddNew_OnlyNewExist)
    {
        SyncAwait(this->Test_RecoveryRedo_DeleteCheckpointedStateProviders_AllItemsAdded(1024, 512));
    }

    // Scenario:    Verify that unreferenced state provider folders are deleted.
    //              There are four types of state providers we care about in this scenario
    //                  1. Active & !Checkpointed: Must be deleted.   
    //                  2. Active & Checkpointed: Must not be deleted
    //                  3. Deleted & Checkpointed as stale: Must not be deleted.
    //                  4. Deleted & passed Safe LSN: Must be deleted.
    //              Note that due to removing folder as part of cleanup in complete, 4th is not tested here.
    BOOST_AUTO_TEST_CASE(RemoveUnreferencedStateProviders_AllTypes_CorrectFoldersAreDeleted)
    {
        SyncAwait(this->Test_RemoveUnreferencedStateProviders_AllTypes_CorrectFoldersAreDeleted(1));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
