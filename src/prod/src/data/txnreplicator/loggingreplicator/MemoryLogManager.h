// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class MemoryLogManager final
            : public LogManager
        {
            K_FORCE_SHARED(MemoryLogManager)

        public:
            static MemoryLogManager::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> CreateCopyLogAsync(
                __in TxnReplicator::Epoch const startingEpoch,
                __in LONG64 startingLsn,
                __out LogRecordLib::IndexingLogRecord::SPtr & newHead) override;

            ktl::Awaitable<NTSTATUS> DeleteLogAsync() override;

            ktl::Awaitable<NTSTATUS> RenameCopyLogAtomicallyAsync() override;

        protected:

            ktl::Awaitable<NTSTATUS> CreateLogFileAsync(
                __in bool createNew,
                __in ktl::CancellationToken const & cancellationToken,
                __out Data::Log::ILogicalLog::SPtr & result) override;

        private:
            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;

            MemoryLogManager(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

        };
    }
}