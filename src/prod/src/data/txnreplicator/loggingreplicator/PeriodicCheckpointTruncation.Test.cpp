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
// Disable these tests until they are fixed
// RDBUG 13414908
#if 0
    using namespace std;
    using namespace ktl;
    using namespace Data::LogRecordLib;
    using namespace Data::LoggingReplicator;
    using namespace TxnReplicator;
    using namespace Data::Utilities;
    using namespace Common;

    StringLiteral const TraceComponent = "PeriodicCheckpointTruncationUnitTest";

    class PeriodicCheckpointTruncationUnitTests
    {
    public:
        TestReplica::SPtr primaryTestReplica_;

    protected:
        void EndTest();

        TRInternalSettingsSPtr GetInternalSettings()
        {
            TransactionalReplicatorSettingsUPtr tmp;
            ErrorCode e = TransactionalReplicatorSettings::FromPublicApi(publicSettings_, tmp);
            CODING_ERROR_ASSERT(e.IsSuccess());
            return TRInternalSettings::Create(move(tmp), make_shared<TransactionalReplicatorConfig>());
        }

        void InitializeTest(
            __in LONG64 periodicIntervalDurationSeconds = 2,
            __in bool resetSettings = true)
        {
            KAllocator & allocator = underlyingSystem_->NonPagedAllocator();
            invalidLogRecords_ = InvalidLogRecords::Create(allocator);
            apiFaultUtility_ = ApiFaultUtility::Create(allocator);

            // 2 seconds by default
            UpdateTruncationInterval(periodicIntervalDurationSeconds, resetSettings);

            primaryTestReplica_ = TestReplica::Create(
                pId_,
                *invalidLogRecords_,
                true,
                nullptr,
                *apiFaultUtility_,
                allocator);

            primaryTestReplica_->Initialize(
                GetTickCount(),
                false,
                true,
                false, // Use prod truncation manager
                &publicSettings_);

            ULONG mbToBytesMultiplier = 1024 * 1024;
            auto internalSettings = GetInternalSettings();
            checkpointIntervalBytes_ = internalSettings->CheckpointThresholdInMB * mbToBytesMultiplier;
        }

        void AddTransaction()
        {
            int opCount = 2;
            Transaction::SPtr tx = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, underlyingSystem_->NonPagedAllocator());

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(
                    *tx,
                    120,
                    100,
                    *primaryTestReplica_->StateManager);
            }

            // Commit tx1, initiating a checkpoint
            // Since an interval > PeriodicCheckpointTruncationInterval has passed, other configured values should be ignored
            SyncAwait(tx->CommitAsync());
            tx->Dispose();
        }

        void AddOperationToTransaction(
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

        void UpdateTruncationInterval(__in LONG64 interval, __in bool resetSettings = true)
        {
            if (resetSettings)
            {
                publicSettings_ = { 0 };
            }

            publicSettings_.LogTruncationIntervalSeconds = static_cast<DWORD>(interval);
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS;

            configuredInterval_ = TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds);
            periodicIntervalMs_ = publicSettings_.LogTruncationIntervalSeconds * 1000;
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
        
        // Verify the elapsed duration is >= 99% of configured interval
        void VerifyElapsedDuration(
            __in DateTime initialTime,
            __in DateTime finalTime)
        {
            TimeSpan diff = finalTime - initialTime;
            VERIFY_IS_TRUE((diff) >= configuredInterval_ || (diff.Ticks > (configuredInterval_.Ticks * 0.99)));
        }

        BeginCheckpointLogRecord::SPtr GetLastBeginCheckpointLogRecord()
        {
            BeginCheckpointLogRecord::SPtr beginCheckpoint = primaryTestReplica_->ReplicatedLogManager->LastInProgressCheckpointRecord;

            if (beginCheckpoint == nullptr)
            {
                beginCheckpoint = primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord;
            }

            VERIFY_IS_TRUE(beginCheckpoint != nullptr);
            return beginCheckpoint;
        }

        LONG64 GetBytesFromLogHead()
        {
            return static_cast<LONG64>(GetBytesUsed(*primaryTestReplica_, *primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord));
        }

        void VerifyTruncationState(
            __in Data::LoggingReplicator::PeriodicCheckpointTruncationState::Enum const & state,
            __in LONG64 timeoutSeconds = 1)
        {
            Common::DateTime startTime = DateTime::Now();
            Common::DateTime deadLine = startTime + TimeSpan::FromSeconds((double)timeoutSeconds);

            while (primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState != state)
            {
                if (DateTime::Now() > deadLine)
                {
                    break;
                }

                Sleep(100);
            }

            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == state);
        }

        KGuid pId_;
        ::FABRIC_REPLICA_ID rId_;
        PartitionedReplicaId::SPtr prId_;
        KtlSystem * underlyingSystem_;

        InvalidLogRecords::SPtr invalidLogRecords_;
        ApiFaultUtility::SPtr apiFaultUtility_;

        TRANSACTIONAL_REPLICATOR_SETTINGS publicSettings_;
        TimeSpan configuredInterval_;
        ULONG periodicIntervalMs_;

        LONG64 checkpointIntervalBytes_;
    };

    void PeriodicCheckpointTruncationUnitTests::EndTest()
    {
        prId_.Reset();
        invalidLogRecords_.Reset();
        apiFaultUtility_.Reset();

        if (primaryTestReplica_ != nullptr)
        {
            primaryTestReplica_.Reset();
        }
    }

    BOOST_FIXTURE_TEST_SUITE(PeriodicCheckpointTruncationUnitTestsSuite, PeriodicCheckpointTruncationUnitTests)
 
    BOOST_AUTO_TEST_CASE(Periodic_Timer_Duration_Test)
    {
        TEST_TRACE_BEGIN("Periodic_Timer_Duration_Test")
        {
            publicSettings_ = { 0 };
            publicSettings_.LogTruncationIntervalSeconds = 120;
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_LOG_TRUNCATION_INTERVAL_SECONDS;

            auto internalSettings = GetInternalSettings();

            TimeSpan configuredInterval = TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds);
            ULONG configuredIntervalMs = static_cast<ULONG>(configuredInterval.TotalMilliseconds());

            ULONG calculatedTimerDuration = 0;

            // Half the configured interval has passed since last periodic checkpoint, state is 'NotStarted'
            // Confirm target wait duration is lowered appropriately to ensure the callback is fired after 1 interval exactly
            DateTime lastPeriodicCheckpointTime = DateTime::Now() - (configuredInterval / 2);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::NotStarted);

            VERIFY_IS_TRUE(calculatedTimerDuration == (configuredIntervalMs / 2) || 
                          ((configuredIntervalMs / 2) - calculatedTimerDuration <= 10));

            // Configured interval has already passed and periodic checkpoint process has not started, confirm timer is expected to fire immediately
            lastPeriodicCheckpointTime = DateTime::Now() - TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds + 1);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::NotStarted);
            VERIFY_IS_TRUE(calculatedTimerDuration == 0);

            // Configured interval has already passed, but periodic checkpoint process has started
            // Confirm target wait duration equals configured interval
            lastPeriodicCheckpointTime = DateTime::Now() - TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds + 1);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::Ready);
            VERIFY_IS_TRUE(calculatedTimerDuration == configuredIntervalMs);

            lastPeriodicCheckpointTime = DateTime::Now() - TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds + 1);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::CheckpointStarted);
            VERIFY_IS_TRUE(calculatedTimerDuration == configuredIntervalMs);

            lastPeriodicCheckpointTime = DateTime::Now() - TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds + 1);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::CheckpointCompleted);
            VERIFY_IS_TRUE(calculatedTimerDuration == configuredIntervalMs);

            lastPeriodicCheckpointTime = DateTime::Now() - TimeSpan::FromSeconds(publicSettings_.LogTruncationIntervalSeconds + 1);
            calculatedTimerDuration = CheckpointManager::CalculateTruncationTimerDurationMs(lastPeriodicCheckpointTime, internalSettings, PeriodicCheckpointTruncationState::TruncationStarted);
            VERIFY_IS_TRUE(calculatedTimerDuration == configuredIntervalMs);
        }
    }

    //Test is temporarily disabled to figure out intermittent failures
    //RD Bug: 12924977
    /*BOOST_AUTO_TEST_CASE(Basic_Recovery_Validation)
    {
        TEST_TRACE_BEGIN("Basic_Recovery_Validation")
        {
            publicSettings_ = { 0 };
            publicSettings_.CheckpointThresholdInMB = 1;
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_CHECKPOINT_THRESHOLD_MB;
            publicSettings_.MinLogSizeInMB = 2;
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_MIN_LOG_SIZE_MB;
            publicSettings_.TruncationThresholdFactor = 10;
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_TRUNCATION_THRESHOLD_FACTOR;
            publicSettings_.ThrottlingThresholdFactor = 11;
            publicSettings_.Flags |= FABRIC_TRANSACTIONAL_REPLICATOR_THROTTLING_THRESHOLD_FACTOR;

            // Disable timer
            InitializeTest(0, false);

            IndexingLogRecord::SPtr initialLogHead = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord;

            // Confirm time has been initialized
            DateTime initialPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime initialPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            // VERIFY_IS_TRUE(initialPeriodicCheckpointTime == initialPeriodicTruncTime);

            int opCount = 240;
            Transaction::SPtr tx = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, underlyingSystem_->NonPagedAllocator());

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(
                    *tx,
                    120,
                    1024,
                    *primaryTestReplica_->StateManager);
            }

            SyncAwait(tx->CommitAsync());
            tx->Dispose();
            Sleep(4000);

            tx = Transaction::CreateTransaction(*primaryTestReplica_->TestTransactionManager, underlyingSystem_->NonPagedAllocator());

            for (int i = 0; i < opCount; i++)
            {
                AddOperationToTransaction(
                    *tx,
                    120,
                    1024,
                    *primaryTestReplica_->StateManager);
            }

            SyncAwait(tx->CommitAsync());
            tx->Dispose();

            SyncAwait(primaryTestReplica_->OperationProcessor->WaitForAllRecordsProcessingAsync());

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(false, false);

            // Re-initialize replica
            primaryTestReplica_->Initialize(
                seed,
                false,
                true,
                false, // Use prod truncation manager
                &publicSettings_);

            // Verify that timestamps are recovered appropriately
            // NOTE: Repro now functioning for broken case
            DateTime recoveredPeriodicCheckptTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime recoveredPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            VERIFY_IS_TRUE(recoveredPeriodicCheckptTime == initialPeriodicCheckpointTime);
            VERIFY_IS_TRUE(recoveredPeriodicTruncTime == initialPeriodicTruncTime);

            // Close replica and clean up test
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(true, true);
        }
    }*/

    BOOST_AUTO_TEST_CASE(Basic_LifeCycle)
    {
        TEST_TRACE_BEGIN("Basic_LifeCycle")
        {
            InitializeTest();

            IndexingLogRecord::SPtr initialLogHead = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord;

            // Confirm time has been initialized
            DateTime initialPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime initialPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            VERIFY_IS_TRUE(initialPeriodicCheckpointTime == initialPeriodicTruncTime);

            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Initial",
                initialPeriodicCheckpointTime,
                initialPeriodicCheckpointTime.Ticks,
                initialPeriodicTruncTime,
                initialPeriodicTruncTime.Ticks);

            // Commit tx1 immediately, initiating a checkpoint
            AddTransaction();

            // Delay 1 interval
            // Since an interval > PeriodicCheckpointTruncationInterval has passed, a checkpoint should be initiated immediately
             SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_, nullptr));

            // Normal checkpoints are only initiated if bytes written the last checkpoint are > checkpointIntervalBytes_
            // By default checkpointIntervalBytes_ = 52428800, but we have written less than that and still expect a checkpoint initiation
            LONG64 bytesUsed = GetBytesFromLogHead();
            VERIFY_IS_TRUE(bytesUsed < checkpointIntervalBytes_);

            BeginCheckpointLogRecord::SPtr beginCheckpoint = GetLastBeginCheckpointLogRecord();
            SyncAwait(beginCheckpoint->AwaitProcessing());

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primaryTestReplica_->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            // Commit another transaction, initiating head truncation
            // For a regular TruncateHead request "proposedHeadRecord pos - currentHeadRecord pos < minTruncation == true" prevents truncation            
            // minTruncationAmountInBytes = 1048576, proposedHeadRecord position = 370853, currentHeadRecord position = 0 --->  (370853 - 0 < 1048576) == true
            // PeriodicCheckpointTruncationIntervalSeconds ensures that a truncation is performed, ignoring the size of said truncation.
            AddTransaction();

            if (primaryTestReplica_->ReplicatedLogManager->LastInProgressTruncateHeadRecord != nullptr)
            {
                SyncAwait(primaryTestReplica_->ReplicatedLogManager->LastInProgressTruncateHeadRecord->AwaitProcessing());
            }

            SyncAwait(primaryTestReplica_->OperationProcessor->WaitForAllRecordsProcessingAsync());

            // Confirm truncation occurred
            auto initialHeadPsn = initialLogHead->Psn;
            auto newHeadPsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord->Psn;
            VERIFY_IS_TRUE(initialHeadPsn < newHeadPsn);

            // Confirm the timestamps have been updated
            DateTime finalPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime finalPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;

            VerifyElapsedDuration(initialPeriodicCheckpointTime, finalPeriodicCheckpointTime);
            VerifyElapsedDuration(initialPeriodicTruncTime, finalPeriodicTruncTime);
 
            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Final",
                finalPeriodicCheckpointTime,
                finalPeriodicCheckpointTime.Ticks,
                finalPeriodicTruncTime,
                finalPeriodicTruncTime.Ticks);

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());

            // Do not reset or clean up log
            primaryTestReplica_->EndTest(false, false);

            // Re-initialize replica with disabled config
            UpdateTruncationInterval(0);
            primaryTestReplica_->Initialize(
                seed,
                false,
                true,
                false, // Use prod truncation manager
                &publicSettings_);

            // Verify that timestamps are recovered appropriately
            DateTime recoveredPeriodicCheckptTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime recoveredPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;

            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Recovered",
                recoveredPeriodicCheckptTime,
                recoveredPeriodicCheckptTime.Ticks,
                recoveredPeriodicTruncTime,
                recoveredPeriodicTruncTime.Ticks);

            VERIFY_IS_TRUE(recoveredPeriodicCheckptTime == finalPeriodicCheckpointTime);
            VERIFY_IS_TRUE(recoveredPeriodicTruncTime == finalPeriodicTruncTime);

            // Close replica and clean up test
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(true, true);
        }
    }

    BOOST_AUTO_TEST_CASE(TruncationIncomplete_LifeCycle)
    {
        TEST_TRACE_BEGIN("TruncationIncomplete_LifeCycle")
        {
            InitializeTest();
            IndexingLogRecord::SPtr initialLogHead = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord;

            // Confirm time has been initialized
            DateTime initialPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime initialPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            VERIFY_IS_TRUE(initialPeriodicCheckpointTime == initialPeriodicTruncTime);

            BeginCheckpointLogRecord::SPtr initialBeginChkpt = primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord;

            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Initial",
                initialPeriodicCheckpointTime,
                initialPeriodicCheckpointTime.Ticks,
                initialPeriodicTruncTime,
                initialPeriodicTruncTime.Ticks);

            // Commit tx1, initiating a checkpoint
            // Since an interval > PeriodicCheckpointTruncationInterval has passed, other configured values should be ignored
            AddTransaction();

            // Delay 1 interval
            SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_, nullptr));

            // Normal checkpoints are only initiated if bytes written the last checkpoint are > checkpointIntervalBytes_
            // By default checkpointIntervalBytes_ = 52428800, but we have written less than that and still expect a checkpoint initiation
            LONG64 bytesUsed = static_cast<LONG64>(GetBytesUsed(*primaryTestReplica_, *primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord));
            VERIFY_IS_TRUE(bytesUsed < checkpointIntervalBytes_);

            BeginCheckpointLogRecord::SPtr beginCheckpoint = GetLastBeginCheckpointLogRecord();
            SyncAwait(beginCheckpoint->AwaitProcessing());

            VERIFY_ARE_EQUAL(beginCheckpoint->CheckpointState, CheckpointState::Enum::Completed);
            VERIFY_ARE_EQUAL(primaryTestReplica_->ReplicatedLogManager->LastInProgressCheckpointRecord, nullptr);

            // Confirm a new checkpoint has been issued
            VERIFY_IS_TRUE(primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord != initialBeginChkpt);
            VERIFY_IS_TRUE(primaryTestReplica_->ReplicatedLogManager->LastCompletedBeginCheckpointRecord == beginCheckpoint);

            // Confirm truncation has NOT occurred
            VERIFY_IS_TRUE(initialLogHead->Psn == primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord->Psn);

            // Confirm checkpoint HAS occurred and timestamp updated
            VerifyElapsedDuration(
                initialPeriodicCheckpointTime,
                primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime);

            LONG64 currentLogTailLsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogTailLsn;

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());

            // Do not reset or clean up log
            primaryTestReplica_->EndTest(false, false);

            // Delay 1 interval
            SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_, nullptr));

            // Re-initialize replica
            primaryTestReplica_->Initialize(
                seed,
                false,
                true,
                false, // Use prod truncation manager
                &publicSettings_);

            // Reset log tail LSN
            primaryTestReplica_->StateReplicator->LSN = currentLogTailLsn + 1;

            // Confirm the periodic checkpoint / truncation state has been recovered
            VerifyTruncationState(PeriodicCheckpointTruncationState::CheckpointCompleted);

            // Commit another transaction, initiating head truncation
            // For a regular TruncateHead request "proposedHeadRecord pos - currentHeadRecord pos < minTruncation == true" prevents truncation            
            // minTruncationAmountInBytes = 1048576, proposedHeadRecord position = 370853, currentHeadRecord position = 0 --->  (370853 - 0 < 1048576) == true
            // PeriodicCheckpointTruncationIntervalSeconds ensures that a truncation is performed, ignoring the size of said truncation.
            AddTransaction();

            if (primaryTestReplica_->ReplicatedLogManager->LastInProgressTruncateHeadRecord != nullptr)
            {
                SyncAwait(primaryTestReplica_->ReplicatedLogManager->LastInProgressTruncateHeadRecord->AwaitProcessing());
            }

            // Confirm truncation occurred
            VERIFY_IS_TRUE(initialLogHead->Psn < primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord->Psn);

            // Confirm truncation interval is expected
            VerifyElapsedDuration(
                initialPeriodicTruncTime,
                primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime);

            // Close replica and clean up test
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(true, true);
        }
    }

    BOOST_AUTO_TEST_CASE(CheckpointDuration_GreaterThan_ConfiguredInterval)
    {
        TEST_TRACE_BEGIN("CheckpointDuration_GreaterThan_ConfiguredInterval")
        {
            // Initialive shorter duration
            InitializeTest();

            IndexingLogRecord::SPtr initialLogHead = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord;

            // Confirm time has been initialized
            DateTime initialPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime initialPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            VERIFY_IS_TRUE(initialPeriodicCheckpointTime == initialPeriodicTruncTime);

            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Initial",
                initialPeriodicCheckpointTime,
                initialPeriodicCheckpointTime.Ticks,
                initialPeriodicTruncTime,
                initialPeriodicTruncTime.Ticks);

            // Block checkpoint
            primaryTestReplica_->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            AddTransaction();

            // Delay 2 intervals
            SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_ * 2, nullptr));

            BeginCheckpointLogRecord::SPtr beginCheckpoint = GetLastBeginCheckpointLogRecord();

            // Confirm that the state has not been overridden by subsequent intervals expiring
            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointStarted);

            primaryTestReplica_->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());

            // Confirm checkpoint was completed successfully
            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointCompleted);

            // Confirm duration between periodic checkpoints is valid
            VerifyElapsedDuration(
                initialPeriodicCheckpointTime,
                primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime);

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(true, true);
        }
    }

    BOOST_AUTO_TEST_CASE(CheckpointFailure_Recovery_Lifecycle)
    {
        TEST_TRACE_BEGIN("CheckpointFailure_Recovery_Lifecycle")
        {
            InitializeTest();

            IndexingLogRecord::SPtr initialLogHead = primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord;

            // Confirm time has been initialized
            DateTime initialPeriodicCheckpointTime = primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime;
            DateTime initialPeriodicTruncTime = primaryTestReplica_->CheckpointManager->LastPeriodicTruncationTime;
            VERIFY_IS_TRUE(initialPeriodicCheckpointTime == initialPeriodicTruncTime);

            Trace.WriteInfo(
                TraceComponent,
                "{0}: {1} | LastPeriodicCheckpointTime = ({2},ticks={3}), | LastPeriodicTruncationTime = ({4},ticks={5})",
                prId_->TraceId,
                "Initial",
                initialPeriodicCheckpointTime,
                initialPeriodicCheckpointTime.Ticks,
                initialPeriodicTruncTime,
                initialPeriodicTruncTime.Ticks);

            // Fail first checkpoint
            primaryTestReplica_->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);
            primaryTestReplica_->ApiFaultUtility->FailApi(ApiName::PerformCheckpoint, STATUS_INSUFFICIENT_POWER);

            // Commit a transaction
            // Once an interval > PeriodicCheckpointTruncationInterval has passed, other configured values should be ignored
            AddTransaction();

            // Delay 1 interval
            SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_, nullptr));

            // Normal checkpoints are only initiated if bytes written the last checkpoint are > checkpointIntervalBytes_
            // By default checkpointIntervalBytes_ = 52428800, but we have written less than that and still expect a checkpoint initiation
            LONG64 bytesUsed = static_cast<LONG64>(GetBytesUsed(*primaryTestReplica_, *primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord));
            VERIFY_IS_TRUE(bytesUsed < checkpointIntervalBytes_);
            BeginCheckpointLogRecord::SPtr beginCheckpoint = GetLastBeginCheckpointLogRecord();

            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointStarted);

            primaryTestReplica_->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());

            // Confirm checkpoint was reset
            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::Ready);

            LONG64 currentLogTailLsn = primaryTestReplica_->ReplicatedLogManager->CurrentLogTailLsn;

            primaryTestReplica_->ApiFaultUtility->ClearFault(ApiName::PerformCheckpoint);

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(false, false);

            // Re-initialize replica
            primaryTestReplica_->Initialize(
                seed,
                false,
                true,
                false, // Use prod truncation manager
                &publicSettings_);

            // Re-block PrepareCheckpoint
            primaryTestReplica_->ApiFaultUtility->BlockApi(ApiName::PrepareCheckpoint);

            // Reset log tail LSN
            primaryTestReplica_->StateReplicator->LSN = currentLogTailLsn + 1;

            // Confirm the LastPeriodicCheckpointTime was recovered, even though the checkpoint did not complete
            VERIFY_IS_TRUE(primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime == initialPeriodicCheckpointTime);

            // Delay 1 interval
            SyncAwait(KTimer::StartTimerAsync(allocator, 1, periodicIntervalMs_ * 2, nullptr));

            bytesUsed = static_cast<LONG64>(GetBytesUsed(*primaryTestReplica_, *primaryTestReplica_->ReplicatedLogManager->CurrentLogHeadRecord));
            VERIFY_IS_TRUE(bytesUsed < checkpointIntervalBytes_);

            beginCheckpoint = primaryTestReplica_->ReplicatedLogManager->LastInProgressCheckpointRecord;
            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointStarted);

            primaryTestReplica_->ApiFaultUtility->ClearFault(ApiName::PrepareCheckpoint);

            SyncAwait(beginCheckpoint->AwaitProcessing());

            // Confirm checkpoint completed
            VERIFY_IS_TRUE(primaryTestReplica_->LogTruncationManager->PeriodicCheckpointTruncationState == PeriodicCheckpointTruncationState::CheckpointCompleted);

            // Confirm duration between periodic checkpoints is valid
            VerifyElapsedDuration(
                initialPeriodicCheckpointTime,
                primaryTestReplica_->CheckpointManager->LastPeriodicCheckpointTime);

            // Close replica
            SyncAwait(primaryTestReplica_->CloseAndQuiesceReplica());
            primaryTestReplica_->EndTest(true, true);
        }
    }

    BOOST_AUTO_TEST_SUITE_END()
#endif
}
