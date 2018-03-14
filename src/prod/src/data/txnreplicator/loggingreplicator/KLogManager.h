// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------


#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // 
        // KTL Log Manager class
        // Handles TransactionalReplicator communication with KTLLog
        //
        class KLogManager final 
            : public LogManager
        {
            K_FORCE_SHARED(KLogManager)

        public:

            static KLogManager::SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders const & runtimeFolders,
                __in Data::Log::LogManager & appHostLogManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in KAllocator & allocator);

            ktl::Awaitable<NTSTATUS> CreateCopyLogAsync(
                __in TxnReplicator::Epoch const startingEpoch,
                __in LONG64 startingLsn,
                __out LogRecordLib::IndexingLogRecord::SPtr & newHead) override;

            ktl::Awaitable<NTSTATUS> DeleteLogAsync() override;

            ktl::Awaitable<NTSTATUS> RenameCopyLogAtomicallyAsync() override;

            ktl::Awaitable<NTSTATUS> InitializeAsync(__in KString const & dedicatedLogPath) override;

            void Dispose() override;

            __declspec(property(get = get_PhysicalLogHandle)) Data::Log::IPhysicalLogHandle::SPtr & PhysicalLogHandle;
            Data::Log::IPhysicalLogHandle::SPtr & get_PhysicalLogHandle()
            {
                return physicalLogHandle_;
            }

        protected:

            ktl::Awaitable<NTSTATUS> CreateLogFileAsync(
                __in bool createNew,
                __in ktl::CancellationToken const & cancellationToken,
                __out Data::Log::ILogicalLog::SPtr & result) override;

        private:

            static NTSTATUS StringToGuid(
                __in std::wstring const & string,
                __out KGuid& guid
                );

            static Common::Guid ToCommonGuid(KGuid & g)
            {
                return Common::Guid(g);
            }

            KLogManager(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in TxnReplicator::IRuntimeFolders const & runtimeFolders,
                __in Data::Log::LogManager & appHostLogManager,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in TxnReplicator::SLInternalSettingsSPtr const & ktlLoggerSharedLogConfig,
                __in TxnReplicator::TRPerformanceCountersSPtr const & perfCounters,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient);

            ktl::Awaitable<void> DeleteLogFileAsync(
                __in KString const & alias,
                __in ktl::CancellationToken & cancellationToken);

            ktl::Awaitable<NTSTATUS> OpenPhysicalLogHandleAsync();

            void InitializeLogSuffix();

            void DisposeHandles() noexcept;

            TxnReplicator::IRuntimeFolders::CSPtr runtimeFolders_;

            Data::Log::LogManager::SPtr dataLogManager_;
            Data::Log::ILogManagerHandle::SPtr dataLogManagerHandle_;
            Data::Log::IPhysicalLogHandle::SPtr physicalLogHandle_;

            KString::SPtr logicalLogFileSuffix_;

            // Pointer to a config object shared throughout this replicator instance
            TxnReplicator::TRInternalSettingsSPtr const transactionalReplicatorConfig_;
            TxnReplicator::SLInternalSettingsSPtr const ktlLoggerSharedLogConfig_;

            TxnReplicator::TRPerformanceCountersSPtr const perfCounters_;
            Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const healthClient_;
        };
    }
}
