// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        interface IOperationProcessor
        {
            K_SHARED_INTERFACE(IOperationProcessor);
            K_WEAKREF_INTERFACE(IOperationProcessor);

        public:

            virtual RecordProcessingMode::Enum IdentifyProcessingModeForRecord(__in LogRecordLib::LogRecord const & record) = 0;

            virtual ktl::Awaitable<void> ImmediatelyProcessRecordAsync(
                __in LogRecordLib::LogRecord & logRecord,
                __in NTSTATUS flushErrorCode,
                __in RecordProcessingMode::Enum processingMode) = 0;

            virtual void PrepareToProcessLogicalRecord() = 0;

            virtual void PrepareToProcessPhysicalRecord() = 0;

            virtual ktl::Awaitable<void> ProcessLoggedRecordAsync(__in LogRecordLib::LogRecord & logRecord) = 0;

            virtual void UpdateDispatchingBarrierTask(__in TxnReplicator::CompletionTask & barrierTask) = 0;

            virtual void Unlock(__in LogRecordLib::LogicalLogRecord & record) = 0;

            // Notification APIs
            virtual NTSTATUS RegisterTransactionChangeHandler(
                __in TxnReplicator::ITransactionChangeHandler & transactionChangeHandler) noexcept = 0;

            virtual NTSTATUS UnRegisterTransactionChangeHandler() noexcept = 0;
        };
    }
}
