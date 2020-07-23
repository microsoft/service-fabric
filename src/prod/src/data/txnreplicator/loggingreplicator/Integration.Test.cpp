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
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;
    using namespace Data::TestCommon;
        
    #define INTEGRATIONTEST_TAG 'pTNI'
    StringLiteral const TraceComponent = "IntegrationTest";

    wstring const TestLogFolder = L"log";
    std::wstring const windowsDirectoryPath = L"UpgradeTests";
    std::wstring const linuxDirectoryPath = Common::Path::Combine(L"data_upgrade", L"5.1");

    struct LRTestGlobal {
        LRTestGlobal()
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
            Trace.WriteInfo("LRTestGlobal", "LRTestGlobal Fixture Created");
        }

        ~LRTestGlobal()
        {
            ErrorCode err = TestReplica::DeleteWorkingDirectory();

            Trace.WriteInfo(
                "LRTestGlobal",
                "TestReplica::DeleteWorkingDirectory Cleanup returned {0}",
                err);
        }
    };

#pragma region IntegrationTests_Class
    class IntegrationTests 
    {
    public:
        TestReplica::SPtr primaryTestReplica_;
        TestReplica::SPtr secondaryTestReplica_;
    protected:

        void InitializeTest();
        void EndTest();

        void AddOperationToTransaction(
            __in Transaction & transaction,
            __in Common::Random & rndNumber,
            __in TestStateProviderManager & stateManager);

        void AddOperationToTransaction(
            __in Transaction & transaction,
            __in ULONG bufferCount,
            __in ULONG bufferSize,
            __in TestStateProviderManager & stateManager);

        Awaitable<NTSTATUS> WaitForLogFlushUptoLsn(__in LONG64 upToLsn);
        CopyStream::WaitForLogFlushUpToLsnCallback GetWaitForLogFlushUpToLsnCallback();

        void CloseAndReopenTestReplica(
            __in TestReplica::SPtr & testReplica,
            __in int seed);

        void CreateCopyStream(
            __in TestReplica::SPtr & newPrimary,
            __in TestReplica::SPtr & oldPrimary,
            __in CopyContext::SPtr & copyContext,
            __in KAllocator & allocator,
            __out CopyStream::SPtr & copyStream,
            __in LONG64 upToLsn = 50);

        void CreateNewReplicaFromCopyStream(
            __in TestReplica::SPtr & newPrimary,
            __in TestReplica::SPtr & oldPrimary,
            __in CopyStream::SPtr & copyStream,
            __in KAllocator & allocator,
            __in LONG64 lastFlushedPsn = 11);

        void CleanupTruncateTailFalseProgressTest(
            __in TestReplica::SPtr & newPrimary,
            __in TestReplica::SPtr & oldPrimary,
            __in CopyStream::SPtr & copyStream);

        void CreatePrimaryReplica(
            __in int seed,
            __in KAllocator & allocator,
            __out TestReplica::SPtr & newReplica);

        void CreateSecondaryReplica(
            __in int seed,
            __in ApiFaultUtility::SPtr & faultUtility,
            __in KAllocator & allocator,
            __out TestReplica::SPtr & newReplica);

        void CreateAndInitializeReplica(
            __in bool isPrimary,
            __in ApiFaultUtility::SPtr & faultUtility,
            __in int seed,
            __in KAllocator & allocator,
            __out TestReplica::SPtr & newReplica);

        void CreateLogRecordUpgradeTest_WorkingDirectory(
            __in std::wstring & upgradeVersionPath,
            __in KAllocator & allocator,
            __out KString::SPtr & workDir)
        {
            std::wstring workingDir = wformatString("{0}_tmp_integration", upgradeVersionPath);
            std::wstring dir2 = L"";

#if !defined(PLATFORM_UNIX)        
            dir2 = windowsDirectoryPath;
#else
            dir2 = linuxDirectoryPath;
#endif

            ASSERT_IFNOT(dir2 != L"", "Directory path must be set");

            // Current directory
            std::wstring prevLogFileDirectory = Common::Directory::GetCurrentDirectoryW();
            std::wstring workingDirectory = Common::Directory::GetCurrentDirectoryW();

            prevLogFileDirectory = Common::Path::Combine(prevLogFileDirectory, dir2);
            workingDirectory = Common::Path::Combine(workingDirectory, dir2);

            prevLogFileDirectory = Common::Path::Combine(prevLogFileDirectory, upgradeVersionPath);
            workingDirectory = Common::Path::Combine(workingDirectory, workingDir);

            // Verify the target upgrade path must exist
            // Ex. WindowsFabric\out\debug-amd64\bin\FabricUnitTests\UpgradeTests\version_60
            bool directoryExists = Common::Directory::Exists(prevLogFileDirectory);
            ASSERT_IFNOT(directoryExists, "Existing log path: {0} not found", prevLogFileDirectory);

            ErrorCode er = Common::Directory::Copy(prevLogFileDirectory, workingDirectory, true);
            ASSERT_IFNOT(er.IsSuccess(), "Failed to create tmp working directory. Error : {0}", er.ErrorCodeValueToString());

            workDir = KString::Create(workingDirectory.c_str(), allocator);
            ASSERT_IFNOT(workDir != nullptr, "Working directory cannot be null");
        };

        bool VerifyInvalidRecordFields(__in LogRecord & record)
        {
            bool ret =
                record.Lsn == Constants::InvalidLsn &&
                record.Psn == Constants::InvalidPsn &&
                record.RecordPosition == Constants::InvalidRecordPosition &&
                record.RecordLength == Constants::InvalidRecordLength;

            switch (record.RecordType)
            {
            case LogRecordType::BeginCheckpoint:
            {
                BeginCheckpointLogRecord * beginCheckpointRecord = dynamic_cast<BeginCheckpointLogRecord *>(&record);
                ret &= 
                    beginCheckpointRecord->EpochValue == Epoch::InvalidEpoch() &&
                    beginCheckpointRecord->CheckpointState == CheckpointState::Invalid &&
                    beginCheckpointRecord->EarliestPendingTransactionOffest == Constants::InvalidLogicalRecordOffset &&
                    beginCheckpointRecord->LastStableLsn == Constants::InvalidLsn;

                break;
            }
            case LogRecordType::BeginTransaction:
            {
                BeginTransactionOperationLogRecord * beginTxOpLogRecord = dynamic_cast<BeginTransactionOperationLogRecord *>(&record);
                ret &= beginTxOpLogRecord->RecordEpoch == Epoch::InvalidEpoch();
                break;
            }
            case LogRecordType::Indexing:
            {
                IndexingLogRecord * logHeadRecord = dynamic_cast<IndexingLogRecord *>(&record);
                ret &= logHeadRecord->CurrentEpoch == Epoch::InvalidEpoch();

                break;
            }
            default:
                break;
            }

            return ret;
        }

        ULONG64 GetBytesUsed(__in TestReplica & replica, __in LogRecord & startingRecord)
        {
            ULONG64 tail = replica.ReplicatedLogManager->InnerLogManager->CurrentLogTailPosition;
            LONG64 bufferedBytes = replica.ReplicatedLogManager->InnerLogManager->BufferedBytes;
            LONG64 pendingFlushBytes = replica.ReplicatedLogManager->InnerLogManager->PendingFlushBytes;
            ULONG tailRecordSize = replica.ReplicatedLogManager->InnerLogManager->CurrentLogTailRecord->ApproximateSizeOnDisk;
            auto bytesUsed = tail + (ULONG64)tailRecordSize + (ULONG64)bufferedBytes + (ULONG64)pendingFlushBytes - startingRecord.RecordPosition;
            return bytesUsed;
        }

        wstring cwd_;
        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;

        InvalidLogRecords::SPtr invalidLogRecords_;
        ApiFaultUtility::SPtr apiFaultUtility_;
    };

    Awaitable<NTSTATUS> IntegrationTests::WaitForLogFlushUptoLsn(__in LONG64 upToLsn)
    {
        ULONG32 randomSleep = upToLsn % 3;
        if (randomSleep == 0)
        {
            co_return STATUS_SUCCESS;
        }
        else
        {
            co_await apiFaultUtility_->WaitUntilSignaledAsync(ApiName::WaitForLogFlushUptoLsn);
            co_return STATUS_SUCCESS;
        }
    }

    CopyStream::WaitForLogFlushUpToLsnCallback IntegrationTests::GetWaitForLogFlushUpToLsnCallback()
    {
        CopyStream::WaitForLogFlushUpToLsnCallback callback(this, &IntegrationTests::WaitForLogFlushUptoLsn);
        return callback;
    }

    void IntegrationTests::CloseAndReopenTestReplica(
        __in TestReplica::SPtr & testReplica,
        __in int seed)
    {
        PartitionedReplicaId::SPtr prId = testReplica->PartitionId;

        testReplica->EndTest(true);
        Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
        testReplica->Initialize(seed, true);
        testReplica->PartitionId = prId;
    }

    void IntegrationTests::CreateCopyStream(
        __in TestReplica::SPtr & newPrimary,
        __in TestReplica::SPtr & oldPrimary,
        __in CopyContext::SPtr & copyContext,
        __in KAllocator & allocator,
        __out CopyStream::SPtr & copyStream,
        __in LONG64 upToLsn)
    {
        CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);
        CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
        auto currentConfig = oldPrimary->TxnReplicatorConfig;
        copyStream = CopyStream::Create(
            *newPrimary->PartitionId,
            *newPrimary->ReplicatedLogManager,
            *newPrimary->StateManager,
            *newPrimary->CheckpointManager,
            callback,
            oldPrimary->PartitionId->ReplicaId,
            upToLsn,
            *copyContext,
            *copyStageBuffers,
            currentConfig,
            true,
            allocator);
    }

    void IntegrationTests::CreateNewReplicaFromCopyStream(
        __in TestReplica::SPtr & newPrimary,
        __in TestReplica::SPtr & oldPrimary,
        __in CopyStream::SPtr & copyStream,
        __in KAllocator & allocator,
        __in LONG64 lastFlushedPsn)
    {
        newPrimary->StateManager->Reuse();
        oldPrimary->RoleContextDrainState->Reuse();
        oldPrimary->RoleContextDrainState->ChangeRole(FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
        oldPrimary->RoleContextDrainState->OnDrainCopy();

        TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
        TransactionalReplicatorSettingsUPtr tmp;
        TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

        TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
            move(tmp),
            make_shared<TransactionalReplicatorConfig>());

        SecondaryDrainManager::SPtr p1DrainManager = SecondaryDrainManager::Create(
            *oldPrimary->PartitionId,
            *oldPrimary->RecoveredOrCopiedCheckpointState,
            *oldPrimary->RoleContextDrainState,
            *oldPrimary->OperationProcessor,
            *oldPrimary->StateReplicator,
            *oldPrimary->BackupManager,
            *oldPrimary->CheckpointManager,
            *oldPrimary->TransactionManager,
            *oldPrimary->ReplicatedLogManager,
            *oldPrimary->StateManager,
            *oldPrimary->RecoveryManager,
            config,
            *invalidLogRecords_,
            allocator);

        oldPrimary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

        //11 is the default number of PSNs that stays in P1 after tail truncation
        oldPrimary->LastFlushedPsn = lastFlushedPsn;
        p1DrainManager->StartBuildingSecondary();
        SyncAwait(p1DrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

        p1DrainManager.Reset();
    }

    void IntegrationTests::CleanupTruncateTailFalseProgressTest(
        __in TestReplica::SPtr & newPrimary,
        __in TestReplica::SPtr & oldPrimary,
        __in CopyStream::SPtr & copyStream)
    {
        SyncAwait(newPrimary->CloseAndQuiesceReplica());
        newPrimary->EndTest(false, true);

        SyncAwait(oldPrimary->CloseAndQuiesceReplica());
        oldPrimary->EndTest(false, true);

        copyStream->Dispose();
    }

    void IntegrationTests::CreatePrimaryReplica(
        __in int seed,
        __in KAllocator & allocator,
        __out TestReplica::SPtr & newReplica)
    {
        CreateAndInitializeReplica(
            true,
            apiFaultUtility_,
            seed,
            allocator,
            newReplica);
    }

    void IntegrationTests::CreateSecondaryReplica(
        __in int seed,
        __in ApiFaultUtility::SPtr & faultUtility,
        __in KAllocator & allocator,
        __out TestReplica::SPtr & newReplica)
    {
        CreateAndInitializeReplica(
            false,
            faultUtility,
            seed,
            allocator,
            newReplica);
    }

    void IntegrationTests::CreateAndInitializeReplica(
        __in bool isPrimary,
        __in ApiFaultUtility::SPtr & faultUtility,
        __in int seed,
        __in KAllocator & allocator,
        __out TestReplica::SPtr & newReplica)
    {
        if(isPrimary)
        {
            newReplica = TestReplica::Create(
                pId_,
                *invalidLogRecords_,
                true,
                nullptr,
                *apiFaultUtility_,
                allocator);

            primaryTestReplica_ = newReplica;
            newReplica->Initialize(seed);
        }
        else
        {
            newReplica = TestReplica::Create(
                pId_,
                *invalidLogRecords_,
                true,
                nullptr,
                *faultUtility,
                allocator);

            secondaryTestReplica_ = newReplica;
            //updating the seed for the new replica
            newReplica->Initialize(seed);
        }
    }

    void IntegrationTests::AddOperationToTransaction(
        __in Transaction & transaction,
        __in Common::Random & rndNumber,
        __in TestStateProviderManager & stateManager)
    {
        AddOperationToTransaction(
            transaction,
            rndNumber.Next(0, 5),
            rndNumber.Next(0, 50),
            stateManager);
    }

    void IntegrationTests::AddOperationToTransaction(
        __in Transaction & transaction,
        __in ULONG bufferCount,
        __in ULONG bufferSize,
        __in TestStateProviderManager & stateManager)
    {
        TestStateProviderManager::SPtr stateManagerPtr = &stateManager;
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

        Trace.WriteInfo(
            TraceComponent,
            "{0}: Adding Operation Data with buffer count {1} bufferSize {2} to Tx {3}",
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

    void IntegrationTests::InitializeTest()
    {
        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
        invalidLogRecords_ = InvalidLogRecords::Create(allocator);
        apiFaultUtility_ = ApiFaultUtility::Create(allocator);
    }

    void IntegrationTests::EndTest()
    {
        prId_.Reset(); 
        invalidLogRecords_.Reset();
        apiFaultUtility_.Reset();

        if (primaryTestReplica_ != nullptr)
        {
            primaryTestReplica_.Reset();
        }

        if (secondaryTestReplica_ != nullptr)
        {
            secondaryTestReplica_.Reset();
        }
    }

#pragma endregion
    BOOST_GLOBAL_FIXTURE(LRTestGlobal);
    BOOST_FIXTURE_TEST_SUITE(IntegrationTestsSuite, IntegrationTests)

#pragma region TRANSACTION_TESTS

    BOOST_AUTO_TEST_CASE(Transaction_CommitCancellation)
    {
        TEST_TRACE_BEGIN("Transaction_CommitCancellation")

        {
            InitializeTest();
            
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            // 1 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::MaxValue, cts->Token);

                cts->Cancel();

                status = SyncAwait(task);

                VERIFY_IS_TRUE(status == STATUS_CANCELLED);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            // 2 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::MaxValue, cts->Token);

                cts->Cancel();

                status = SyncAwait(task);

                VERIFY_IS_TRUE(status == STATUS_CANCELLED);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            // 0 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::MaxValue, cts->Token);
                cts->Cancel();
                status = SyncAwait(task);

                VERIFY_IS_TRUE(status == STATUS_SUCCESS);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            SyncAwait(replica->CloseAndQuiesceReplica());
            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(Transaction_CommitTimeout)
    {
        TEST_TRACE_BEGIN("Transaction_CommitTimeout")

        {
            InitializeTest();
            
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
            
            // 1 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::FromMilliseconds(100), cts->Token);
                status = SyncAwait(task);
                VERIFY_IS_TRUE(status == SF_STATUS_TIMEOUT);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            // 2 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::FromMilliseconds(100), cts->Token);
                status = SyncAwait(task);

                VERIFY_IS_TRUE(status == SF_STATUS_TIMEOUT);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            // 0 operation transaction
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                CancellationTokenSource::SPtr cts = nullptr;
                CancellationTokenSource::Create(allocator, INTEGRATIONTEST_TAG, cts);

                replica->StateReplicator->SetBeginReplicateException(SF_STATUS_NO_WRITE_QUORUM);
                auto task = transaction->CommitAsync(TimeSpan::FromMilliseconds(100), cts->Token);
                status = SyncAwait(task);
                VERIFY_IS_TRUE(status == STATUS_SUCCESS);
                transaction->Dispose();
                replica->StateReplicator->SetBeginReplicateException(STATUS_SUCCESS);
            }

            SyncAwait(replica->CloseAndQuiesceReplica());
            replica->EndTest(false, true);
        }
    }

#pragma endregion

#pragma region RECOVERY_TESTS

    BOOST_AUTO_TEST_CASE(Recovery_1CommittedTransaction_VerifyState)
    {
        TEST_TRACE_BEGIN("Recovery_1CommittedTransaction_VerifyState")

        {
            InitializeTest();
            
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            SyncAwait(transaction->CommitAsync());
            transaction->Dispose();
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(1, 1, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());
            
            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            EndTest();
            replica->EndTest(true);
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            // Start of Recovery
            InitializeTest();
            replica->Initialize(seed, true);

            isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(1, 0, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(Recovery_1AbortedTransaction_VerifyState)
    {
        TEST_TRACE_BEGIN("Recovery_1AbortedTransaction_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            transaction->Abort();
            transaction->Dispose();
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above abort are completely flushed
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(0, 2, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            EndTest();
            replica->EndTest(true);
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            // Start of Recovery
            InitializeTest();
            replica->Initialize(seed, true);

            isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(0, 0, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(Recovery_RandomCommittedAndAbortedTransaction_VerifyState)
    {
        TEST_TRACE_BEGIN("Recovery_RandomCommittedAndAbortedTransaction_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            int txCount = r.Next(10, 60);
            int expectedApplyCount = 0;
            int expectedUnlockCount = 0;
            KArray<Transaction::SPtr> transactions(allocator);
            
            for (int i = 0; i < txCount; i++)
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                transactions.Append(transaction);
                int opCount = r.Next(2, 5); // operation count is at least 2 as single operation tx aborts skip the unlock

                for (int j = 0; j < opCount; j++)
                {
                    AddOperationToTransaction(*transaction, r, *replica->StateManager);
                }

                expectedUnlockCount += opCount;

                if (r.Next() % 2 == 0)
                {
                    expectedApplyCount += opCount;
                    auto commitAsync = transaction->CommitAsync();
                    ktl::ToTask(commitAsync);
                }
                else
                {
                    transaction->Abort();
                }
            }
            Sleep(1000); // wait for aborttx to get accepted
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(expectedApplyCount, expectedUnlockCount, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            for (int i = 0; i < txCount; i++)
            {
                transactions[i]->Dispose();
            }
            
            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            EndTest();
            replica->EndTest(true);
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            // Start of Recovery
            InitializeTest();
            replica->Initialize(seed, true);

            isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(expectedApplyCount, 0, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(Recovery_RandomCommittedAndAbortedTransactionThrowsDuringApplyOrUnlock_VerifyState)
    {
        TEST_TRACE_BEGIN("Recovery_RandomCommittedAndAbortedTransactionThrowsDuringApplyOrUnlock_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            int txCount = r.Next(10, 60);
            int expectedApplyCount = 0;
            int expectedUnlockCount = 0;
            KArray<Transaction::SPtr> transactions(allocator);
            
            for (int i = 0; i < txCount; i++)
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                transactions.Append(transaction);
                int opCount = r.Next(2, 5); // operation count is at least 2 as single operation tx aborts skip the unlock

                for (int j = 0; j < opCount; j++)
                {
                    AddOperationToTransaction(*transaction, r, *replica->StateManager);
                }

                expectedUnlockCount += opCount;

                if (r.Next() % 2 == 0)
                {
                    expectedApplyCount += opCount;
                    auto commitAsync = transaction->CommitAsync();
                    ktl::ToTask(commitAsync);
                }
                else
                {
                    transaction->Abort();
                }
            }
            Sleep(1000); // wait for aborttx to get accepted
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(expectedApplyCount, expectedUnlockCount, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());
            
            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            replica->EndTest(true);
            EndTest();
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            for (int i = 0; i < txCount; i++)
            {
                transactions[i]->Dispose();
            }

            // Start of Recovery
            InitializeTest();

            replica->ApiFaultUtility->FailApi(ApiName::ApplyAsync, STATUS_INSUFFICIENT_POWER);
            replica->ApiFaultUtility->FailApi(ApiName::Unlock, STATUS_INSUFFICIENT_POWER);

            try
            {
                replica->Initialize(seed, true);
            }
            catch (Exception e)
            {
                VERIFY_ARE_EQUAL(e.GetStatus(), STATUS_INSUFFICIENT_POWER);
            }
            catch (...)
            {
                CODING_ERROR_ASSERT(false);
            }

            // During recovery, any exception leads to open failing rather than reporting fault.
            // This check verifies the behavior
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, false);

            replica->EndTest(false, true);
        }
    }


    BOOST_AUTO_TEST_CASE(Recovery_CommittedTransactionsBeforePrepareCheckpointNotApplied_VerifyState)
    {
        TEST_TRACE_BEGIN("Recovery_CommittedTransactionsBeforePrepareCheckpointNotApplied_VerifyState")
        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            
            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            replica->EndTest(true);
            EndTest();
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            // Start of Recovery
            InitializeTest();
            replica->Initialize(seed, true);

            isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(3, 0, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(Recovery_CompleteCheckpointMissing_Verify)
    {
        TEST_TRACE_BEGIN("Recovery_CompleteCheckpointMissing_Verify")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
            }

            replica->ApiFaultUtility->FailApi(ApiName::CompleteCheckpoint, STATUS_INSUFFICIENT_POWER);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();
            SyncAwait(replica->CloseAndQuiesceReplica());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(2, 2, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, true); // Report fault must have been invoked

            // Verify that complete checkpoint was not logged
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastLinkedPhysicalRecord->RecordType, LogRecordType::Enum::EndCheckpoint);

            PartitionedReplicaId::SPtr prId = replica->PartitionId;
            replica->EndTest(true);
            EndTest();
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            prId_ = prId;

            // Start of Recovery
            InitializeTest();
            replica->Initialize(seed, true);

            // Verify that complete checkpoint was logged
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastLinkedPhysicalRecord->RecordType, LogRecordType::Enum::CompleteCheckpoint);

            replica->EndTest(false, true);
        }
    }

#pragma endregion

#pragma region CHECKPOINT_TESTS
    BOOST_AUTO_TEST_CASE(Checkpoint_VerifyState)
    {
        TEST_TRACE_BEGIN("Checkpoint_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            
            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpoint async
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(PrepareCheckpointException_VerifyState)
    {
        TEST_TRACE_BEGIN("PrepareCheckpointException_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();
            
            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);
            
            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            
            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            Awaitable<NTSTATUS> commitTask = transaction2->CommitAsync(); // Do this before throwing exception to ensure commit is accepted
            // this should ensure the commit will be flushed and written to disk and processed later
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            replica->ApiFaultUtility->FailApi(ApiName::PrepareCheckpoint, STATUS_INSUFFICIENT_POWER);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            SyncAwait(commitTask);
            transaction2->Dispose();

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Faulted);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction, nullptr); // This is the first checkpoint during recovery
            VERIFY_ARE_EQUAL(endCheckpoint->Lsn, Constants::OneLsn); // This is the first checkpoint during recovery
            
            SyncAwait(replica->CloseAndQuiesceReplica());
            
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, true); // Report fault must have been invoked

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(PerformCheckpointException_VerifyState)
    {
        TEST_TRACE_BEGIN("PerformCheckpointException_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);
 
            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            Awaitable<NTSTATUS> commitTask = transaction2->CommitAsync(); // Do this before throwing exception to ensure commit is accepted
            // this should ensure the commit will be flushed and written to disk and processed later
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            replica->ApiFaultUtility->FailApi(ApiName::PerformCheckpoint, STATUS_INSUFFICIENT_POWER);
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            SyncAwait(commitTask);
            transaction2->Dispose();

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Faulted);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction, nullptr); // This is the first checkpoint during recovery
            VERIFY_ARE_EQUAL(endCheckpoint->Lsn, Constants::OneLsn); // This is the first checkpoint during recovery

            SyncAwait(replica->CloseAndQuiesceReplica());
            
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, true); // Report fault must have been invoked

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(CompleteCheckpointException_VerifyState)
    {
        TEST_TRACE_BEGIN("CompleteCheckpointException_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            Awaitable<NTSTATUS> commitTask = transaction2->CommitAsync(); // Do this before throwing exception to ensure commit is accepted
            // this should ensure the commit will be flushed and written to disk and processed later
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            replica->ApiFaultUtility->FailApi(ApiName::CompleteCheckpoint, STATUS_INSUFFICIENT_POWER);
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            SyncAwait(commitTask);
            transaction2->Dispose();

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed); // Checkpoint is considered completed even if complete failed as we re-log on recovery
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            SyncAwait(replica->CloseAndQuiesceReplica());
            
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, true); // Report fault must have been invoked

            // Verify that complete checkpoint was not logged
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastLinkedPhysicalRecord->RecordType, LogRecordType::Enum::EndCheckpoint);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(AbortPendingCheckpoint_VerifyState)
    {
        TEST_TRACE_BEGIN("AbortPendingCheckpoint_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();
            
            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);
            
            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            
            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            Awaitable<NTSTATUS> commitTask = transaction2->CommitAsync(); // Do this before throwing exception to ensure commit is accepted
            // this should ensure the commit will be flushed and written to disk and processed later
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            replica->CheckpointManager->AbortPendingCheckpoint();

            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            SyncAwait(commitTask);
            transaction2->Dispose();

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Aborted);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction, nullptr); // This is the first checkpoint during recovery
            VERIFY_ARE_EQUAL(endCheckpoint->Lsn, Constants::OneLsn); // This is the first checkpoint during recovery
            
            SyncAwait(replica->CloseAndQuiesceReplica());
            
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            VERIFY_ARE_EQUAL(replica->RoleContextDrainState->IsClosing, true); // Report fault must have been invoked

            replica->EndTest(false, true);
        }
    }

#pragma endregion

#pragma region TRUNCATION_TESTS
    BOOST_AUTO_TEST_CASE(TruncateHead_VerifyState)
    {
        TEST_TRACE_BEGIN("TruncateHead_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = replica->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);
            
            // Now that the checkpoint is done, initiate a truncate head operation
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            replica->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            replica->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            SyncAwait(replica->CloseAndQuiesceReplica());

            VERIFY_IS_TRUE(currentLogHead->Psn < replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn);

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ActiveLogReaders_VerifyTruncateHeadBlocked)
    {
        TEST_TRACE_BEGIN("ActiveLogReaders_VerifyTruncateHeadBlocked")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = replica->ReplicatedLogManager->CurrentLogHeadRecord;
            KString::SPtr reader1Name = KString::Create(L"reader1to20", allocator, true);
            KString::SPtr reader2Name = KString::Create(L"reader1to21", allocator, true);
            KString::SPtr reader3Name = KString::Create(L"reader2to15", allocator, true);

            IPhysicalLogReader::SPtr reader1to20 = PhysicalLogReader::Create(
                *replica->LogManager,
                1,
                20,
                1,
                *reader1Name,
                LogReaderType::Enum::Backup,
                allocator);

            IPhysicalLogReader::SPtr reader1to21 = PhysicalLogReader::Create(
                *replica->LogManager,
                1,
                21,
                1,
                *reader1Name,
                LogReaderType::Enum::Backup,
                allocator);

            IPhysicalLogReader::SPtr reader2to15 = PhysicalLogReader::Create(
                *replica->LogManager,
                2,
                15,
                1,
                *reader1Name,
                LogReaderType::Enum::Backup,
                allocator);

            VERIFY_ARE_EQUAL(reader1to20->IsValid, true);
            VERIFY_ARE_EQUAL(reader1to21->IsValid, true);
            VERIFY_ARE_EQUAL(reader2to15->IsValid, true);

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);
            
            // Now that the checkpoint is done, initiate a truncate head operation
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            replica->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            replica->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            TruncateHeadLogRecord::SPtr pendingTruncateHead = replica->ReplicatedLogManager->LastInProgressTruncateHeadRecord;
            VERIFY_IS_TRUE(pendingTruncateHead != nullptr);
            VERIFY_IS_TRUE(pendingTruncateHead->TruncationState == TruncationState::Enum::Applied);
            VERIFY_IS_TRUE(pendingTruncateHead->LogHeadLsn > currentLogHead->Lsn);
            VERIFY_IS_TRUE(pendingTruncateHead->HeadRecord->Lsn > currentLogHead->Lsn);
            VERIFY_IS_TRUE(currentLogHead->Psn < replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn);
            
            // Dispose in random order
            int order = r.Next() % 3;
            if (order == 0)
            {
                reader1to20->Dispose();
                reader1to21->Dispose();
                reader2to15->Dispose();
            }
            else if (order == 1)
            {
                reader1to20->Dispose();
                reader2to15->Dispose();
                reader1to21->Dispose();
            }
            else if (order == 2)
            {
                reader2to15->Dispose();
                reader1to21->Dispose();
                reader1to20->Dispose();
            }

            SyncAwait(pendingTruncateHead->AwaitProcessing());
            VERIFY_IS_TRUE(pendingTruncateHead->TruncationState == TruncationState::Enum::Completed);
            pendingTruncateHead = replica->ReplicatedLogManager->LastInProgressTruncateHeadRecord;
            VERIFY_ARE_EQUAL(pendingTruncateHead, nullptr);

            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(AbortPendingTruncateHead_VerifyState)
    {
        TEST_TRACE_BEGIN("AbortPendingTruncateHead_VerifyState")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = replica->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *replica->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *replica->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(replica->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = replica->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);
            
            // Now that the checkpoint is done, initiate a truncate head operation
            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            replica->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            replica->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // this is needed to ensure truncate head's barrier is not stable
            replica->ApiFaultUtility->BlockApi(ApiName::ReplicateAsync);

            // This should trigger the truncate head record
            Awaitable<NTSTATUS> commitTask = transaction2->CommitAsync();

            TruncateHeadLogRecord::SPtr pendingTruncateHead = replica->ReplicatedLogManager->LastInProgressTruncateHeadRecord;
            VERIFY_IS_TRUE(pendingTruncateHead != nullptr);
            replica->CheckpointManager->AbortPendingLogHeadTruncation();
            VERIFY_IS_TRUE(pendingTruncateHead->TruncationState == TruncationState::Enum::Aborted);

            replica->ApiFaultUtility->ClearFault(ApiName::ReplicateAsync);
            SyncAwait(commitTask);
            transaction2->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            // replicated log manager state is still updated
            VERIFY_IS_TRUE(pendingTruncateHead->LogHeadLsn > currentLogHead->Lsn);
            VERIFY_IS_TRUE(pendingTruncateHead->HeadRecord->Lsn > currentLogHead->Lsn);
            VERIFY_IS_TRUE(currentLogHead->Psn < replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn);
            
            SyncAwait(pendingTruncateHead->AwaitProcessing());
            pendingTruncateHead = replica->ReplicatedLogManager->LastInProgressTruncateHeadRecord;
            VERIFY_ARE_EQUAL(pendingTruncateHead, nullptr);

            // Ensure creating a reader is valid to ensure log is not truncated
            KString::SPtr reader1Name = KString::Create(L"reader1to20", allocator, true);
            IPhysicalLogReader::SPtr reader1to20 = PhysicalLogReader::Create(
                *replica->LogManager,
                1,
                20,
                1,
                *reader1Name,
                LogReaderType::Enum::Backup,
                allocator);

            VERIFY_ARE_EQUAL(reader1to20->IsValid, true);

            reader1to20->Dispose();

            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

#pragma endregion

#pragma region TRANSACTION_TESTS

    BOOST_AUTO_TEST_CASE(FailedEndReplicateMustFailCommit)
    {
        TEST_TRACE_BEGIN("FailedEndReplicateMustFailCommit")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);

            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            auto task = transaction->CommitAsync();
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed

            apiFaultUtility_->FailApi(ApiName::ReplicateAsync, STATUS_CANCELLED);

            status = SyncAwait(task);
            VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

            SyncAwait(barrier->AwaitProcessing());
            transaction->Dispose();

            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(1, 1, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(OperationCommittedTx_VerifyApplyUnlockCount)
    {
        TEST_TRACE_BEGIN("OperationCommittedTx_VerifyApplyUnlockCount")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            SyncAwait(transaction->CommitAsync());
            transaction->Dispose();
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(1, 1, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(OperationAbortedTx_VerifyApplyUnlockCount)
    {
        TEST_TRACE_BEGIN("OperationAbortedTx_VerifyApplyUnlockCount")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            transaction->Abort();
            transaction->Dispose();
            Sleep(1000); // wait for aborttx to get accepted
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(0, 0, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(RandomOperationCountCommittedTx_VerifyApplyUnlockCount)
    {
        TEST_TRACE_BEGIN("RandomOperationCountCommittedTx_VerifyApplyUnlockCount")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = r.Next(2, 20);

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction, r, *replica->StateManager);
            }

            SyncAwait(transaction->CommitAsync());
            transaction->Dispose();
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(opCount, opCount, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(RandomOperationCountAbortedTx_VerifyApplyUnlockCount)
    {
        TEST_TRACE_BEGIN("RandomOperationCountAbortedTx_VerifyApplyUnlockCount")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            int opCount = r.Next(2, 10); // operation count is at least 2 as single operation tx aborts skip the unlock

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction, r, *replica->StateManager);
            }

            transaction->Abort();
            transaction->Dispose();
            Sleep(1000); // wait for aborttx to get accepted
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(0, opCount, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }

    BOOST_AUTO_TEST_CASE(RandomOperationAndTxCount_VerifyApplyUnlockCount)
    {
        TEST_TRACE_BEGIN("RandomOperationAndTxCount_VerifyApplyUnlockCount")

        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            int txCount = r.Next(10, 60);
            int expectedApplyCount = 0;
            int expectedUnlockCount = 0;
            KArray<Transaction::SPtr> transactions(allocator);
            
            for (int i = 0; i < txCount; i++)
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                transactions.Append(transaction);
                int opCount = r.Next(2, 5); // operation count is at least 2 as single operation tx aborts skip the unlock

                for (int j = 0; j < opCount; j++)
                {
                    AddOperationToTransaction(*transaction, r, *replica->StateManager);
                }

                expectedUnlockCount += opCount;

                if (r.Next() % 2 == 0)
                {
                    expectedApplyCount += opCount;
                    auto commitAsync = transaction->CommitAsync();
                    ktl::ToTask(commitAsync);
                }
                else
                {
                    transaction->Abort();
                }
            }
            Sleep(1000); // wait for aborttx to get accepted
            BarrierLogRecord::SPtr barrier;
            replica->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitProcessing());
            bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(expectedApplyCount, expectedUnlockCount, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);
            SyncAwait(replica->CloseAndQuiesceReplica());

            for (int i = 0; i < txCount; i++)
            {
                transactions[i]->Dispose();
            }

            replica->EndTest(false, true);
        }
    }
#pragma endregion

#pragma region PRIMARY_COPYSTREAM_TESTS
    BOOST_AUTO_TEST_CASE(CopyStream_ModeCopyLog_VerifyState)
    {
        TEST_TRACE_BEGIN("CopyStream_ModeCopyLog_VerifyState")
        {
            InitializeTest();

            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
            AddOperationToTransaction(*transaction, r, *replica->StateManager);
            SyncAwait(transaction->CommitAsync());
            transaction->Dispose();
            replica->StateManager->WaitForProcessingToComplete(1, 1, Common::TimeSpan::FromSeconds(1));

            LONG64 expectedReplicaId = 1234;
            LONG64 expectedLogTailLsn = Constants::ZeroLsn;
            LONG64 expectedLatestRecoveredAtomicRedoOperationLsn = Constants::ZeroLsn;

            LONG64 expectedEpochDataLossNumber = 0;
            LONG64 expectedEpochConfigurationNumber = 0;
            LONG64 expectedStartingLogicalSequenceNumber = Constants::ZeroLsn;
            DateTime expectedTimeStamp;

            LONG64 expectedLogHeadRecordLsn = Constants::ZeroLsn;

            CopyContext::SPtr copyContext = replica->CreateACopyContext(
                expectedReplicaId,
                expectedLogTailLsn,
                expectedLatestRecoveredAtomicRedoOperationLsn,
                expectedEpochDataLossNumber,
                expectedEpochConfigurationNumber,
                expectedStartingLogicalSequenceNumber,
                expectedLogHeadRecordLsn,
                expectedTimeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            std::shared_ptr<TransactionalReplicatorConfig> globalConfig = make_shared<TransactionalReplicatorConfig>();
            globalConfig->CopyBatchSizeInKb = 0;

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                globalConfig);

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            LONG64 expectedSourceReplicaId = 5678;
            CopyStream::SPtr copyStream = CopyStream::Create(
                *replica->PartitionId, 
                *replica->ReplicatedLogManager, 
                *replica->StateManager, 
                *replica->CheckpointManager, 
                callback, 
                expectedSourceReplicaId, 
                5, 
                *copyContext, 
                *copyStageBuffers,
                config,
                true,
                allocator);

            OperationData::CSPtr op1;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op1));
            
            // first record must be a copy metadata containing the CopyLog stage and source replica id
            VERIFY_ARE_NOT_EQUAL(op1, nullptr);
            KBuffer::CSPtr buffer = (*op1)[0];
            BinaryReader br(*buffer, allocator);
            ULONG32 metadataVersion;
            ULONG32 copyStage;
            LONG64 sourceReplicaId;
            br.Read(metadataVersion);
            br.Read(copyStage);
            br.Read(sourceReplicaId);
            VERIFY_ARE_EQUAL(metadataVersion, 1);
            VERIFY_ARE_EQUAL(copyStage, static_cast<int>(CopyStage::Enum::CopyLog));
            VERIFY_ARE_EQUAL(sourceReplicaId, expectedSourceReplicaId);

            OperationData::CSPtr op2;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op2));
            
            // second record must be an updateEpochLogRecord
            VERIFY_ARE_NOT_EQUAL(op2, nullptr);
            LogRecord::SPtr record2 = LogRecord::ReadFromOperationData(*op2, *InvalidLogRecords::Create(allocator), allocator);

            VERIFY_ARE_EQUAL(record2->RecordType, LogRecordType::Enum::UpdateEpoch);
            
            OperationData::CSPtr op3;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op3));
            
            // third record must be a barrier
            VERIFY_ARE_NOT_EQUAL(op3, nullptr);
            LogRecord::SPtr record3 = LogRecord::ReadFromOperationData(*op3, *InvalidLogRecords::Create(allocator), allocator);
            VERIFY_ARE_EQUAL(record3->RecordType, LogRecordType::Enum::Barrier);

            OperationData::CSPtr op4;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op4));
            
            // fourth record must be a beginTransaction
            VERIFY_ARE_NOT_EQUAL(op4, nullptr);
            LogRecord::SPtr record4 = LogRecord::ReadFromOperationData(*op4, *InvalidLogRecords::Create(allocator), allocator);
            VERIFY_ARE_EQUAL(record4->RecordType, LogRecordType::Enum::BeginTransaction);

            OperationData::CSPtr op5;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op5));
            
            // fifth record must be a barrier
            VERIFY_ARE_NOT_EQUAL(op5, nullptr);
            LogRecord::SPtr record5 = LogRecord::ReadFromOperationData(*op5, *InvalidLogRecords::Create(allocator), allocator);
            VERIFY_ARE_EQUAL(record5->RecordType, LogRecordType::Enum::Barrier);
            
            OperationData::CSPtr op6;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op6));
            
            // sixth record must be a barrier
            VERIFY_ARE_NOT_EQUAL(op6, nullptr);
            LogRecord::SPtr record6 = LogRecord::ReadFromOperationData(*op6, *InvalidLogRecords::Create(allocator), allocator);
            VERIFY_ARE_EQUAL(record6->RecordType, LogRecordType::Enum::Barrier);

            OperationData::CSPtr op7;
            SyncAwait(copyStream->GetNextAsync(CancellationToken::None, op7));
            
            // seventh record must be a nullptr
            VERIFY_ARE_EQUAL(op7, nullptr);

            copyStream->Dispose();
            SyncAwait(replica->CloseAndQuiesceReplica());

            replica->EndTest(false, true);
        }
    }
