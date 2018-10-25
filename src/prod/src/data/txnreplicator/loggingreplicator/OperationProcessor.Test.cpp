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
    
    #define TESTOPPROCESSOR_TAG 'PpOT'
    StringLiteral const TraceComponent = "OperationProcessorTest";

    class OperationProcessorTests 
    {
    protected:
        OperationProcessorTests() 
        {
            CommonConfig config; // load the config object as its needed for the tracing to work
        }
        
        void InitializeTest(__in int seed, __in bool recoveryCompleted);
        void EndTest();

        static void AddExpectedOperationData(
            __in TestStateProviderManager & testStateManager,
            __in ULONG bufferCount,
            __in ULONG bufferSize,
            __in ULONG numberOfOperations,
            __in LONG64 transactionId,
            __in ULONG exceptionIndex,
            __in KAllocator & allocator);

        static void AddExpectedOperationData(
            __in TestStateProviderManager & testStateManager,
            __in ULONG bufferCount,
            __in ULONG bufferSize,
            __in ULONG numberOfOperations,
            __in LONG64 transactionId,
            __in KAllocator & allocator);

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;

        InvalidLogRecords::SPtr invalidLogRecords_;
        RecoveredOrCopiedCheckpointState::SPtr recoveredOrCopiedCheckpointState_;
        RoleContextDrainState::SPtr roleContextDrainState_;
        TestStateProviderManager::SPtr testStateManager_;
        TestLoggingReplicatorToVersionManager::SPtr versionManager_;
        TestCheckpointManager::SPtr testCheckpointManager_;
        OperationProcessor::SPtr operationProcessor_;
        LogRecordsDispatcher::SPtr recordsDispatcher_;
        TestStatefulServicePartition::SPtr testPartition_;
        ApiFaultUtility::SPtr apiFaultUtility_;
    };

    void OperationProcessorTests::EndTest()
    {
        prId_.Reset();
        invalidLogRecords_.Reset();
        recoveredOrCopiedCheckpointState_.Reset();
        roleContextDrainState_.Reset();
        testStateManager_.Reset();
        versionManager_.Reset();
        testCheckpointManager_.Reset();
        operationProcessor_.Reset();
        recordsDispatcher_.Reset();
        testPartition_.Reset();
        apiFaultUtility_.Reset();
    }

    void OperationProcessorTests::InitializeTest(__in int seed, __in bool recoveryCompleted)
    {
        UNREFERENCED_PARAMETER(seed);

        KAllocator & allocator = underlyingSystem_->NonPagedAllocator();

        apiFaultUtility_ = ApiFaultUtility::Create(allocator);
        apiFaultUtility_->DelayApi(ApiName::ReplicateAsync, Common::TimeSpan::FromMilliseconds(1));
        apiFaultUtility_->DelayApi(ApiName::PrepareCheckpoint, Common::TimeSpan::FromMilliseconds(20));
        apiFaultUtility_->DelayApi(ApiName::PerformCheckpoint, Common::TimeSpan::FromMilliseconds(20));
        apiFaultUtility_->DelayApi(ApiName::CompleteCheckpoint, Common::TimeSpan::FromMilliseconds(20));

        testPartition_ = TestStatefulServicePartition::Create(allocator);
        invalidLogRecords_ = InvalidLogRecords::Create(allocator);
        recoveredOrCopiedCheckpointState_ = RecoveredOrCopiedCheckpointState::Create(allocator);
        recoveredOrCopiedCheckpointState_->Update(0);

        roleContextDrainState_ = RoleContextDrainState::Create(*testPartition_, allocator);
        testStateManager_ = TestStateProviderManager::Create(*apiFaultUtility_, allocator);
        versionManager_ = TestLoggingReplicatorToVersionManager::Create(allocator);
        testCheckpointManager_ = TestCheckpointManager::Create(allocator);

        TestLogManager::SPtr testLogManager = TestLogManager::Create(allocator);
        testLogManager->InitializeWithNewLogRecords();
        TestReplicatedLogManager::SPtr testReplicatedLogManager = TestReplicatedLogManager::Create(allocator);

        std::wstring currentDirectory = Common::Directory::GetCurrentDirectoryW();
        KString::SPtr mockWorkFolder = Data::Utilities::KPath::Combine(currentDirectory.c_str(), L"OperationProcessorTests", allocator);
        TRInternalSettingsSPtr settings = TRInternalSettings::Create(nullptr, make_shared<TransactionalReplicatorConfig>());
        TestHealthClientSPtr healthClient = TestHealthClient::Create();
        TestTransactionReplicator::SPtr txnReplicator = TestTransactionReplicator::Create(allocator);

        BackupManager::SPtr backupManager = BackupManager::Create(
            *prId_,
            *mockWorkFolder,
            *testStateManager_,
            *testReplicatedLogManager,
            *testLogManager,
            settings,
            healthClient,
            *invalidLogRecords_,
            allocator);

        operationProcessor_ = OperationProcessor::Create(
            *prId_, 
            *recoveredOrCopiedCheckpointState_,
            *roleContextDrainState_,
            *versionManager_,
            *testCheckpointManager_,
            *testStateManager_,
            *backupManager,
            *invalidLogRecords_,
            settings,
            *txnReplicator,
            allocator);
       
        if (Common::DateTime::Now().Ticks % 2 == 0)
        {
            recordsDispatcher_ = SerialLogRecordsDispatcher::Create(*prId_, *operationProcessor_, settings, allocator);
        }
        else 
        {
            recordsDispatcher_ = ParallelLogRecordsDispatcher::Create(*prId_, *operationProcessor_, settings, allocator);
        }

        roleContextDrainState_->Reuse();
        roleContextDrainState_->OnRecovery();

        if (recoveryCompleted)
        {
            roleContextDrainState_->OnRecoveryCompleted();
            roleContextDrainState_->ChangeRole(FABRIC_REPLICA_ROLE_PRIMARY);
        }
    }

    void OperationProcessorTests::AddExpectedOperationData(
        __in TestStateProviderManager & testStateManager,
        __in ULONG numberOfOperations,
        __in ULONG bufferCount,
        __in ULONG bufferSize,
        __in LONG64 transactionId,
        __in ULONG exceptionIndex,
        __in KAllocator & allocator)
    {
        KSharedArray<Data::Utilities::OperationData::CSPtr>::SPtr metaData = _new(TESTOPPROCESSOR_TAG, allocator)KSharedArray<Data::Utilities::OperationData::CSPtr>();
        KSharedArray<Data::Utilities::OperationData::CSPtr>::SPtr applyData = _new(TESTOPPROCESSOR_TAG, allocator)KSharedArray<Data::Utilities::OperationData::CSPtr>();

        ULONG opCount = numberOfOperations + 1; // Account for the first operation which is part of begintx itself. this count is 1 more than the actual number of operations

        for (ULONG i = 0; i < opCount; i++)
        {
            OperationData::SPtr opData = OperationData::Create(allocator);

            for (ULONG j = 0; j < bufferCount; j++)
            {
                KBuffer::SPtr buffer;
                NTSTATUS status = KBuffer::Create(
                    bufferSize,
                    buffer,
                    allocator);

                CODING_ERROR_ASSERT(status == STATUS_SUCCESS);

                char * bufferContent = (char *) buffer->GetBuffer();

                for (ULONG index = 0; index < bufferSize; index++)
                {
                    *(bufferContent + index) = (char)j % TestTransaction::BufferContentMaxValue; 
                }

                opData->Append(*buffer);
            }

            metaData->Append(opData.RawPtr());
            applyData->Append(opData.RawPtr());
        }

        testStateManager.AddExpectedTransactionExceptionAtNthApply(transactionId, exceptionIndex, *metaData, *applyData);
    }

    void OperationProcessorTests::AddExpectedOperationData(
        __in TestStateProviderManager & testStateManager,
        __in ULONG numberOfOperations,
        __in ULONG bufferCount,
        __in ULONG bufferSize,
        __in LONG64 transactionId,
        __in KAllocator & allocator)
    {
        AddExpectedOperationData(
            testStateManager,
            numberOfOperations,
            bufferCount,
            bufferSize,
            transactionId,
            MAXULONG,
            allocator);
    }

    BOOST_FIXTURE_TEST_SUITE(OperationProcessorTestsSuite, OperationProcessorTests)

    BOOST_AUTO_TEST_CASE(OneCommittedTx_VerifyApplyDataContentsAndOrder)
    {
        TEST_TRACE_BEGIN("OneCommittedTx_VerifyApplyDataContentsAndOrder")

        {
            InitializeTest(seed, true);
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;
            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, true, allocator));
            AddExpectedOperationData(*testStateManager_, 2, 2, 20, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(3, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ManyCommittedTx_VerifyApplyDataContentsAndOrder)
    {
        TEST_TRACE_BEGIN("ManyCommittedTx_VerifyApplyDataContentsAndOrder")

        {
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(0, 0, 0, true, allocator));
            AddExpectedOperationData(*testStateManager_, 0, 0, 0, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(0, 5, 0, true, allocator));
            AddExpectedOperationData(*testStateManager_, 0, 5, 0, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(0, 1, 10, true, allocator));
            AddExpectedOperationData(*testStateManager_, 0, 1, 10, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(1, 1, 10, true, allocator));
            AddExpectedOperationData(*testStateManager_, 1, 1, 10, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, true, allocator));
            AddExpectedOperationData(*testStateManager_, 2, 2, 20, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(3, 3, 30, true, allocator));
            AddExpectedOperationData(*testStateManager_, 3, 3, 30, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(4, 4, 40, true, allocator));
            AddExpectedOperationData(*testStateManager_, 4, 4, 40, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(5, 5, 50, true, allocator));
            AddExpectedOperationData(*testStateManager_, 5, 5, 50, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(6, 6, 0, true, allocator));
            AddExpectedOperationData(*testStateManager_, 6, 6, 0, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(7, 0, 0, true, allocator));
            AddExpectedOperationData(*testStateManager_, 7, 0, 0, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 10, 0, 1, 3, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(38, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(ManyPendingTx_VerifyNoApplyIsInvoked)
    {
        TEST_TRACE_BEGIN("ManyPendingTx_VerifyNoApplyIsInvoked")

        {
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(1, 1, 10, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(3, 3, 30, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(4, 4, 40, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(5, 5, 50, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(6, 6, 0, false, allocator));
            testTxList.Append(TestTransactionGenerator::Create(7, 0, 0, false, allocator));

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 0, 0, 1, 3, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);
            
            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(0, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(RecoveredLsnLargerThanCommittedTxLsn_VerifyNoAppliesAreInvoked)
    {
        TEST_TRACE_BEGIN("RecoveredLsnLargerThanCommittedTxLsn_VerifyNoAppliesAreInvoked")

        {
            InitializeTest(seed, true);

            recoveredOrCopiedCheckpointState_->Update(9999);
            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;
            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, true, allocator));
            AddExpectedOperationData(*testStateManager_, 2, 2, 20, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);
            
            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(0, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(IdentifyProcessingModeForBarrierAndBeginCheckpoint_ApplyImmediatelyForPrimaryRole)
    {
        TEST_TRACE_BEGIN("IdentifyProcessingModeForBarrierAndBeginCheckpoint_ApplyImmediatelyForPrimaryRole")

        {
            InitializeTest(seed, true);

            BarrierLogRecord::SPtr barrier = BarrierLogRecord::CreateOneBarrierRecord(*invalidLogRecords_->Inv_PhysicalLogRecord, allocator);
            RecordProcessingMode::Enum mode = operationProcessor_->IdentifyProcessingModeForRecord(*barrier);

            VERIFY_ARE_EQUAL(mode, RecordProcessingMode::Enum::ApplyImmediately);

            BeginCheckpointLogRecord::SPtr beginCheckpoint = BeginCheckpointLogRecord::CreateZeroBeginCheckpointLogRecord(*invalidLogRecords_->Inv_PhysicalLogRecord, *invalidLogRecords_->Inv_BeginTransactionOperationLogRecord, allocator);
            beginCheckpoint->Lsn = 10;
            mode = operationProcessor_->IdentifyProcessingModeForRecord(*beginCheckpoint);

            VERIFY_ARE_EQUAL(mode, RecordProcessingMode::Enum::ApplyImmediately);
        }
    }

    BOOST_AUTO_TEST_CASE(IdentifyProcessingModeForBarrierAndBeginCheckpoint_ProcessImmediatelyForUnknownRole)
    {
        TEST_TRACE_BEGIN("IdentifyProcessingModeForBarrierAndBeginCheckpoint_ProcessImmediatelyForUnknownRole")

        {
            InitializeTest(seed, false);

            BarrierLogRecord::SPtr barrier = BarrierLogRecord::CreateOneBarrierRecord(*invalidLogRecords_->Inv_PhysicalLogRecord, allocator);
            RecordProcessingMode::Enum mode = operationProcessor_->IdentifyProcessingModeForRecord(*barrier);

            VERIFY_ARE_EQUAL(mode, RecordProcessingMode::Enum::ProcessImmediately);

            BeginCheckpointLogRecord::SPtr beginCheckpoint = BeginCheckpointLogRecord::CreateZeroBeginCheckpointLogRecord(*invalidLogRecords_->Inv_PhysicalLogRecord, *invalidLogRecords_->Inv_BeginTransactionOperationLogRecord, allocator);
            beginCheckpoint->Lsn = 10;
            mode = operationProcessor_->IdentifyProcessingModeForRecord(*beginCheckpoint);

            VERIFY_ARE_EQUAL(mode, RecordProcessingMode::Enum::ProcessImmediately);
        }
    }

    BOOST_AUTO_TEST_CASE(FailedFlush_VerifyApplyIsNotInvoked)
    {
        TEST_TRACE_BEGIN("FailedFlush_VerifyApplyIsNotInvoked")

        {
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(1, 1, 10, true, SF_STATUS_OBJECT_CLOSED, allocator));
            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            LoggedRecords::CSPtr loggedRecords = LoggedRecords::Create(txnRecords, SF_STATUS_OBJECT_CLOSED, allocator);
            operationProcessor_->ProcessLogError(L"Test", *testTxList[0]->LogRecords[0], SF_STATUS_OBJECT_CLOSED);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(0, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(FailedFlushAfterSuccessfulFlushes_VerifyApplyCount)
    {
        TEST_TRACE_BEGIN("FailedFlushAfterSuccessfulFlushes_VerifyApplyCount")

        {
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(1, 1, 10, true, allocator));
            AddExpectedOperationData(*testStateManager_, 1, 1, 10, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, true, allocator));
            AddExpectedOperationData(*testStateManager_, 2, 2, 20, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(3, 3, 30, true, allocator));
            AddExpectedOperationData(*testStateManager_, 3, 3, 30, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            testTxList.Append(TestTransactionGenerator::Create(4, 4, 40, true, allocator));
            AddExpectedOperationData(*testStateManager_, 4, 4, 40, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 4, 0, 0, 0, seed, allocator));
            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);
            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);
            
            // This Transaction has flush exception - NOTE This is a hypothetical condition as in reality, the barrier inserted here cannot be successfully flushed
            KArray<TestTransaction::SPtr> testTxListFlushException(allocator);
            testTxListFlushException.Append(TestTransactionGenerator::Create(5, 5, 50, true, SF_STATUS_OBJECT_CLOSED, allocator));
            KArray<LogRecord::SPtr> txnRecordsFlushException = TestTransactionGenerator::InterleaveTransactions(testTxListFlushException, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecordsFlushException, 1, 0, 0, 0, seed, allocator));
            operationProcessor_->ProcessLogError(L"Test", *testTxListFlushException[0]->LogRecords[0], SF_STATUS_OBJECT_CLOSED);
            LoggedRecords::CSPtr loggedRecordsWithFlushException = LoggedRecords::Create(txnRecordsFlushException, SF_STATUS_OBJECT_CLOSED, allocator);
            recordsDispatcher_->DispatchLoggedRecords(*loggedRecordsWithFlushException);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(14, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(FailedApply_VerifyNoMoreApplies)
    {
        TEST_TRACE_BEGIN("FailedApply_VerifyNoMoreApplies")

        { 
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;

            testTxList.Append(TestTransactionGenerator::Create(5, 5, 50, true, STATUS_SUCCESS, allocator));
            AddExpectedOperationData(*testStateManager_, 5, 5, 50, testTxList[testTxList.Count() - 1]->Tx->TransactionId, 3, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);
            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(3, 0, Common::TimeSpan::FromSeconds(2));

            VERIFY_ARE_EQUAL(isProcessingComplete, true);
        }
    }

    BOOST_AUTO_TEST_CASE(OneCommittedTx_VerifyWaitForLogicalRecordsProcessing)
    {
        TEST_TRACE_BEGIN("OneCommittedTx_VerifyWaitForLogicalRecordsProcessing")

        {
            InitializeTest(seed, true);

            KArray<TestTransaction::SPtr> testTxList(allocator);
            KArray<TestGroupCommitValidationResult> expectedResults(allocator);

            LONG64 lsn = 1;
            testTxList.Append(TestTransactionGenerator::Create(2, 2, 20, true, allocator));
            AddExpectedOperationData(*testStateManager_, 2, 2, 20, testTxList[testTxList.Count() - 1]->Tx->TransactionId, allocator);

            KArray<LogRecord::SPtr> txnRecords = TestTransactionGenerator::InterleaveTransactions(testTxList, lsn, seed, allocator, lsn);

            expectedResults.Append(TestTransactionGenerator::InsertBarrier(txnRecords, 1, 0, 0, 0, seed, allocator));

            LoggedRecords::SPtr loggedRecords = LoggedRecords::Create(txnRecords, allocator);

            recordsDispatcher_->DispatchLoggedRecords(*loggedRecords);

            bool isProcessingComplete = testStateManager_->WaitForProcessingToComplete(3, 0, Common::TimeSpan::FromSeconds(2));
            VERIFY_ARE_EQUAL(isProcessingComplete, true);

            Common::Threadpool::Post([this, loggedRecords]
            {
                for (ULONG i = 0; i < 5; i++)
                {
                    LogicalLogRecord::SPtr logicalRecord = dynamic_cast<LogicalLogRecord *>((*loggedRecords)[i].RawPtr());
                    this->operationProcessor_->ProcessedLogicalRecord(*logicalRecord);
                }
            },
                Common::TimeSpan::FromSeconds(2));

            Awaitable<void> waitForLogicalProcessing = operationProcessor_->WaitForLogicalRecordsProcessingAsync();
            Awaitable<void> waitForPhysicalProcessing = operationProcessor_->WaitForPhysicalRecordsProcessingAsync();

            SyncAwait(waitForPhysicalProcessing); // This must complete immediately
            SyncAwait(waitForLogicalProcessing); // This must complete after all the records are processed

            Awaitable<void> waitForAllProcessing = operationProcessor_->WaitForAllRecordsProcessingAsync();
            SyncAwait(waitForAllProcessing); // This must complete after all the records are processed
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
}
