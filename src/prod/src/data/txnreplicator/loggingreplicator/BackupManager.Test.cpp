// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#include "stdafx.h"
#include "TestHeaders.h"

#include <stdlib.h>
#include <boost/test/unit_test.hpp>
#include "Common/boost-taef.h"

namespace LoggingReplicatorTests
{
    using namespace Common;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace Data::Utilities;
    using namespace TxnReplicator;

    StringLiteral const TraceComponent("BackupManagerTests");

    class BackupManagerTests
    {
    public:
        Awaitable<void> Test_BackupInfo_Sanity(
            __in KString const & backupFolder,
            __in Common::Random & rndNumber,
            __in int seed);

        Awaitable<void> Test_ShouldCleanState(
            __in KString const & backupFolder,
            __in bool restoreTokenExists);

        Awaitable<void> Test_IncrementalBackup_VariableNumberOfRecords_Success(
            __in KString const & backupFolder,
            __in ULONG numberOfRecords,
            __in Common::Random & rndNumber,
            __in int seed);

    protected:
        void AddOperationToTransaction(
            __in Transaction & transaction,
            __in Common::Random & rndNumber,
            __in TestStateProviderManager & stateManager);

        wstring CreateFileName(__in wstring const & folderName);

        void EndTest();

        CommonConfig config; // load the config object as its needed for the tracing to work
        FABRIC_REPLICA_ID rId_;
        KGuid pId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;
    };

    Awaitable<void> BackupManagerTests::Test_BackupInfo_Sanity(
        __in KString const & backupFolder,
        __in Common::Random & rndNumber,
        __in int seed)
    {
        // Setup
        KAllocator & allocator = underlyingSystem_->PagedAllocator();

        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);
            ApiFaultUtility::SPtr apiFaultUtility = ApiFaultUtility::Create(allocator);
            TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(backupFolder, allocator);

            TestReplica::SPtr replica = TestReplica::Create(
                pId_,
                *invalidLogRecords,
                true, // isPrimary 
                nullptr,
                *apiFaultUtility,
                allocator);

            co_await replica->InitializeAsync(
                seed,
                false, // skipRecovery
                true); // delayApis

                       // Setup backup manager
            BackupManager::SPtr backupManager(replica->BackupManager);

            backupManager->EnableBackup();

            // 1. Do full backup to get started.
            co_await backupManager->BackupAsync(*backupCallbackHandler);