#pragma endregion

#pragma region SECONDARY_INTEGRATION_TESTS

#pragma region DRAIN_MANAGER_TESTS
    BOOST_AUTO_TEST_CASE(DrainManager_CopyLog)
    {
        TEST_TRACE_BEGIN("DrainManager_CopyLog")
        {
            // Test Scenario:
            // 1 primary - 1 secondary
            // primary logs 1 transaction with 10 operations with no checkpoint and no log truncation
            // secondary comes up with empty data
            // primary tries to build secondary in PARTIAL_COPY mode

            InitializeTest();

            TestReplica::SPtr primary = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = primary;
            primary->Initialize(seed);

            TestReplica::SPtr secondary = TestReplica::Create(pId_, *invalidLogRecords_, false, primary->StateManager.RawPtr(), *apiFaultUtility_, allocator);
            secondaryTestReplica_ = secondary;
            //updating the seed for the new replica
            seed = r.Next();
            secondary->Initialize(seed);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*transaction, r, *primary->StateManager);
            }
            SyncAwait(transaction->CommitAsync());
            transaction->Dispose();

            DateTime expectedTimeStamp;
            CopyContext::SPtr copyContext = secondary->CreateACopyContext(
                secondary->PartitionId->ReplicaId,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                0,
                0,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                expectedTimeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            LONG64 expectedSourceReplicaId = primary->PartitionId->ReplicaId;
            CopyStream::SPtr copyStream = CopyStream::Create(
                *primary->PartitionId,
                *primary->ReplicatedLogManager,
                *primary->StateManager,
                *primary->CheckpointManager,
                callback,
                expectedSourceReplicaId,
                50,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            primary->StateManager->Reuse();

            RecoveryManager::SPtr secondaryRecoveryManager = secondary->RecoveryManager;

            SecondaryDrainManager::SPtr secondaryDrainManager = SecondaryDrainManager::Create(
                *secondary->PartitionId,
                *secondary->RecoveredOrCopiedCheckpointState,
                *secondary->RoleContextDrainState,
                *secondary->OperationProcessor,
                *secondary->StateReplicator,
                *secondary->BackupManager,
                *secondary->CheckpointManager,
                *secondary->TransactionManager,
                *secondary->ReplicatedLogManager,
                *primary->StateManager,
                *secondaryRecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            secondary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

            secondaryDrainManager->StartBuildingSecondary();
            SyncAwait(secondaryDrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            secondaryRecoveryManager.Reset();
            secondaryDrainManager.Reset();
            SyncAwait(secondary->CloseAndQuiesceReplica());
            secondary->EndTest(false, true);

            SyncAwait(primary->CloseAndQuiesceReplica());
            primary->EndTest(false, true);
            
            copyStream->Dispose();
        }
    }

    BOOST_AUTO_TEST_CASE(DrainManager_FullCopyAbortVerifyNoEndSettingCurrentStateCalled)
    {

        TEST_TRACE_BEGIN("DrainManager_FullCopyAbortVerifyNoEndSettingCurrentStateCalled")
        {
            // Test Scenario:
            // This test is a full copy test with a change that
            // secondary receives null operation after 2 GetNext calls on copy stream (2 successfull operations are allowed to ensure secondary goes into the state draining logic and BeginSettingCurrentState is called)
            // which causes the replica to finish the copy process (aborting)
            // without any call to state provider's EndSettingCurrentState

            InitializeTest();

            TestReplica::SPtr primary = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = primary;

            primary->Initialize(seed);
            primary->StateManager->StateStreamSuccessfulOperationCount = 2;

            ApiFaultUtility::SPtr secondaryApiFaultUtility = ApiFaultUtility::Create(allocator);
            TestReplica::SPtr secondary = TestReplica::Create(pId_, *invalidLogRecords_, false, primary->StateManager.RawPtr(), *secondaryApiFaultUtility, allocator);
            secondaryTestReplica_ = secondary;
            //updating the seed for the new replica
            seed = r.Next();
            secondary->Initialize(seed);
            secondary->StateReplicator->CopyStreamMaximumSuccessfulOperationCount = 2;

            IndexingLogRecord::SPtr currentLogHead = primary->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *primary->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *primary->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            primary->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            primary->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *primary->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            primary->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = primary->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            primary->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primary->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = primary->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // Now that the checkpoint is done, initiate a truncate head operation
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            primary->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            primary->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = primary->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            DateTime timeStamp;
            CopyContext::SPtr copyContext = secondary->CreateACopyContext(
                secondary->PartitionId->ReplicaId,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                0,
                0,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                timeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            CopyStream::SPtr copyStream = CopyStream::Create(
                *primary->PartitionId,
                *primary->ReplicatedLogManager,
                *primary->StateManager,
                *primary->CheckpointManager,
                callback,
                primary->PartitionId->ReplicaId,
                50,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            // reuse the primary state manager so the validation will be run again for all the records being received in the secondary side
            primary->StateManager->Reuse();

            RecoveryManager::SPtr secondaryRecoveryManager = secondary->RecoveryManager;

            SecondaryDrainManager::SPtr secondaryDrainManager = SecondaryDrainManager::Create(
                *secondary->PartitionId,
                *secondary->RecoveredOrCopiedCheckpointState,
                *secondary->RoleContextDrainState,
                *secondary->OperationProcessor,
                *secondary->StateReplicator,
                *secondary->BackupManager,
                *secondary->CheckpointManager,
                *secondary->TransactionManager,
                *secondary->ReplicatedLogManager,
                *primary->StateManager,
                *secondaryRecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            secondary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

            // Resetting the LastFlushedPsn on secondary replica to zero, as the full_copy mode starts with a fresh log file ans zero psn
            secondary->LastFlushedPsn = 0;
            secondaryDrainManager->StartBuildingSecondary();
            SyncAwait(secondaryDrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            VERIFY_ARE_EQUAL(primary->StateManager->BeginSettingCurrentStateApiCount, 1);
            VERIFY_ARE_EQUAL(primary->StateManager->EndSettingCurrentStateApiCount, 0);

            secondaryRecoveryManager.Reset();
            secondaryDrainManager.Reset();
            SyncAwait(secondary->CloseAndQuiesceReplica());
            secondary->EndTest(false, true);

            copyStream->Dispose();

            SyncAwait(primary->CloseAndQuiesceReplica());

            primary->EndTest(false, true);
            secondaryApiFaultUtility.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(DrainManager_CopyFalseProgress)
    {
        TEST_TRACE_BEGIN("DrainManager_CopyFalseProgress")
        {
            // Test Scenario:
            // 2 primaries
            // primary #1 logs 1 transactions with 10 operations while its ReplicateAsync api is blocked
            // primary #1 goes down and comes up as an idle secondary. Does the recovery from its log and ends up in a false progress mode
            // primary #2 starts and logs 1 transcation with 2 operations
            // primary #2 starts building primary #1 in a false progress mode

            InitializeTest();

            TestReplica::SPtr p1 = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = p1;
            p1->Initialize(seed);

            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            Transaction::SPtr transaction = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*transaction, r, *p1->StateManager);
            }
            Awaitable<NTSTATUS> commitTask = transaction->CommitAsync();

            apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(3));
            SyncAwait(p1->CloseAndQuiesceReplica());
            SyncAwait(commitTask);
            transaction->Dispose();

            // reopen p1 for recovery
            PartitionedReplicaId::SPtr prId = p1->PartitionId;
            p1->EndTest(true);
            Sleep(1000); // This is here to let any background log close for the reader stream finish as it is asynchronous
            p1->Initialize(seed, true);
            p1->PartitionId = prId;

            //create a copy context from p1
            DateTime timeStamp;
            CopyContext::SPtr copyContext = p1->CreateACopyContext(
                p1->PartitionId->ReplicaId,
                p1->ReplicatedLogManager->CurrentLogTailLsn,
                Constants::ZeroLsn,
                p1->ReplicatedLogManager->CurrentLogTailEpoch.DataLossVersion,
                p1->ReplicatedLogManager->CurrentLogTailEpoch.ConfigurationVersion,
                Constants::ZeroLsn,
                p1->ReplicatedLogManager->CurrentLogHeadRecord->Lsn,
                timeStamp);

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2 = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *fUtility, allocator);
            secondaryTestReplica_ = p2;
            //updating the seed for the new replica
            seed = r.Next();
            p2->Initialize(seed);

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*transaction2, r, *p2->StateManager);
            }
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());
            
            //creating a copy stream from p2
            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);
            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            CopyStream::SPtr p2CopyStream = CopyStream::Create(
                *p2->PartitionId,
                *p2->ReplicatedLogManager,
                *p2->StateManager,
                *p2->CheckpointManager,
                callback,
                p1->PartitionId->ReplicaId,
                50,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            p2->StateManager->Reuse();

            p1->RoleContextDrainState->Reuse();
            p1->RoleContextDrainState->ChangeRole(FABRIC_REPLICA_ROLE_IDLE_SECONDARY);
            p1->RoleContextDrainState->OnDrainCopy();

            SecondaryDrainManager::SPtr p1DrainManager = SecondaryDrainManager::Create(
                *p1->PartitionId,
                *p1->RecoveredOrCopiedCheckpointState,
                *p1->RoleContextDrainState,
                *p1->OperationProcessor,
                *p1->StateReplicator,
                *p1->BackupManager,
                *p1->CheckpointManager,
                *p1->TransactionManager,
                *p1->ReplicatedLogManager,
                *p2->StateManager,
                *p1->RecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            p1->StateReplicator->InitializePrimaryCopyStream(*p2CopyStream);

            //11 is the number of PSNs that stays in P1 after tail truncation
            p1->LastFlushedPsn = 11;
            p1DrainManager->StartBuildingSecondary();
            SyncAwait(p1DrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            p1DrainManager.Reset();
            SyncAwait(p2->CloseAndQuiesceReplica());
            p2->EndTest(false, true);

            SyncAwait(p1->CloseAndQuiesceReplica());
            p1->EndTest(false, true);

            p2CopyStream->Dispose();
        }
    }

    BOOST_AUTO_TEST_CASE(DrainManager_CopyFull)
    {
        TEST_TRACE_BEGIN("DrainManager_CopyFull")
        {
            // Test Scenario:
            // 1 primary - 1 secondary
            // primary logs 2 transactions each with 2 operations
            // primary prepares and completes a checkpoint and truncate its head
            // secondary comes up with empty data
            // primary tries to build secondary in FULL_COPY mode

            InitializeTest();

            TestReplica::SPtr primary = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = primary;

            primary->Initialize(seed);

            ApiFaultUtility::SPtr secondaryApiFaultUtility = ApiFaultUtility::Create(allocator);
            TestReplica::SPtr secondary = TestReplica::Create(pId_, *invalidLogRecords_, false, primary->StateManager.RawPtr(), *secondaryApiFaultUtility, allocator);
            secondaryTestReplica_ = secondary;
            //updating the seed for the new replica
            seed = r.Next();
            secondary->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = primary->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            int opCount = 2;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *primary->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *primary->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            primary->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            primary->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *primary->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            primary->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = primary->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            primary->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primary->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = primary->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // Now that the checkpoint is done, initiate a truncate head operation
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            primary->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            primary->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = primary->StateManager->WaitForProcessingToComplete(5, 5, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            DateTime timeStamp;
            CopyContext::SPtr copyContext = secondary->CreateACopyContext(
                secondary->PartitionId->ReplicaId,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                0,
                0,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                timeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            CopyStream::SPtr copyStream = CopyStream::Create(
                *primary->PartitionId,
                *primary->ReplicatedLogManager,
                *primary->StateManager,
                *primary->CheckpointManager,
                callback,
                primary->PartitionId->ReplicaId,
                50,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            // reuse the primary state manager so the validation will be run again for all the records being received in the secondary side
            primary->StateManager->Reuse();

            RecoveryManager::SPtr secondaryRecoveryManager = secondary->RecoveryManager;

            SecondaryDrainManager::SPtr secondaryDrainManager = SecondaryDrainManager::Create(
                *secondary->PartitionId,
                *secondary->RecoveredOrCopiedCheckpointState,
                *secondary->RoleContextDrainState,
                *secondary->OperationProcessor,
                *secondary->StateReplicator,
                *secondary->BackupManager,
                *secondary->CheckpointManager,
                *secondary->TransactionManager,
                *secondary->ReplicatedLogManager,
                *primary->StateManager,
                *secondaryRecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            secondary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

            // Resetting the LastFlushedPsn on secondary replica to zero, as the full_copy mode starts with a fresh log file ans zero psn
            secondary->LastFlushedPsn = 0;
            secondaryDrainManager->StartBuildingSecondary();
            SyncAwait(secondaryDrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            secondaryRecoveryManager.Reset();
            secondaryDrainManager.Reset();
            SyncAwait(secondary->CloseAndQuiesceReplica());
            secondary->EndTest(false, true);

            copyStream->Dispose();

            SyncAwait(primary->CloseAndQuiesceReplica());

            primary->EndTest(false, true);
            secondaryApiFaultUtility.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(DrainManager_CopyFull_Batch)
    {
        TEST_TRACE_BEGIN("DrainManager_CopyFull_Batch")
        {
            // Test Scenario:
            // 1 primary - 1 secondary
            // Similar to Full Copy scenario with the change that Primary logs more operations and batch size config is set to 1, 10 or 100Kb randomly

            InitializeTest();

            TestReplica::SPtr primary = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = primary;

            primary->Initialize(seed);

            ApiFaultUtility::SPtr secondaryApiFaultUtility = ApiFaultUtility::Create(allocator);
            TestReplica::SPtr secondary = TestReplica::Create(pId_, *invalidLogRecords_, false, primary->StateManager.RawPtr(), *secondaryApiFaultUtility, allocator);
            secondaryTestReplica_ = secondary;
            //updating the seed for the new replica
            seed = r.Next();
            secondary->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = primary->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            int transactionCount = 1;
            int opCount = 200;

            for (int t = 0; t < transactionCount; t++)
            {
                Transaction::SPtr transaction1 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);


                for (int i = 0; i < opCount; i++)
                {
                    AddOperationToTransaction(*transaction1, r, *primary->StateManager);
                }

                // commit tx1
                SyncAwait(transaction1->CommitAsync());
                transaction1->Dispose();
            }

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *primary->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            primary->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            primary->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *primary->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            primary->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = primary->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            primary->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primary->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = primary->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // Now that the checkpoint is done, initiate a truncate head operation
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            primary->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            primary->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = primary->StateManager->WaitForProcessingToComplete(401, 401, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            DateTime timeStamp;
            CopyContext::SPtr copyContext = secondary->CreateACopyContext(
                secondary->PartitionId->ReplicaId,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                0,
                0,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                timeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            std::shared_ptr<TransactionalReplicatorConfig> globalConfig = make_shared<TransactionalReplicatorConfig>();
            globalConfig->CopyBatchSizeInKb = (uint) pow(10, r.Next(0, 3));

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                globalConfig);

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            CopyStream::SPtr copyStream = CopyStream::Create(
                *primary->PartitionId,
                *primary->ReplicatedLogManager,
                *primary->StateManager,
                *primary->CheckpointManager,
                callback,
                primary->PartitionId->ReplicaId,
                1000,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            // reuse the primary state manager so the validation will be run again for all the records being received in the secondary side
            primary->StateManager->Reuse();

            RecoveryManager::SPtr secondaryRecoveryManager = secondary->RecoveryManager;

            SecondaryDrainManager::SPtr secondaryDrainManager = SecondaryDrainManager::Create(
                *secondary->PartitionId,
                *secondary->RecoveredOrCopiedCheckpointState,
                *secondary->RoleContextDrainState,
                *secondary->OperationProcessor,
                *secondary->StateReplicator,
                *secondary->BackupManager,
                *secondary->CheckpointManager,
                *secondary->TransactionManager,
                *secondary->ReplicatedLogManager,
                *primary->StateManager,
                *secondaryRecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            secondary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

            // Resetting the LastFlushedPsn on secondary replica to zero, as the full_copy mode starts with a fresh log file ans zero psn
            secondary->LastFlushedPsn = 0;
            secondaryDrainManager->StartBuildingSecondary();
            SyncAwait(secondaryDrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            secondaryRecoveryManager.Reset();
            secondaryDrainManager.Reset();
            SyncAwait(secondary->CloseAndQuiesceReplica());
            secondary->EndTest(false, true);

            copyStream->Dispose();

            SyncAwait(primary->CloseAndQuiesceReplica());

            primary->EndTest(false, true);
            secondaryApiFaultUtility.Reset();
        }
    }

    BOOST_AUTO_TEST_CASE(FullCopy_UpToLsnLessThanCheckPointLsn_VerifyLogRename)
    {
        TEST_TRACE_BEGIN("FullCopy_UpToLsnLessThanCheckPointLsn_VerifyLogRename")
        {
            // Test Scenario:
            // 1 primary, 1 secondary
            // primary logs transactions and checkpoint data
            // secondary comes up fresh
            // full build is initiated by the primary but with uptolsn value of 10 which is less than the checkpoint LSN that the primary has
            // primary should detect this and bump the uptolsn value to be equal to checkpoint LSN
            // when secondary is fully build, it should rename its copy log successfully and persist the new data permananetly
            // verification is done by closing the secondary and recovering from the log again. log tails after full copy and after recovery must be equal.

            InitializeTest();

            TestReplica::SPtr primary = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = primary;

            primary->Initialize(seed);

            ApiFaultUtility::SPtr secondaryApiFaultUtility = ApiFaultUtility::Create(allocator);
            TestReplica::SPtr secondary = TestReplica::Create(pId_, *invalidLogRecords_, false, primary->StateManager.RawPtr(), *secondaryApiFaultUtility, allocator);
            secondaryTestReplica_ = secondary;
            //updating the seed for the new replica
            seed = r.Next();
            secondary->Initialize(seed);

            IndexingLogRecord::SPtr currentLogHead = primary->ReplicatedLogManager->CurrentLogHeadRecord;

            // Insert indexing records frequently to ensure we have sufficient options for head record after a committed transaction
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            Transaction::SPtr transaction1 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            int opCount = 10;

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction1, r, *primary->StateManager);
            }

            // commit tx1
            SyncAwait(transaction1->CommitAsync());
            transaction1->Dispose();

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*primary->TestTransactionManager, allocator);
            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(*transaction2, r, *primary->StateManager);
            }

            // Do not commit tx2 yet

            // Block PrepareCheckpointAsync
            primary->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // enable 1 checkpoint
            primary->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // add 3rd operation to tx2
            AddOperationToTransaction(*transaction2, r, *primary->StateManager);

            // this should trigger a checkpoint 
            BarrierLogRecord::SPtr barrier;
            primary->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = primary->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            VERIFY_ARE_EQUAL(beginCheckpoint->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // continue preparecheckpointasync
            primary->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primary->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = primary->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_ARE_EQUAL(endCheckpoint->LastCompletedBeginCheckpointRecord->EarliestPendingTransaction->BaseTransaction.TransactionId, transaction2->TransactionId);

            // Now that the checkpoint is done, initiate a truncate head operation
            primary->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
            primary->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            primary->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            // This should trigger the truncate head record
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            bool isProcessingComplete = primary->StateManager->WaitForProcessingToComplete(21, 21, Common::TimeSpan::FromSeconds(1));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            DateTime timeStamp;
            CopyContext::SPtr copyContext = secondary->CreateACopyContext(
                secondary->PartitionId->ReplicaId,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                0,
                0,
                Constants::ZeroLsn,
                Constants::ZeroLsn,
                timeStamp);

            CopyStageBuffers::SPtr copyStageBuffers = CopyStageBuffers::Create(allocator);

            TRANSACTIONAL_REPLICATOR_SETTINGS txrSettings = { 0 };
            TransactionalReplicatorSettingsUPtr tmp;
            TransactionalReplicatorSettings::FromPublicApi(txrSettings, tmp);

            TxnReplicator::TRInternalSettingsSPtr config = TRInternalSettings::Create(
                move(tmp),
                make_shared<TransactionalReplicatorConfig>());

            CopyStream::WaitForLogFlushUpToLsnCallback callback = GetWaitForLogFlushUpToLsnCallback();
            CopyStream::SPtr copyStream = CopyStream::Create(
                *primary->PartitionId,
                *primary->ReplicatedLogManager,
                *primary->StateManager,
                *primary->CheckpointManager,
                callback,
                primary->PartitionId->ReplicaId,
                10,
                *copyContext,
                *copyStageBuffers,
                config,
                true,
                allocator);

            // reuse the primary state manager so the validation will be run again for all the records being received in the secondary side
            primary->StateManager->Reuse();

            RecoveryManager::SPtr secondaryRecoveryManager = secondary->RecoveryManager;

            SecondaryDrainManager::SPtr secondaryDrainManager = SecondaryDrainManager::Create(
                *secondary->PartitionId,
                *secondary->RecoveredOrCopiedCheckpointState,
                *secondary->RoleContextDrainState,
                *secondary->OperationProcessor,
                *secondary->StateReplicator,
                *secondary->BackupManager,
                *secondary->CheckpointManager,
                *secondary->TransactionManager,
                *secondary->ReplicatedLogManager,
                *primary->StateManager,
                *secondaryRecoveryManager,
                config,
                *invalidLogRecords_,
                allocator);

            secondary->StateReplicator->InitializePrimaryCopyStream(*copyStream);

            // Resetting the LastFlushedPsn on secondary replica to zero, as the full_copy mode starts with a fresh log file ans zero psn
            secondary->LastFlushedPsn = 0;
            secondaryDrainManager->StartBuildingSecondary();
            SyncAwait(secondaryDrainManager->WaitForCopyAndReplicationDrainToCompleteAsync());

            LONG64 tailLsnAfterCopy = secondary->ReplicatedLogManager->CurrentLogTailLsn;

            secondaryRecoveryManager.Reset();
            secondaryDrainManager.Reset();
            SyncAwait(secondary->CloseAndQuiesceReplica());

            CloseAndReopenTestReplica(secondary, seed);

            LONG64 tailLsnAfterRecovery = secondary->ReplicatedLogManager->CurrentLogTailLsn;

            VERIFY_ARE_EQUAL(tailLsnAfterCopy, tailLsnAfterRecovery);

            SyncAwait(secondary->CloseAndQuiesceReplica());
            secondary->EndTest(false, true);

            copyStream->Dispose();

            SyncAwait(primary->CloseAndQuiesceReplica());
            primary->EndTest(false, true);

            secondaryApiFaultUtility.Reset();
        }
    }

#pragma endregion

#pragma region TRUNCATE_TAIL_TESTS

    BOOST_AUTO_TEST_CASE(TruncateTail_BeginTxLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_BeginTxLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 blocks ReplicateAsync API
            - P1 creates a transaction T1, adds 10 operations and commits T1
            - P1 performs a single operation transaction, represented by an atomic BeginTransactionOperationLogRecord
            - P1 goes down and comes up as an idle secondary.
            - P2 comes up as a primary, opens a transaction and commits after 2 operations
            - P2 starts building P1 and detects false progress due to the additional single operation transaction committed in T1. When traversing log records to reach the target LSN, the BeginTransactionOperationLogRecord will be truncated as its LSN is greater than the target tail
            */
            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // p1 logs 1 transactions with 10 operations 
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }
            Awaitable<NTSTATUS> commit1 = t1->CommitAsync();

            // Add a single operation transaction
            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit2 = t2->CommitAsync();

            // Add a multi-operation transaction
            Transaction::SPtr t3 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t3, r, *p1->StateManager);
            AddOperationToTransaction(*t3, r, *p1->StateManager);
            AddOperationToTransaction(*t3, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit3 = t3->CommitAsync();

            apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(3));
            SyncAwait(p1->CloseAndQuiesceReplica());
            SyncAwait(commit1);
            SyncAwait(commit2);
            SyncAwait(commit3);
            t1->Dispose();
            t2->Dispose();
            t3->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            // Create a copy context from p1
            CopyContext::SPtr copyContext = p1->CreateACopyContext();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            // p2 performs a transaction with 2 operations
            Transaction::SPtr p2_t1 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*p2_t1, r, *p2->StateManager);
            }
            SyncAwait(p2_t1->CommitAsync());
            p2_t1->Dispose();

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection 
            // Resulting tail truncation includes BeginTransactionOperationLogRecord
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_EndTxLogRecord_FalseProgress_FaultyApply)
    {
        TEST_TRACE_BEGIN("TruncateTail_EndTxLogRecord_FalseProgress_FaultyApply")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 blocks ReplicateAsync API
            - P1 creates a transaction T1, adds 10 operations and commits T1
            - P1 creates a transaction T2, adds 2 operations and commits T2
            - P1 goes down and comes up as an idle secondary.
            - P2 comes up as a primary, opens a transaction and commits after 2 operations
            - P1 is configured to fail calls to ApplyAsync in the TestStateProviderManager
            - P2 starts building P1 and detects false progress due to the additional transaction committed in T1. 
            - P1 attempts to truncate its log tail and the first call to ApplyAsync fails and the secondary transitions to a closing state. This first ApplyAsync call is encountered when truncating the EndTransactionLogRecord associated with transaction T2
            */
            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // p1 logs 1 transactions with 10 operations 
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }
            Awaitable<NTSTATUS> commit1 = t1->CommitAsync();
            
            // p2 creates and commits a transaction with 2 operations
            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit2 = t2->CommitAsync();

            apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(3));
            SyncAwait(p1->CloseAndQuiesceReplica());
            SyncAwait(commit1);
            SyncAwait(commit2);
            t1->Dispose();
            t2->Dispose();

            CloseAndReopenTestReplica(p1, seed);

            CopyContext::SPtr copyContext = p1->CreateACopyContext();
            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            Transaction::SPtr p2_t1 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*p2_t1, r, *p2->StateManager);
            }
            SyncAwait(p2_t1->CommitAsync());
            p2_t1->Dispose();

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Fail ApplyAsync API calls during truncation
            apiFaultUtility_->FailApi(ApiName::ApplyAsync, SF_STATUS_TRANSACTION_ABORTED);

            // Create a new replica 
            // Expected false progress detection 
            // Resulting tail truncation fails during ApplyAsync and faults p1->roleContextDrainState_
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing,
                "Expected faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_BackupLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_BackupLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 blocks ReplicateAsync API
            - P1 creates a transaction T1, adds 10 operations and commits T1
            - P1 replicates and logs a BackupLogRecord, then invokes a flush operation
            - P1 explicitly fails replication for records after the BackupLogRecord via TestStateReplicator->FailReplicationAfterLSN.
            - P1 explicitly marks the record as applied and unlocks the record in P1->OperationProcessor
            - P1 goes down and comes up as an idle secondary.
            - P2 comes up as a primary, opens a transaction and commits after 2 operations
            - P2 starts building P1 and detects false progress due to the additional operations committed in T1. When traversing log records to reach the target LSN, the BackupLogRecord will be truncated as its LSN is greater than the target tail
            */
            InitializeTest();

            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // p1 logs 1 transactions with 10 operations 
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }

            Awaitable<NTSTATUS> commit1 = t1->CommitAsync();

            KGuid temp;
            temp.CreateNew();

            // p1 inserts a BackupLogRecord and invokes a flush operation
            BackupLogRecord::SPtr lastCompletedBackup = BackupLogRecord::Create(
                temp,
                p1->ReplicatedLogManager->CurrentLogTailEpoch,
                1,
                1,
                1,
                *invalidLogRecords_->Inv_PhysicalLogRecord,
                allocator);

            LONG64 buffered;
            p1->ReplicatedLogManager->ReplicateAndLog(*lastCompletedBackup, buffered, p1->TransactionManager.RawPtr());
            SyncAwait(p1->ReplicatedLogManager->InnerLogManager->FlushAsync(L"TruncateTail_BackupLogRecord_FalseProgress"));
            SyncAwait(WaitForLogFlushUptoLsn(lastCompletedBackup->Lsn));

            p1->StateReplicator->FailReplicationAfterLSN(
                lastCompletedBackup->Lsn,
                STATUS_CANCELLED);

            // Add a multi-operation transaction
            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit2 = t2->CommitAsync();

            // p1 is closed
            Awaitable<void> closeTask = p1->CloseAndQuiesceReplica();
            Sleep(500);
            
            apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(3));

            status = SyncAwait(lastCompletedBackup->AwaitApply());
            ASSERT_IFNOT(NT_SUCCESS(status), "Failed to apply backup log record");

            // Unlock the BackupLogRecord
            p1->OperationProcessor->Unlock(*lastCompletedBackup);

            SyncAwait(closeTask);
            SyncAwait(commit1);
            SyncAwait(commit2);
            t1->Dispose();
            t2->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            // Create a copy context from p1
            CopyContext::SPtr copyContext = p1->CreateACopyContext();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            // Create a new secondary replica p2
            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            // p2 performs a transaction with 2 operations
            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*transaction2, r, *p2->StateManager);
            }
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection
            // Resulting tail truncation includes BackupLogRecord
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_UpdateEpochLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_UpdateEpochLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 blocks ReplicateAsync API
            - P1 creates a transaction T1, adds 10 operations and commits T1
            - P1 inserts an UpdateEpochLogRecord and flushes
            - P1 goes down and comes up as an idle secondary.
            - P2 comes up as a primary, opens a transaction and commits after 2 operations
            - P2 starts building P1 and detects false progress due to the additional operations committed in T1. When traversing log records to reach the target LSN, the UpdateEpochLogRecord will be truncated as its LSN is greater than the target tail
            */
            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // p1 logs 1 transactions with 10 operations 
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }
            Awaitable<NTSTATUS> commit1 = t1->CommitAsync();
            LONG64 expectedBarrierLsnAfterTx = 13;

            // Why explicitly wait for LSN 13 to flush here? 
            // After the above transaction with 10 operations, an EndTransactionLogRecord with LSN 12 
            // has been logged. 
            //
            //      Replica Open (CompleteChkptRecord) LSN = 1
            //      BeginTransaction + Operation 0 LSN = 2
            //      Operation 1 - 9 LSNs = [3 -> 11]
            //      EndTransaction LSN = 12
            //
            // The next LSN increment (13) is the barrier record logged from group commit initiated by above end tx
            // 
            // ReplicatedLogManager->CurrentLogTailLsn is not used as a reference for desired log flush LSN to prevent
            // potential race between the value updating within ReplicatedLogManager and this WaitForLogFlush. 
            // Using static 13 allows us to ensure the barrier is indeed flushed prior to insertion of UpdateEpochLogRecord
            SyncAwait(WaitForLogFlushUptoLsn(expectedBarrierLsnAfterTx));

            // Confirm the log tail is equal to expected barrier LSN
            ASSERT_IFNOT(
                p1->ReplicatedLogManager->CurrentLogTailLsn == expectedBarrierLsnAfterTx,
                "Expected  tail LSN 13 after inserting barrier");

            // Create and insert an UpdateEpochLogRecord
            Epoch currentEpoch = p1->ReplicatedLogManager->CurrentLogTailEpoch;
            Epoch newEpoch(currentEpoch.DataLossVersion, currentEpoch.ConfigurationVersion + 1);

            UpdateEpochLogRecord::SPtr updateEpochRecord = UpdateEpochLogRecord::Create(
                newEpoch,
                rId_,
                *invalidLogRecords_->Inv_PhysicalLogRecord,
                allocator);

            updateEpochRecord->Lsn = p1->ReplicatedLogManager->CurrentLogTailLsn;
            p1->ReplicatedLogManager->UpdateEpochRecord(*updateEpochRecord);

            // Flush the current physical log writer
            SyncAwait(p1->ReplicatedLogManager->InnerLogManager->FlushAsync(L"TruncateTail_UpdateEpochLogRecord_FalseProgress"));
            SyncAwait(updateEpochRecord->AwaitProcessing());

            // Close current primary replica p1
            auto closeTask = p1->CloseAndQuiesceReplica();
            apiFaultUtility_->ClearFault(ApiName::ReplicateAsync);
            SyncAwait(closeTask);
            SyncAwait(commit1);
            t1->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            // Create a copy context from p1 using the existing progress vector
            CopyContext::SPtr copyContext = p1->CreateACopyContextWReplicaProgressVector();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            // Create a new secondary replica p2
            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            Transaction::SPtr transaction2 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*transaction2, r, *p2->StateManager);
            }
            SyncAwait(transaction2->CommitAsync());
            transaction2->Dispose();

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection
            // Resulting tail truncation includes UpdateEpochLogRecord
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_BeginCheckpointLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_BeginCheckpointLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 blocks ReplicateAsync and PrepareCheckpoint API's
            - P1 creates a transaction T1, adds 10 operations
            - P1 enables checkpoint 
            - P1 replicates a barrier record to trigger checkpoint and confirms a checkpoint is in progress 
            - P1 adds 2 additional operations and commits T1. PrepareCheckpoint is still blocked at this point
            - P1 explicitly fails replication for records after the BeginCheckpointLogRecord via TestStateReplicator->FailReplicationAfterLSN.
            - P1 unblocks replication and PrepareCheckpoint, aborts the pending checkpoint and closes
            - P1 reopens as an idle secondary
            - P2 comes up as a primary, opens a transaction and commits after 4 operations
            - P2 starts building P1 and detects false progress due to the additional operations committed in T1. When traversing log records to reach the target LSN, the BeginCheckpointLogRecord will be truncated as its LSN is greater than the target tail 
            */

            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);
            apiFaultUtility_->BlockApi(ApiName::PrepareCheckpoint);

            // p1 logs 1 transactions with 10 operations 
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 10; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }

            // Enable 1 checkpoint
            p1->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // Barrier replication triggers the start of a checkpoint
            BarrierLogRecord::SPtr barrier;
            p1->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed
            SyncAwait(barrier->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint = p1->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);

            AddOperationToTransaction(*t1, r, *p1->StateManager);
            AddOperationToTransaction(*t1, r, *p1->StateManager);
            Awaitable<NTSTATUS> commitTask = t1->CommitAsync();

            // Ensure no records are processed after BeginCheckpointLogRecord
            // Any records processed after the valid LSN will still throw the configured exception.
            // Since log truncation is not permitted past the last stable LSN of a replica, this behavior is 
            // to prevent the insertion of additional barrier records which shift the last known stable LSN.
            p1->StateReplicator->FailReplicationAfterLSN(
                beginCheckpoint->Lsn,
                STATUS_CANCELLED);

            // Wait for flush of the BeginCheckpointLogRecord
            SyncAwait(WaitForLogFlushUptoLsn(beginCheckpoint->Lsn));

            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit2 = t2->CommitAsync();

            auto closeTask = p1->CloseAndQuiesceReplica();

            apiFaultUtility_->ClearFault(ApiName::ReplicateAsync);
            apiFaultUtility_->ClearFault(ApiName::PrepareCheckpoint);

            p1->CheckpointManager->AbortPendingCheckpoint();

            status = SyncAwait(commitTask);
            VERIFY_ARE_EQUAL(status, SF_STATUS_TRANSACTION_ABORTED);

            SyncAwait(closeTask);
            SyncAwait(commit2);
            t1->Dispose();
            t2->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            CopyContext::SPtr copyContext = p1->CreateACopyContext();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            // Commit 4 operations in the first tx
            Transaction::SPtr p2_t1 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 3; i++)
            {
                AddOperationToTransaction(*p2_t1, r, *p2->StateManager);
            }
            SyncAwait(p2_t1->CommitAsync());
            p2_t1->Dispose();

            // Disable callback assert on PSN during Truncation
            p1->AssertInFlushCallback = false;

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection
            // Resulting tail truncation includes BeginCheckpointLogRecord
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_CompleteCheckpointLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_CompleteCheckpointLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
                - 2 primaries - P1, P2
                - P1 blocks ReplicateAsync API
                - P1 creates a transaction T1 and adds 10 operations
                - P1 enables checkpoint and blocks PerformCheckpoint to ensure that end checkpoint is not logged
                - P1 replicates a barrier record to trigger checkpoint
                - P1 adds 6 additional operations to T1
                - Confirm a checkpoint is in progress on P1, unblock ReplicateAsync and PerformCheckpoint
                - P1 blocks replication after the EndCheckpointLogRecord to ensure no barrier is stabilized
                - P1 adds 5 additional operations to T1 and commits
                - After the beginCheckpointLogRecord has been processed, close P1
                - P1 goes down and reopens as an idle secondary, inserting a CompleteCheckpointLogRecord if missing
                - P2 comes up as a primary, opens a transaction and commits after 10 operations
                - P2 replicates a barrier record to move the stable LSN. This is to ensure the last stable lsn is <= the target tail LSN
                - P2 starts building P1 and detects false progress due to the additional operation committed in T1. When traversing log records to reach the target LSN, the CompleteCheckpointLogRecord will be truncated as its LSN is greater than the target tail 
            */

            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // p1 begins a transaction with 10 operations, does not commit
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 8; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }
            Awaitable<NTSTATUS> commitTask = t1->CommitAsync();

            p1->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // Block PerformCheckpoint
            // Ensures additional records can be inserted before EndCheckpoint is logged
            apiFaultUtility_->BlockApi(ApiName::PerformCheckpoint);

            // Barrier replication triggers the start of a checkpoint
            BarrierLogRecord::SPtr barrier;
            p1->CheckpointManager->ReplicateBarrier(barrier); // Ensures all records after the above commit are completely flushed
            SyncAwait(barrier->AwaitFlush());

            // Confirm a checkpoint is in progress
            BeginCheckpointLogRecord::SPtr beginCheckpoint = p1->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);

            // Block CompleteCheckpoint
            apiFaultUtility_->BlockApi(ApiName::CompleteCheckpoint);

            // Unblock PerformCheckpoint
            apiFaultUtility_->ClearFault(ApiName::PerformCheckpoint);
            Sleep(500); // Allow EndCheckpoint to be logged

            // Any barriers inserted after the current log tail will fail replication
            // This is intended to prevent stabilization
            p1->StateReplicator->FailReplicationAfterLSN(
                p1->ReplicatedLogManager->CurrentLogTailLsn,
                STATUS_CANCELLED);

            // Add a new transaction and operations before the CompleteCheckpoint is processed
            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 8; i++)
            {
                AddOperationToTransaction(*t2, r, *p1->StateManager);
            }
            // Do not commit t2
            // ^Prevent below assert when building secondary:
            // BuildOrCopyReplica lastStableLsn <= tailLsn. last stable lsn: 22

            // Unblock CompleteCheckpoint
            apiFaultUtility_->ClearFault(ApiName::CompleteCheckpoint);

            // Unblock replication
            apiFaultUtility_->ClearFault(ApiName::ReplicateAsync);

            // Wait for the BeginCheckpointLogRecord to complete processing
            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(p1->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            auto closeTask = p1->CloseAndQuiesceReplica();

            SyncAwait(commitTask);
            SyncAwait(closeTask);
            t1->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            // Create a copy context from p1 using the existing progress vector in p1
            CopyContext::SPtr copyContext = p1->CreateACopyContextWReplicaProgressVector();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            // Commit 10 operations in the first tx
            Transaction::SPtr p2_t1 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 8; i++)
            {
                AddOperationToTransaction(*p2_t1, r, *p2->StateManager);
            }
            SyncAwait(p2_t1->CommitAsync());
            p2_t1->Dispose();

            // Disable callback assert on PSN during Truncation
            p1->AssertInFlushCallback = false;

            // Move stable LSN through barrier replication but do not enable checkpoint
            BarrierLogRecord::SPtr barrierp2;
            p2->CheckpointManager->ReplicateBarrier(barrierp2);
            SyncAwait(barrierp2->AwaitFlush());
            VERIFY_ARE_EQUAL(p2->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection
            // Resulting tail truncation includes CompleteCheckpoint
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncateTail_TruncateHeadLogRecord_FalseProgress)
    {
        TEST_TRACE_BEGIN("TruncateTail_TruncateHeadLogRecord_FalseProgress")
        {
            /*
            Test Scenario:
            - 2 primaries - P1, P2
            - P1 explicitly inserts an indexing log record after opening. This is important when finding the right log head candidate in ReplicatedLogManager->TruncateHead. 
                The current log head record cannot be the new target head indexing log record
            - P1 creates a transaction T1 and adds 10 operations
            - P1 blocks the ReplicateAsync API, enables checkpoint and indexing record
            - P1 adds an additional operation to the open transaction T1
            - P1 replicates a barrier record to trigger checkpoint and unblocks replication
            - P1 confirms the checkpoint has completed and blocks replication again as well as explicitly failing any further records with the STATUS_CANCELLED exception through TestStateReplicator->FailReplicationAfterLSN
            - P1 adds 2 additional operations to T1
            - P1 enables truncateHead and isGoodLogHeadCandidate in the TestLogTruncationManager
            - P1 unblocks replication
            - P1 triggers a truncate head operation by committing the open transaction t1. Why does t1->CommitAsync() initiate a truncate head operation? 
                Physical records are inserted if necessary after every logical operation and in this case, we have enabled truncateHead and isGoodLogHeadCandidate before committing this transaction.

                    Workflow:
                    Transaction->CommitAsync
                    Transaction->PrivateCommitAsync
                    ITransactionManager->CommitTransactionAsync       | Implemented in LoggingReplicatorImpl
                    TransactionManager->CommitTransactionAsync
                    TransactionManager->EndTransactionAsync
                    TransactionManager->ProcessLogicalRecordOnPrimaryAsync
                    TransactionManager->ReplicateAndLogLogicalRecord
                    TransactionManager->InitiateLogicalRecordProcessingOnPrimary
                    CheckpointManager->InsertPhysicalRecordsIfNecessary
                    CheckpointManager->IndexIfNecessary
                    CheckpointManager->TruncateHeadIfNecessary
                    LogTruncationManager->ShouldTruncateHead

                    If ShouldTruncateHead returns true, CheckpointManager calls ReplicatedLogManager->TruncateHead
                    - Traverse physical records until the previous indexing record is found
                    - Indexing record must be before the earliest pending tx record and not equal to the log head
                    - If this is a good log head candidate, insert a TruncateHeadLogRecord

            - P1 closes and reopens as an idle secondary. By recovering logged operations that were not replicated, P1 has ended up in a false progress state
            - P2 comes up as a primary and starts building P1 at which point false progress is detected.
                The truncated records will include the TruncateHeadLogRecord inserted at t1->CommitAsync()
            */

            InitializeTest();

            // Create primary replica p1
            TestReplica::SPtr p1;
            CreatePrimaryReplica(seed, allocator, p1);

            // Explicitly insert an indexing record
            p1->ReplicatedLogManager->Index();

            // p1 creates a transaction and adds 2 operations
            Transaction::SPtr t1 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }

            // p1 blocks ReplicateAsync API
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            p1->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            p1->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            AddOperationToTransaction(*t1, r, *p1->StateManager);

            // Barrier replication triggers the start of a checkpoint
            BarrierLogRecord::SPtr barrier;
            p1->CheckpointManager->ReplicateBarrier(barrier);
            SyncAwait(barrier->AwaitFlush());

            // Confirm a checkpoint has started
            BeginCheckpointLogRecord::SPtr beginCheckpoint = p1->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);

            // Unblock replication, allow checkpoint to complete
            apiFaultUtility_->ClearFault(ApiName::ReplicateAsync);

            SyncAwait(beginCheckpoint->AwaitProcessing());
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(p1->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            EndCheckpointLogRecord::SPtr endCheckpoint = p1->ReplicatedLogManager->LastCompletedEndCheckpointRecord;
            VERIFY_IS_TRUE(endCheckpoint != nullptr);

            // Block replication on p1 again
            apiFaultUtility_->BlockApi(ApiName::ReplicateAsync);

            // Add 8 additional operations to the open transaction t1
            // These operations will be undone as part of the tail truncation
            for (int i = 0; i < 8; i++)
            {
                AddOperationToTransaction(*t1, r, *p1->StateManager);
            }

            // Enable TruncateHead / IsGoodLogHeadCandidate operations from TestLogTruncationManager 
            p1->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::Yes);
            p1->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::Yes);

            // Initiate truncate head operation
            Awaitable<NTSTATUS> commitTask = t1->CommitAsync();

            // Add a multi-operation transaction
            Transaction::SPtr t2 = Transaction::CreateTransaction(*p1->TestTransactionManager, allocator);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            AddOperationToTransaction(*t2, r, *p1->StateManager);
            Awaitable<NTSTATUS> commit2 = t2->CommitAsync();

            apiFaultUtility_->ClearFault(ApiName::ReplicateAsync);
            auto closeTask = p1->CloseAndQuiesceReplica();

            // Abort the pending truncation on p1
            p1->CheckpointManager->AbortPendingLogHeadTruncation();

            SyncAwait(commitTask);
            SyncAwait(closeTask);
            SyncAwait(commit2);
            t1->Dispose();
            t2->Dispose();

            // Reopen p1 for recovery
            CloseAndReopenTestReplica(p1, seed);

            // Create a copy context from p1
            CopyContext::SPtr copyContext = p1->CreateACopyContext();

            ApiFaultUtility::SPtr fUtility = ApiFaultUtility::Create(allocator);

            TestReplica::SPtr p2;
            CreateSecondaryReplica(r.Next(), fUtility, allocator, p2);

            // Commit 2 operations in the first tx
            Transaction::SPtr p2_t1 = Transaction::CreateTransaction(*p2->TestTransactionManager, allocator);
            for (int i = 0; i < 2; i++)
            {
                AddOperationToTransaction(*p2_t1, r, *p2->StateManager);
            }
            SyncAwait(p2_t1->CommitAsync());
            p2_t1->Dispose();

            // Enable checkpoint
            p2->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // Trigger checkpoint by replicating barrier 
            BarrierLogRecord::SPtr barrierp2;
            p2->CheckpointManager->ReplicateBarrier(barrierp2);
            SyncAwait(barrierp2->AwaitFlush());

            BeginCheckpointLogRecord::SPtr beginCheckpoint2 = p2->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint2 != nullptr);
            SyncAwait(beginCheckpoint2->AwaitProcessing());

            // Disable callback assert on PSN during Truncation
            p1->AssertInFlushCallback = false;

            // Creating a copy stream from p2
            CopyStream::SPtr p2CopyStream;
            CreateCopyStream(p2, p1, copyContext, allocator, p2CopyStream);

            // Create a new replica 
            // Expected false progress detection
            // Resulting tail truncation includes TruncateHeadLogRecord
            CreateNewReplicaFromCopyStream(p2, p1, p2CopyStream, allocator);

            ASSERT_IFNOT(
                p1->RoleContextDrainState->IsClosing == false,
                "Unexpectedly faulted RoleContextDrainState after failed ApplyAsync. Current status {0}",
                p1->RoleContextDrainState->IsClosing);

            // Clean up both test replicas and the copy stream
            CleanupTruncateTailFalseProgressTest(p2, p1, p2CopyStream);
        }
    }
