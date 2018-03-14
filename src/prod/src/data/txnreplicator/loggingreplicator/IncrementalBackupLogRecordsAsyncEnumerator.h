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
        // An abstraction for an iterator over log records that are created from a valid physical log reader
        //
        class IncrementalBackupLogRecordsAsyncEnumerator
            : public Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>
            , public KObject<IncrementalBackupLogRecordsAsyncEnumerator>
            , public KShared<IncrementalBackupLogRecordsAsyncEnumerator>
        {

            K_FORCE_SHARED(IncrementalBackupLogRecordsAsyncEnumerator);
            K_SHARED_INTERFACE_IMP(IDisposable);
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator);   

        public: // Static creator
            static SPtr Create(
                __in Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr> & source,
                __in LogRecordLib::BackupLogRecord const & backupLogRecord,
                __in IReplicatedLogManager const & loggingReplicator,
                __in KAllocator & allocator);

        public: // Properties
            __declspec(property(get = get_StartingEpoch)) TxnReplicator::Epoch StartingEpoch;
            TxnReplicator::Epoch get_StartingEpoch() const;

            __declspec(property(get = get_StartingLSN)) FABRIC_SEQUENCE_NUMBER StartingLSN;
            FABRIC_SEQUENCE_NUMBER get_StartingLSN() const;

            __declspec(property(get = get_HighestBackedUpEpoch)) TxnReplicator::Epoch HighestBackedUpEpoch;
            TxnReplicator::Epoch get_HighestBackedUpEpoch() const;

            __declspec(property(get = get_HighestBackedUpLSN)) FABRIC_SEQUENCE_NUMBER HighestBackedUpLSN;
            FABRIC_SEQUENCE_NUMBER get_HighestBackedUpLSN() const;

        public: // IAsyncEnumerator
            LogRecordLib::LogRecord::SPtr GetCurrent() override;

            ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) override;

            void Reset() override;

            void Dispose() override;

        public:
            ULONG32 Count();

            ktl::Awaitable<void> VerifyDrainedAsync();

        private:
            NOFAIL IncrementalBackupLogRecordsAsyncEnumerator(
                __in Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr> & source,
                __in LogRecordLib::BackupLogRecord const & backupLogRecord,
                __in IReplicatedLogManager const & loggingReplicator) noexcept;

        private: // Initializer list initalized consts.
            const LogRecordLib::BackupLogRecord::CSPtr backupLogRecordCSPtr_;

        private: // Initializer list initialized non-const.
            Utilities::IAsyncEnumerator<LogRecordLib::LogRecord::SPtr>::SPtr sourceSPtr_;
            IReplicatedLogManager::CSPtr loggingReplicatorCSPtr_;
            
            bool isDisposed_;

            TxnReplicator::Epoch startingEpoch_;
            FABRIC_SEQUENCE_NUMBER startingLSN_;

            TxnReplicator::Epoch lastEpoch_;
            FABRIC_SEQUENCE_NUMBER lastLSN_;

            ULONG count_ = 0;
        };
    }
}
