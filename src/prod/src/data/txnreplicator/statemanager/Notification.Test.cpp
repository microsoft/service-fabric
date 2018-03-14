// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

using namespace Common;
using namespace std;
using namespace ktl;
using namespace TxnReplicator;
using namespace Data::StateManager;
using namespace Data::Utilities;

namespace StateManagerTests
{
    class NotificationTest : public StateManagerTestBase
    {
        // Load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        NotificationTest()
        {
        }

        ~NotificationTest()
        {
        }

        Awaitable<void> Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            __in ULONG numberOfStateProviders,
            __in bool explicitAbort);

        Awaitable<void> Negative_AbortOrDispose_RemoveAsync_NotificationsAreNotFired(
            __in ULONG numberOfStateProviders,
            __in bool explicitAbort);

        Awaitable<void> Primary_AddAndRemoveStateProviders_NotificationCountIsCorrect(
            __in ULONG numberOfIterations,
            __in ULONG numberOfStateProvidersPerTxn,
            __in bool withRegisterAndUnRegister);

        Awaitable<void> Primary_RegisterUnRegister_RegisteredNotificationsAreFired(
            __in ULONG numberOfRegistered,
            __in ULONG numberOfUnRegistered);

        Awaitable<void> Recovery_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            __in ULONG numberOfRecoveredFromCheckpoint,
            __in ULONG numberOfAddsReplayed);

        Awaitable<void> FalseProgress_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            __in ULONG numberOfAddUndos,
            __in ULONG numberOfDeleteUndos);

        Awaitable<void> Copy_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            __in ULONG numberOfRecoveredFromCheckpoint,
            __in ULONG numberOfAddsReplayed,
            __in bool recoverInPlace);

    private:
        Awaitable<void> PopulateAsync(
            __in vector<KUri::CSPtr> & nameList,
            __in TestStateManagerChangeHandler & handler,
            __in ULONG numberOfIterations,
            __in ULONG numberOfStateProvidersPerTxn,
            __in bool withRegisterAndUnRegister);

        Awaitable<void> DepopulateAsync(
            __in vector<KUri::CSPtr> & nameList,
            __in TestStateManagerChangeHandler & handler,
            __in ULONG numberOfIterations,
            __in ULONG numberOfStateProvidersPerTxn,
            __in bool withRegisterAndUnRegister);
    };

    Awaitable<void> NotificationTest::Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
        __in ULONG numberOfStateProviders,
        __in bool explicitAbort)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto handler = TestStateManagerChangeHandler::Create(GetAllocator());

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Register
        status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Transaction that adds state providers and then aborts.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            for (ULONG i = 0; i < numberOfStateProviders; i++)
            {
                auto stateProviderName = GetStateProviderName(NameType::Random);

                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            if (explicitAbort)
            {
                txnSPtr->Abort();
            }
        }

        // Verification
        VERIFY_ARE_EQUAL(0, handler->RebuildCount);
        VERIFY_ARE_EQUAL(0, handler->AddCount);
        VERIFY_ARE_EQUAL(0, handler->RemoveCount);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> NotificationTest::Negative_AbortOrDispose_RemoveAsync_NotificationsAreNotFired(
        __in ULONG numberOfStateProviders,
        __in bool explicitAbort)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        vector<KUri::CSPtr> nameList(numberOfStateProviders);
        for (ULONG i = 0; i < numberOfStateProviders; i++)
        {
            nameList.push_back(GetStateProviderName(NameType::Random));
        }

        auto handler = TestStateManagerChangeHandler::Create(GetAllocator());

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Register
        status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Preparation: Add the state providers so that they can be removed.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            for (ULONG i = 0; i < numberOfStateProviders; i++)
            {
                auto stateProviderName = nameList[i];
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Transaction that removes state providers and then aborts.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            for (ULONG i = 0; i < numberOfStateProviders; i++)
            {
                auto stateProviderName = nameList[i];
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            if (explicitAbort)
            {
                txnSPtr->Abort();
            }
        }

        // Verification
        VERIFY_ARE_EQUAL(0, handler->RebuildCount);
        VERIFY_ARE_EQUAL(0, handler->AddCount);
        VERIFY_ARE_EQUAL(0, handler->RemoveCount);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> NotificationTest::Primary_AddAndRemoveStateProviders_NotificationCountIsCorrect(
        __in ULONG numberOfIterations,
        __in ULONG numberOfStateProvidersPerTxn,
        __in bool withUnregisterAndRegisterLoop)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        ULONG expectedCount = numberOfIterations * numberOfStateProvidersPerTxn;

        vector<KUri::CSPtr> nameList;
        for (ULONG i = 0; i < expectedCount; i++)
        {
            nameList.push_back(GetStateProviderName(NameType::Random));
        }

        auto handler = TestStateManagerChangeHandler::Create(GetAllocator());

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Register
        status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Add State Providers.
        co_await PopulateAsync(nameList, *handler, numberOfIterations, numberOfStateProvidersPerTxn, withUnregisterAndRegisterLoop);

        VERIFY_ARE_EQUAL(0, handler->RebuildCount);
        VERIFY_ARE_EQUAL(expectedCount, handler->AddCount);
        VERIFY_ARE_EQUAL(0, handler->RemoveCount);

        // Transaction that removes state providers and then aborts.
        co_await DepopulateAsync(nameList, *handler, numberOfIterations, numberOfStateProvidersPerTxn, withUnregisterAndRegisterLoop);

        // Verification
        VERIFY_ARE_EQUAL(0, handler->RebuildCount);
        VERIFY_ARE_EQUAL(expectedCount, handler->AddCount);
        VERIFY_ARE_EQUAL(expectedCount, handler->RemoveCount);

        Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
        status = stateManagerSPtr->CreateEnumerator(false, enumerator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        handler->VerifyState(*enumerator);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> NotificationTest::Primary_RegisterUnRegister_RegisteredNotificationsAreFired(
        __in ULONG numberOfRegistered,
        __in ULONG numberOfUnRegistered)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        vector<KUri::CSPtr> registeredNameList;
        for (ULONG i = 0; i < numberOfRegistered; i++)
        {
            registeredNameList.push_back(GetStateProviderName(NameType::Random));
        }

        vector<KUri::CSPtr> unregisteredNameList;
        for (ULONG i = 0; i < numberOfUnRegistered; i++)
        {
            unregisteredNameList.push_back(GetStateProviderName(NameType::Random));
        }

        auto handlerAdd = TestStateManagerChangeHandler::Create(GetAllocator());
        auto handlerRemove = TestStateManagerChangeHandler::Create(GetAllocator());

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Registered Test
        status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handlerAdd);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        co_await PopulateAsync(registeredNameList, *handlerAdd, numberOfRegistered, 1, false);

        // UnRegistered Test
        status = stateManagerSPtr->UnRegisterStateManagerChangeHandler();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        co_await PopulateAsync(unregisteredNameList, *handlerAdd, numberOfUnRegistered, 1, false);

        VERIFY_ARE_EQUAL(0, handlerAdd->RebuildCount);
        VERIFY_ARE_EQUAL(numberOfRegistered, handlerAdd->AddCount);
        VERIFY_ARE_EQUAL(0, handlerAdd->RemoveCount);

        // Registered Test
        status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handlerRemove);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        co_await DepopulateAsync(registeredNameList, *handlerRemove, numberOfRegistered, 1, false);

        // UnRegistered Test
        status = stateManagerSPtr->UnRegisterStateManagerChangeHandler();
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        co_await DepopulateAsync(unregisteredNameList, *handlerRemove, numberOfUnRegistered, 1, false);

        // Verification
        VERIFY_ARE_EQUAL(0, handlerAdd->RebuildCount);
        VERIFY_ARE_EQUAL(numberOfRegistered, handlerAdd->AddCount);
        VERIFY_ARE_EQUAL(0, handlerAdd->RemoveCount);

        VERIFY_ARE_EQUAL(0, handlerRemove->RebuildCount);
        VERIFY_ARE_EQUAL(0, handlerRemove->AddCount);
        VERIFY_ARE_EQUAL(numberOfRegistered, handlerRemove->RemoveCount);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> NotificationTest::Recovery_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
        __in ULONG numberOfRecoveredFromCheckpoint, 
        __in ULONG numberOfAddsReplayed)
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> checkpointNameList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        for (ULONG index = 0; index < numberOfRecoveredFromCheckpoint; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = checkpointNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> checkpointStaleNameList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        for (ULONG index = 0; index < numberOfRecoveredFromCheckpoint; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = checkpointStaleNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> addReplayNameList(GetAllocator(), numberOfAddsReplayed);
        for (ULONG index = 0; index < numberOfAddsReplayed; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = addReplayNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> checkpointedIdList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        KArray<LONG64> checkpointedStaleIdList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        KArray<LONG64> notCheckpointedAddIdList(GetAllocator(), numberOfAddsReplayed);
        LONG64 prepareCheckpointLSN = 0;
        LONG64 firstRemoveCheckpointLSN = 0;
        {
            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Stage 1: Generate a checkpoint with relevant add and stale state providers.
            co_await StateManagerTestBase::PopulateAsync(checkpointNameList);
            checkpointedIdList = StateManagerTestBase::PopulateStateProviderIdArray(checkpointNameList);

            co_await StateManagerTestBase::PopulateAsync(checkpointStaleNameList);
            checkpointedStaleIdList = StateManagerTestBase::PopulateStateProviderIdArray(checkpointStaleNameList);

            status = testTransactionalReplicatorSPtr_->GetLastCommittedSequenceNumber(firstRemoveCheckpointLSN);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            firstRemoveCheckpointLSN += 1;

            co_await StateManagerTestBase::DepopulateAsync(checkpointStaleNameList);
            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Stage 2: Generate state provider ids to replay adds and removes.
            co_await StateManagerTestBase::PopulateAsync(addReplayNameList);
            notCheckpointedAddIdList = StateManagerTestBase::PopulateStateProviderIdArray(addReplayNameList);
            
            // Stage 3: Close the replica.
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }
       
        // Test
        {
            // Setup new instance of the replica.
            LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = secondaryTransactionalReplicator->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Stage 4: Restart and recover checkpoint.
            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(checkpointNameList);

            // Stage 5: Duplicate adds do not fire notifications
            // Note that deleteIdList contains the ids of state providers should be in "added" mode in checkpoint.
            KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr contextArray = co_await ApplyMultipleAsync(
                checkpointNameList,
                checkpointedIdList,
                0,
                checkpointedIdList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == 0);

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(checkpointNameList);

            // Stage 6: Duplicate removes do not fire notifications
            contextArray = co_await ApplyMultipleAsync(
                checkpointStaleNameList,
                checkpointedStaleIdList,
                0,
                checkpointedStaleIdList.Count(),
                firstRemoveCheckpointLSN,
                firstRemoveCheckpointLSN,
                ApplyType::Delete,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == 0);

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(checkpointNameList);

            // Stage 7: Verify Add replay notifications.
            contextArray = co_await ApplyMultipleAsync(
                addReplayNameList,
                notCheckpointedAddIdList,
                0,
                notCheckpointedAddIdList.Count(),
                prepareCheckpointLSN + 1,
                prepareCheckpointLSN + 1,
                ApplyType::Insert,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == addReplayNameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(notCheckpointedAddIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(addReplayNameList);

            // Stage 8: Verify Removes replay notifications.
            contextArray = co_await ApplyMultipleAsync(
                checkpointNameList,
                checkpointedIdList,
                0,
                checkpointedIdList.Count(),
                prepareCheckpointLSN + addReplayNameList.Count() + 1,
                prepareCheckpointLSN + addReplayNameList.Count() + 1,
                ApplyType::Delete,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == checkpointNameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(notCheckpointedAddIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(checkpointedIdList.Count(), handler->RemoveCount);

            handler->VerifyExits(addReplayNameList);
            handler->VerifyExits(checkpointNameList, false);

            // Stage 9: Verify full state.
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);


            Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
            status = stateManagerSPtr->CreateEnumerator(false, enumerator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            handler->VerifyState(*enumerator);

            // Shutdown
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> NotificationTest::FalseProgress_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
        __in ULONG numberOfAddUndos, 
        __in ULONG numberOfDeleteUndos)
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> addUndoNameList(GetAllocator(), numberOfAddUndos);
        for (ULONG index = 0; index < numberOfAddUndos; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = addUndoNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> deleteUndoNameList(GetAllocator(), numberOfDeleteUndos);
        for (ULONG index = 0; index < numberOfDeleteUndos; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = deleteUndoNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> addIdList(GetAllocator(), numberOfAddUndos);
        KArray<LONG64> deleteIdList(GetAllocator(), numberOfDeleteUndos);

        LONG64 prepareCheckpointLSN = 0;
        {
            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Stage 1: Peparing for deletes we will false progress.
            //          Checkpoint to be deleted and false progressed state providers.
            co_await StateManagerTestBase::PopulateAsync(deleteUndoNameList);
            deleteIdList = StateManagerTestBase::PopulateStateProviderIdArray(deleteUndoNameList);

            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            // Stage 2: Peparing for adds we will false progress.
            co_await StateManagerTestBase::PopulateAsync(addUndoNameList);
            addIdList = StateManagerTestBase::PopulateStateProviderIdArray(addUndoNameList);

            // Stage 3: Close the replica.
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        // Test
        {
            // Setup new instance of the replica.
            LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = secondaryTransactionalReplicator->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Stage 4: Restart and recover checkpoint.
            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(addUndoNameList, false);
            handler->VerifyExits(deleteUndoNameList, true);

            // Stage 5: Apply Add operations that will be undone.
            KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr contextArray = co_await ApplyMultipleAsync(
                addUndoNameList,
                addIdList,
                0,
                addIdList.Count(),
                prepareCheckpointLSN + 1,
                prepareCheckpointLSN + 1,
                ApplyType::Insert,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == addIdList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(addIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            // Stage 6: Apply delete operations that will be undone.
            contextArray = co_await ApplyMultipleAsync(
                deleteUndoNameList,
                deleteIdList,
                0,
                deleteIdList.Count(),
                prepareCheckpointLSN + 1 + addIdList.Count(),
                prepareCheckpointLSN + 1 + addIdList.Count(),
                ApplyType::Delete,
                ApplyContext::RecoveryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == addIdList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(addIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(deleteIdList.Count(), handler->RemoveCount);

            // Stage 7: Become Idle Secondary.
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);

            // Stage 8: Undo Adds.
            contextArray = co_await ApplyMultipleAsync(
                addUndoNameList,
                addIdList,
                0,
                addIdList.Count(),
                prepareCheckpointLSN + 1,
                prepareCheckpointLSN + 1,
                ApplyType::Insert,
                ApplyContext::SecondaryFalseProgress,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(0 == contextArray->Count());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(addIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(deleteIdList.Count() + addIdList.Count(), handler->RemoveCount);

            // Stage 9: Undo Deletes.
            contextArray = co_await ApplyMultipleAsync(
                deleteUndoNameList,
                deleteIdList,
                0,
                deleteIdList.Count(),
                prepareCheckpointLSN + 1 + addIdList.Count(),
                prepareCheckpointLSN + 1 + addIdList.Count(),
                ApplyType::Delete,
                ApplyContext::SecondaryFalseProgress,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(0 == contextArray->Count());

            VERIFY_ARE_EQUAL(1, handler->RebuildCount);
            VERIFY_ARE_EQUAL(addIdList.Count() + deleteIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(deleteIdList.Count() + addIdList.Count(), handler->RemoveCount);

            // Stage 10: Verify full state.
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);
            
            Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
            status = stateManagerSPtr->CreateEnumerator(false, enumerator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            handler->VerifyState(*enumerator);

            // Shutdown
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> NotificationTest::Copy_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            __in ULONG numberOfRecoveredFromCheckpoint,
            __in ULONG numberOfAddsReplayed,
            __in bool recoverInPlace)
    {
        NTSTATUS status;

        // Expected
        KArray<KUri::CSPtr> checkpointNameList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        for (ULONG index = 0; index < numberOfRecoveredFromCheckpoint; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = checkpointNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> checkpointStaleNameList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        for (ULONG index = 0; index < numberOfRecoveredFromCheckpoint; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = checkpointStaleNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> addReplayNameList(GetAllocator(), numberOfAddsReplayed);
        for (ULONG index = 0; index < numberOfAddsReplayed; index++)
        {
            auto name = GetStateProviderName(NameType::Random);
            status = addReplayNameList.Append(name);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        // Setup
        KArray<LONG64> checkpointedIdList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        KArray<LONG64> checkpointedStaleIdList(GetAllocator(), numberOfRecoveredFromCheckpoint);
        KArray<LONG64> notCheckpointedAddIdList(GetAllocator(), numberOfAddsReplayed);
        LONG64 prepareCheckpointLSN = 0;
        LONG64 firstRemoveCheckpointLSN = 0;
        OperationDataStream::SPtr copyStream;
        {
            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

            // Stage 1: Generate a checkpoint with relevant add and stale state providers.
            co_await StateManagerTestBase::PopulateAsync(checkpointNameList);
            checkpointedIdList = StateManagerTestBase::PopulateStateProviderIdArray(checkpointNameList);

            co_await StateManagerTestBase::PopulateAsync(checkpointStaleNameList);
            checkpointedStaleIdList = StateManagerTestBase::PopulateStateProviderIdArray(checkpointStaleNameList);

            status = testTransactionalReplicatorSPtr_->GetLastCommittedSequenceNumber(firstRemoveCheckpointLSN);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            firstRemoveCheckpointLSN += 1;

            co_await StateManagerTestBase::DepopulateAsync(checkpointStaleNameList);
            prepareCheckpointLSN = co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

            LONG64 targetReplicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            status = testTransactionalReplicatorSPtr_->StateManager->GetCurrentState(targetReplicaId, copyStream);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Stage 2: Generate state provider ids to replay adds and removes.
            co_await StateManagerTestBase::PopulateAsync(addReplayNameList);
            notCheckpointedAddIdList = StateManagerTestBase::PopulateStateProviderIdArray(addReplayNameList);

            // Clean up
            if (recoverInPlace == false)
            {
                co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            }

            // Stage 3: Close the replica.
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        // Test
        {
            // Setup new instance of the replica.
            LONG64 replicaId = recoverInPlace ? partitionedReplicaIdCSPtr_->ReplicaId : partitionedReplicaIdCSPtr_->ReplicaId + 1;
            ULONG expectedRebuildCount = recoverInPlace ? 1 : 0;
            auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

            auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
            auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
            TestTransactionManager::SPtr secondaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

            auto handler = TestStateManagerChangeHandler::Create(GetAllocator());
            auto stateManagerSPtr = secondaryTransactionalReplicator->StateManager;
            status = stateManagerSPtr->RegisterStateManagerChangeHandler(*handler);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Stage 4: Restart and recover checkpoint.
            co_await secondaryTransactionalReplicator->OpenAsync(CancellationToken::None);

            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);
            handler->VerifyExits(checkpointNameList, recoverInPlace);

            // Stage 5: Copy from Primary
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CopyAsync(*copyStream, CancellationToken::None);
            expectedRebuildCount++;

            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);
            handler->VerifyExits(checkpointNameList);

            // Stage 5: Duplicate adds do not fire notifications
            // Note that deleteIdList contains the ids of state providers should be in "added" mode in checkpoint.
            KSharedArray<TxnReplicator::OperationContext::CSPtr>::SPtr contextArray = co_await ApplyMultipleAsync(
                checkpointNameList,
                checkpointedIdList,
                0,
                checkpointedIdList.Count(),
                1,
                1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == 0);
            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);
            handler->VerifyExits(checkpointNameList);

            // Stage 6: Duplicate removes do not fire notifications
            contextArray = co_await ApplyMultipleAsync(
                checkpointStaleNameList,
                checkpointedStaleIdList,
                0,
                checkpointedStaleIdList.Count(),
                firstRemoveCheckpointLSN,
                firstRemoveCheckpointLSN,
                ApplyType::Delete,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == 0);
            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(0, handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);
            handler->VerifyExits(checkpointNameList);

            // Stage 7: Verify Add replay notifications.
            contextArray = co_await ApplyMultipleAsync(
                addReplayNameList,
                notCheckpointedAddIdList,
                0,
                notCheckpointedAddIdList.Count(),
                prepareCheckpointLSN + 1,
                prepareCheckpointLSN + 1,
                ApplyType::Insert,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            // Indicates that all operations where applied.
            VERIFY_IS_TRUE(contextArray->Count() == addReplayNameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(notCheckpointedAddIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(0, handler->RemoveCount);

            handler->VerifyExits(addReplayNameList);

            // Stage 8: Verify Removes replay notifications.
            contextArray = co_await ApplyMultipleAsync(
                checkpointNameList,
                checkpointedIdList,
                0,
                checkpointedIdList.Count(),
                prepareCheckpointLSN + addReplayNameList.Count() + 1,
                prepareCheckpointLSN + addReplayNameList.Count() + 1,
                ApplyType::Delete,
                ApplyContext::SecondaryRedo,
                secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_IS_TRUE(contextArray->Count() == checkpointNameList.Count());
            Unlock(*contextArray, secondaryTransactionalReplicator->StateManager.RawPtr());

            VERIFY_ARE_EQUAL(expectedRebuildCount, handler->RebuildCount);
            VERIFY_ARE_EQUAL(notCheckpointedAddIdList.Count(), handler->AddCount);
            VERIFY_ARE_EQUAL(checkpointedIdList.Count(), handler->RemoveCount);

            handler->VerifyExits(addReplayNameList);
            handler->VerifyExits(checkpointNameList, false);

            // Stage 9: Verify full state.
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_IDLE_SECONDARY, CancellationToken::None);
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_ACTIVE_SECONDARY, CancellationToken::None);

            Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, IStateProvider2::SPtr>>::SPtr enumerator;
            status = stateManagerSPtr->CreateEnumerator(false, enumerator);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            handler->VerifyState(*enumerator);

            // Shutdown
            co_await secondaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await secondaryTransactionalReplicator->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> NotificationTest::PopulateAsync(
        __in vector<KUri::CSPtr>& nameList, 
        __in TestStateManagerChangeHandler & handler,
        __in ULONG numberOfIterations,
        __in ULONG numberOfStateProvidersPerTxn, 
        __in bool withRegisterAndUnRegister)
    {
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        for (ULONG iterationIndex = 0; iterationIndex < numberOfIterations; iterationIndex++)
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            for (ULONG opIndex = 0; opIndex < numberOfStateProvidersPerTxn; opIndex++)
            {
                auto stateProviderName = nameList[(iterationIndex * numberOfStateProvidersPerTxn) + opIndex];
                NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        co_return;
    }

    Awaitable<void> NotificationTest::DepopulateAsync(
        __in vector<KUri::CSPtr>& nameList,
        __in TestStateManagerChangeHandler & handler,
        __in ULONG numberOfIterations,
        __in ULONG numberOfStateProvidersPerTxn,
        __in bool withRegisterAndUnRegister)
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        for (ULONG iterationIndex = 0; iterationIndex < numberOfIterations; iterationIndex++)
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] {txnSPtr->Dispose(); });

            if (withRegisterAndUnRegister)
            {
                status = stateManagerSPtr->UnRegisterStateManagerChangeHandler();
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            for (ULONG opIndex = 0; opIndex < numberOfStateProvidersPerTxn; opIndex++)
            {
                auto stateProviderName = nameList[(iterationIndex * numberOfStateProvidersPerTxn) + opIndex];
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            if (withRegisterAndUnRegister)
            {
                status = stateManagerSPtr->RegisterStateManagerChangeHandler(handler);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(NotificationTestSuite, NotificationTest);

    //
    // Scenario:        Aborted transaction which had tried adding state providers.
    //                  Note that single operation optimization path is excercised.
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Abort_AddAsync_Single_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            1, 
            true /*explicitAbort*/));
    }

    //
    // Scenario:        Aborted transaction which had tried adding state providers.
    //                  Note that multiple operations are done in the transaction to avoid single operation optimization
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Abort_AddAsync_Multiple_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            8,
            true /*explicitAbort*/));
    }

    //
    // Scenario:        Disposed transaction which had tried adding state providers.
    //                  Note that single operation optimization path is excercised.
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Dispose_AddAsync_Single_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            1,
            false /*explicitAbort*/));
    }

    //
    // Scenario:        Disposed transaction which had tried adding state providers.
    //                  Note that multiple operations are done in the transaction to avoid single operation optimization
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Dispose_AddAsync_Multiple_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            8,
            false /*explicitAbort*/));
    }

    //
    // Scenario:        Aborted transaction which had tried removing state providers.
    //                  Note that single operation optimization path is excercised.
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Abort_RemoveAsync_Single_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            1,
            true /*explicitAbort*/));
    }

    //
    // Scenario:        Aborted transaction which had tried removing state providers.
    //                  Note that multiple operations are done in the transaction to avoid single operation optimization
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Abort_RemoveAsync_Multiple_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            8,
            true /*explicitAbort*/));
    }

    //
    // Scenario:        Disposed transaction which had tried removing state providers.
    //                  Note that single operation optimization path is excercised.
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Dispose_RemoveAsync_Single_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            1,
            false /*explicitAbort*/));
    }

    //
    // Scenario:        Disposed transaction which had tried removing state providers.
    //                  Note that multiple operations are done in the transaction to avoid single operation optimization
    // Expected Result: Not notification should be fired.
    // 
    BOOST_AUTO_TEST_CASE(Notification_Dispose_RemoveAsync_Multiple_NotificationsAreNotFired)
    {
        SyncAwait(this->Negative_AbortOrDispose_AddAsync_NotificationsAreNotFired(
            8,
            false /*explicitAbort*/));
    }

    //
    // Scenario:        Add muliple state providers and ensure that notifications are fired.
    //                  Note that single operation is done to excercise single operation optimization
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Primary_AddAndRemove_Single_NotificationCountIsCorrect)
    {
        SyncAwait(this->Primary_AddAndRemoveStateProviders_NotificationCountIsCorrect(
            8,
            1,
            false /* With UnRegister and Register */));
    }

    //
    // Scenario:        Add muliple state providers and ensure that notifications are fired.
    //                  Note that multiple operations are done in the transaction to avoid single operation optimization
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Primary_AddAndRemove_Multiple_NotificationCountIsCorrect)
    {
        SyncAwait(this->Primary_AddAndRemoveStateProviders_NotificationCountIsCorrect(
            8,
            8,
            false /* With UnRegister and Register */));
    }

    //
    // Scenario:        Testing that register and unregister while state providers are added and removed (not concurrent).
    //                  Unregisteration and registeration is done at times that would avoid loss notifications.
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Primary_RegisterAndUnRegister_NotificationCountIsCorrect)
    {
        SyncAwait(this->Primary_AddAndRemoveStateProviders_NotificationCountIsCorrect(
            8,
            8,
            true /* With UnRegister and Register */));
    }

    //
    // Scenario:        Register and Unregister test that ensures only registered notifications are recieved.
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Primary_RegisterUnRegister_RegisteredNotificationsAreFired)
    {
        SyncAwait(this->Primary_RegisterUnRegister_RegisteredNotificationsAreFired(
            8,
            8));
    }

    //
    // Scenario:        Tests recovery rebuild notification, as well as recovery apply notifications.
    //                  Rebuild contains stale state providers as well.
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Recovery_CheckpointAndLogReplay_RegisteredNotificationsAreFired)
    {
        SyncAwait(this->Recovery_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            8,
            8));
    }

    //
    // Scenario:        Tests false progress notification.
    //                  
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_FalseProgress_CheckpointAndLogReplay_RegisteredNotificationsAreFired)
    {
        SyncAwait(this->FalseProgress_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            8,
            8));
    }

    //
    // Scenario:        Copy Notifications: Rebuild + Applys from Primary
    //                  
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Copy_CheckpointAndLogReplay_RegisteredNotificationsAreFired)
    {
        SyncAwait(this->Copy_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            8,
            8,
            false /*inplace recover*/));
    }

    //
    // Scenario:        Copy Notifications: Rebuild + Applys from Primary
    //                  
    // Expected Result: All expected notifications fired. 
    // 
    BOOST_AUTO_TEST_CASE(Notification_Copy_CheckpointAndLogReplay_InPlace_RegisteredNotificationsAreFired)
    {
        SyncAwait(this->Copy_CheckpointAndLogReplay_RegisteredNotificationsAreFired(
            8,
            8,
            true /*inplace recover*/));
    }

    BOOST_AUTO_TEST_SUITE_END()
}