#pragma endregion

#pragma endregion

#pragma region LOGRECORD_UPGRADE_TESTS
    BOOST_AUTO_TEST_CASE(LogRecord_ReadFromPreviousVersion_51)
    {
        TEST_TRACE_BEGIN("LogRecord_ReadFromPreviousVersion_51") 
        {
            InitializeTest();
            Common::Guid existingGuid(L"eccb292c-45a0-4d5d-a9d9-3ee717d83b56");

            pId_ = existingGuid.AsGUID();
            rId_ = (FABRIC_REPLICA_ID)1;

            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

            primaryTestReplica_ = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_->PartitionId = prId_;
            primaryTestReplica_->ReplicaId = (FABRIC_REPLICA_ID)1;
            primaryTestReplica_->UseFileLogManager = true;  // Use FileLogManager

            ASSERT_IFNOT(primaryTestReplica_->ReplicaId == 1, "Invalid replica Id");

            KString::SPtr workDirKstr;
            std::wstring targetUpgradeVersion = L"version_51";
            CreateLogRecordUpgradeTest_WorkingDirectory(targetUpgradeVersion, allocator, workDirKstr);

            primaryTestReplica_->DedicatedLogPath = *workDirKstr;
            primaryTestReplica_->Initialize(seed);

            LONG64 currentLogTailLsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogTailLsn;
            ASSERT_IFNOT(currentLogTailLsn > 2, "Invalid LSN after recovery");

            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord != nullptr,
                "Must recover LastCompletedBeginCheckpointRecord");

            LONG64 lastCompletedBeginChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord->Lsn;

            ASSERT_IFNOT(lastCompletedBeginChkptLsn > 1500, "Invalid begin checkpoint");
            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord != nullptr,
                "Must recover LastCompletedEndCheckpointRecord");

            LONG64 lastCompletedEndChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->Lsn;
            LONG64 logHeadLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->LogHeadLsn;

            ASSERT_IFNOT(lastCompletedEndChkptLsn > 1500, "Invalid end checkpoint");
            ASSERT_IFNOT(logHeadLsn > 1500, "Invalid log head after recovery");

            primaryTestReplica_->StateReplicator->LSN = currentLogTailLsn + 1;

            Transaction::SPtr t1 = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, allocator);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            SyncAwait(t1->CommitAsync());

            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());

            primaryTestReplica_->EndTest(true, true);

            ErrorCode er = Common::Directory::Delete_WithRetry(workDirKstr->operator LPCWSTR(), true, true);
            ASSERT_IFNOT(er.IsSuccess(), "Failed to delete working directory");
        }
    }

    BOOST_AUTO_TEST_CASE(LogRecord_ReadFromPreviousVersion_50)
    {
        TEST_TRACE_BEGIN("LogRecord_ReadFromPreviousVersion_50")
        {
            InitializeTest();
            Common::Guid existingGuid(L"1d2cc444-8335-416c-b3cf-bbdd13bde9d6");

            pId_ = existingGuid.AsGUID();
            rId_ = (FABRIC_REPLICA_ID)1;

            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

            primaryTestReplica_ = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_->PartitionId = prId_;
            primaryTestReplica_->ReplicaId = (FABRIC_REPLICA_ID)1;
            primaryTestReplica_->UseFileLogManager = true;  // Use FileLogManager

            ASSERT_IFNOT(primaryTestReplica_->ReplicaId == 1, "Invalid replica Id");

            KString::SPtr workDirKstr;
            std::wstring targetUpgradeVersion = L"version_50";
            CreateLogRecordUpgradeTest_WorkingDirectory(targetUpgradeVersion, allocator, workDirKstr);

            primaryTestReplica_->DedicatedLogPath = *workDirKstr;
            primaryTestReplica_->Initialize(seed);

            LONG64 currentLogTailLsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogTailLsn;
            ASSERT_IFNOT(currentLogTailLsn > 2, "Invalid LSN after recovery");
            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord != nullptr,
                "Must recover LastCompletedBeginCheckpointRecord");

            LONG64 lastCompletedBeginChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord->Lsn;

            ASSERT_IFNOT(lastCompletedBeginChkptLsn > 1500, "Invalid begin checkpoint");
            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord != nullptr,
                "Must recover LastCompletedEndCheckpointRecord");

            LONG64 lastCompletedEndChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->Lsn;
            LONG64 logHeadLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->LogHeadLsn;

            ASSERT_IFNOT(lastCompletedEndChkptLsn > 1500, "Invalid end checkpoint");
            ASSERT_IFNOT(logHeadLsn > 1500, "Invalid log head after recovery");

            primaryTestReplica_->StateReplicator->LSN = currentLogTailLsn + 1;

            Transaction::SPtr t1 = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, allocator);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            SyncAwait(t1->CommitAsync());

            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());

            primaryTestReplica_->EndTest(true, true);

            ErrorCode er = Common::Directory::Delete_WithRetry(workDirKstr->operator LPCWSTR(), true, true);
            ASSERT_IFNOT(er.IsSuccess(), "Failed to delete working directory");
        }
    }

    BOOST_AUTO_TEST_CASE(LogRecord_ReadFromPreviousVersion_60)
    {
        TEST_TRACE_BEGIN("LogRecord_ReadFromPreviousVersion_60")
        {
            InitializeTest();
            Common::Guid existingGuid(L"b1f04935-dc04-43e3-b8d1-194e0c3cc2a2");

            pId_ = existingGuid.AsGUID();
            rId_ = (FABRIC_REPLICA_ID)1;

            prId_ = PartitionedReplicaId::Create(pId_, rId_, allocator);

            primaryTestReplica_ = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_->PartitionId = prId_;
            primaryTestReplica_->ReplicaId = (FABRIC_REPLICA_ID)1;
            primaryTestReplica_->UseFileLogManager = true;  // Use FileLogManager

            ASSERT_IFNOT(primaryTestReplica_->ReplicaId == 1, "Invalid replica Id");

            KString::SPtr workDirKstr;
            std::wstring targetUpgradeVersion = L"version_60";
            CreateLogRecordUpgradeTest_WorkingDirectory(targetUpgradeVersion, allocator, workDirKstr);

            primaryTestReplica_->DedicatedLogPath = *workDirKstr;
            primaryTestReplica_->Initialize(seed);

            LONG64 currentLogTailLsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogTailLsn;
            ASSERT_IFNOT(currentLogTailLsn > 2, "Invalid LSN after recovery");
            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord != nullptr,
                "Must recover LastCompletedBeginCheckpointRecord");

            LONG64 lastCompletedBeginChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord->Lsn;

            ASSERT_IFNOT(lastCompletedBeginChkptLsn > 1500, "Invalid begin checkpoint");
            ASSERT_IFNOT(
                primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord != nullptr,
                "Must recover LastCompletedEndCheckpointRecord");

            LONG64 lastCompletedEndChkptLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->Lsn;
            LONG64 logHeadLsn = primaryTestReplica_->ReplicatedLogManager->LastCompletedEndCheckpointRecord->LogHeadLsn;

            ASSERT_IFNOT(lastCompletedEndChkptLsn > 1500, "Invalid end checkpoint");
            ASSERT_IFNOT(logHeadLsn > 1500, "Invalid log head after recovery");

            primaryTestReplica_->StateReplicator->LSN = currentLogTailLsn + 1;

            Transaction::SPtr t1 = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, allocator);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            AddOperationToTransaction(*t1, r, *primaryTestReplica_->StateManager);
            SyncAwait(t1->CommitAsync());

            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());

            primaryTestReplica_->EndTest(true, true);

            ErrorCode er = Common::Directory::Delete_WithRetry(workDirKstr->operator LPCWSTR(), true, true);
            ASSERT_IFNOT(er.IsSuccess(), "Failed to delete working directory");
        }
    }
