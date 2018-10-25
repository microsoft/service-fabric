// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestOperationProcessor final 
        : public Data::LoggingReplicator::IOperationProcessor
        , public KObject<TestOperationProcessor>
        , public KShared<TestOperationProcessor>
        , public KWeakRefType<TestOperationProcessor>
    {
        K_FORCE_SHARED(TestOperationProcessor)
        K_SHARED_INTERFACE_IMP(IOperationProcessor)
        K_WEAKREF_INTERFACE_IMP(IOperationProcessor, TestOperationProcessor)

    public:

        static TestOperationProcessor::SPtr Create(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in int minDelay, 
            __in int maxDelay, 
            __in int seed,
            __in TxnReplicator::TRInternalSettingsSPtr const & config,
            __in KAllocator & allocator);

        static TestOperationProcessor::SPtr CreateWithParallelDispatcher(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in int minDelay, 
            __in int maxDelay, 
            __in int seed,
            __in TxnReplicator::TRInternalSettingsSPtr const & config,
            __in KAllocator & allocator);

        __declspec(property(get = get_LogRecordsDispatcher)) Data::LoggingReplicator::LogRecordsDispatcher & RecordsDispatcher;
        Data::LoggingReplicator::LogRecordsDispatcher & get_LogRecordsDispatcher()
        {
            return *recordsDispatcher_;
        }

        __declspec(property(get = get_GroupCommits)) KArray<TestGroupCommitValidationResult> & GroupCommits;
        KArray<TestGroupCommitValidationResult> & get_GroupCommits()
        {
            return groupCommits_;
        }

        __declspec(property(get = get_BarrierCount)) UINT BarrierCount;
        UINT get_BarrierCount() const
        {
            return barrierCount_;
        }

        __declspec(property(get = get_ImmediateCalledCount)) UINT ImmediateCalledCount;
        UINT get_ImmediateCalledCount() const
        {
            return immediateCalledCount_;
        }

        __declspec(property(get = get_NormalCalledCount)) UINT NormalCalledCount;
        UINT get_NormalCalledCount() const
        {
            return normalCalledCount_;
        }

        __declspec(property(get = get_ProcessingRecordsCount)) UINT ProcessingRecordsCount;
        UINT get_ProcessingRecordsCount() const
        {
            return processingRecordsCount_;
        }

        __declspec(property(get = get_ProcessedRecordsCount)) UINT ProcessedRecordsCount;
        UINT get_ProcessedRecordsCount() const
        {
            return processedRecordsCount_;
        }

        __declspec(property(get = get_UpdateDipsatchedBarrierTaskCount)) UINT UpdateDipsatchedBarrierTaskCount;
        UINT get_UpdateDipsatchedBarrierTaskCount() const
        {
            return updateDispatchedBarrierTaskCount_;
        }

        void Close();

        Data::LoggingReplicator::RecordProcessingMode::Enum IdentifyProcessingModeForRecord(__in Data::LogRecordLib::LogRecord const & logRecord) override;

        ktl::Awaitable<void> ImmediatelyProcessRecordAsync(
            __in Data::LogRecordLib::LogRecord & logRecord,
            __in NTSTATUS flushErrorCode,
            __in Data::LoggingReplicator::RecordProcessingMode::Enum processingMode) override;

        void PrepareToProcessLogicalRecord() override;

        void PrepareToProcessPhysicalRecord() override;

        ktl::Awaitable<void> ProcessLoggedRecordAsync(__in Data::LogRecordLib::LogRecord & logRecord) override;

        void UpdateDispatchingBarrierTask(__in TxnReplicator::CompletionTask & barrierTask) override;

        bool WaitForProcessingToComplete(
            __in UINT targetProcessedRecord, 
            __in Common::TimeSpan const & timeout);

        bool WaitForBarrierProcessingToComplete(
            __in UINT expectedBarrierCount, 
            __in Common::TimeSpan const & timeout);

        void Unlock(__in Data::LogRecordLib::LogicalLogRecord & record) override;

        // Notification APIs
        NTSTATUS RegisterTransactionChangeHandler(
            __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept override;

        NTSTATUS UnRegisterTransactionChangeHandler() noexcept override;

    private:

        TestOperationProcessor(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in int minDelay,
            __in int maxDelay,
            __in int seed,
            __in TxnReplicator::TRInternalSettingsSPtr const & config,
            __in bool useParallelDispatcher,
            __in KAllocator & allocator);

        Data::LoggingReplicator::LogRecordsDispatcher::SPtr recordsDispatcher_;
        KArray<TestGroupCommitValidationResult> groupCommits_;
        
        UINT barrierCount_;
        UINT immediateCalledCount_;
        UINT normalCalledCount_;
        UINT processingRecordsCount_;
        UINT processedRecordsCount_;
        UINT updateDispatchedBarrierTaskCount_;

        Common::Random random_;
        int minDelay_;
        int maxDelay_;
    };
}
