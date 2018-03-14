// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestLogTruncationManager final
        : public Data::LoggingReplicator::ILogTruncationManager
        , public KObject<TestLogTruncationManager>
        , public KShared<TestLogTruncationManager>
    {
        K_FORCE_SHARED(TestLogTruncationManager)
        K_SHARED_INTERFACE_IMP(ILogTruncationManager)

    public:

        typedef enum OperationProbability
        {
            No,
            Yes,
            YesOnceThenRandom,
            YesOnceThenNo,
            NoOnceThenYes,
            Random
        }OperationProbability;

        static TestLogTruncationManager::SPtr Create(
            __in int seed,
            __in Data::LoggingReplicator::ReplicatedLogManager & replicatedLogManager,
            __in KAllocator & allocator);

        ktl::Awaitable<void> BlockSecondaryPumpIfNeeded(__in LONG64 lastStableLsn) override;

        bool ShouldBlockOperationsOnPrimary() override;

        KArray<Data::LogRecordLib::BeginTransactionOperationLogRecord::SPtr> GetOldTransactions(__in Data::LoggingReplicator::TransactionMap & transactionMap) override;

        bool ShouldIndex() override;

        bool ShouldCheckpointOnPrimary(
            __in Data::LoggingReplicator::TransactionMap & transactionMap,
            __out KArray<Data::LogRecordLib::BeginTransactionOperationLogRecord::SPtr> & abortTxList) override;

        bool ShouldCheckpointOnSecondary(__in Data::LoggingReplicator::TransactionMap & transactionMap) override;

        bool ShouldTruncateHead() override;

        Data::LoggingReplicator::ReplicatedLogManager::IsGoodLogHeadCandidateCalculator GetGoodLogHeadCandidateCalculator() override;

        void SetIsGoodLogHead(OperationProbability value);
        void SetCheckpoint(OperationProbability value);
        void SetIndex(OperationProbability value);
        void SetTruncateHead(OperationProbability value);

    private:

        TestLogTruncationManager(
            __in int seed,
            __in Data::LoggingReplicator::ReplicatedLogManager & replicatedLogManager);

        bool GetRandomBoolValue(OperationProbability & value);

        bool IsGoodLogHead(__in Data::LogRecordLib::IndexingLogRecord & indexingRecord);

        Common::Random rnd_;
        Data::LoggingReplicator::ReplicatedLogManager::SPtr const replicatedLogManager_;

        OperationProbability isGoodLogHead_;
        OperationProbability shouldCheckpoint_;
        OperationProbability shouldIndex_;
        OperationProbability shouldTruncateHead_;
    };
}