#pragma endregion

#pragma region MISC_BUG_REPROS
    // This test case validates that when there are a LOT of physical records linked, freeing the last one
    // does not result in stack overflow due to the recursion chain being so deep
    //
    // 1 example of when this happened is fabrictest with very low index interval
    BOOST_AUTO_TEST_CASE(Scale_10KPhysicalLinks)
    {
        TEST_TRACE_BEGIN("Scale_10KPhysicalLinks")

        // There are 2 code paths where a long physical link can cause a stack overflow:
        // 1. When a new log head is selected during truncate head. It is initiated by a call to "FreePreviousPhysicalRecordChain"
        // 2. When the above cannot happen and instead a replica is closing with a long link - destruction of map or any other object
        
        // Both code paths are tested here randomly to get coverage
        {
            InitializeTest();
            int opcount = 10000;
            
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            TRANSACTIONAL_REPLICATOR_SETTINGS settings = { 0 };
            settings.Flags = FABRIC_TRANSACTIONAL_REPLICATOR_MAX_STREAM_SIZE_MB | FABRIC_TRANSACTIONAL_REPLICATOR_OPTIMIZE_LOG_FOR_LOWER_DISK_USAGE;
            settings.MaxStreamSizeInMB = 1024;
            settings.OptimizeLogForLowerDiskUsage = false;
            replica->Initialize(seed, false, true, true, &settings);

            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);
    
            // Add a large transaction with a lot of indexing records
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);

                for (int i = 0; i < opcount; i++)
                {
                    AddOperationToTransaction(*transaction, r, *replica->StateManager);
                    if (i == (opcount - 500))
                    {
                        // Set indexing to NO so that the new HEAD is here
                        replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::No);
                    }
                }

                // Add a second transaction so that the first one is removed from unstable list
                // The first large transaction has a link to every index record added. Removing it will make the ref count go to 0 when a new head is chosen

                bool runCodePath2 = ((transaction->TransactionId % 2) == 0);

                if (runCodePath2)
                {
                    Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                    AddOperationToTransaction(*transaction2, r, *replica->StateManager);
                    AddOperationToTransaction(*transaction2, r, *replica->StateManager);

                    SyncAwait(transaction->CommitAsync());
                    SyncAwait(transaction2->CommitAsync());
                    transaction2->Dispose();

                    opcount += 2;
                }
                else
                {
                    SyncAwait(transaction->CommitAsync());
                }

                transaction->Dispose();

                bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(opcount, opcount, Common::TimeSpan::FromSeconds(1));
                VERIFY_ARE_EQUAL(isProcessingComplete, true);
            }

            replica->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // Trigger checkpoint
            {
                BarrierLogRecord::SPtr barrier;
                replica->CheckpointManager->ReplicateBarrier(barrier);
                SyncAwait(barrier->AwaitFlush());
            }

            BeginCheckpointLogRecord::SPtr beginCheckpoint = replica->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(beginCheckpoint != nullptr);

            // continue preparecheckpointasync
            replica->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());

            replica->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // This is NoOnceThenYes so that the second to last index record is the new HEAD instead of the last so that
            // we have only 1 ref count on it
            replica->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::NoOnceThenYes);

            // This transaction is to initiate the truncation
            {
                Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);
                AddOperationToTransaction(*transaction2, r, *replica->StateManager);
                SyncAwait(transaction2->CommitAsync());
                transaction2->Dispose();
            }
            
            // This is to stabilize the truncation and ensure the truncate head was applied/processed
            {
                BarrierLogRecord::SPtr barrier;
                replica->CheckpointManager->ReplicateBarrier(barrier);
                SyncAwait(barrier->AwaitProcessing());
            }

            SyncAwait(replica->CloseAndQuiesceReplica());

            VERIFY_IS_TRUE(replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn > 1000);
            
            replica->EndTest(false, true);
        }
    }

    //
    // Physical record chain today is released when head is truncated.
    // However, there was a bug wherein physical records newer than the head were pointing to physical records before the head causing leaks of records
    // KTL leak detection cannot catch this as the leaks go away as soon as the replica goes down
    // 
    // The exact repro was something like this and we will try to get this order in this test and verify that once the head truncates, all the older 
    // references need to go away
    /*
                                                        Current Log HEAD LSN -  359876338

        AnyRecord -             359943667

        Indexing -              359943603 

        Indexing -              359942434  

        Indexing -              359941445  

        Indexing -              359940285   

        Indexing -              359939156    

        Indexing -              359938165   

        Indexing -              359937384   

        Indexing -              359936406   
        ....
        Indexing -              359899984  

        TruncateHead -          359899176  --> Points to IndexingRecord - current HEAD at LSN 359876338(which has no links backwards)

        CompleteCheckpoint -    359899003

        EndCheckpoint -         359899003  --> Points to BeginCheckpoint and points to previous log head at 359852912

        BeginCheckpoint -       359899002  --> Earliest Pending = nullptr

        ...
        ...


        Indexing -              359877163   --> PREVIOUS POINTS TO HEAD LSN 359876338(which has no links backwards)
                                                Linked points to the truncatehead record below

        TruncateHead -          359875900  --> Points to log head record 359852912(which has no links backwards)
                                               Previous points to indexing log record LSN 359875336 
                                               Linked points to CC record with LSN 359853068


        Indexing -              359852912  ---> Previous Log Head
    */
    BOOST_AUTO_TEST_CASE(SlowLeak_PhysicalRecordChain)
    {
        TEST_TRACE_BEGIN("SlowLeak_PhysicalRecordChain")

        {
            InitializeTest();
            int opCount = 0;
            
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);
            
            // stores all the physical records in the vector. there can be repetetions but that is fine
            // It is used during verification to ensure none of the records earlier than the head are referencing other records
            vector<KWeakRef<PhysicalLogRecord>::SPtr> physicalLogRecords;
            physicalLogRecords.push_back(replica->ReplicatedLogManager->InnerLogManager->CurrentLastPhysicalRecord->GetWeakRef());

            replica->TestLogTruncationManager->SetIndex(TestLogTruncationManager::OperationProbability::Yes);

            // Commit 5 transactions
            {
                int txCount = 5;
                int addopCount = 5;
                opCount += addopCount * txCount;

                for (int i = 0; i < txCount; i++)
                {
                    Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);

                    for (int j = 0; j < addopCount; j++)
                    {
                        AddOperationToTransaction(*transaction, r, *replica->StateManager);
                        physicalLogRecords.push_back(replica->ReplicatedLogManager->InnerLogManager->CurrentLastPhysicalRecord->GetWeakRef());
                    }

                    SyncAwait(transaction->CommitAsync());
                    transaction->Dispose();
                }
                

                bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(opCount, opCount, Common::TimeSpan::FromSeconds(1));
                VERIFY_ARE_EQUAL(isProcessingComplete, true);
            }

            // at this point, the last indexing record must be the new head. we can stop capturing the physical records into the vector
            
            // We will do a checkpoint after all transactions
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
    
            // Add another 2 transactions
            {
                int txCount = 2;
                int addopCount = 2;
                opCount += addopCount * txCount;

                for (int i = 0; i < txCount; i++)
                {
                    Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);

                    for (int j = 0; j < addopCount; j++)
                    {
                        AddOperationToTransaction(*transaction, r, *replica->StateManager);
                    }

                    SyncAwait(transaction->CommitAsync());
                    transaction->Dispose();
                }

                bool isProcessingComplete = replica->StateManager->WaitForProcessingToComplete(opCount, opCount, Common::TimeSpan::FromSeconds(1));
                VERIFY_ARE_EQUAL(isProcessingComplete, true);
            }

            replica->TestLogTruncationManager->SetIsGoodLogHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);
            replica->TestLogTruncationManager->SetTruncateHead(TestLogTruncationManager::OperationProbability::YesOnceThenNo);

            // This transaction is to initiate the truncation
            // Ensure at least one additional checkpoint is performed after the truncation
            replica->TestLogTruncationManager->SetCheckpoint(TestLogTruncationManager::OperationProbability::NoOnceThenYes);

            {
                int txCount = 5;
                int addOpCount = 5;
                for (int i = 0; i < txCount; i++)
                {
                    Transaction::SPtr transaction2 = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);

                    for (int j = 0; j < addOpCount; j++)
                    {
                        AddOperationToTransaction(*transaction2, r, *replica->StateManager);
                    }

                    SyncAwait(transaction2->CommitAsync());
                    transaction2->Dispose();
                }
            }
            
            // This is to stabilize the truncation and ensure the truncate head was applied/processed
            {
                BarrierLogRecord::SPtr barrier;
                replica->CheckpointManager->ReplicateBarrier(barrier);
                SyncAwait(barrier->AwaitProcessing());
            }

            // This is to replace the last information record (Which was recovered record before the head) to something new
            // And ensure all activity is completed
            {
                SyncAwait(replica->ReplicatedLogManager->FlushInformationRecordAsync(InformationEvent::Enum::RestoredFromBackup, false, L"Test"));
                SyncAwait(replica->ReplicatedLogManager->LastInformationRecord->AwaitProcessing());
            }

            // We have verified the head is new
            VERIFY_IS_TRUE(replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn > 10);
    
            // VERIFICATION
            // Ensure all the physical records until the current head are destructed
            for (auto record : physicalLogRecords)
            {
                PhysicalLogRecord::SPtr logRecord = record->TryGetTarget();

                ASSERT_IF(
                    logRecord != nullptr && (logRecord->Psn <= replica->ReplicatedLogManager->CurrentLogHeadRecord->Psn),
                    "Physical record lsn: {0} psn: {1} was not destructed",
                    logRecord->Lsn, 
                    logRecord->Psn);
            }

            // Ensure all links are valid
            IndexingLogRecord::SPtr newHeadRecord = replica->ReplicatedLogManager->CurrentLogHeadRecord;
            PhysicalLogRecord::SPtr traverseRecord(newHeadRecord.RawPtr());
            PhysicalLogRecord::SPtr currentLastPhysicalRecord = replica->LogManager->CurrentLastPhysicalRecord;
            LONG64 refPsn;

            while (traverseRecord != currentLastPhysicalRecord)
            {
                refPsn = traverseRecord->PreviousPhysicalRecord != nullptr 
                    ? traverseRecord->PreviousPhysicalRecord->Psn 
                    : -1;

                // Confirm PreviousPhysicalRecord references are above the new head PSN, null, or invalid
                ASSERT_IFNOT(
                    traverseRecord->PreviousPhysicalRecord != nullptr||
                        VerifyInvalidRecordFields(*traverseRecord->PreviousPhysicalRecord) ||
                        traverseRecord->PreviousPhysicalRecord->Psn >= newHeadRecord->Psn,
                    "New head PSN : {0}. PreviousPhysicalRecord PSN: {1}",
                    newHeadRecord->Psn,
                    refPsn);

                refPsn = traverseRecord->LinkedPhysicalRecord != nullptr
                    ? traverseRecord->LinkedPhysicalRecord->Psn
                    : -1;

                // Confirm LinkedPhysicalRecord references are above the new head PSN, null, or invalid
                ASSERT_IFNOT(
                    traverseRecord->LinkedPhysicalRecord != nullptr ||
                        VerifyInvalidRecordFields(*traverseRecord->LinkedPhysicalRecord) ||
                        traverseRecord->LinkedPhysicalRecord->Psn >= newHeadRecord->Psn,
                    "New head PSN : {0}. LinkedPhysicalRecord PSN: {1}",
                    newHeadRecord->Psn,
                    refPsn);

                switch (traverseRecord->RecordType)
                {
                case LogRecordType::BeginCheckpoint:
                {
                    BeginCheckpointLogRecord::SPtr beginCheckpointTraverseRecord(dynamic_cast<BeginCheckpointLogRecord *>(traverseRecord.RawPtr()));

                    refPsn = beginCheckpointTraverseRecord->EarliestPendingTransaction != nullptr ?
                        beginCheckpointTraverseRecord->EarliestPendingTransaction->Psn : -1;

                    ASSERT_IFNOT(
                        beginCheckpointTraverseRecord->EarliestPendingTransaction == nullptr ||
                            VerifyInvalidRecordFields(*beginCheckpointTraverseRecord->EarliestPendingTransaction) ||
                            beginCheckpointTraverseRecord->EarliestPendingTransaction->Psn >= newHeadRecord->Psn,
                        "New head PSN : {0}. EarliestPendingTransaction PSN: {1}",
                        newHeadRecord->Psn,
                        refPsn);

                    break;
                }
                case LogRecordType::EndCheckpoint:
                {
                    EndCheckpointLogRecord::SPtr endCheckpointTraverseRecord(dynamic_cast<EndCheckpointLogRecord *>(traverseRecord.RawPtr()));

                    refPsn = endCheckpointTraverseRecord->LastCompletedBeginCheckpointRecord != nullptr ?
                        endCheckpointTraverseRecord->LastCompletedBeginCheckpointRecord->Psn : -1;

                    ASSERT_IFNOT(
                        endCheckpointTraverseRecord->LastCompletedBeginCheckpointRecord == nullptr ||
                            VerifyInvalidRecordFields(*endCheckpointTraverseRecord->LastCompletedBeginCheckpointRecord) ||
                            endCheckpointTraverseRecord->LastCompletedBeginCheckpointRecord->Psn >= newHeadRecord->Psn,
                        "New head PSN : {0}. LastCompletedBeginCheckpointRecord PSN: {1}",
                        newHeadRecord->Psn,
                        refPsn);

                    break;
                }
                case LogRecordType::CompleteCheckpoint:
                case LogRecordType::TruncateHead:
                {
                    LogHeadRecord::SPtr logHeadTraverseRecord(dynamic_cast<LogHeadRecord *>(traverseRecord.RawPtr()));
                    refPsn = logHeadTraverseRecord->HeadRecord != nullptr ?
                        logHeadTraverseRecord->HeadRecord->Psn : -1;

                    ASSERT_IFNOT(
                        logHeadTraverseRecord->HeadRecord == nullptr ||
                            VerifyInvalidRecordFields(*logHeadTraverseRecord->HeadRecord) ||
                            logHeadTraverseRecord->HeadRecord->Psn >= newHeadRecord->Psn,
                        "New head PSN : {0}. HeadRecord PSN: {1}",
                        newHeadRecord->Psn,
                        refPsn);

                    break;
                }
                default:
                    break;

                }

                // Continue to the next record
                traverseRecord = traverseRecord->NextPhysicalRecord;
            }

            SyncAwait(replica->CloseAndQuiesceReplica());
            replica->EndTest(true, true);
        }
    }

    //
    // Await LSN Ordering insert in replicated log manager had a bug where 
    // the thread which did the ordered insert never signalled the awaiter because 
    // it was checking the queue empty condition before removing items from the queue 
    // that just got inserted
    //
    // This is a unit test that verifies the bug is fixed
    //
    BOOST_AUTO_TEST_CASE(AwaitLsnOrderingTcsTest_OutOfOrderReplicates)
    {
        TEST_TRACE_BEGIN("AwaitLsnOrderingTcsTest_OutOfOrderReplicates")

        {
            InitializeTest();
            TestReplica::SPtr replica = TestReplica::Create(pId_, *invalidLogRecords_, true, nullptr, *apiFaultUtility_, allocator);
            primaryTestReplica_ = replica;
            replica->Initialize(seed);

            // Add the first 2 operations so that LSN 2 and 3 are invoked BeginReplicate
            {
                Transaction::SPtr transaction = Transaction::CreateTransaction(*replica->TestTransactionManager, allocator);

                // close the PLW so that no records are actually buffered
                // otherwise, it is hard to simulate this scenario
                replica->LogManager->PhysicalLogWriter->PrepareToClose();

                for (int i = 0; i < 2; i++)
                {
                    AddOperationToTransaction(*transaction, r, *replica->StateManager);
                }

                // Now LSN must be 4
                ASSERT_IFNOT(replica->StateReplicator->LSN == 4, "Lsn must be 4. It is {0}", replica->StateReplicator->LSN);

                // Replicate 7,6,5 and ensure Await LSN Ordering TCS is signalled once all of them complete

                // First replicate 7
                // After adding this operation, LSN assigned should be 7
                replica->StateReplicator->LSN = 6; // +1 is done during replicate
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                Awaitable<void> awaitLsnOrderingTcs = replica->ReplicatedLogManager->AwaitLsnOrderingTaskOnPrimaryAsync();
                Awaitable<void> awaitLsnOrderingTcs2;

                if ((r.Next() % 2) == 0)
                {
                    Trace.WriteInfo(
                        TraceComponent,
                        "{0}: Waiting in lsn ordering tcs for consecutive calls",
                        prId_->TraceId);

                    awaitLsnOrderingTcs2 = replica->ReplicatedLogManager->AwaitLsnOrderingTaskOnPrimaryAsync();
                    ASSERT_IF(awaitLsnOrderingTcs.IsComplete(), "LsnOrderingTcs is not expected to be complete");
                }

                ASSERT_IF(awaitLsnOrderingTcs.IsComplete(), "LsnOrderingTcs is not expected to be complete");

                replica->StateReplicator->LSN = 5;
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                replica->StateReplicator->LSN = 4;
                AddOperationToTransaction(*transaction, r, *replica->StateManager);

                SyncAwait(awaitLsnOrderingTcs);
                if (awaitLsnOrderingTcs2)
                {
                    SyncAwait(awaitLsnOrderingTcs2);
                }

                // Set the LSN back to the highest number
                replica->StateReplicator->LSN = 7;
                transaction->Dispose();
            }
            
            SyncAwait(replica->CloseAndQuiesceReplica());
            replica->EndTest(false, true);
        }
    }

#pragma endregion

    BOOST_AUTO_TEST_SUITE_END()
}
