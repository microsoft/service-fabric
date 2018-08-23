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
    class StateManagerTest : public StateManagerTestBase
    {
        // load the config object as its needed for the tracing to work
        CommonConfig config;

    public:
        StateManagerTest()
        {
        }

        ~StateManagerTest()
        {
        }

    public:
        enum BackupAsyncTestMode
        {
            EmptyState = 0,
            WithStates = 1,
            Checkpointed = 2,
        };

    public:
        Awaitable<void> Test_Life_CreateOpenClose_Success() noexcept;

        Awaitable<void> Test_Add_SingleOperation_Abort_Success(__in bool useDispose) noexcept;
        Awaitable<void> Test_Add_SingleOperation_Commit_Success() noexcept;
        Awaitable<void> Test_Add_BlockedReplication_InfiniteTimeout_CompleteOnceUnblocked() noexcept;
        Awaitable<void> Test_Add_InvalidName(__in NameType nameType) noexcept;

        Awaitable<void> Test_GetOrAdd_Success(__in bool isExist) noexcept;
        Awaitable<void> Test_GetOrAdd_BothPath_Return_OutStateProvider() noexcept;
        Awaitable<void> Test_GetOrAdd_ConcurrentMultipleCall_Success(__in ULONG taskNum) noexcept;
        Awaitable<void> Test_GetOrAdd_Abort_Success(__in bool isExist, __in bool useDispose) noexcept;
        Awaitable<void> Test_GetOrAdd_InvalidName(__in NameType nameType) noexcept;
        Awaitable<void> Test_GetOrAdd_ReadAccessIsNotPresent() noexcept;

        Awaitable<void> Test_Get_NotExist_Error() noexcept;
        Awaitable<void> Test_Get_InvalidName(__in NameType nameType) noexcept;

        Awaitable<void> Test_Remove_Commit_Success(__in ULONG32 numberOfStateProviders) noexcept;
        Awaitable<void> Test_Remove_Abort_NoEffect(__in ULONG32 numberOfStateProviders) noexcept;
        Awaitable<void> Test_Remove_Abort_ThenRemove_Commit(__in ULONG32 numberOfStateProviders) noexcept;

        Awaitable<void> Test_Dispatch_MultipleOperationTxn_Success(
            __in TestOperation::Type type,
            __in ULONG32 transactionCount,
            __in ULONG32 operationCount,
            __in bool nullOperationContext) noexcept;

        Awaitable<void> Test_Dispatch_AtomicOperationTxn_Success(
            __in TestOperation::Type type,
            __in ULONG32 operationCount,
            __in bool nullOperationContext) noexcept;

        Awaitable<void> Test_Dispatch_AtomicRedoOperationTxn_Success(
            __in TestOperation::Type type,
            __in ULONG32 operationCount,
            __in bool nullOperationContext) noexcept;

        Awaitable<void> Test_CreateEnumerable_LateBound() noexcept;

        Awaitable<void> Test_VerifyWriteFollowedByWriteToTheSameKeyInATx() noexcept;

        Awaitable<void> Test_VerifyWriteFollowedReadWithAddToTheSameKeyInATx() noexcept;

        Awaitable<void> Test_VerifyWriteFollowedReadWithRemoveToTheSameKeyInATx() noexcept;

        Awaitable<void> Test_VerifyReplicaCloseBeforeTxDispose(__in bool isAdd) noexcept;

        Awaitable<void> Test_Replica_Close_During_StateProvider_Operations_Throw() noexcept;

        Awaitable<void> Test_Replica_Change_Role_Retry_WhenUserServiceFailsChangeRole() noexcept;

        Awaitable<void> Test_AddOperation_StateProviderNotRegistered_Throw() noexcept;

        Awaitable<void> Test_AddOperation_OnRemovedStateProvider_Throw() noexcept;
        
        Awaitable<void> Test_AddOperation_OperationAfterRemoveThrow(__in ULONG taskCount) noexcept;

        Awaitable<void> Test_AddOperation_Concurrent_Tasks(__in ULONG taskCount) noexcept;

        Awaitable<void> Test_Replica_Closed_Throw() noexcept;

        Awaitable<void> Test_BackupAsync_FileSaved(
            __in BackupAsyncTestMode mode,
            __in ULONG numberOfStateProvider) noexcept;

        Awaitable<void> Test_BackupAndRestore(
            __in BackupAsyncTestMode mode,
            __in ULONG numberOfStateProvider) noexcept;

        Awaitable<void> Test_BackupAndRestore_NewStatesNotExist(
            __in BackupAsyncTestMode mode,
            __in ULONG numberOfStateProvider,
            __in ULONG numberOfNewStateProvider) noexcept;

        Awaitable<void> Test_Restore_NoBackupFile() noexcept;

        Awaitable<void> Test_Restore_DataCorruption(__in ULONG corruptPosition) noexcept;

        Awaitable<void> Test_CleanUpIncompleteRestore_DuringOpen() noexcept;

        Awaitable<void> Test_Backup_PrepareCheckpoint_Race_BackupBasepoint(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange) noexcept;

        Awaitable<void> Test_Backup_PerformCheckpoint_Race(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange,
            __in bool isBakcupFirst) noexcept;

        Awaitable<void> Test_Backup_CompleteCheckpoint_Race(
            __in ULONG32 numAddsBeforeCheckpoint,
            __in ULONG32 numChangesAfterCheckpoint,
            __in bool isAddChange) noexcept;

        Awaitable<void> Test_FactoryArguments_Properties() noexcept;

        Awaitable<void> Test_StateProvider_Remove_BlockAddOperation(__in bool isCommit) noexcept;

        Awaitable<void> Test_Recover_Replica_Call_CompleteCheckpoint(__in ULONG32 numStateProviders) noexcept;

        Awaitable<void> Test_Verify_PrepareCheckpoint_FirstCheckpointDuringOpen() noexcept;

    private:
        Awaitable<void> ConcurrentGetOrAddTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskID,
            __in ConcurrentDictionary<ULONG, bool> & dict) noexcept;

        Awaitable<void> InfinityStateProviderOperationsTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;

        Awaitable<void> StateManagerTest::CloseReplicaTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;

        Awaitable<void> GetOrAddThenAddOperationTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount) noexcept;
        
        Awaitable<void> DeletingTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount) noexcept;
        
        Awaitable<void> GetThenAddOperationTask(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in KUriView const & stateProviderName,
            __in ULONG taskCount) noexcept;

        Awaitable<void> PrepareCheckpointTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;
        Awaitable<void> PerformCheckpoint(
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in bool isCopyFirst,
            __in_opt AwaitableCompletionSource<bool> * const copyCompletion) noexcept;
        Awaitable<void> CompleteCheckpointTask(
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;

        Awaitable<void> BackupTask(
            __in KString const & backupDirectory,
            __in AwaitableCompletionSource<bool> & signalCompletion,
            __in_opt bool isBackupFirst = false,
            __in_opt AwaitableCompletionSource<bool> * const backupCompletion = nullptr) noexcept;

        ktl::Task GetStateProviderAndAddOperationTask(
            __in bool shouldThrow,
            __in KUriView const & stateProviderName,
            __in AwaitableCompletionSource<bool> & signalCompletion) noexcept;
    };

    Awaitable<void> StateManagerTest::Test_Life_CreateOpenClose_Success() noexcept
    {
        // Test
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Add_InvalidName(__in NameType nameType) noexcept
    {
        auto stateProviderName = GetStateProviderName(nameType);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        KFinally([&] { txnSPtr->Dispose(); });

        NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
        VERIFY_ARE_EQUAL(status, SF_STATUS_INVALID_NAME_URI);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Add_SingleOperation_Abort_Success(__in bool useDispose) noexcept
    {
        // Input variables
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup Test.
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            if (!useDispose)
            {
                txnSPtr->Abort();
            }
        }

        // Clean up.
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Add_SingleOperation_Commit_Success() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
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

        // Verification
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_GetOrAdd_Success(__in bool isExist) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        if (isExist)
        {
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
        }

        // Verification
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        if (isExist)
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
        }
        else
        {
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);
        }

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(isExist, alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: To verify GetOrAddAsync function Add Path and Get Path all return outStateProvider
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add Path test, GetOrAddAsync on a non-exist state provider.
    //    3. Verify the result is as expected.
    //    4. Get Path test, GetOrAddAsync on the same state provider.
    //    5. Verify the result
    //    6. Clean up and shut down.
    //
    Awaitable<void> StateManagerTest::Test_GetOrAdd_BothPath_Return_OutStateProvider() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        IStateProvider2::SPtr outStateProvider;

        // First test AddPath, GetOrAddAsync should return false and outStateProvider should be as expected
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_FALSE(alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Reset outStateProvider to null and test the GetPath
        outStateProvider = nullptr;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_TRUE(alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_GetOrAdd_ConcurrentMultipleCall_Success(
        __in ULONG taskNum) noexcept
    {
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Verify the state provider does not exist
        IStateProvider2::SPtr outStateProvider;
        NTSTATUS status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        ConcurrentDictionary<ULONG, bool>::SPtr resultDict;
        status = ConcurrentDictionary<ULONG, bool>::Create(
            GetAllocator(),
            resultDict);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG i = 0; i < taskNum; ++i)
        {
            status = taskArray.Append(ConcurrentGetOrAddTask(*signalCompletion, *stateProviderName, i, *resultDict));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        signalCompletion->SetResult(true);

        co_await (TaskUtilities<void>::WhenAll(taskArray));

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        VERIFY_IS_TRUE(resultDict->Count == taskNum);

        ULONG addCount = 0;
        auto enumerator = resultDict->GetEnumerator();
        while (enumerator->MoveNext())
        {
            if (enumerator->Current().Value == false)
            {
                ++addCount;
            }
        }

        VERIFY_IS_TRUE(addCount == 1);

        // Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }


    Awaitable<void> StateManagerTest::ConcurrentGetOrAddTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KUriView const & stateProviderName,
        __in ULONG taskID,
        __in ConcurrentDictionary<ULONG, bool> & dict) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();
        IStateProvider2::SPtr outStateProvider;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        bool alreadyExist = false;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(stateProviderName, outStateProvider->GetName());
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        dict.Add(taskID, alreadyExist);

        status = stateManagerSPtr->Get(stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(stateProviderName, outStateProvider->GetName());

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_GetOrAdd_InvalidName(NameType nameType) noexcept
    {
        auto stateProviderName = GetStateProviderName(nameType);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        {
            // Test
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            IStateProvider2::SPtr outStateProvider;
            bool alreadyExist = false;
            NTSTATUS status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_ARE_EQUAL(status, SF_STATUS_INVALID_NAME_URI);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_GetOrAdd_ReadAccessIsNotPresent() noexcept
    {
        KGuid partitionId;
        partitionId.CreateNew();

        PartitionedReplicaId::SPtr partitionedReplicaId = PartitionedReplicaId::Create(partitionId, random_.Next(), GetAllocator());
        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        IStatefulPartition::SPtr partition = TestHelper::CreateStatefulServicePartition(
            Guid::NewGuid(),
            GetAllocator(),
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING,
            FABRIC_SERVICE_PARTITION_ACCESS_STATUS_RECONFIGURATION_PENDING);
        TestTransactionManager::SPtr primaryTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await primaryTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await primaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        IStateManager::SPtr stateManagerSPtr = primaryTransactionalReplicator->StateManager;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        {
            // Test
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            IStateProvider2::SPtr outStateProvider;
            bool alreadyExist = false;
            NTSTATUS status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NOT_READABLE);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Shutdown
        co_await primaryTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await primaryTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_GetOrAdd_Abort_Success(bool isExist, bool useDispose) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        // Input variables
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup Test.
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        if (isExist)
        {
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
        }

        // Verification
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        if (isExist)
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
        }
        else
        {
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_ARE_EQUAL(isExist, alreadyExist);
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

            if (!useDispose)
            {
                txnSPtr->Abort();
            }
        }

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        if (isExist)
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
        }
        else
        {
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Because the txn GetOrAddAsync should abort/dispose, the lock should be released
        // so if the state provider is previous exist, we remove it, if not, add the state provider 
        // In this way, we can verify last GetOrAddAsync call release the locks.
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            if (isExist)
            {
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
            }
            else
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
            }

            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        if (isExist)
        {
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(outStateProvider);
        }
        else
        {
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
        }

        // Clean up.
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Add_BlockedReplication_InfiniteTimeout_CompleteOnceUnblocked() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName0 = GetStateProviderName(NameType::Random);
        auto stateProviderName1 = GetStateProviderName(NameType::Random);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            testTransactionalReplicatorSPtr_->SetReplicationBlocking(true);
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName0,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            auto addTask = stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName1,
                TestStateProvider::TypeName);

            status = co_await KTimer::StartTimerAsync(GetAllocator(), TEST_TAG, 4096, nullptr, nullptr);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            VERIFY_IS_FALSE(addTask.IsComplete());

            testTransactionalReplicatorSPtr_->SetReplicationBlocking(false);
            status = co_await addTask;
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verification
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName0, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName0, outStateProvider->GetName());

        status = stateManagerSPtr->Get(*stateProviderName1, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName1, outStateProvider->GetName());

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Get_NotExist_Error() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Verification
        IStateProvider2::SPtr outStateProvider;
        NTSTATUS status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(status == SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Get_InvalidName(__in NameType nameType) noexcept
    {
        auto stateProviderName = GetStateProviderName(nameType);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Verification
        IStateProvider2::SPtr outStateProvider;

        NTSTATUS status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(status == SF_STATUS_INVALID_NAME_URI);
        VERIFY_IS_NULL(outStateProvider);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Remove_Commit_Success(__in ULONG32 numberOfStateProvider) noexcept
    {
        // Expected
        KArray<KUri::CSPtr> perCheckpointList(GetAllocator(), numberOfStateProvider);
        for (ULONG index = 0; index < numberOfStateProvider; ++index)
        {
            NTSTATUS status = perCheckpointList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        co_await PopulateAsync(perCheckpointList);
        VerifyExist(perCheckpointList, true);

        // Test: Remove a single state provider.
        co_await this->DepopulateAsync(perCheckpointList);

        // Verification
        VerifyExist(perCheckpointList, false);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Remove_Abort_NoEffect(::ULONG32 numberOfStateProviders) noexcept
    {
        // Expected
        KArray<KUri::CSPtr> perCheckpointList(GetAllocator(), numberOfStateProviders);
        for (ULONG index = 0; index < numberOfStateProviders; ++index)
        {
            NTSTATUS status = perCheckpointList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        co_await PopulateAsync(perCheckpointList);
        VerifyExist(perCheckpointList, true);

        // Test: Remove a single state provider.
        {
            Transaction::SPtr txn;
            testTransactionalReplicatorSPtr_->CreateTransaction(txn);
            co_await DepopulateAsync(perCheckpointList, txn.RawPtr());
            txn->Abort();
            txn->Dispose();
        }

        // Verification
        VerifyExist(perCheckpointList, true);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Test used to verify the remove state provider call with aborted should have no effect on the state provider,
    // and then remove state provider call with txn committed should successfully remove the state provider.
    // 
    // Algorithm:
    //    1. Populate the expected state providers list
    //    2. Bring up the primary replica
    //    3. Remove the state providers in the list with txn aborted
    //    4. Verify the state providers still exist
    //    5. Remove the state providers in the list with txn committed
    //    6. Verify the state providers have been removed
    //    7. Clean up and shut down
    //
    Awaitable<void> StateManagerTest::Test_Remove_Abort_ThenRemove_Commit(__in ULONG32 numberOfStateProviders) noexcept
    {
        // Expected
        KArray<KUri::CSPtr> perCheckpointList(GetAllocator(), numberOfStateProviders);
        for (ULONG index = 0; index < numberOfStateProviders; ++index)
        {
            NTSTATUS status = perCheckpointList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        co_await PopulateAsync(perCheckpointList);
        VerifyExist(perCheckpointList, true);

        // Remove a single state provider with txn aborted
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            co_await DepopulateAsync(perCheckpointList, txnSPtr.RawPtr());
            txnSPtr->Abort();
        }

        // Verification
        VerifyExist(perCheckpointList, true);

        // Remove a single state provider with txn committed
        co_await DepopulateAsync(perCheckpointList);

        // Verification
        VerifyExist(perCheckpointList, false);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Dispatch_MultipleOperationTxn_Success(
        __in TestOperation::Type type,
        __in ULONG32 transactionCount,
        __in ULONG32 operationCount,
        __in bool nullOperationContext) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Setup
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

        // Get State Provider
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

        for (ULONG32 i = 0; i < transactionCount; ++i)
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            for (ULONG32 operationIndex = 0; operationIndex < operationCount; ++operationIndex)
            {
                TestOperation testOperation(type, testStateProvider.StateProviderId, nullOperationContext, GetAllocator());
                co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
            }

            testStateProvider.ExpectCommit(txnSPtr->TransactionId);
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        testStateProvider.Verify();

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Dispatch_AtomicOperationTxn_Success(
        __in TestOperation::Type type,
        __in ULONG32 operationCount,
        __in bool nullOperationContext) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Setup
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

        // Get State Provider
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

        for (ULONG32 i = 0; i < operationCount; ++i)
        {
            AtomicOperation::SPtr atomicOperation = testTransactionalReplicatorSPtr_->CreateAtomicOperation();

            TestOperation testOperation(type, testStateProvider.StateProviderId, nullOperationContext, GetAllocator());
            co_await testStateProvider.AddAsync(*atomicOperation, testOperation);
            testStateProvider.ExpectCommit(atomicOperation->TransactionId);
            atomicOperation->Dispose();
        }

        testStateProvider.Verify();

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Dispatch_AtomicRedoOperationTxn_Success(
        __in TestOperation::Type type,
        __in ULONG32 operationCount,
        __in bool nullOperationContext) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Setup
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

        // Get State Provider
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

        for (ULONG32 i = 0; i < operationCount; ++i)
        {
            AtomicRedoOperation::SPtr atomicOperation = testTransactionalReplicatorSPtr_->CreateAtomicRedoOperation();

            TestOperation testOperation(type, testStateProvider.StateProviderId, nullOperationContext, GetAllocator());
            co_await testStateProvider.AddAsync(*atomicOperation, testOperation);
            testStateProvider.ExpectCommit(atomicOperation->TransactionId);
            atomicOperation->Dispose();
        }

        testStateProvider.Verify();

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_CreateEnumerable_LateBound() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        KArray<KUri::CSPtr> AddedStateProviders(GetAllocator());
        KArray<KUri::CSPtr> RemovedStateProviders(GetAllocator());
        KArray<KUri::CSPtr> AddDuringEnumerationStateProviders(GetAllocator());

        // StateProvider 0-9 will be added at the beginning, and StateProvider 5-9 will be removed during enumeration
        // StateProvider 10-14 will be added during enumeration.
        // So StateProvider 0-4 must exist, and 5-14 may or may not exist.
        for (ULONG i = 0; i < 15; ++i)
        {
            auto stateProviderName = GetStateProviderName(NameType::Hierarchy, i, 0);

            if (i < 10)
            {
                status = AddedStateProviders.Append(stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            if (i >= 5 && i < 10)
            {
                status = RemovedStateProviders.Append(stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            if (i >= 10)
            {
                status = AddDuringEnumerationStateProviders.Append(stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            for (ULONG i = 0; i < AddedStateProviders.Count(); ++i)
            {
                status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *(AddedStateProviders[i]),
                    TestStateProvider::TypeName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
            }

            TEST_COMMIT_ASYNC(txnSPtr);
        }

        // Verification
        KSharedPtr<Data::IEnumerator<Data::KeyValuePair<KUri::CSPtr, KSharedPtr<IStateProvider2>>>> enumerator;
        status = stateManagerSPtr->CreateEnumerator(true, enumerator);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        ULONG count = 0;
        KArray<KUri::CSPtr> existsStateProvider(GetAllocator());

        while (enumerator->MoveNext())
        {
            //During the enumeration, remove some state providers and add some
            if (count == 5)
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });

                for (ULONG i = 0; i < AddDuringEnumerationStateProviders.Count(); ++i)
                {
                    status = co_await stateManagerSPtr->AddAsync(
                        *txnSPtr,
                        *(AddDuringEnumerationStateProviders[i]),
                        TestStateProvider::TypeName);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                for (ULONG i = 0; i < RemovedStateProviders.Count(); ++i)
                {
                    status = co_await stateManagerSPtr->RemoveAsync(
                        *txnSPtr,
                        *(RemovedStateProviders[i]));
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                }

                TEST_COMMIT_ASYNC(txnSPtr);
            }

            status = existsStateProvider.Append(enumerator->Current().Key);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            ++count;
        }

        // The state providers number must be larege than 5
        VERIFY_IS_TRUE(existsStateProvider.Count() >= 5);

        // Added but have not been removed state providers must exist.
        for (ULONG i = 0; i < 5; ++i)
        {
            KUri::CSPtr name = AddedStateProviders[i];

            bool findTheStateProvider = false;
            for (ULONG j = 0; j < existsStateProvider.Count(); ++j)
            {
                if (*(existsStateProvider[j]) == *name)
                {
                    findTheStateProvider = true;
                    break;
                }
            }

            VERIFY_IS_TRUE(findTheStateProvider);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_VerifyWriteFollowedByWriteToTheSameKeyInATx() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            status = co_await stateManagerSPtr->AddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_ALREADY_EXISTS);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_VerifyWriteFollowedReadWithAddToTheSameKeyInATx() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            NTSTATUS status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            IStateProvider2::SPtr outStateProvider;
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_ALREADY_EXISTS);
            VERIFY_IS_NULL(outStateProvider);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_VerifyWriteFollowedReadWithRemoveToTheSameKeyInATx() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        {
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

            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    *stateProviderName);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                IStateProvider2::SPtr outStateProvider;
                bool alreadyExist = false;
                status = co_await stateManagerSPtr->GetOrAddAsync(
                    *txnSPtr,
                    *stateProviderName,
                    TestStateProvider::TypeName,
                    outStateProvider,
                    alreadyExist);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_TRUE(alreadyExist);
                VERIFY_IS_NOT_NULL(outStateProvider);
                VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
                TEST_COMMIT_ASYNC(txnSPtr);
            }
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_VerifyReplicaCloseBeforeTxDispose(__in bool isAdd) noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Test
        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);

        NTSTATUS status = co_await stateManagerSPtr->AddAsync(
            *txnSPtr,
            *stateProviderName,
            TestStateProvider::TypeName);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // isAdd indicates whether txn that Adds or Removes gets disposed after closeAsync.
        if (!isAdd)
        {
            TEST_COMMIT_ASYNC(txnSPtr);
            txnSPtr->Dispose();

            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        txnSPtr->Dispose();

        co_return;
    }
 
    Awaitable<void> StateManagerTest::InfinityStateProviderOperationsTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Since we dont have stack trace, a count used here to track where the exception thrown.
        ULONG position = 0;
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Empty);

        try
        {
            co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

            AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
            co_await tempCompletion->GetAwaitable();

            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            while (true)
            {
                stateProviderName = GetStateProviderName(NameType::Valid);
                {
                    position = 1;
                    Transaction::SPtr txnSPtr = nullptr;
                    status = testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    KFinally([&] { txnSPtr->Dispose(); });

                    status = co_await stateManagerSPtr->AddAsync(
                        *txnSPtr,
                        *stateProviderName,
                        TestStateProvider::TypeName,
                        nullptr);
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        // If close task wins the race, AddAsync API is supposed to return NTSTATUS error code SF_STATUS_OBJECT_CLOSED.
                        co_return;
                    }
                    else
                    {
                        // If this task wins the race, AddAsync API should return STATUS_SUCCESSFUL.
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }

                    status = co_await txnSPtr->CommitAsync();
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        // In the Txn commit call, it will check TransactionManager status, return SF_STATUS_OBJECT_CLOSED if nullptr.
                        co_return;
                    }
                    else
                    {
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                }

                {
                    position++;
                    IStateProvider2::SPtr stateProvider = nullptr;
                    status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        // If close task wins the race, Get API is supposed to return NTSTATUS error code SF_STATUS_OBJECT_CLOSED and out sp is null.
                        VERIFY_IS_NULL(stateProvider);
                        co_return;
                    }
                    else
                    {
                        // If this task wins the race, Get API should return STATUS_SUCCESSFUL and the right sp is taken out.
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                        VERIFY_IS_NOT_NULL(stateProvider);
                        VERIFY_ARE_EQUAL(*stateProviderName, stateProvider->GetName());
                    }
                }

                {
                    position++;
                    Transaction::SPtr txnSPtr = nullptr;
                    status = testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    KFinally([&] { txnSPtr->Dispose(); });

                    status = co_await stateManagerSPtr->RemoveAsync(
                        *txnSPtr,
                        *stateProviderName);
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        // If close task wins the race, RemoveAsync API is supposed to return NTSTATUS error code SF_STATUS_OBJECT_CLOSED.
                        co_return;
                    }
                    else
                    {
                        // If this task wins the race, RemoveAsync API should return STATUS_SUCCESSFUL.
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }

                    status = co_await txnSPtr->CommitAsync();
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        co_return;
                    }
                    else
                    {
                        VERIFY_IS_TRUE(NT_SUCCESS(status));
                    }
                }

                {
                    position++;
                    IStateProvider2::SPtr stateProvider = nullptr;
                    status = testTransactionalReplicatorSPtr_->StateManager->Get(*stateProviderName, stateProvider);
                    VERIFY_IS_NULL(stateProvider);
                    if (status == SF_STATUS_OBJECT_CLOSED)
                    {
                        // If close task wins the race, Get API is supposed to return NTSTATUS error code SF_STATUS_OBJECT_CLOSED.
                        co_return;
                    }
                    else
                    {
                        // If this task wins the race, Get API should return SF_STATUS_NAME_DOES_NOT_EXIST.
                        VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
                    }
                }
            }
        }
        catch (Exception & exception)
        {
            ASSERT_IF(
                true,
                "InfinityStateProviderOperationsTask failed, SPName: {0} status: {1}, ThrowPosition: {2} - 0:ktl, 1:AddAsync, 2:Get, 3:RemoveAsync, 4:Get",
                ToStringLiteral(*stateProviderName),
                exception.GetStatus(),
                position);
        }

        co_return;
    }

    Awaitable<void> StateManagerTest::CloseReplicaTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        // Flag to check whether the exception thrown from CloseAsync.
        bool closeAsyncStart = false;

        try
        {
            co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

            AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
            co_await tempCompletion->GetAwaitable();

            closeAsyncStart = true;

            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }
        catch (Exception & exception)
        {
            ASSERT_IF(
                true, 
                "CloseReplicaTask failed, status: {0}, CloseAsync throw: {1}", 
                exception.GetStatus(), 
                closeAsyncStart);
        }

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Replica_Close_During_StateProvider_Operations_Throw() noexcept
    {
        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));


        status = taskArray.Append(InfinityStateProviderOperationsTask(*signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = taskArray.Append(CloseReplicaTask(*signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        signalCompletion->SetResult(true);

        co_await (TaskUtilities<void>::WhenAll(taskArray));

        // Open the replica again to clean up the folder.
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, partitionedReplicaIdCSPtr_->ReplicaId, GetAllocator());
        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr reOpenReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);
        co_await reOpenReplicator->OpenAsync(CancellationToken::None);
        co_await reOpenReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await reOpenReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Replica_Change_Role_Retry_WhenUserServiceFailsChangeRole() noexcept
    {
        auto stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        // Call should not fail.
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        CODING_ERROR_ASSERT(testTransactionalReplicatorSPtr_->Role == FABRIC_REPLICA_ROLE_PRIMARY);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_AddOperation_StateProviderNotRegistered_Throw() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        LONG64 stateProviderId = KDateTime::Now();

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Setup
        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = stateManagerSPtr->AddOperation(*txnSPtr, stateProviderId, nullptr, nullptr, nullptr, nullptr);
            CODING_ERROR_ASSERT(status == STATUS_INVALID_PARAMETER);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Test used to simulate the bug that AddOperation(takes in Name) call on a state provider which has been
    //       removed but added again with same name, in this case the state provider object has changed, but AddOperation
    //       call can not differential them. Now AddOperation takes in StateProviderId instead of Name, it should throw in this case.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider, get the state provider id
    //    3. Removed the state provider and added with the same name
    //    4. Call AddOperation with the previous state provider id
    //    5. Verify AddOperation throws
    //    6. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_AddOperation_OnRemovedStateProvider_Throw() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        auto stateProviderName = GetStateProviderName(NameType::Valid);
        
        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
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

        // Get State Provider
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

        // Remove the state provider and added with the same name
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

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(status == SF_STATUS_NAME_DOES_NOT_EXIST);
        VERIFY_IS_NULL(outStateProvider);

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

        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = stateManagerSPtr->AddOperation(*txnSPtr, testStateProvider.StateProviderId, nullptr, nullptr, nullptr, nullptr);
            CODING_ERROR_ASSERT(status == STATUS_INVALID_PARAMETER);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Test Add path works properly with the AddOperation Name to Id change.
    //       Add a state provider and keep calling AddOperation on it, then we remove the state provider at some point 
    //       all AddOperation before the remove should success and all after should throw
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add a state provider, Verify the state provider exists
    //    3. Keep calling AddOperation on the state provider
    //    4. Remove the state provider at some point 
    //    5. Verify all AddOperation before the remove should success and all after should throw
    //    6. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_AddOperation_OperationAfterRemoveThrow(__in ULONG taskCount) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;
        auto stateProviderName = GetStateProviderName(NameType::Valid);
        bool isThrow = false;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Add a state provider
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

        // Get State Provider, Verify the state provider exists
        IStateProvider2::SPtr outStateProvider;
        status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

        // Set a random point to remove the state provider
        ULONG numShouldPass = random_.Next(0, taskCount);

        for (ULONG i = 0; i < taskCount; ++i)
        {
            // Remove the state provider at previously set random point
            if (i == numShouldPass)
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

            if (i < numShouldPass)
            {
                // All Operations before the remove should success
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });

                TestOperation testOperation(TestOperation::Random, testStateProvider.StateProviderId, true, GetAllocator());
                co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
                testStateProvider.ExpectCommit(txnSPtr->TransactionId);
                TEST_COMMIT_ASYNC(txnSPtr);

                testStateProvider.Verify();
            }
            else
            {
                // All Operations after the remove should throw
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });

                try
                {
                    TestOperation testOperation(TestOperation::Random, testStateProvider.StateProviderId, true, GetAllocator());
                    co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
                    CODING_ASSERT("Test should not come here");
                }
                catch (Exception & exception)
                {
                    // If it throws the exception, verify the exception code 
                    CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_INVALID_PARAMETER);
                    txnSPtr->Abort();
                    isThrow = true;
                }
                CODING_ERROR_ASSERT(isThrow);

                // Reset the isThrow flag to ensure the exception is thrown
                isThrow = false;
            }
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: Concurrently run 3 tasks, one task keeps on doing GetOrAdd and then AddOperation on SP, 
    //       one task keeps on deleting SP and one task that keeps Get SP and doing an AddOperation
    //       AddOperation should only throw STATUS_INVALID_PARAMETER exception or success
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Add 3 tasks: GetOrAddThenAddOperation, Deleting, GetThenAddOperation
    //    3. Run the tasks concurrently and wait for all tasks finish
    //    4. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_AddOperation_Concurrent_Tasks(__in ULONG taskCount) noexcept
    {
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        status = taskArray.Append(GetOrAddThenAddOperationTask(*signalCompletion, *stateProviderName, taskCount));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = taskArray.Append(DeletingTask(*signalCompletion, *stateProviderName, taskCount));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = taskArray.Append(GetThenAddOperationTask(*signalCompletion, *stateProviderName, taskCount));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        signalCompletion->SetResult(true);

        co_await (TaskUtilities<void>::WhenAll(taskArray));

        // Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    // The task keep calling GetOrAdd state provider, then AddOperation on the state provider if it exists
    Awaitable<void> StateManagerTest::GetOrAddThenAddOperationTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KUriView const & stateProviderName,
        __in ULONG taskCount) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        IStateProvider2::SPtr outStateProvider = nullptr;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        for (ULONG i = 0; i < taskCount; ++i)
        {
            // GetOrAdd state provider with the name specified
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                bool alreadyExist = false;
                NTSTATUS status = co_await stateManagerSPtr->GetOrAddAsync(
                    *txnSPtr,
                    stateProviderName,
                    TestStateProvider::TypeName,
                    outStateProvider,
                    alreadyExist);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(outStateProvider);
                VERIFY_ARE_EQUAL(stateProviderName, outStateProvider->GetName());
                TEST_COMMIT_ASYNC(txnSPtr);
            }

            // Since GetOrAdd will return the outStateProvider in both Get or Add path, 
            // we can just use the outStateProvider instead of calling Get again.
            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
            LONG64 transactionId;

            {
                // If the state provider has been removed, it will only throw the exception STATUS_INVALID_PARAMETER
                // Otherwise, the add operation should success
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                transactionId = txnSPtr->TransactionId;

                try
                {
                    TestOperation testOperation(TestOperation::Random, testStateProvider.StateProviderId, true, GetAllocator());
                    co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
                    testStateProvider.ExpectCommit(transactionId);
                    TEST_COMMIT_ASYNC(txnSPtr);
                }
                catch (Exception & exception)
                {
                    // If it throws the exception, verify the exception code and then continue next iteration.
                    CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_INVALID_PARAMETER);
                    continue;
                }
            }

            // Verify the add operation success
            testStateProvider.Verify(transactionId);
        }

        co_return;
    }

    // The task keep calling Remove state provider
    Awaitable<void> StateManagerTest::DeletingTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KUriView const & stateProviderName,
        __in ULONG taskCount) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        for (ULONG i = 0; i < taskCount; ++i)
        {
            // Delete the state provider if it exists.
            {
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });

                NTSTATUS status = co_await stateManagerSPtr->RemoveAsync(
                    *txnSPtr,
                    stateProviderName);
                if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
                {
                    // The remove call may failed in the situation that two Remove calls happened in a row
                    // Then Remove call will return SF_STATUS_NAME_DOES_NOT_EXIST NTSTATUS error code.
                    continue;
                }
                else
                {
                    VERIFY_IS_TRUE(NT_SUCCESS(status));
                    TEST_COMMIT_ASYNC(txnSPtr);
                }
            }
        }

        co_return;
    }

    // The task keep calling Get state provider, then AddOperation on the state provider if it exists
    Awaitable<void> StateManagerTest::GetThenAddOperationTask(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in KUriView const & stateProviderName,
        __in ULONG taskCount) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        IStateProvider2::SPtr outStateProvider = nullptr;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        for (ULONG i = 0; i < taskCount; ++i)
        {
            NTSTATUS status = stateManagerSPtr->Get(stateProviderName, outStateProvider);

            if (status == SF_STATUS_NAME_DOES_NOT_EXIST)
            {
                VERIFY_IS_NULL(outStateProvider);
                continue;
            }

            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
            LONG64 transactionId;

            {
                // If the state provider has been removed, it will only throw the exception STATUS_INVALID_PARAMETER
                // Otherwise, the add operation should success
                Transaction::SPtr txnSPtr;
                testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
                KFinally([&] { txnSPtr->Dispose(); });
                transactionId = txnSPtr->TransactionId;

                try
                {
                    TestOperation testOperation(TestOperation::Random, testStateProvider.StateProviderId, true, GetAllocator());
                    co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
                    testStateProvider.ExpectCommit(txnSPtr->TransactionId);
                    TEST_COMMIT_ASYNC(txnSPtr);
                }
                catch (Exception & exception)
                {
                    // If it throws the exception, verify the exception code and then continue next iteration.
                    CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_INVALID_PARAMETER);
                    continue;
                }
            }

            // Verify the add operation success
            testStateProvider.Verify(transactionId);
        }

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Replica_Closed_Throw() noexcept
    {
        LONG64 stateProviderId = KDateTime::Now();

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        NTSTATUS status = stateManagerSPtr->ErrorIfStateProviderIsNotRegistered(stateProviderId, 1);
        VERIFY_ARE_EQUAL(status, SF_STATUS_OBJECT_CLOSED);
    }

    //
    // Goal: State manager BackupAsync function test, the state manger should be 
    //       backed up and the back up file should be saved, and the state manager state should not be changed
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Call BackupAsync
    //    3. Verify the back up file exists
    //    4. Verify the state manager state has not been changed
    //    5. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_BackupAsync_FileSaved(
        __in BackupAsyncTestMode mode,
        __in ULONG numberOfStateProvider) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Populate the state provider list
        KArray<KUri::CSPtr> stateProviderList(GetAllocator(), numberOfStateProvider);
        for (ULONG index = 0; index < numberOfStateProvider; ++index)
        {
            status = stateProviderList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        if (mode != EmptyState)
        {
            co_await PopulateAsync(stateProviderList);
            VerifyExist(stateProviderList, true);

            if (mode == Checkpointed)
            {
                co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            }
        }

        StateManager & statemanager = dynamic_cast<StateManager &>(*stateManagerSPtr);
        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        // Test: Call BackupAsync
        status = co_await statemanager.BackupCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Verify the back up file exists
        KString::SPtr expectedFile = KPath::Combine(*currentPath, L"backup.chkpt", GetAllocator());

        CODING_ERROR_ASSERT(Common::File::Exists(static_cast<LPCWSTR>(*expectedFile)));

        // Verify state manager state has not been changed
        VerifyExist(stateProviderList, (mode == EmptyState ? false : true));
        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: State manager BackupAsync and RestoreAsync function test, the state manger should be 
    //       backed up and the back up file should be saved, and the state manager state should not be changed,
    //       After the RestoreAsync call, the states should be the same as before.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Call BackupAsync
    //    3. Verify the back up file exists
    //    4. Verify the state manager state has not been changed
    //    5. Clean up and Shut down
    //    6. Create a new replica
    //    7. Verify the states are the same as before
    //    8. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_BackupAndRestore(
        __in BackupAsyncTestMode mode,
        __in ULONG numberOfStateProvider) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Populate the state provider list
        KArray<KUri::CSPtr> stateProviderList(GetAllocator(), numberOfStateProvider);
        for (ULONG index = 0; index < numberOfStateProvider; ++index)
        {
            status = stateProviderList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        if (mode != EmptyState)
        {
            co_await PopulateAsync(stateProviderList);
            VerifyExist(stateProviderList, true);

            if (mode == Checkpointed)
            {
                co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            }
        }

        StateManager & statemanager = dynamic_cast<StateManager &>(*stateManagerSPtr);
        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        // Test: Call BackupAsync
        status = co_await statemanager.BackupCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Verify the back up file exists
        KString::SPtr expectedFile = KPath::Combine(*currentPath, L"backup.chkpt", GetAllocator());

        CODING_ERROR_ASSERT(Common::File::Exists(static_cast<LPCWSTR>(*expectedFile)));

        // Verify state manager state has not been changed
        VerifyExist(stateProviderList, (mode == EmptyState ? false : true));

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr newTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await newTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto newStateManagerSPtr = newTransactionalReplicator->StateManager;
        for (ULONG i = 0; i < stateProviderList.Count(); ++i)
        {
            IStateProvider2::SPtr stateProvider;
            status = newStateManagerSPtr->Get(*stateProviderList[i], stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider);
        }

        // After restore, the state should be as before.
        StateManager & newStatemanager = dynamic_cast<StateManager &>(*newStateManagerSPtr);
        status = co_await newStatemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG i = 0; i < stateProviderList.Count(); ++i)
        {
            IStateProvider2::SPtr stateProvider;
            status = newStateManagerSPtr->Get(*stateProviderList[i], stateProvider);
            VERIFY_ARE_EQUAL(NT_SUCCESS(status), (mode == Checkpointed ? true : false));
        }

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await newTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: State manager BackupAsync and RestoreAsync function test, add some new states after BackupAsync call
    //       the new states should not exist.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Call BackupAsync
    //    3. Verify the back up file exists
    //    4. Verify the state manager state has not been changed
    //    5. Add some new states
    //    6. Clean up and Shut down
    //    7. Create a new replica
    //    8. Verify the states are the same as before and new states should not exist
    //    9. Clean up and Shut down
    //
    Awaitable<void> StateManagerTest::Test_BackupAndRestore_NewStatesNotExist(
        __in BackupAsyncTestMode mode,
        __in ULONG numberOfStateProvider,
        __in ULONG numberOfNewStateProvider) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        // Populate the state provider lists
        KArray<KUri::CSPtr> stateProviderList(GetAllocator(), numberOfStateProvider);
        for (ULONG index = 0; index < numberOfStateProvider; ++index)
        {
            status = stateProviderList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        KArray<KUri::CSPtr> newStateProviderList(GetAllocator(), numberOfNewStateProvider);
        for (ULONG index = 0; index < numberOfNewStateProvider; ++index)
        {
            status = newStateProviderList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        if (mode != EmptyState)
        {
            co_await PopulateAsync(stateProviderList);
            VerifyExist(stateProviderList, true);

            if (mode == Checkpointed)
            {
                co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);
            }
        }

        StateManager & statemanager = dynamic_cast<StateManager &>(*stateManagerSPtr);
        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        // Test: Call BackupAsync
        status = co_await statemanager.BackupCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_await PopulateAsync(newStateProviderList);

        // Verify the back up file exists
        KString::SPtr expectedFile = KPath::Combine(*currentPath, L"backup.chkpt", GetAllocator());

        CODING_ERROR_ASSERT(Common::File::Exists(static_cast<LPCWSTR>(*expectedFile)));

        // Verify state manager state has not been changed
        VerifyExist(stateProviderList, (mode == EmptyState ? false : true));
        VerifyExist(newStateProviderList, true);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);


        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr newTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await newTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto newStateManagerSPtr = newTransactionalReplicator->StateManager;
        for (ULONG i = 0; i < stateProviderList.Count(); ++i)
        {
            IStateProvider2::SPtr stateProvider;
            status = newStateManagerSPtr->Get(*stateProviderList[i], stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider);
        }

        // After restore, the state should be as before.
        StateManager & newStatemanager = dynamic_cast<StateManager &>(*newStateManagerSPtr);
        status = co_await newStatemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        for (ULONG i = 0; i < stateProviderList.Count(); ++i)
        {
            IStateProvider2::SPtr stateProvider;
            status = newStateManagerSPtr->Get(*stateProviderList[i], stateProvider);
            VERIFY_ARE_EQUAL(NT_SUCCESS(status), (mode == Checkpointed ? true : false));
        }

        for (ULONG i = 0; i < newStateProviderList.Count(); ++i)
        {
            IStateProvider2::SPtr stateProvider;
            status = newStateManagerSPtr->Get(*newStateProviderList[i], stateProvider);
            VERIFY_ARE_EQUAL(status, SF_STATUS_NAME_DOES_NOT_EXIST);
            VERIFY_IS_NULL(stateProvider);
        }

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await newTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: State manager RestoreAsync call with no backup file, NTSTATUS error code should be returned.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Call RestoreAsync
    //    3. Verify the returned NTSTATUS error code
    //
    Awaitable<void> StateManagerTest::Test_Restore_NoBackupFile() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        StateManager & statemanager = dynamic_cast<StateManager &>(*stateManagerSPtr);
        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        status = co_await statemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(status == STATUS_INTERNAL_DB_CORRUPTION);

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: State manager RestoreAsync call with backup file which has data corruption, NTSTATUS error code should be returned.
    // Algorithm:
    //    1. Bring up the primary replica
    //    2. Populate state providers, take checkpoint
    //    3. BackupAsync call, and verify file exists, no state changed
    //    4. Inject the data corruption.
    //    5. RestoreAsync
    //    6. Verify the returned NTSTATUS error code
    //    7. Clean up and shut down
    //
    Awaitable<void> StateManagerTest::Test_Restore_DataCorruption(__in ULONG corruptPosition) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Setup: Bring up the primary replica
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        IStateManager::SPtr stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;
        const ULONG numberOfStateProvider = 16;

        // Populate the state provider list
        KArray<KUri::CSPtr> stateProviderList(GetAllocator(), numberOfStateProvider);
        for (ULONG index = 0; index < numberOfStateProvider; ++index)
        {
            status = stateProviderList.Append(GetStateProviderName(NameType::Random));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        co_await this->PopulateAsync(stateProviderList);
        VerifyExist(stateProviderList, true);
        co_await testTransactionalReplicatorSPtr_->CheckpointAsync(CancellationToken::None);

        StateManager & statemanager = dynamic_cast<StateManager &>(*stateManagerSPtr);
        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        // BackupAsync call on the current state 
        status = co_await statemanager.BackupCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Verify the back up file exists
        KString::SPtr expectedFile = KPath::Combine(*currentPath, L"backup.chkpt", GetAllocator());

        CODING_ERROR_ASSERT(Common::File::Exists(static_cast<LPCWSTR>(*expectedFile)));

        // Verify state manager state has not been changed
        VerifyExist(stateProviderList, true);

        // Inject the data corruption.
        {
            KBlockFile::SPtr FileSPtr = nullptr;
            io::KFileStream::SPtr fileStreamSPtr = nullptr;
            KWString fileName(GetAllocator(), *expectedFile);

            status = co_await KBlockFile::CreateSparseFileAsync(
                fileName,
                FALSE,
                KBlockFile::eOpenExisting,
                FileSPtr,
                nullptr,
                GetAllocator(),
                TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = io::KFileStream::Create(fileStreamSPtr, GetAllocator(), TEST_TAG);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            status = co_await fileStreamSPtr->OpenAsync(*FileSPtr);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            LONG64 fileSize = fileStreamSPtr->GetLength();
            CODING_ERROR_ASSERT(corruptPosition < fileSize);

            BinaryWriter writer(GetAllocator());
            writer.Write(static_cast<ULONG64>(99));

            fileStreamSPtr->SetPosition(corruptPosition);
            status = co_await fileStreamSPtr->WriteAsync(*writer.GetBuffer(0));
            CODING_ERROR_ASSERT(NT_SUCCESS(status));

            if (fileStreamSPtr != nullptr)
            {
                status = co_await fileStreamSPtr->CloseAsync();
                CODING_ERROR_ASSERT(NT_SUCCESS(status));
                fileStreamSPtr = nullptr;
            }

            FileSPtr->Close();
        }

        // Test
        status = co_await statemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(status == STATUS_INTERNAL_DB_CORRUPTION);

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    //
    // Goal: State manager Open with cleanupRestore, which will call CleanUpIncompleteRestoreAsync, should succeed
    // Algorithm:
    //    1. Bring up the primary replica, Open with cleanupRestore
    //    2. Clean up and shut down
    //
    Awaitable<void> StateManagerTest::Test_CleanUpIncompleteRestore_DuringOpen() noexcept
    {
        // Open replica with cleanupRestore
        try
        {
            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None, false, true);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        }
        catch (Exception &)
        {
            CODING_ERROR_ASSERT(false);
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Goal: Simulate the race between backup and PrepareCheckpoint, verify backup on base point
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add some more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between backup and PrepareCheckpoint
    //    4. new replica restore from the backup file
    //    5. Verify state provider state as expected
    Awaitable<void> StateManagerTest::Test_Backup_PrepareCheckpoint_Race_BackupBasepoint(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange) noexcept
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

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

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

        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Set the race between copy and PrepareCheckpoint
        status = taskArray.Append(PrepareCheckpointTask(*signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));
        status = taskArray.Append(BackupTask(*currentPath, *signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        signalCompletion->SetResult(true);
        co_await (TaskUtilities<void>::WhenAll(taskArray));

        // Finish the second checkpoint.
        co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr newTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await newTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto newStateManagerSPtr = newTransactionalReplicator->StateManager;
        VerifyExist(addsBeforeCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
        VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());

        // After restore, the state should be as before.
        StateManager & newStatemanager = dynamic_cast<StateManager &>(*newStateManagerSPtr);
        status = co_await newStatemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        VerifyExist(addsBeforeCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());

        if (isAddChange)
        {
            VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
        }

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await newTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Goal: Simulate the race between backup and PerformCheckpoint, If backup happened first, 
    //       backup on the base checkpoint, if PerformCheckpoint happened first, backup on the new checkpointed states
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add some more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between backup and PerformCheckpoint
    //    4. new replica restore from the backup file
    //    5. Verify state provider state as expected
    Awaitable<void> StateManagerTest::Test_Backup_PerformCheckpoint_Race(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange,
        __in bool isBakcupFirst) noexcept
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

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

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

        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        AwaitableCompletionSource<bool>::SPtr backupCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, backupCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Set the race between copy and PerformCheckpoint
        status = taskArray.Append(BackupTask(*currentPath, *signalCompletion, isBakcupFirst, backupCompletion.RawPtr()));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
        status = taskArray.Append(PerformCheckpoint(*signalCompletion, isBakcupFirst, backupCompletion.RawPtr()));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        signalCompletion->SetResult(true);
        co_await (TaskUtilities<void>::WhenAll(taskArray));

        // Finish the checkpoint
        co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr newTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await newTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto newStateManagerSPtr = newTransactionalReplicator->StateManager;
        VerifyExist(addsBeforeCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
        VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());

        // After restore, the state should be as before.
        StateManager & newStatemanager = dynamic_cast<StateManager &>(*newStateManagerSPtr);
        status = co_await newStatemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        if (isBakcupFirst)
        {
            VerifyExist(addsBeforeCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());

            if (isAddChange)
            {
                VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
            }
        }
        else
        {
            if (isAddChange)
            {
                VerifyExist(addsBeforeCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
                VerifyExist(changesAfterCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
            }
            else
            {
                VerifyExist(notChangedAfterCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
                VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
            }
        }

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await newTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Goal: Simulate the race between backup and CompleteCheckpoint, backup on the new checkpointed states
    // Algorithm:
    //    1. Add some state providers and take checkpoint, which considered as base checkpoint
    //    2. Add some more state providers or remove partial state providers for the new checkpoint 
    //    3. Set the race between backup and CompleteCheckpoint
    //    4. new replica restore from the backup file
    //    5. Verify state provider state as expected
    Awaitable<void> StateManagerTest::Test_Backup_CompleteCheckpoint_Race(
        __in ULONG32 numAddsBeforeCheckpoint,
        __in ULONG32 numChangesAfterCheckpoint,
        __in bool isAddChange) noexcept
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

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

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

        KString::CSPtr currentPath = TestHelper::CreateFileString(GetAllocator(), L"TempFolder");
        Common::ErrorCode errorCode = Common::Directory::Create2(static_cast<LPCWSTR>(*currentPath));
        CODING_ERROR_ASSERT(errorCode.IsSuccess());

        KArray<Awaitable<void>> taskArray(GetAllocator());

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Set the race between copy and PerformCheckpoint
        status = taskArray.Append(BackupTask(*currentPath, *signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
        co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
        status = taskArray.Append(CompleteCheckpointTask(*signalCompletion));
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        signalCompletion->SetResult(true);
        co_await (TaskUtilities<void>::WhenAll(taskArray));

        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        LONG64 replicaId = partitionedReplicaIdCSPtr_->ReplicaId + 1;
        auto partitionedReplicaId = PartitionedReplicaId::Create(partitionedReplicaIdCSPtr_->PartitionId, replicaId, GetAllocator());

        auto runtimeFolders = TestHelper::CreateRuntimeFolders(GetAllocator());
        auto partition = TestHelper::CreateStatefulServicePartition(GetAllocator());
        TestTransactionManager::SPtr newTransactionalReplicator = CreateReplica(*partitionedReplicaId, *runtimeFolders, *partition);

        co_await newTransactionalReplicator->OpenAsync(CancellationToken::None);
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        auto newStateManagerSPtr = newTransactionalReplicator->StateManager;

        VerifyExist(addsBeforeCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
        VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());

        // After restore, the state should be as before.
        StateManager & newStatemanager = dynamic_cast<StateManager &>(*newStateManagerSPtr);
        status = co_await newStatemanager.RestoreCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        if (isAddChange)
        {
            VerifyExist(addsBeforeCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(changesAfterCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
        }
        else
        {
            VerifyExist(notChangedAfterCheckpoint, true, newTransactionalReplicator->StateManager.RawPtr());
            VerifyExist(changesAfterCheckpoint, false, newTransactionalReplicator->StateManager.RawPtr());
        }

        Common::Directory::Delete(static_cast<LPCWSTR>(*currentPath), true);

        // Clean up
        co_await newTransactionalReplicator->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await newTransactionalReplicator->CloseAsync(CancellationToken::None);

        co_return;
    }

    // Goal: Used to verify the factory arguments are correctly passed
    // Algorithm:
    //    1. Create a state provider with children.
    //    2. Verify the PartitionId and ReplicaId are as expected
    //    3. Verify the state provider id is as expected.
    Awaitable<void> StateManagerTest::Test_FactoryArguments_Properties() noexcept
    {
        const ULONG childrenCount = 5;
        KArray<KUri::CSPtr> nameArray(GetAllocator());

        // Get the parent name and put the name in the name array
        auto stateProviderName = GetStateProviderName(NameType::Hierarchy, 0, 0);
        NTSTATUS status = nameArray.Append(stateProviderName);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Put all children name in the name array, in this way, we can just enumerate 
        // the name array and do the verification.
        for (ULONG i = 1; i <= childrenCount; i++)
        {
            auto childrenStateProviderName = GetStateProviderName(NameType::Hierarchy, 0, i);
            status = nameArray.Append(childrenStateProviderName);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        OperationData::SPtr initParameters;

        try
        {
            initParameters = TestStateProvider::CreateInitializationParameters(childrenCount, GetAllocator());
        }
        catch (Exception & exception)
        {
            CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_UNSUCCESSFUL);
            CODING_ERROR_ASSERT(false);
        }

        // Setup
        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            status = co_await stateManagerSPtr->AddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                initParameters.RawPtr());
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        for (KUri::CSPtr name : nameArray)
        {
            // Verification
            IStateProvider2::SPtr outStateProvider;
            status = stateManagerSPtr->Get(*name, outStateProvider);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*name, outStateProvider->GetName());

            TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

            // Verify PartitionId and ReplicaId are correct.
            VERIFY_ARE_EQUAL(partitionedReplicaIdCSPtr_->PartitionId, testStateProvider.PartitionId);
            VERIFY_ARE_EQUAL(partitionedReplicaIdCSPtr_->ReplicaId, testStateProvider.ReplicaId);

            // Verify the correctness of passing StateProviderId through FactoryArgument.
            // Test in this way because if StateProviderId is not passing to the state provider and the state provider
            // get a random id, MetadataManager and StateProvider will get different Ids, thus the 
            // state provider will not be registered and will throw.
            FABRIC_STATE_PROVIDER_ID stateProviderId = testStateProvider.StateProviderId;
            status = stateManagerSPtr->ErrorIfStateProviderIsNotRegistered(stateProviderId, 1);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }
    
    // Goal: RemoveAsync call block AddOperation, 2 cases in this test.
    //    1. If RemoveAsync committed in the end. Waiting AddOperation call should throw exception.
    //    2. If RemoveAsync aborted in the end. Waiting AddOperation call should succeed.
    // Algorithm:
    //    1. Create a state provider
    //    2. Create a txn and starts RemoveAsync call.
    //    3. Before txn commit or abort, starts the AddOperation call on test state provider.
    //    4. Commit or abort the txn. AddOperation call should be as expected.
    //    5. Shut down the replica and clean up.
    Awaitable<void> StateManagerTest::Test_StateProvider_Remove_BlockAddOperation(__in bool isCommit) noexcept
    {
        KUri::CSPtr stateProviderName = GetStateProviderName(NameType::Valid);

        // Setup
        co_await (testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None));

        IStateProvider2::SPtr outStateProvider = nullptr;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        AwaitableCompletionSource<bool>::SPtr signalCompletion = nullptr;
        NTSTATUS status = AwaitableCompletionSource<bool>::Create(GetAllocator(), 0, signalCompletion);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        // Add a state provider
        {  
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });
            bool alreadyExist = false;
            status = co_await stateManagerSPtr->GetOrAddAsync(
                *txnSPtr,
                *stateProviderName,
                TestStateProvider::TypeName,
                outStateProvider,
                alreadyExist);
            VERIFY_IS_TRUE(NT_SUCCESS(status));
            VERIFY_IS_NOT_NULL(outStateProvider);
            VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());
            TEST_COMMIT_ASYNC(txnSPtr);
        }

        {
            Transaction::SPtr txnSPtr;
            testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
            KFinally([&] { txnSPtr->Dispose(); });

            status = co_await stateManagerSPtr->RemoveAsync(
                *txnSPtr,
                *stateProviderName);
            VERIFY_IS_TRUE(NT_SUCCESS(status));

            // Before RemoveAsync call unlock, AddOperation on state provider.
            GetStateProviderAndAddOperationTask(isCommit, *stateProviderName, *signalCompletion);

            if (isCommit)
            {
                TEST_COMMIT_ASYNC(txnSPtr);
            }
            else
            {
                txnSPtr->Abort();
            }
        }

        co_await signalCompletion->GetAwaitable();

        // Shutdown
        co_await (testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None));
        co_await (testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None));

        co_return;
    }

    // Goal: Verify CompleteCheckpointAsync is called on every state provider during recovery
    // Algorithm:
    //    1. Populate state providers
    //    2. Call PrepareCheckpoint and PerformCheckpoint
    //    3. Shut down the replica and Open again with completeCheckpoint flag
    //    4. Verify CompleteCheckpointAsync is called on every state provider
    //    5. Shut down the replica and clean up.
    Awaitable<void> StateManagerTest::Test_Recover_Replica_Call_CompleteCheckpoint(__in ULONG32 numStateProviders) noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        // Populate the state providers.
        KArray<KUri::CSPtr> stateProviderArray(GetAllocator(), numStateProviders);

        for (ULONG index = 0; index < numStateProviders; index++)
        {
            KUri::CSPtr name = GetStateProviderName(NameType::Random);
            status = stateProviderArray.Append(name);
            CODING_ERROR_ASSERT(NT_SUCCESS(status));
        }

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        co_await this->PopulateAsync(stateProviderArray);
        VerifyExist(stateProviderArray, true);

        // Take PrepareCheckpoint and PerformCheckpoint.
        co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();
        co_await testTransactionalReplicatorSPtr_->PerformCheckpointAsync(CancellationToken::None);
        
        // At this point, shut down the replica and bring up again.
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        testTransactionalReplicatorSPtr_.Reset();

        // Bring up the replica again.
        {
            testTransactionalReplicatorSPtr_ = CreateReplica(
                *partitionedReplicaIdCSPtr_,
                *runtimeFolders_,
                *partitionSPtr_);

            co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None, true, false);
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);
            auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

            // The OpenAsync call will invoke the CompleteCheckpoint and call CompleteCheckpoint on all state providers.
            // So here we verify all the state providers exist and checkpointed.
            for (KUri::CSPtr stateProviderName : stateProviderArray) 
            {
                IStateProvider2::SPtr outStateProvider;
                status = stateManagerSPtr->Get(*stateProviderName, outStateProvider);
                VERIFY_IS_TRUE(NT_SUCCESS(status));
                VERIFY_IS_NOT_NULL(outStateProvider);
                VERIFY_ARE_EQUAL(*stateProviderName, outStateProvider->GetName());

                TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);

                testStateProvider.ExpectCheckpoint(FABRIC_INVALID_SEQUENCE_NUMBER, true);
                testStateProvider.Verify(false, true);
            }

            // Shutdown
            co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
            co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);
        }

        co_return;
    }

    Awaitable<void> StateManagerTest::Test_Verify_PrepareCheckpoint_FirstCheckpointDuringOpen() noexcept
    {
        NTSTATUS status = STATUS_UNSUCCESSFUL;

        co_await testTransactionalReplicatorSPtr_->OpenAsync(CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_PRIMARY, CancellationToken::None);

        KString::SPtr workDir = TestHelper::GetStateManagerWorkDirectory(
            runtimeFolders_->get_WorkDirectory(), 
            *partitionedReplicaIdCSPtr_, 
            GetAllocator());

        KString::SPtr filePathSPtr = KPath::Combine(*workDir, L"StateManager.cpt", GetAllocator());
        KWString filePath = KWString(GetAllocator(), filePathSPtr->ToUNICODE_STRING());

        CheckpointFile::SPtr checkpointFileSPtr = nullptr;
        status = CheckpointFile::Create(
            *partitionedReplicaIdCSPtr_,
            filePath,
            GetAllocator(),
            checkpointFileSPtr);
        VERIFY_IS_TRUE(NT_SUCCESS(status));

        // Verify the first empty checkpoing file PrepareCheckpointLSN is set to FABRIC_AUTO_SEQUENCE_NUMBER(0);
        co_await checkpointFileSPtr->ReadAsync(CancellationToken::None);
        VERIFY_ARE_EQUAL(checkpointFileSPtr->PropertiesSPtr->PrepareCheckpointLSN, FABRIC_AUTO_SEQUENCE_NUMBER);

        // Shutdown
        co_await testTransactionalReplicatorSPtr_->ChangeRoleAsync(FABRIC_REPLICA_ROLE_NONE, CancellationToken::None);
        co_await testTransactionalReplicatorSPtr_->CloseAsync(CancellationToken::None);

        co_return;
    }

    ktl::Task StateManagerTest::GetStateProviderAndAddOperationTask(
        __in bool shouldThrow,
        __in KUriView const & stateProviderName,
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        IStateProvider2::SPtr outStateProvider = nullptr;
        auto stateManagerSPtr = testTransactionalReplicatorSPtr_->StateManager;

        bool isThrow = false;
        NTSTATUS status = stateManagerSPtr->Get(stateProviderName, outStateProvider);
        VERIFY_IS_TRUE(NT_SUCCESS(status));
        VERIFY_IS_NOT_NULL(outStateProvider);
        VERIFY_ARE_EQUAL(stateProviderName, outStateProvider->GetName());

        TestStateProvider & testStateProvider = dynamic_cast<TestStateProvider &>(*outStateProvider);
        LONG64 transactionId;

        Transaction::SPtr txnSPtr;
        testTransactionalReplicatorSPtr_->CreateTransaction(txnSPtr);
        KFinally([&] { txnSPtr->Dispose(); });
        transactionId = txnSPtr->TransactionId;

        try
        {
            TestOperation testOperation(TestOperation::Random, testStateProvider.StateProviderId, true, GetAllocator());
            co_await testStateProvider.AddAsync(*txnSPtr, testOperation);
            testStateProvider.ExpectCommit(txnSPtr->TransactionId);
            TEST_COMMIT_ASYNC(txnSPtr);

            // Verify the add operation success
            testStateProvider.Verify(transactionId);
        }
        catch (Exception & exception)
        {
            // If it throws the exception, verify the exception code and then continue next iteration.
            CODING_ERROR_ASSERT(exception.GetStatus() == STATUS_INVALID_PARAMETER);
            isThrow = true;
        }

        // If the RemoveAsync call committed, AddOperation call on state provider should throw.
        // Otherwise it should pass.
        CODING_ERROR_ASSERT(isThrow == shouldThrow);

        signalCompletion.SetResult(true);

        co_return;
    }

    Awaitable<void> StateManagerTest::BackupTask(
        __in KString const & backupDirectory,
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in_opt bool isBackupFirst,
        __in_opt AwaitableCompletionSource<bool> * const backupCompletion) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        if (!isBackupFirst && backupCompletion != nullptr)
        {
            co_await backupCompletion->GetAwaitable();
        }

        StateManager & statemanager = dynamic_cast<StateManager &>(*(testTransactionalReplicatorSPtr_->StateManager));
        KString::CSPtr currentPath = &backupDirectory;

        // BackupAsync call 
        NTSTATUS status = co_await statemanager.BackupCheckpointAsync(*currentPath, CancellationToken::None);
        CODING_ERROR_ASSERT(NT_SUCCESS(status));

        if (isBackupFirst && backupCompletion != nullptr)
        {
            backupCompletion->SetResult(true);
        }

        co_return;
    }

    Awaitable<void> StateManagerTest::PrepareCheckpointTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        co_await testTransactionalReplicatorSPtr_->PrepareCheckpointAsync();

        co_return;
    }

    Awaitable<void> StateManagerTest::PerformCheckpoint(
        __in AwaitableCompletionSource<bool> & signalCompletion,
        __in bool isCopyFirst,
        __in_opt AwaitableCompletionSource<bool> * const copyCompletion) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

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

    Awaitable<void> StateManagerTest::CompleteCheckpointTask(
        __in AwaitableCompletionSource<bool> & signalCompletion) noexcept
    {
        co_await ktl::CorHelper::ThreadPoolThread(underlyingSystem_->DefaultThreadPool());

        // Remove the PrepareCheckpointAsync and PerformCheckpointAsync since async call leads to 
        // thread change and GetAwaitable will not wait on AwaitableCompletionSource.
        AwaitableCompletionSource<bool>::SPtr tempCompletion(&signalCompletion);
        co_await tempCompletion->GetAwaitable();

        co_await testTransactionalReplicatorSPtr_->CompleteCheckpointAsync(CancellationToken::None);

        co_return;
    }

    BOOST_FIXTURE_TEST_SUITE(StateManagerTestSuite, StateManagerTest)

    // Test verifies Create and get Properties.
    BOOST_AUTO_TEST_CASE(Life_CreateOpenClose_Success)
    {
        SyncAwait(this->Test_Life_CreateOpenClose_Success());
    }

    BOOST_AUTO_TEST_CASE(Add_SingleOperation_Dispose_Success)
    {
        SyncAwait(this->Test_Add_SingleOperation_Abort_Success(true));
    }

    BOOST_AUTO_TEST_CASE(Add_SingleOperation_Abort_Success)
    {
        SyncAwait(this->Test_Add_SingleOperation_Abort_Success(false));
    }

    BOOST_AUTO_TEST_CASE(Add_SingleOperation_Commit_Success)
    {
        SyncAwait(this->Test_Add_SingleOperation_Commit_Success());
    }

    BOOST_AUTO_TEST_CASE(Add_InvalidName_Empty)
    {
        SyncAwait(this->Test_Add_InvalidName(StateManagerTestBase::NameType::Empty));
    }

    BOOST_AUTO_TEST_CASE(Add_RetryReplication_Commit_SuccessOnceUnblocked)
    {
        SyncAwait(this->Test_Add_BlockedReplication_InfiniteTimeout_CompleteOnceUnblocked());
    }

    BOOST_AUTO_TEST_CASE(Add_InvalidName_Schemaless)
    {
        SyncAwait(this->Test_Add_InvalidName(StateManagerTestBase::NameType::Schemaless));
    }

    // GetOrAdd test, the state provider exists, GetOrAdd call return the right state provider
    BOOST_AUTO_TEST_CASE(GetOrAdd_Exist_Read)
    {
        SyncAwait(this->Test_GetOrAdd_Success(true));
    }

    // GetOrAdd test, the state provider does not exist, GetOrAdd call will add the state provider
    BOOST_AUTO_TEST_CASE(GetOrAdd_NotExist_Added)
    {
        SyncAwait(this->Test_GetOrAdd_Success(false));
    }

    //
    // Scenario:        Test GetOrAdd Get Path and Add Path
    // Expected Result: Both path should return back outStateProvider
    // 
    BOOST_AUTO_TEST_CASE(GetOrAdd_BothPath_Return_OutStateProvider)
    {
        SyncAwait(this->Test_GetOrAdd_BothPath_Return_OutStateProvider());
    }

    //GetOrAdd test, the state provider does not exist, the first GetOrAdd call will add the state provider
    //ant the rest call will return the right state provider
    BOOST_AUTO_TEST_CASE(GetOrAdd_ConcurrentMultipleCall_OneCallAddAndOthersCallGets)
    {
        const ULONG taskNum = 10;
        SyncAwait(this->Test_GetOrAdd_ConcurrentMultipleCall_Success(taskNum));
    }

    // GetOrAdd test, before GetOrAdd the state provider already exists,
    // Dispose the txn with GetOrAdd call, state provider still exists
    // Then call remove the state provider to verify last GetOrAdd released the lock
    // and now the state provider doesn't exist
    BOOST_AUTO_TEST_CASE(GetOrAdd_Exist_Dispose_LockReleased)
    {
        SyncAwait(this->Test_GetOrAdd_Abort_Success(true, true));
    }

    // GetOrAdd test, before GetOrAdd the state provider already exists,
    // Abort the txn with GetOrAdd call, state provider still exists
    // Then call remove the state provider to verify last GetOrAdd released the lock
    // and now the state provider doesn't exist
    BOOST_AUTO_TEST_CASE(GetOrAdd_Exist_Abort_LockReleased)
    {
        SyncAwait(this->Test_GetOrAdd_Abort_Success(true, false));
    }

    // GetOrAdd test, before GetOrAdd the state provider does not exist,
    // Dispose the txn with GetOrAdd call, state provider does not exist,
    // Then call add the state provider to verify last GetOrAdd released the lock, 
    // and now the state provider exists
    BOOST_AUTO_TEST_CASE(GetOrAdd_NotExist_Dispose_LockReleased)
    {
        SyncAwait(this->Test_GetOrAdd_Abort_Success(false, true));
    }

    // GetOrAdd test, before GetOrAdd the state provider does not exist,
    // Abort the txn with GetOrAdd call, state provider does not exist,
    // Then call add the state provider to verify last GetOrAdd released the lock, 
    // and now the state provider exists
    BOOST_AUTO_TEST_CASE(GetOrAdd_NotExist_Abort_LockReleased)
    {
        SyncAwait(this->Test_GetOrAdd_Abort_Success(false, false));
    }

    // GetOrAdd test, call with a InvalidName, should return NTSTATUS error code
    BOOST_AUTO_TEST_CASE(GetOrAdd_InvalidName_Empty)
    {
        SyncAwait(this->Test_GetOrAdd_InvalidName(NameType::Empty));
    }

    // GetOrAdd test, call with a InvalidName, should return NTSTATUS error code
    BOOST_AUTO_TEST_CASE(GetOrAdd_InvalidName_Schemaless)
    {
        SyncAwait(this->Test_GetOrAdd_InvalidName(NameType::Schemaless));
    }

    // GetOrAdd call when the read access is not present, and should return NTSTATUS error code
    BOOST_AUTO_TEST_CASE(GetOrAdd_ReadAccessIsNotPresent)
    {
        SyncAwait(this->Test_GetOrAdd_ReadAccessIsNotPresent());
    }

    // TODO: #9023441 Add test which causes failure in AddLockContext
    //BOOST_AUTO_TEST_CASE(GetOrAdd_AddLockContextFailed)
    //{
    //    SyncAwait(this->Test_GetOrAdd_AddLockContextFailed());
    //}

    BOOST_AUTO_TEST_CASE(Get_NotExist_Error)
    {
        SyncAwait(this->Test_Get_NotExist_Error());
    }

    BOOST_AUTO_TEST_CASE(Get_InvalidName_Empty)
    {
        SyncAwait(this->Test_Get_InvalidName(NameType::Empty));
    }

    BOOST_AUTO_TEST_CASE(Get_InvalidName_Schemaless)
    {
        SyncAwait(this->Test_Get_InvalidName(NameType::Schemaless));
    }

    BOOST_AUTO_TEST_CASE(Remove_SingleOperation_Commit_Success)
    {
        SyncAwait(this->Test_Remove_Commit_Success(1));
    }

    BOOST_AUTO_TEST_CASE(Remove_MultipleOperation_Commit_Success)
    {
        SyncAwait(this->Test_Remove_Commit_Success(32));
    }

    BOOST_AUTO_TEST_CASE(Remove_SingleOperation_Abort_StillExist)
    {
        SyncAwait(this->Test_Remove_Abort_NoEffect(1));
    }

    BOOST_AUTO_TEST_CASE(Remove_MultipleOperation_Commit_StillExist)
    {
        SyncAwait(this->Test_Remove_Abort_NoEffect(32));
    }

    //
    // Scenario:        Remove a state provider with txn aborted, then remove the state provider with txn committed
    // Expected Result: First remove should have no effect on the state provider, and the second call should 
    //                  successfully remove the state provider
    // 
    BOOST_AUTO_TEST_CASE(Remove_SingleOperation_Abort_StillExist_ThenRemove_Commit)
    {
        SyncAwait(this->Test_Remove_Abort_ThenRemove_Commit(1));
    }

    //
    // Scenario:        Remove multiple state providers with txn aborted, then remove all state providers with txn committed
    // Expected Result: First remove should have no effect on the state providers, and the second call should 
    //                  successfully remove all the state providers
    // 
    BOOST_AUTO_TEST_CASE(Remove_MultipleOperation_Abort_StillExist_ThenRemove_Commit)
    {
        SyncAwait(this->Test_Remove_Abort_ThenRemove_Commit(32));
    }

    // TODO: Add add/remove timeout test (blocked replication) when the exception bug is fixed.

    BOOST_AUTO_TEST_CASE(Dispatch_SingleOperationTxn_Null_Success)
    {
        SyncAwait(this->Test_Dispatch_MultipleOperationTxn_Success(TestOperation::Null, 1, 1, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_SingleOperationTxn_Random_Success)
    {
        SyncAwait(this->Test_Dispatch_MultipleOperationTxn_Success(TestOperation::Random, 1, 1, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_MultipleOperationTxn_Null_Success)
    {
        SyncAwait(this->Test_Dispatch_MultipleOperationTxn_Success(TestOperation::Null, 8, 2, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_MultipleOperationTxn_Random_Success)
    {
        SyncAwait(this->Test_Dispatch_MultipleOperationTxn_Success(TestOperation::Random, 8, 8, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_MultipleOperationTxn_Random_WithOperationContext_Success)
    {
        SyncAwait(this->Test_Dispatch_MultipleOperationTxn_Success(TestOperation::Random, 8, 8, false));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicOperationTxn_Null_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicOperationTxn_Success(TestOperation::Null, 32, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicOperationTxn_Random_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicOperationTxn_Success(TestOperation::Random, 32, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicOperationTxn_Random_WithOperationContext_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicOperationTxn_Success(TestOperation::Random, 32, false));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicRedoOperationTxn_Null_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicRedoOperationTxn_Success(TestOperation::Null, 32, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicRedoOperationTxn_Random_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicRedoOperationTxn_Success(TestOperation::Random, 32, true));
    }

    BOOST_AUTO_TEST_CASE(Dispatch_AtomicRedoOperationTxn_Random_WithOperationContext_Success)
    {
        SyncAwait(this->Test_Dispatch_AtomicRedoOperationTxn_Success(TestOperation::Random, 32, false));
    }

    // Test is used to verify the CreateEnumerable late bound.
    // We have 3 groups of state providers 
    // StateProvider 0-9 will be added at the begining, and StateProvider 5-9 will be removed during enumeration
    // StateProvider 10-14 will be added during enumeration.
    // So StateProvider 0-4 must exist, and 5-14 may or may not exist.
    BOOST_AUTO_TEST_CASE(CreateEnumerable_LateBound_Success)
    {
        SyncAwait(this->Test_CreateEnumerable_LateBound());
    }

    //
    // Scenario:        Add the same state provider in a txn.
    // Expected Result: The second one should return NTSTATUS error code.
    // 
    BOOST_AUTO_TEST_CASE(VerifyWriteFollowedByWriteToTheSameKeyInATx)
    {
        SyncAwait(this->Test_VerifyWriteFollowedByWriteToTheSameKeyInATx());
    }

    //
    // Scenario:        Add the state provider and use GetOrAddAsync to read the state provider in a txn.
    // Expected Result: The second one should return NTSTATUS error code.
    // 
    BOOST_AUTO_TEST_CASE(WriteFollowedRead_AddToTheSameKey_InATx)
    {
        SyncAwait(this->Test_VerifyWriteFollowedReadWithAddToTheSameKeyInATx());
    }

    //
    // Scenario:        Add the state provider and then remove the state provider and use GetOrAddAsync to read the state provider in a txn
    // Expected Result: The GetOrAddAsync should read.
    // 
    BOOST_AUTO_TEST_CASE(WriteFollowedRead_RemoveToTheSameKey_InATx)
    {
        SyncAwait(this->Test_VerifyWriteFollowedReadWithRemoveToTheSameKeyInATx());
    }

    //
    // Scenario:        Replica close before the add txn dispose.
    // Expected Result: Txn is disposed after replica closed.
    // 
    // Note: managed test, commit 5505081a: Fix race between close and unlock in SM by not clearing in memory state on close
    BOOST_AUTO_TEST_CASE(Replica_Closed_BeforeAddTxDispose)
    {
        SyncAwait(this->Test_VerifyReplicaCloseBeforeTxDispose(true));
    }

    //
    // Scenario:        Replica close before the remove txn dispose.
    // Expected Result: Txn is disposed after replica closed.
    // 
    // Note: managed test, commit 5505081a: Fix race between close and unlock in SM by not clearing in memory state on close
    BOOST_AUTO_TEST_CASE(Replica_Closed_BeforeRemoveTxDispose)
    {
        SyncAwait(this->Test_VerifyReplicaCloseBeforeTxDispose(false));
    }

    //
    // Scenario:        Close the replica during the state provider operations
    // Expected Result: should throw Closed exception.
    // 
    BOOST_AUTO_TEST_CASE(Replica_Close_During_StateProvider_Operations_Throw)
    {
        SyncAwait(this->Test_Replica_Close_During_StateProvider_Operations_Throw());
    }

    //
    // Scenario:        Simulate the situation that change role retry when user service fails change role.
    // Expected Result: Role is successfully changed.
    // 
    BOOST_AUTO_TEST_CASE(Replica_Change_Role_Retry_SUCCESS)
    {
        SyncAwait(this->Test_Replica_Change_Role_Retry_WhenUserServiceFailsChangeRole());
    }

    //
    // Scenario:        AddOperation on a not registered state provider
    // Expected Result: Should throw exception
    // 
    BOOST_AUTO_TEST_CASE(AddOperation_StateProviderNotRegistered_Throw)
    {
        SyncAwait(this->Test_AddOperation_StateProviderNotRegistered_Throw());
    }

    //
    // Scenario:        Add a state provider then removed it, call AddOperation on the state provider
    // Expected Result: Should throw exception
    // 
    BOOST_AUTO_TEST_CASE(AddOperation_OnRemovedStateProvider_Throw)
    {
        SyncAwait(this->Test_AddOperation_OnRemovedStateProvider_Throw());
    }

    //
    // Scenario:        Add a state provider then Keep Call AddOperation on it, Remove the state provider at same point,
    // Expected Result: All AddOperation before the remove should success and all after should throw
    // 
    BOOST_AUTO_TEST_CASE(AddOperation_OperationAfterRemoveThrow)
    {
        const ULONG AddOperationCount = 16;
        SyncAwait(this->Test_AddOperation_OperationAfterRemoveThrow(AddOperationCount));
    }

    //
    // Scenario:        Concurrently run 3 tasks, one task keeps on doing GetOrAdd and then AddOperation on SP, 
    //                  one task keeps on deleting SP and one task that keeps Get SP and doing an AddOperation
    // Expected Result: AddOperation on the GetOrAdd and Get task may success or throw exception, if throws, the
    //                  exception must be STATUS_INVALID_PARAMETER
    // 
    BOOST_AUTO_TEST_CASE(AddOperation_Concurrent_Tasks)
    {
        const ULONG taskCount = 128;
        SyncAwait(this->Test_AddOperation_Concurrent_Tasks(taskCount));
    }

    //
    // Scenario:        Call state manager function with replica closed
    // Expected Result: Should throw exception
    // 
    BOOST_AUTO_TEST_CASE(Replica_Closed_Throw)
    {
        SyncAwait(this->Test_Replica_Closed_Throw());
    }

    //
    // Scenario:        Test state manager function BackupAsync with empty state, the state manager should be backed up
    //                  and the back up file should be saved.
    // Expected Result: Back up file exists. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAsync_EmptyState_FileSaved)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAsync_FileSaved(EmptyState, numberOfStateProvider));
    }

    //
    // Scenario:        Test state manager function BackupAsync with states, the state manager should be backed up
    //                  and the back up file should be saved.
    // Expected Result: Back up file exists. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAsync_WithStates_FileSaved)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAsync_FileSaved(WithStates, numberOfStateProvider));
    }

    //
    // Scenario:        Test state manager function BackupAsync with states and the states are checkpointed, 
    //                  the state manager should be backed up and the back up file should be saved.
    // Expected Result: Back up file exists. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAsync_WithCheckpointedStates_FileSaved)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAsync_FileSaved(Checkpointed, numberOfStateProvider));
    }

    //
    // Scenario:        Test state manager Backup and Restore with empty state, the state manager should be backed up
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_EmptyState)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore(EmptyState, numberOfStateProvider));
    }

    //
    // Scenario:        Test state manager Backup and Restore with states, the state manager should be backed up
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_WithStates)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore(WithStates, numberOfStateProvider));
    }

    //
    // Scenario:        Test state manager Backup and Restore with checkpointed states, the state manager should be backed up
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_WithCheckpointedStates)
    {
        const ULONG numberOfStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore(Checkpointed, numberOfStateProvider));
    }

    //
    // Scenario:        State manager Backup and Restore test with empty state, new states added after BackupAsync call.
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    //                  new states after BackupAsync call should not exist.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_BackupEmptyState_NewStatesNotExist)
    {
        const ULONG numberOfStateProvider = 16;
        const ULONG numberOfNewStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore_NewStatesNotExist(EmptyState, numberOfStateProvider, numberOfNewStateProvider));
    }

    //
    // Scenario:        State manager Backup and Restore test with some states, new states added after BackupAsync call.
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    //                  new states after BackupAsync call should not exist.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_BackupWithStates_NewStatesNotExist)
    {
        const ULONG numberOfStateProvider = 16;
        const ULONG numberOfNewStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore_NewStatesNotExist(WithStates, numberOfStateProvider, numberOfNewStateProvider));
    }

    //
    // Scenario:        State manager Backup and Restore test with checkpointed states, new states added after BackupAsync call.
    // Expected Result: Back up file should be changed to current checkpoint file. The state manger state should not be changed.
    //                  new states after BackupAsync call should not exist.
    // 
    BOOST_AUTO_TEST_CASE(BackupAndRestore_BackupCheckpointedStates_NewStatesNotExist)
    {
        const ULONG numberOfStateProvider = 16;
        const ULONG numberOfNewStateProvider = 16;
        SyncAwait(this->Test_BackupAndRestore_NewStatesNotExist(Checkpointed, numberOfStateProvider, numberOfNewStateProvider));
    }

    //
    // Scenario:        State manager RestoreAsync call with no backup file
    // Expected Result: Should return NTSTATUS error code
    // 
    BOOST_AUTO_TEST_CASE(Restore_NoBackupFile)
    {
        SyncAwait(this->Test_Restore_NoBackupFile());
    }

    //
    // Scenario:        State manager RestoreAsync call with backup file which has data corruption
    // Expected Result: Should return NTSTATUS error code
    // 
    BOOST_AUTO_TEST_CASE(Restore_DataCorruption)
    {
        const ULONG corruptPosition = 100;
        SyncAwait(this->Test_Restore_DataCorruption(corruptPosition));
    }

    //
    // Scenario:        State manager Open with cleanupRestore, which will call CleanUpIncompleteRestoreAsync, should succeed
    // Expected Result: Should succeed
    // 
    BOOST_AUTO_TEST_CASE(CleanUpIncompleteRestore_DuringOpen)
    {
        SyncAwait(this->Test_CleanUpIncompleteRestore_DuringOpen());
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then backup race with the second checkpoint PrepareCheckpoint function.
    // Expected Result: All state providers in the first checkpoint should exist after resotre, 
    //                  state providers added after the first checkpoint should not exist
    // 
    BOOST_AUTO_TEST_CASE(Backup_PrepareCheckpoint_Race_BackupBasepoint_AddCase)
    {
        SyncAwait(this->Test_Backup_PrepareCheckpoint_Race_BackupBasepoint(16, 8, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove some state providers.
    //                  Then backup race with the second checkpoint PrepareCheckpoint function.
    // Expected Result: All state providers in the first checkpoint should exist after resotre, 
    //                  state providers removed after the first checkpoint should still exist
    // 
    BOOST_AUTO_TEST_CASE(Backup_PrepareCheckpoint_Race_BackupBasepoint_RemoveCase)
    {
        SyncAwait(this->Test_Backup_PrepareCheckpoint_Race_BackupBasepoint(16, 8, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then backup race with the second checkpoint PerformCheckpoint function,
    // Expected Result: If backup happened first, base checkpoint will be restored, otherwise the
    //                  new checkpoint will be restored
    // 
    BOOST_AUTO_TEST_CASE(Backup_PerformCheckpoint_Race_AddCase_BakcupFirst)
    {
        SyncAwait(this->Test_Backup_PerformCheckpoint_Race(16, 8, true, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then backup race with the second checkpoint PerformCheckpoint function,
    // Expected Result: If backup happened first, base checkpoint will be restored, otherwise the
    //                  new checkpoint will be restored
    // 
    BOOST_AUTO_TEST_CASE(Backup_PerformCheckpoint_Race_AddCase_PerformCheckpointFirst)
    {
        SyncAwait(this->Test_Backup_PerformCheckpoint_Race(16, 8, true, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove some state providers.
    //                  Then backup race with the second checkpoint PerformCheckpoint function,
    // Expected Result: If backup happened first, base checkpoint will be restored, otherwise the
    //                  new checkpoint will be restored
    // 
    BOOST_AUTO_TEST_CASE(Backup_PerformCheckpoint_Race_RemoveCase_BakcupFirst)
    {
        SyncAwait(this->Test_Backup_PerformCheckpoint_Race(16, 8, false, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove some state providers.
    //                  Then backup race with the second checkpoint PerformCheckpoint function,
    // Expected Result: If backup happened first, base checkpoint will be restored, otherwise the
    //                  new checkpoint will be restored
    // 
    BOOST_AUTO_TEST_CASE(Backup_PerformCheckpoint_Race_RemoveCase_PerformCheckpointFirst)
    {
        SyncAwait(this->Test_Backup_PerformCheckpoint_Race(16, 8, false, false));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, add more state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function. 
    // Expected Result: All state providers in the second checkpoint should be restored, 
    // 
    BOOST_AUTO_TEST_CASE(Backup_CompleteCheckpoint_Race_AddCase)
    {
        SyncAwait(this->Test_Backup_CompleteCheckpoint_Race(16, 8, true));
    }

    //
    // Scenario:        Add state providers, take first checkpoint, remove partial state providers.
    //                  Then copy race with the second checkpoint CompleteCheckpoint function. 
    // Expected Result: All state providers in the second checkpoint should be restored, 
    // 
    BOOST_AUTO_TEST_CASE(Backup_CompleteCheckpoint_Race_RemoveCase)
    {
        SyncAwait(this->Test_Backup_CompleteCheckpoint_Race(16, 8, false));
    }

    //
    // Scenario:        Create a state provider and verify the factory arguments are correctly set
    // Expected Result: ReplicaId and PartitionId should be as expected
    // 
    BOOST_AUTO_TEST_CASE(FactoryArguments_Properties)
    {
        SyncAwait(this->Test_FactoryArguments_Properties());
    }

    //
    // Scenario:        RemoveAsync call blocks AddOperation, commit RemoveAsync in the end.
    // Expected Result: AddOperation should throw exception.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_Remove_BlockAddOperation_RemoveCommit_AddOperationThrow)
    {
        SyncAwait(this->Test_StateProvider_Remove_BlockAddOperation(true));
    }

    //
    // Scenario:        RemoveAsync call blocks AddOperation, abort RemoveAsync in the end.
    // Expected Result: AddOperation should succeed.
    // 
    BOOST_AUTO_TEST_CASE(StateProvider_Remove_BlockAddOperation_RemoveAbort_AddOperationSucceed)
    {
        SyncAwait(this->Test_StateProvider_Remove_BlockAddOperation(false));
    }

    //
    // Note: Regression test to verify the CompleteCheckpointAsync is called on all the state providers.
    //
    // Scenario:        StateManager specified with completeCheckpoint during recovery
    // Expected Result: CompleteCheckpointAsync will be called on every state provider.
    // 
    BOOST_AUTO_TEST_CASE(Recover_Replica_Call_CompleteCheckpoint)
    {
        const ULONG32 numStateProviders = 16;
        SyncAwait(this->Test_Recover_Replica_Call_CompleteCheckpoint(numStateProviders));
    }

    //
    // Scenario:        During StateManager Open, it will write first empty checkpoing file.
    // Expected Result: For the first empty checkpoint file, the PrepareCheckpoint property should be FABRIC_AUTO_SEQUENCE_NUMBER(0)
    // 
    BOOST_AUTO_TEST_CASE(Verify_PrepareCheckpoint_FirstCheckpointDuringOpen)
    {
        SyncAwait(this->Test_Verify_PrepareCheckpoint_FirstCheckpointDuringOpen());
    }

    BOOST_AUTO_TEST_SUITE_END()
}
