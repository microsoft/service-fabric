// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LogRecordLib
    {
        //
        // An abstraction for an iterator over log records that are created from a valid physical log reader
        //
        class LogRecords
            : public Utilities::IAsyncEnumerator<LogRecord::SPtr>
            , public KObject<LogRecords>
            , public KShared<LogRecords>
        {

            K_FORCE_SHARED(LogRecords)
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator)

        public:

            static Utilities::IAsyncEnumerator<LogRecord::SPtr>::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in ILogManagerReadOnly & logManager,
                __in ULONG64 enumerationStartingPosition,
                __in ULONG64 enumerationEndingPosition,
                __in LONG64 enumerationStartedAtLsn,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType,
                __in LONG64 readAheadCacheSize,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            static LogRecords::SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLogReadStream & readStream,
                __in InvalidLogRecords & invalidLogRecords,
                __in ULONG64 enumerationStartingPosition,
                __in ULONG64 enumerationEndingPosition,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig,
                __in KAllocator & allocator);

            __declspec(property(get = get_LastPhysicalRecord)) PhysicalLogRecord::SPtr LastPhysicalRecord;
            PhysicalLogRecord::SPtr get_LastPhysicalRecord() const
            {
                return lastPhysicalRecord_;
            }

            LogRecord::SPtr GetCurrent() override
            {
                return currentRecord_;
            }

            void Dispose();

            ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) override;

            void Reset() override;

        private:

            LogRecords(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in ILogManagerReadOnly & logManager,
                __in ULONG64 enumerationStartingPosition,
                __in ULONG64 enumerationEndingPosition,
                __in LONG64 enumerationStartedAtLsn,
                __in LONG64 readAheadCacheSize,
                __in KStringView const & readerName,
                __in LogReaderType::Enum readerType,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            LogRecords(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Data::Log::ILogicalLogReadStream & readStream,
                __in InvalidLogRecords & invalidLogRecords,
                __in ULONG64 enumerationStartingPosition,
                __in ULONG64 enumerationEndingPosition,
                __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
                __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig);

            ktl::Task DisposeReadStream();

            bool isDisposed_;
            ILogManagerReadOnly::SPtr const logManager_;
            ULONG64 const enumerationEndingPosition_;
            KLocalString<Constants::LogReaderNameMaxLength> readerName_;
            LogReaderType::Enum const readerType_;
            Data::Log::ILogicalLogReadStream::SPtr const readStream_;
            
            ULONG64 enumerationStartingPosition_;
            LONG64 enumerationStartAtLsn_;
            LogRecord::SPtr currentRecord_;
            PhysicalLogRecord::SPtr lastPhysicalRecord_;

            InvalidLogRecords::SPtr invalidLogRecords_;
            TxnReplicator::IOMonitor::SPtr ioMonitor_;
            TxnReplicator::TRInternalSettingsSPtr transactionalReplicatorConfig_;
        };
    }
}