            // 2. Do multiple operations
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                KFinally([&] { transaction->Dispose(); });
                AddOperationToTransaction(*transaction, rndNumber, *replica->StateManager);
                co_await transaction->CommitAsync();
            }

            // 3. Do an incremental backup
            co_await backupManager->BackupAsync(*backupCallbackHandler, FABRIC_BACKUP_OPTION_INCREMENTAL, TimeSpan::FromMinutes(1), CancellationToken::None);

            // 4. Verification
            KArray<TxnReplicator::BackupInfo> backupInfoArray = backupCallbackHandler->BackupInfoArray;
            VERIFY_ARE_EQUAL(backupInfoArray.Count(), static_cast<ULONG>(2));
            VERIFY_ARE_EQUAL(backupInfoArray[0].BackupId, backupInfoArray[0].ParentBackupId);
            VERIFY_ARE_EQUAL(backupInfoArray[1].ParentBackupId, backupInfoArray[0].BackupId);

            // 5. Close the replica.
            co_await backupManager->DisableBackupAndDrainAsync();
            co_await replica->CloseAndQuiesceReplica();
            co_await replica->EndTestAsync(false, true);
        }

        co_return;
    }

    Awaitable<void> BackupManagerTests::Test_ShouldCleanState(
        __in KString const & backupFolder,
        __in bool restoreTokenExists)
    {
        KAllocator & allocator = underlyingSystem_->PagedAllocator();

        InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);

        TestBackupRestoreProvider::SPtr loggingReplicatorSPtr = TestBackupRestoreProvider::Create(
            allocator);

        ApiFaultUtility::SPtr faultUtility = ApiFaultUtility::Create(allocator);

        TestStateProviderManager::SPtr testStateManager = TestStateProviderManager::Create(*faultUtility, allocator);
        TestReplicatedLogManager::SPtr testReplicatedLogManager = TestReplicatedLogManager::Create(allocator);
        TestCheckpointManager::SPtr checkpointManager = TestCheckpointManager::Create(allocator);

        TestLogManager::SPtr testLogManager = TestLogManager::Create(allocator);
        testLogManager->InitializeWithNewLogRecords();

        TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());

        TestOperationProcessor::SPtr testOperationProcessor = TestOperationProcessor::Create(*prId_, 0, 10, 7, settings, allocator);
        KFinally([&] {testOperationProcessor->Close(); });

        TestHealthClientSPtr healthClient = TestHealthClient::Create();

        BackupManager::SPtr backupManager = BackupManager::Create(
            *prId_,
            backupFolder,
            *testStateManager,
            *testReplicatedLogManager,
            *testLogManager,
            settings,
            healthClient,
            *invalidLogRecords,
            allocator);

        backupManager->Initialize(*loggingReplicatorSPtr, *checkpointManager, *testOperationProcessor);

        if (restoreTokenExists)
        {
            co_await backupManager->CreateRestoreTokenAsync(Common::Guid::Empty());
        }

        bool actualResult = backupManager->ShouldCleanState();
        VERIFY_ARE_EQUAL(actualResult, restoreTokenExists);

        co_return;
    }

    Awaitable<void> BackupManagerTests::Test_IncrementalBackup_VariableNumberOfRecords_Success(
        __in KString const & backupFolder,
        __in ULONG numberOfRecords,
        __in Common::Random & rndNumber,
        __in int seed)
    {
        // Setup
        KAllocator & allocator = underlyingSystem_->PagedAllocator();

        // 1. Take backups.
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);
            ApiFaultUtility::SPtr apiFaultUtility = ApiFaultUtility::Create(allocator);
            TestBackupCallbackHandler::SPtr backupCallbackHandler = TestBackupCallbackHandler::Create(backupFolder, allocator);

            TestReplica::SPtr replica = TestReplica::Create(
                pId_,
                *invalidLogRecords,
                true, // isPrimary 
                nullptr,
                *apiFaultUtility,
                allocator);

            co_await replica->InitializeAsync(
                seed,
                false, // skipRecovery
                true); // delayApis

            // Setup backup manager
            BackupManager::SPtr backupManager(replica->BackupManager);

            backupManager->EnableBackup();

            // 1. Do full backup to get started.
            co_await backupManager->BackupAsync(*backupCallbackHandler);

            // 2. Do multiple operations
            for (ULONG i = 0; i < numberOfRecords; i++)
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                KFinally([&] { transaction->Dispose(); });
                AddOperationToTransaction(*transaction, rndNumber, *replica->StateManager);
                co_await transaction->CommitAsync();
            }

            // 3. Do an incremental backup
            co_await backupManager->BackupAsync(
                *backupCallbackHandler, 
                FABRIC_BACKUP_OPTION_INCREMENTAL, 
                TimeSpan::FromMinutes(1), 
                CancellationToken::None);

            // 4. Verification
            KGuid readId;
            readId.CreateNew();
            BackupFolderInfo::SPtr backupFolderInfoSPtr = BackupFolderInfo::Create(*prId_, readId, backupFolder, allocator);
            co_await backupFolderInfoSPtr->AnalyzeAsync(CancellationToken::None);
            VERIFY_ARE_EQUAL(backupFolderInfoSPtr->BackupMetadataArray.Count(), 2);

            // 5. Close the replica.
            co_await backupManager->DisableBackupAndDrainAsync();
            co_await replica->CloseAndQuiesceReplica();
            co_await replica->EndTestAsync(false, true);
        }

        // 2. Create a new (empty replica) to restore the state.
        {
            InvalidLogRecords::SPtr invalidLogRecords = InvalidLogRecords::Create(allocator);
            ApiFaultUtility::SPtr apiFaultUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr replica = TestReplica::Create(
                pId_,
                *invalidLogRecords,
                true, // isPrimary 
                nullptr,
                *apiFaultUtility,
                allocator);
            co_await replica->InitializeAsync(
                seed, 
                false, // skipRecovery.
                true); // delayApis

            // Setup backup manager
            BackupManager::SPtr backupManager(replica->BackupManager);

            backupManager->EnableRestore();

            co_await backupManager->RestoreAsync(backupFolder);

            backupManager->DisableRestore();

            co_await backupManager->DisableBackupAndDrainAsync();
            co_await replica->CloseAndQuiesceReplica();
            co_await replica->EndTestAsync(false, true);
        }
      
        co_return;
    }

    void BackupManagerTests::AddOperationToTransaction(
        __in Transaction & transaction,
        __in Common::Random & rndNumber,
        __in TestStateProviderManager & stateManager)
    {
        TestStateProviderManager::SPtr stateManagerPtr = &stateManager;
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
        int bufferCount = rndNumber.Next(0, 5);
        int bufferSize = rndNumber.Next(0, 50);

        Trace.WriteInfo(
            TraceComponent,
            "{0}: Adding Operation Data with buffer count {1} bufferSize {2} to Txn {3}",
            prId_->TraceId,
            bufferCount,
            bufferSize,
            transaction.TransactionId);

        OperationData::CSPtr data = TestTransaction::GenerateOperationData(bufferCount, bufferSize, allocator).RawPtr();

        stateManagerPtr->AddExpectedTransactionApplyData(
            transaction.TransactionId,
            data,
            data);

        TestTransaction::TestOperationContext::CSPtr context = TestTransaction::TestOperationContext::Create(allocator);
        LONG64 stateProviderId = KDateTime::Now();

        transaction.AddOperation(
            data.RawPtr(),
            data.RawPtr(),
            data.RawPtr(),
            stateProviderId,
            context.RawPtr());
    }

    wstring BackupManagerTests::CreateFileName(
        __in wstring const & folderName)
    {
        wstring testFolderPath = Directory::GetCurrentDirectoryW();
        Path::CombineInPlace(testFolderPath, folderName);

        return testFolderPath;
    }

    void BackupManagerTests::EndTest()
    {
        prId_.Reset();
    }

    BOOST_FIXTURE_TEST_SUITE(BackupManagerTestSuite, BackupManagerTests);

    BOOST_AUTO_TEST_CASE(BackupInfo_Sanity)
    {
        // Setup
        wstring testName(L"BackupInfo_Sanity");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_BackupInfo_Sanity(*folderPath, r, seed));
        }

        // Post clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_CASE(ShouldCleanState_RestoreTokenDoesNotExist_ReturnFalse)
    {
        // Setup
        wstring testName(L"ShouldCleanState_RestoreTokenDoesNotExist_ReturnFalse");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_ShouldCleanState(*folderPath, false));
        }


        // Post clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_CASE(ShouldCleanState_RestoreTokenExists_ReturnTrue)
    {
        // Setup
        wstring testName(L"ShouldCleanState_RestoreTokenExists_ReturnTrue");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_ShouldCleanState(*folderPath, true));
        }

        // Post clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_CASE(IncrementalBackup_Empty_Success)
    {
        // Setup
        wstring testName(L"IncrementalBackup_Empty_Success");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_IncrementalBackup_VariableNumberOfRecords_Success(*folderPath, 0, r, seed));
        }

        // Post clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_CASE(IncrementalBackup_SingleRecord_Success)
    {
        // Setup
        wstring testName(L"IncrementalBackup_SingleRecord_Success");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_IncrementalBackup_VariableNumberOfRecords_Success(*folderPath, 1, r, seed));
        }

        // Post clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_CASE(IncrementalBackup_MultipleRecord_Success)
    {
        // Setup
        wstring testName(L"IncrementalBackup_MultipleRecord_Success");
        wstring testFolderPath = CreateFileName(testName);

        // Pre-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);

        TEST_TRACE_BEGIN(testName)
        {
            KString::SPtr folderPath = KPath::CreatePath(testFolderPath.c_str(), allocator);
            SyncAwait(Test_IncrementalBackup_VariableNumberOfRecords_Success(*folderPath, 128, r, seed));
        }

        // Post-clean up
        Directory::Delete_WithRetry(testFolderPath, true, true);
    }

    BOOST_AUTO_TEST_SUITE_END();
}
