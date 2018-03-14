// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestLogManager
        : public KObject<TestLogManager>
        , public KShared<TestLogManager>
        , public Data::LoggingReplicator::ILogManager
    {
        K_FORCE_SHARED(TestLogManager);
        K_SHARED_INTERFACE_IMP(ILogManager);
        K_SHARED_INTERFACE_IMP(ILogManagerReadOnly);

    public: // Statics
        static SPtr Create(
            __in KAllocator & allocator);

    public: // Test APIs
        void InitializeWithNewLogRecords();

        Data::LogRecordLib::IndexingLogRecord::SPtr GetLastIndexingLogRecord() const;

        Data::LogRecordLib::BackupLogRecord::SPtr AddBackupLogRecord();
        Data::LogRecordLib::BarrierLogRecord::SPtr AddBarrierLogRecord();

    public: // ILogManager Interface

        __declspec(property(get = get_CurrentLogTailRecord)) Data::LogRecordLib::LogRecord::SPtr CurrentLogTailRecord;
        Data::LogRecordLib::LogRecord::SPtr get_CurrentLogTailRecord() const override
        {
            return testLogRecords_->GetLastLogRecord();
        }

        __declspec(property(get = get_InvalidLogRecords)) Data::LogRecordLib::InvalidLogRecords::SPtr InvalidLogRecords;
        Data::LogRecordLib::InvalidLogRecords::SPtr get_InvalidLogRecords() const override
        {
            return nullptr;
        }

        ktl::Awaitable<void> FlushAsync(__in KStringView const & initiator) override;

        KSharedPtr<Data::LogRecordLib::IPhysicalLogReader> GetPhysicalLogReader(
            __in ULONG64 startingRecordPosition,
            __in ULONG64 endingRecordPosition,
            __in LONG64 startingLsn,
            __in KStringView const & readerName,
            __in Data::LogRecordLib::LogReaderType::Enum readerType) override;

        Data::Log::ILogicalLogReadStream::SPtr CreateReaderStream() override;

        void SetSequentialAccessReadSize(
            __in Data::Log::ILogicalLogReadStream & readStream,
            __in LONG64 sequentialReadSize) override;

        bool AddLogReader(
            __in LONG64 startingLsn,
            __in ULONG64 startingRecordPosition,
            __in ULONG64 endingRecordPosition,
            __in KStringView const & readerName,
            __in Data::LogRecordLib::LogReaderType::Enum readerType) override;

        void RemoveLogReader(__in ULONG64 startingRecordPosition) override;

    private:
        TestLogRecords::SPtr testLogRecords_;
        Data::LogRecordLib::LogRecord::SPtr tailRecord_;
    };
}
