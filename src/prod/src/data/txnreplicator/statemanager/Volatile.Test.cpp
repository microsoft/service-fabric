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
    class VolatileTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        Common::CommonConfig config;

    public:
        VolatileTest() : StateManagerTestBase(/*hasPersistedState:*/false)
        {
        }

        ~VolatileTest()
        {
        }

    public: // Checkpoint Tests
        Awaitable<void> Test_Primary_Empty_Success() noexcept;
        Awaitable<void> Test_Primary_One_AfterPrepare_Success() noexcept;
        Awaitable<void> Test_Primary_Multiple_MultipleCheckpoint_Success(
            __in ULONG numberOfCheckpoints,
            __in ULONG stateProviderCountPerCheckpoint) noexcept;

    private: // Checkpoint test helpers
        void ExpectCommit(__in KArray<KUri::CSPtr> const & nameList, __in LONG64 commitLsn);
        void Verify(__in KArray<KUri::CSPtr> const & nameList);

    public: // Copy tests
        Awaitable<void> Test_Copy_MultipleStateProvider_AllActiveCopied(
            __in ULONG32 numberOfAdds,
            __in ULONG32 numberOfRemoves,
            __in bool checkpointAfterAdd,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(
            __in ULONG32 numberOfAdds,
            __in ULONG32 numberOfRemoves,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_SecondaryApply_New_StateProvidersAdded(
            __in ULONG32 numberOfStateAdds,
            __in ULONG32 numberOfRealAdds,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Apply_CopyEmpty_InsertAndDelete_ItemDeleted(
            __in ULONG32 numberOfItems,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Apply_CopyInsert_Delete_ItemDeleted(
            __in ULONG32 numberOfItems,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Apply_CopyDeleted_InsertAndDelete_ItemDeleted(
            __in ULONG32 numberOfItems,
            __in bool replayInsert,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Copy_PerformCheckpoint_Race(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange,
            __in bool recoverInPlace,
            __in bool isCopyFirst) noexcept;

        Awaitable<void> Test_Copy_CompleteCheckpoint_Race(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Copy_MetadataSize32KOr64K(
            __in bool isFileSize32K,
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_PrepareCheckpoint_CallTwice_CheckpointSecondPrepare(
            __in bool recoverInPlace) noexcept;

        Awaitable<void> Test_Copy_StateProvider_Empty_State(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool recoverInPlace) noexcept;
    
    private: // Copy Test Helpers
        Awaitable<void> CopyTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in FABRIC_REPLICA_ID targetReplicaId,
            __out OperationDataStream::SPtr & copyStream,
            __in bool isCopyFirst = false,
            __in_opt AwaitableCompletionSource<bool> * const copyCompletion = nullptr) noexcept;
        Awaitable<void> PrepareCheckpointTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;
        Awaitable<void> PerformCheckpoint(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in bool isCopyFirst,
            __in_opt AwaitableCompletionSource<bool> * const copyCompletion) noexcept;
        Awaitable<void> CompleteCheckpointTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;
    };

    Awaitable<void> VolatileTest::Test_Primary_Empty_Success() noexcept
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

    Awaitable<void> VolatileTest::Test_Primary_One_AfterPrepare_Success() noexcept
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
        }

        wstring folder = this->runtimeFolders_->get_WorkDirectory();
    }

    Awaitable<void> VolatileTest::Test_Primary_Multiple_MultipleCheckpoint_Success(
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
        }
    }

    void VolatileTest::ExpectCommit(
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

    void VolatileTest::Verify(__in KArray<KUri::CSPtr> const& nameList)
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

    Awaitable<void> VolatileTest::Test_Copy_MultipleStateProvider_AllActiveCopied(
        __in ULONG32 numberOfAdds, 
        __in ULONG32 numberOfRemoves,
        __in bool checkpointAfterAdd, 
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        CODING_ERROR_ASSERT(numberOfRemoves <= numberOfAdds);

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

        OperationDataStream::SPtr copyStream;

        // Setup
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await PopulateAsync(nameList);
            co_await DepopulateAsync(unexpectedNameList);

            if(checkpointAfterAdd)
            {
                co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            }

            // Get the copy stream.
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // This is to verify the test.
            VerifyExist(expectedNameList, true);
            VerifyExist(unexpectedNameList, false);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(expectedNameList, checkpointAfterAdd, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(unexpectedNameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(
        __in ULONG32 numberOfAdds,
        __in ULONG32 numberOfRemoves,
        __in bool recoverInPlace) noexcept
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
        KArray<LONG64> stateProviderIdList(GetAllocator(), 0);
        OperationDataStream::SPtr copyStream;
        LONG64 secondCheckpointLSN;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await PopulateAsync(nameList);
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            VerifyExist(nameList, true);

            stateProviderIdList = PopulateStateProviderIdArray(nameList);

            // Depopulate
            co_await DepopulateAsync(unexpectedNameList);
            secondCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            VerifyExist(unexpectedNameList, false);

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());

            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            KSharedArray<OperationContext::CSPtr>::SPtr operationContextArray = co_await this->ApplyMultipleAsync(
                nameList,
                stateProviderIdList,
                0,
                nameList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where nooped.
            VERIFY_IS_TRUE(operationContextArray->Count() == 0);

            co_await secondaryTransactionalReplicator->CheckpointAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(expectedNameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(unexpectedNameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            KSharedArray<OperationContext::CSPtr>::SPtr applyContextArray = co_await this->ApplyMultipleAsync(
                nameList,
                stateProviderIdList,
                0,
                nameList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where nooped.
            VERIFY_IS_TRUE(operationContextArray->Count() == 0);

            VerifyExist(expectedNameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(unexpectedNameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::Test_SecondaryApply_New_StateProvidersAdded(
        __in ULONG32 numberOfStaleAdds,
        __in ULONG32 numberOfNewAdds,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> staleNameList(GetAllocator(), numberOfStaleAdds);
        for (ULONG index = 0; index < numberOfStaleAdds; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = staleNameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> newNameList(GetAllocator(), numberOfNewAdds);
        for (ULONG index = 0; index < numberOfNewAdds; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = newNameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> staleStateProviderIdList(GetAllocator(), 0);
        KArray<LONG64> newStateProviderIdList(GetAllocator(), 0);

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await PopulateAsync(staleNameList);
            VerifyExist(staleNameList, true);
            staleStateProviderIdList = PopulateStateProviderIdArray(staleNameList);

            // Should not include not checkpointed.
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Populate (to be replayed)
            co_await PopulateAsync(newNameList);
            VerifyExist(newNameList, true);
            newStateProviderIdList = PopulateStateProviderIdArray(newNameList);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            KSharedArray<OperationContext::CSPtr>::SPtr staleOperationContextArray = co_await this->ApplyMultipleAsync(
                staleNameList,
                staleStateProviderIdList,
                0,
                staleNameList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where nooped.
            VERIFY_IS_TRUE(staleOperationContextArray->Count() == staleNameList.Count());

            Unlock(*staleOperationContextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            KSharedArray<OperationContext::CSPtr>::SPtr newOperationContextArray = co_await this->ApplyMultipleAsync(
                newNameList,
                newStateProviderIdList,
                0,
                newNameList.Count(),
                1 + staleNameList.Count(),
                1 + staleNameList.Count(),
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where nooped.
            VERIFY_IS_TRUE(newOperationContextArray->Count() == newNameList.Count());
            Unlock(*newOperationContextArray, secondaryTransactionalReplicator->StateManager.RawPtr());
            
            co_await secondaryTransactionalReplicator->CheckpointAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(staleNameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(newNameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::Test_Apply_CopyEmpty_InsertAndDelete_ItemDeleted(
        __in ULONG32 numberOfItems, 
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfItems);
        for (ULONG index = 0; index < numberOfItems; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> idList(GetAllocator(), 0);

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Empty copy.
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Populate
            co_await PopulateAsync(nameList);
            idList = PopulateStateProviderIdArray(nameList);

            co_await DepopulateAsync(nameList);
            VerifyExist(nameList, false);

            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // Play inserts.
            KSharedArray<OperationContext::CSPtr>::SPtr contextArray = co_await this->ApplyMultipleAsync(
                nameList,
                idList,
                0,
                nameList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == nameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Play deletes.
            contextArray = co_await this->ApplyMultipleAsync(
                nameList,
                idList,
                0,
                nameList.Count(),
                1 + nameList.Count(),
                1 + nameList.Count(),
                ApplyType::Delete,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == nameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            co_await secondaryTransactionalReplicator->CheckpointAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(nameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::Test_Apply_CopyInsert_Delete_ItemDeleted(
        __in ULONG32 numberOfItems,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfItems);
        for (ULONG index = 0; index < numberOfItems; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> idList(GetAllocator(), 0);

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await PopulateAsync(nameList);
            idList = PopulateStateProviderIdArray(nameList);

            // Copy Inserted
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_await DepopulateAsync(nameList);
            VerifyExist(nameList, false);
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // Play deletes.
            auto contextArray = co_await this->ApplyMultipleAsync(
                nameList,
                idList,
                0,
                nameList.Count(),
                1 + nameList.Count(),
                1 + nameList.Count(),
                ApplyType::Delete,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == nameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            co_await secondaryTransactionalReplicator->CheckpointAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(nameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::Test_Apply_CopyDeleted_InsertAndDelete_ItemDeleted(
        __in ULONG32 numberOfItems, 
        __in bool replayInsert,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfItems);
        for (ULONG index = 0; index < numberOfItems; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> idList(GetAllocator(), 0);

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await PopulateAsync(nameList);
            idList = PopulateStateProviderIdArray(nameList);

            co_await DepopulateAsync(nameList);
            VerifyExist(nameList, false);

            // Copy Inserted
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            if (replayInsert)
            {
                // Play insterts.
                auto contextArray = co_await this->ApplyMultipleAsync(
                    nameList,
                    idList,
                    0,
                    nameList.Count(),
                    1 + nameList.Count(),
                    1 + nameList.Count(),
                    ApplyType::Delete,
                    ApplyContext::SecondaryRedo,
                    secondaryTransactionalReplicator->StateManager.RawPtr());

                // Indicates that all operations where applied.
                VERIFY_IS_TRUE(contextArray->Count() == 0);
            }

            // Play deletes.
            auto contextArray = co_await this->ApplyMultipleAsync(
                nameList,
                idList,
                0,
                nameList.Count(),
                1 + nameList.Count(),
                1 + nameList.Count(),
                ApplyType::Delete,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == 0);

            co_await secondaryTransactionalReplicator->CheckpointAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            VerifyExist(nameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::CopyTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in FABRIC_REPLICA_ID targetReplicaId,
        __out OperationDataStream::SPtr & copyStream,
        __in bool isCopyFirst,
        __in_opt AwaitableCompletionSource<bool> * const copyCompletion) noexcept
    {
        co_await signalCompletion.GetAwaitable();

        if (!isCopyFirst && copyCompletion != nullptr)
        {
            co_await copyCompletion->GetAwaitable();
        }

        NTSTATUS status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        if (isCopyFirst && copyCompletion != nullptr)
        {
            copyCompletion->SetResult(true);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::PrepareCheckpointTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();

        co_return;
    }

    Awaitable<void> VolatileTest::PerformCheckpoint(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in bool isCopyFirst,
        __in_opt AwaitableCompletionSource<bool> * const copyCompletion) noexcept
    {
        // Remove the PrepareCheckpointAsync since async call leads to thread change and GetAwaitable will not wait on AwaitableCompletionSource.
        co_await signalCompletion.GetAwaitable();

        if (isCopyFirst)
        {
            co_await copyCompletion->GetAwaitable();
        }

        co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);

        if (!isCopyFirst)
        {
            CODING_ERROR_ASSERT(copyCompletion != nullptr);
            copyCompletion->SetResult(true);
        }

        co_return;
    }

    Awaitable<void> VolatileTest::CompleteCheckpointTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        // Remove the PrepareCheckpointAsync and PerformCheckpointAsync since async call leads to 
        // thread change and GetAwaitable will not wait on AwaitableCompletionSource.
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

        co_return;
    }

    // Goal: Simulate the race between Copy stream and PrepareCheckpoint, verify copy on base point
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add some more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between copy and PrepareCheckpoint
    //    4. Secondary gets the copy stream
    //    5. Verify Secondary copied all the states in the base checkpoint
    Awaitable<void> VolatileTest::Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Expected
        KArray<KUri::CSPtr> addsBeforeCheckpoint(GetAllocator(), numAddsBeforeCheckpoint);
        KArray<KUri::CSPtr> changesAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // This array only used in the remove condition, which indicates the state providers added before but not removed.
        KArray<KUri::CSPtr> notChangedAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // In add condition:
        // addsBeforeCheckpoint has all the state providers added in the first checkpoint
        // changesAfterCheckpoint has all the state providers added in the second checkpoint
        //
        // In remove condition:
        // addsBeforeCheckpoint has all the added state providers before the checkpoint
        // changesAfterCheckpoint has all the removed state providers after the checkpoint
        // notChangedAfterCheckpoint has all the left state providers after the checkpoint
        for (ULONG index = 0; index < (isAddChange ? numAddsBeforeCheckpoint + numChangesAfterCheckpoint : numAddsBeforeCheckpoint); index++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            if (index < numAddsBeforeCheckpoint)
            {
                status = addsBeforeCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (isAddChange && index >= numAddsBeforeCheckpoint)
            {
                status = changesAfterCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
            
            if (!isAddChange)
            {
                // Remove state provider number can not be more then added number
                CODING_ERROR_ASSERT(numChangesAfterCheckpoint <= numAddsBeforeCheckpoint);

                if (index < numChangesAfterCheckpoint)
                {
                    status = changesAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                else
                {
                    status = notChangedAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
            }
        }

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Add some state providers

            co_await this->PopulateAsync(addsBeforeCheckpoint);
            VerifyExist(addsBeforeCheckpoint, true);

            // Take checkpoint, which considered as base checkpoint
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Add some more state providers or remove partial state providers for the new checkpoint
            if (isAddChange)
            {
                // added more state providers and verify all added exist
                co_await this->PopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, true);
            }
            else
            {
                // remove some state providers and verify removed ones got removed, and reset still exist
                co_await this->DepopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, false);
                VerifyExist(notChangedAfterCheckpoint, true);
            }

            KArray<Awaitable<void>> taskArray(GetAllocator());

            AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Set the race between copy and PrepareCheckpoint
            status = taskArray.Append(PrepareCheckpointTask(*signalCompletion));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = taskArray.Append(CopyTask(*signalCompletion, targetReplicaId, copyStream));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            signalCompletion->SetResult(true);
            co_await (TaskUtilities<void>::WhenAll(taskArray));

            // Finish the second checkpoint.
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            // Secondary gets the copy stream
            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // Verify Secondary copied all the states in the base checkpoint
            // In add condition:
            //     All the state providers added before base checkpoint should exist, all the state providers added after base checkpoint 
            //     should not exist
            // In remove condition:
            //     All the state providers added before base checkpoint should exist
            VerifyExist(addsBeforeCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());

            if (isAddChange)
            {
                VerifyExist(changesAfterCheckpoint, false, secondaryTransactionalReplicator->StateManager.RawPtr());
            }

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    // Goal: Simulate the race between Copy stream and PerformCheckpoint. If copy stream happened first, 
    //       copy all the base checkpoint, if PerformCheckpoint happened first, copy all the new checkpointed states
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between copy and PerformCheckpoint
    //    4. Secondary gets the copy stream
    //    5. If copy stream happened first, Verify Secondary copied all the states in the base checkpoint
    //       Else, Verify Secondary copied all the states in the new checkpoint
    Awaitable<void> VolatileTest::Test_Copy_PerformCheckpoint_Race(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange,
        __in bool recoverInPlace,
        __in bool isCopyFirst) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Expected
        KArray<KUri::CSPtr> addsBeforeCheckpoint(GetAllocator(), numAddsBeforeCheckpoint);
        KArray<KUri::CSPtr> changesAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // This array only used in the remove condition, which indicates the state providers added before but not removed.
        KArray<KUri::CSPtr> notChangedAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // In add condition:
        // addsBeforeCheckpoint has all the state providers added in the first checkpoint
        // changesAfterCheckpoint has all the state providers added in the second checkpoint
        //
        // In remove condition:
        // addsBeforeCheckpoint has all the added state providers before the checkpoint
        // changesAfterCheckpoint has all the removed state providers after the checkpoint
        // notChangedAfterCheckpoint has all the left state providers after the checkpoint
        for (ULONG index = 0; index < (isAddChange ? numAddsBeforeCheckpoint + numChangesAfterCheckpoint : numAddsBeforeCheckpoint); index++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            if (index < numAddsBeforeCheckpoint)
            {
                status = addsBeforeCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (isAddChange && index >= numAddsBeforeCheckpoint)
            {
                status = changesAfterCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (!isAddChange)
            {
                // Remove state provider number can not be more then added number
                CODING_ERROR_ASSERT(numChangesAfterCheckpoint <= numAddsBeforeCheckpoint);

                if (index < numChangesAfterCheckpoint)
                {
                    status = changesAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                else
                {
                    status = notChangedAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
            }
        }

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Add some state providers
            co_await this->PopulateAsync(addsBeforeCheckpoint);
            VerifyExist(addsBeforeCheckpoint, true);

            // Take checkpoint, which considered as base checkpoint
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Add more state providers or remove partial state providers for the new checkpoint
            if (isAddChange)
            {
                // added more state providers and verify all added exist
                co_await this->PopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, true);
            }
            else
            {
                // remove some state providers and verify removed ones got removed, and reset still exist
                co_await this->DepopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, false);
                VerifyExist(notChangedAfterCheckpoint, true);
            }

            KArray<Awaitable<void>> taskArray(GetAllocator());

            AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            AwaitableCompletionSource<bool>::SPtr copyCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, copyCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Set the race between copy and PerformCheckpoint
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = taskArray.Append(CopyTask(*signalCompletion, targetReplicaId, copyStream, isCopyFirst, copyCompletion.RawPtr()));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
            status = taskArray.Append(PerformCheckpoint(*signalCompletion, isCopyFirst, copyCompletion.RawPtr()));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            signalCompletion->SetResult(true);
            co_await (TaskUtilities<void>::WhenAll(taskArray));

            // Finish the checkpoint
            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            // Secondary gets the copy stream
            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            if (isCopyFirst)
            {
                // Copy first indicates should get all the base checkpoint states
                // In add condition:
                //     All the state providers added before base checkpoint should eixst, all the state providers added after base checkpoint 
                //     should not exist
                // In remove condition:
                //     All the state providers added before base checkpoint should eixst
                VerifyExist(addsBeforeCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());

                if (isAddChange)
                {
                    VerifyExist(changesAfterCheckpoint, false, secondaryTransactionalReplicator->StateManager.RawPtr());
                }
            }
            else
            {
                // PerformCheckpoint first indicates should get the second checkpoint states
                // In add condition:
                //     All the state providers added before base checkpoint should exist, all the state providers added after base checkpoint 
                //     should also exist
                // In remove condition:
                //     All the state providers removed after base checkpoint should not exist, rest exist.
                if (isAddChange)
                {
                    VerifyExist(addsBeforeCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
                    VerifyExist(changesAfterCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
                }
                else
                {
                    VerifyExist(notChangedAfterCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
                    VerifyExist(changesAfterCheckpoint, false, secondaryTransactionalReplicator->StateManager.RawPtr());
                }   
            }

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }
    
    // Goal: Simulate the race between Copy stream and CompleteCheckpoint. Copy all the new checkpointed states
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between copy and CompleteCheckpoint
    //    4. Secondary gets the copy stream
    //    5. Verify Secondary copied all the states in the new checkpoint
    Awaitable<void> VolatileTest::Test_Copy_CompleteCheckpoint_Race(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Expected
        KArray<KUri::CSPtr> addsBeforeCheckpoint(GetAllocator(), numAddsBeforeCheckpoint);
        KArray<KUri::CSPtr> changesAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // This array only used in the remove condition, which indicates the state providers added before but not removed.
        KArray<KUri::CSPtr> notChangedAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // In add condition:
        // addsBeforeCheckpoint has all the state providers added in the first checkpoint
        // changesAfterCheckpoint has all the state providers added in the second checkpoint
        //
        // In remove condition:
        // addsBeforeCheckpoint has all the added state providers before the checkpoint
        // changesAfterCheckpoint has all the removed state providers after the checkpoint
        // notChangedAfterCheckpoint has all the left state providers after the checkpoint
        for (ULONG index = 0; index < (isAddChange ? numAddsBeforeCheckpoint + numChangesAfterCheckpoint : numAddsBeforeCheckpoint); index++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            if (index < numAddsBeforeCheckpoint)
            {
                status = addsBeforeCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (isAddChange && index >= numAddsBeforeCheckpoint)
            {
                status = changesAfterCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (!isAddChange)
            {
                // Remove state provider number can not be more then added number
                CODING_ERROR_ASSERT(numChangesAfterCheckpoint <= numAddsBeforeCheckpoint);

                if (index < numChangesAfterCheckpoint)
                {
                    status = changesAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
                else
                {
                    status = notChangedAfterCheckpoint.Append(name);
                    CODING_ERROR_ASSERT(NT_SUCCESS(status));
                }
            }
        }

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Add some state providers
            co_await this->PopulateAsync(addsBeforeCheckpoint);
            VerifyExist(addsBeforeCheckpoint, true);

            // Take checkpoint, which considered as base checkpoint
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Add more state providers or remove partial state providers for the new checkpoint 
            if (isAddChange)
            {
                // added more state providers and verify all added exist
                co_await this->PopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, true);
            }
            else
            {
                // remove some state providers and verify removed ones got removed, and reset still exist
                co_await this->DepopulateAsync(changesAfterCheckpoint);
                VerifyExist(changesAfterCheckpoint, false);
                VerifyExist(notChangedAfterCheckpoint, true);
            }

            KArray<Awaitable<void>> taskArray(GetAllocator());

            AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
            status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Set the race between copy and CompleteCheckpoint
            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
            status = taskArray.Append(CompleteCheckpointTask(*signalCompletion));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = taskArray.Append(CopyTask(*signalCompletion, targetReplicaId, copyStream));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            signalCompletion->SetResult(true);
            co_await (TaskUtilities<void>::WhenAll(taskArray));

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            // Secondary gets the copy stream
            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // In this case we should get the second checkpoint states
            // In add condition:
            //     All the state providers added before base checkpoint should exist, all the state providers added after base checkpoint 
            //     should also exist
            // In remove condition:
            //     All the state providers removed after base checkpoint should not exist, rest exist.
            if (isAddChange)
            {
                VerifyExist(addsBeforeCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
                VerifyExist(changesAfterCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            }
            else
            {
                VerifyExist(notChangedAfterCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
                VerifyExist(changesAfterCheckpoint, false, secondaryTransactionalReplicator->StateManager.RawPtr());
            }

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    // Goal: Copy the states which metadata size is 32K or 64K
    // Note: The DesiredBlockSize is 32K, when we write the Metadata to the checkpoint file, 
    //       we flush the metadata record to the stream every time the size reaches DesiredBlockSize.
    //
    // Algorithm:
    //    1. Add some state providers which checkpoint file size is 32K
    //    2. Take checkpoint 
    //    3. Copy
    //    4. Secondary gets the copy stream
    //    5. Verify Secondary copied all the states in the checkpoint
    Awaitable<void> VolatileTest::Test_Copy_MetadataSize32KOr64K(
        __in bool isFileSize32K,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // numberOfItems is set to 160 when we require 32K checkpoint file, and set to 
        // 330 if we need 64K checkpoint file size.
        ULONG numberOfItems = isFileSize32K ? 160 : 330;

        // Expected
        KArray<KUri::CSPtr> nameList(GetAllocator(), numberOfItems);
        for (ULONG index = 0; index < numberOfItems; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = nameList.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await this->PopulateAsync(nameList);
            VerifyExist(nameList, true);
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // Verify
            VerifyExist(nameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    // Goal: Simulate the situation that PrepareCheckpoint-PrepareCheckpoint-PreformCheckpoint-CompleteCheckpoint
    //
    // Algorithm:
    //    1. Add some state providers and do the first PrepareCheckpoint
    //    2. Remove some state providers and do the checkpoint(PrepareCheckpoint-PreformCheckpoint-CompleteCheckpoint)
    //    3. Copy
    //    4. Secondary gets the copy stream
    //    5. Verify Secondary copied all the states in the second PrepareCheckpoint
    Awaitable<void> VolatileTest::Test_PrepareCheckpoint_CallTwice_CheckpointSecondPrepare(
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        const ULONG numberOfAdds = 10;
        const ULONG numberOfRemoves = 5;

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
        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Add some state providers
            co_await this->PopulateAsync(nameList);
            VerifyExist(nameList, true);

            // First PrepareCheckpoint included all the state providers in nameList
            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();

            // Removed all the state providers in the unexpectedNameList and take another checkpoint
            co_await DepopulateAsync(unexpectedNameList);
            VerifyExist(expectedNameList, true);
            VerifyExist(unexpectedNameList, false);

            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Get the copy stream 
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            // Secondary gets the copy stream
            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // Verify
            VerifyExist(expectedNameList, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(unexpectedNameList, false, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    // Goal: Test the situation that state provider has nothing to copy
    //
    // Algorithm:
    //    1. Add some state providers and do the first checkpoint
    //    2. Add more state providers do the PrepareCheckpoint and PerformCheckpoint
    //    3. Copy and CompleteCheckpoint (The state providers added in the step 2 have nothing to copy)
    //    4. Secondary gets the copy stream
    //    5. Verify Secondary copied all the states in the second PrepareCheckpoint
    Awaitable<void> VolatileTest::Test_Copy_StateProvider_Empty_State(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool recoverInPlace) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Expected
        KArray<KUri::CSPtr> addsBeforeCheckpoint(GetAllocator(), numAddsBeforeCheckpoint);
        KArray<KUri::CSPtr> changesAfterCheckpoint(GetAllocator(), numChangesAfterCheckpoint);

        // In add condition:
        // addsBeforeCheckpoint has all the state providers added in the first checkpoint
        // changesAfterCheckpoint has all the state providers added in the second checkpoint
        for (ULONG index = 0; index < numAddsBeforeCheckpoint + numChangesAfterCheckpoint; index++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            if (index < numAddsBeforeCheckpoint)
            {
                status = addsBeforeCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }

            if (index >= numAddsBeforeCheckpoint)
            {
                status = changesAfterCheckpoint.Append(name);
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
            }
        }

        OperationDataStream::SPtr copyStream;
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Populate
            co_await this->PopulateAsync(addsBeforeCheckpoint);
            VerifyExist(addsBeforeCheckpoint, true);

            // The first checkpoint
            co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // added more state providers and verify all added exist
            co_await this->PopulateAsync(changesAfterCheckpoint);
            VerifyExist(changesAfterCheckpoint, true);

            co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
            co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);

            // So the state providers in first checkpoint will have states, and the state providers
            // added in the second checkpoint will have empty state.
            FABRIC_REPLICA_ID targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        {
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateVolatileReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);

            // In add condition:
            //     All the state providers added before base checkpoint should exist, all the state providers added after base checkpoint 
            //     should also exist
            VerifyExist(addsBeforeCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(changesAfterCheckpoint, true, secondaryTransactionalReplicator->StateManager.RawPtr());

            // Clean up
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(VolatileTestSuite, VolatileTest)
    
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
    // Scenario:        Copying an empty SM that has not been checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: No state provider should be copied.
    // 
    BOOST_AUTO_TEST_CASE(Copy_NoCheckpoint_NothingIsCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(0, 0, false, false));
    }

    //
    // Scenario:        Copy an SM with np state providers that are not checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: No state provider should be copied.
    // 
    BOOST_AUTO_TEST_CASE(Copy_EmptyCheckpoint_NothingIsCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(0, 0, true, false));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers that are not checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: No state provider should be copied.
    // 
    BOOST_AUTO_TEST_CASE(Copy_NoCheckpoint_MultipleActiveStateProviders_NothingIsCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 0, false, false));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers that are checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(Copy_Checkpoint_MultipleActiveStateProviders_AllActiveIsCopied)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 0, true, false));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers that are checkpointed.
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers are copied.
    // 
    BOOST_AUTO_TEST_CASE(Copy_AllCopiedStateProvidersAlreadyExist_ReuseRecoveredStateProviders)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 0, true, true));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers that are checkpointed.
    //                  Idle secondary getting the copy has all soft deleted state providers already.
    // Expected Result: State provides are copied in deleted mode and not recreated.
    // 
    BOOST_AUTO_TEST_CASE(Copy_AllSoftDeleted_ReuseRecoveredStateProviders)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 16, true, true));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers and multiple soft deleted state providers 
    //                  that are checkpointed.
    //                  Idle secondary getting the copy has all the active and soft deleted state providers already.
    // Expected Result: All active are resurrected.
    // 
    BOOST_AUTO_TEST_CASE(Copy_SomeSoftDeleted_SomeActive_ReuseRecoveredStateProviders)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 8, true, true));
    }

    //
    // Scenario:        Copy an SM with multiple active state providers and multiple soft deleted state providers 
    //                  that are checkpointed.
    //                  Idle secondary getting the copy is empty.
    // Expected Result: All state providers are copied. All are created.
    // 
    BOOST_AUTO_TEST_CASE(Copy_SomeSoftDeleted_SomeActive_NewSecondary)
    {
        SyncAwait(this->Test_Copy_MultipleStateProvider_AllActiveCopied(16, 8, true, false));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedDeletedMetadataAlready_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 16, false));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedDeletedMetadataAlready_InPlace_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 16, true));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedActiveMetadataAlready_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 0, false));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedActiveMetadataAlready_InPlace_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 0, true));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedActiveMetadataAndDeletedMetadataAlready_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 8, false));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_Duplicate_CopiedActiveMetadataAndDeletedMetadataAlready_InPlace_Nooped)
    {
        SyncAwait(this->Test_SecondaryApply_Duplicate_AlreadyDeleted_NoopAndNoContext(16, 8, true));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_New_Applied)
    {
        SyncAwait(this->Test_SecondaryApply_New_StateProvidersAdded(16, 16, false));
    }

    BOOST_AUTO_TEST_CASE(SM_SecondaryApply_New_StaleOptimization_Applied)
    {
        SyncAwait(this->Test_SecondaryApply_New_StateProvidersAdded(16, 16, true));
    }

    BOOST_AUTO_TEST_CASE(Apply_CopyEmpty_ApplyInsertAndDelete_SoftDeleted)
    {
        SyncAwait(this->Test_Apply_CopyEmpty_InsertAndDelete_ItemDeleted(16, false));
    }

    BOOST_AUTO_TEST_CASE(Apply_CopyEmpty_ApplyInsertAndDelete_Inplace_SoftDeleted)
    {
        SyncAwait(this->Test_Apply_CopyEmpty_InsertAndDelete_ItemDeleted(16, true));
    }

    BOOST_AUTO_TEST_CASE(Apply_CopyInsert_ApplyDelete_SoftDeleted)
    {
        SyncAwait(this->Test_Apply_CopyInsert_Delete_ItemDeleted(16, false));
    }

    BOOST_AUTO_TEST_CASE(Apply_CopyInsert_ApplyDelete_Inplace_SoftDeleted)
    {
        SyncAwait(this->Test_Apply_CopyInsert_Delete_ItemDeleted(16, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PrepareCheckpoint function.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers added after the first checkpoint should not exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PrepareCheckpoint_BaseCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(16, 8, true, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PrepareCheckpoint function.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers removed after the first checkpoint should also exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PrepareCheckpoint_BaseCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(16, 8, false , true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PrepareCheckpoint function,
    //                  The base checkpoint should be copied, Idle secondary getting the copy 
    //                  has all the state providers already.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers added after the first checkpoint should not exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PrepareCheckpoint_Inplace_BaseCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(16, 8, true, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PrepareCheckpoint function,
    //                  The base checkpoint should be copied, Idle secondary getting the copy 
    //                  has all the state providers already.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers removed after the first checkpoint should also exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PrepareCheckpoint_Inplace_BaseCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PrepareCheckpoint_Race_CopyBasepoint(16, 8, false, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens before PerformCheckpoint, so the base checkpoint should be copied
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers added after the first checkpoint should not exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_BaseCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, true, true, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens before PerformCheckpoint, so the base checkpoint should be copied
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers added after the first checkpoint should not exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_Inplace_BaseCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, true, false, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens after PerformCheckpoint, so the second checkpoint should be copied
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_NewCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, true, true, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens after PerformCheckpoint, so the second checkpoint should be copied
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_Inplace_NewCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, true, false, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens before PerformCheckpoint, so the base checkpoint should be copied
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers removed after the first checkpoint should also exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_BaseCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, false, true, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens before PerformCheckpoint, so the base checkpoint should be copied
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers in the first checkpoint should be copied, 
    //                  state providers removed after the first checkpoint should also exist
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_Inplace_BaseCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, false, false, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens after PerformCheckpoint, so the second checkpoint should be copied
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_NewCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, false, true, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint PerformCheckpoint function,
    //                  Copy happens after PerformCheckpoint, so the second checkpoint should be copied
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_PerformCheckpoint_Inplace_NewCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_PerformCheckpoint_Race(16, 8, false, false, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function,
    //                  the second checkpoint should be copied
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_CompleteCheckpoint_NewCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_CompleteCheckpoint_Race(16, 8, true, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function,
    //                  the second checkpoint should be copied. Idle secondary getting the copy 
    //                  has all the state providers already.
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_CompleteCheckpoint_Inplace_NewCheckpointCopied_AddCase)
    {
        SyncAwait(this->Test_Copy_CompleteCheckpoint_Race(16, 8, true, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function,
    //                  the second checkpoint should be copied
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_CompleteCheckpoint_NewCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_CompleteCheckpoint_Race(16, 8, false, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function,
    //                  the second checkpoint should be copied. Idle secondary getting the copy 
    //                  has all the state providers already.
    // Expected Result: All state providers in the second checkpoint should be copied, 
    // 
    BOOST_AUTO_TEST_CASE(Copy_CompleteCheckpoint_Inplace_NewCheckpointCopied_RemoveCase)
    {
        SyncAwait(this->Test_Copy_CompleteCheckpoint_Race(16, 8, false, false));
    }

    //
    // Scenario:        Copy the current state, which the checkpoint file size is 32k
    // Expected Result: All state providers Copied
    // 
    BOOST_AUTO_TEST_CASE(Copy_Checkpoint_32K)
    {
        SyncAwait(this->Test_Copy_MetadataSize32KOr64K(true, false));
    }

    //
    // Scenario:        Copy the current state, which the checkpoint file size is 32k
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers Copied
    // 
    BOOST_AUTO_TEST_CASE(Copy_Checkpoint_32K_Inplace)
    {
        SyncAwait(this->Test_Copy_MetadataSize32KOr64K(true, true));
    }

    //
    // Scenario:        Copy the current state, which the checkpoint file size is 64k
    // Expected Result: All state providers Copied
    // 
    BOOST_AUTO_TEST_CASE(Copy_Checkpoint_64K)
    {
        SyncAwait(this->Test_Copy_MetadataSize32KOr64K(false, false));
    }

    //
    // Scenario:        Copy the current state, which the checkpoint file size is 64k
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: All state providers Copied
    // 
    BOOST_AUTO_TEST_CASE(Copy_Checkpoint_64K_Inplace)
    {
        SyncAwait(this->Test_Copy_MetadataSize32KOr64K(false, true));
    }

    //
    // Scenario:        PrepareCheckpoint, Operation, PrepareCheckpoint, then PreformCheckpoint and CompleteCheckpoint.
    // Expected Result: Copy all the states in second prepare.
    // 
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_CallTwice_CheckpointSecondPrepare)
    {
        SyncAwait(this->Test_PrepareCheckpoint_CallTwice_CheckpointSecondPrepare(false));
    }

    //
    // Scenario:        PrepareCheckpoint, Operation, PrepareCheckpoint, then PreformCheckpoint and CompleteCheckpoint.
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: Copy all the states in second prepare.
    // 
    BOOST_AUTO_TEST_CASE(PrepareCheckpoint_CallTwice_CheckpointSecondPrepare_Inplace)
    {
        SyncAwait(this->Test_PrepareCheckpoint_CallTwice_CheckpointSecondPrepare(true));
    }

    //
    // Scenario:        Test used to cover the case that state provider has nothing to copy,
    //                  Add some state provider first and checkpoint for the first time, 
    //                  add more state provider, PrepareCheckpoint, PerformCheckpoint, get the copy steam then Complete the 
    //                  checkpoint, in this way, these state providers get copied but have no state.
    // Expected Result: Copy all the states in second prepare.
    // 
    BOOST_AUTO_TEST_CASE(Copy_StateProvider_EmptyState)
    {
        SyncAwait(this->Test_Copy_StateProvider_Empty_State(16, 8, false));
    }

    //
    // Scenario:        Test used to cover the case that state provider has nothing to copy,
    //                  Add some state provider first and checkpoint for the first time, 
    //                  add more state provider, PrepareCheckpoint, PerformCheckpoint, get the copy steam then Complete the 
    //                  checkpoint, in this way, these state providers get copied but have no state.
    //                  Idle secondary getting the copy has all the state providers already.
    // Expected Result: Copy all the states in second prepare.
    // 
    BOOST_AUTO_TEST_CASE(Copy_StateProvider_EmptyState_Inplace)
    {
        SyncAwait(this->Test_Copy_StateProvider_Empty_State(16, 8, true));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
