// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestLogRecords
        : public Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr>
        , public KObject<TestLogRecords>
        , public KShared<TestLogRecords>
    {
        K_FORCE_SHARED(TestLogRecords);
        K_SHARED_INTERFACE_IMP(IDisposable)
        K_SHARED_INTERFACE_IMP(IAsyncEnumerator)

    public: // Statics
        static SPtr Create(
            __in KAllocator & allocator);

    public: // Test APIs.

        ULONG GetCount() const;
        ULONG GetLogicalLogRecordCount() const;

        TxnReplicator::Epoch GetLastEpoch();
        FABRIC_SEQUENCE_NUMBER GetLSN();

        Data::LogRecordLib::IndexingLogRecord::SPtr GetLastIndexingLogRecord();
        Data::LogRecordLib::BackupLogRecord::SPtr GetLastBackupLogRecord();
        Data::LogRecordLib::LogRecord::SPtr GetLastLogRecord();

        void InitializeWithNewLogRecords();

        void PopulateWithRandomRecords(
            __in ULONG32 count,
            __in_opt bool includeBackup = true,
            __in_opt bool includeUpdateEpoch = false);

        void AddAtomicOperationLogRecord();

        void AddAtomicRedoOperationLogRecord();

        Data::LogRecordLib::BackupLogRecord::SPtr AddBackupLogRecord();

        void AddBackupLogRecord(
            __in Data::LogRecordLib::BackupLogRecord & backupLogRecord);

        void AddUpdateEpochLogRecord();

        Data::LogRecordLib::BarrierLogRecord::SPtr AddBarrierLogRecord();

        void Truncate(
            __in FABRIC_SEQUENCE_NUMBER lsn,
            __in bool inclusive);

        Data::LogRecordLib::ProgressVector::SPtr CopyProgressVector() const;

    public: // IAsyncEnumerator
        Data::LogRecordLib::LogRecord::SPtr GetCurrent();

        ktl::Awaitable<bool> MoveNextAsync(
            __in ktl::CancellationToken const & cancellationToken);

        void Reset();

    public: // IDisposable
        void Dispose();

    private:
        LONG32 currentIndex_ = -1;
        
        volatile LONG64 dataLossNumber_ = 0;
        volatile LONG64 configurationNumber_ = 0;
        volatile FABRIC_SEQUENCE_NUMBER currentLSN_ = 1;
        volatile LONG64 primaryReplicaId_ = 1;

        Data::LogRecordLib::ProgressVector::SPtr progressVectorSPtr_ = nullptr;
        Data::LogRecordLib::BackupLogRecord::SPtr lastBackupLogRecordSPtr_ = nullptr;
        Data::LogRecordLib::BarrierLogRecord::SPtr lastBarrierLogRecordSPtr_ = nullptr;
        Data::LogRecordLib::IndexingLogRecord::SPtr lastIndexingLogRecordSPtr_ = nullptr;
        Data::LogRecordLib::PhysicalLogRecord::SPtr lastPhysicalLogRecordSPtr = nullptr;

        Data::Utilities::OperationData::CSPtr mockPayLoad_;
        Data::LogRecordLib::InvalidLogRecords::SPtr const invalidLogRecords_;
        KArray<Data::LogRecordLib::LogRecord::SPtr> logRecords_;
        Common::Random random_;
    };
}
