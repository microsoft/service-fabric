// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestPhysicalLogReader
        : public KObject<TestPhysicalLogReader>
        , public KShared<TestPhysicalLogReader>
        , public Data::LogRecordLib::IPhysicalLogReader
    {
        K_FORCE_SHARED(TestPhysicalLogReader);

        K_SHARED_INTERFACE_IMP(IPhysicalLogReader);
        K_SHARED_INTERFACE_IMP(IDisposable);

    public: // Statics
        static SPtr Create(
            __in Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr> & logRecords,
            __in KAllocator & allocator);

    public: // IPhysicalLogReader
        __declspec(property(get = get_StartingRecordPosition)) ULONG64 StartingRecordPosition;
        ULONG64 get_StartingRecordPosition() const override;

        __declspec(property(get = get_EndingRecordPosition)) ULONG64 EndingRecordPosition;
        ULONG64 get_EndingRecordPosition() const override;

        __declspec(property(get = get_IsValid)) bool IsValid;
        bool get_IsValid() const override;

        Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr>::SPtr GetLogRecords(
            __in Data::Utilities::PartitionedReplicaId const & traceId,
            __in KStringView const & readerName,
            __in Data::LogRecordLib::LogReaderType::Enum readerType,
            __in LONG64 readAheadCacheSizeBytes,
            __in Reliability::ReplicationComponent::IReplicatorHealthClientSPtr const & healthClient,
            __in TxnReplicator::TRInternalSettingsSPtr const & transactionalReplicatorConfig) override;

    public: // IDisposable
        void Dispose() override;

    private:
        TestPhysicalLogReader(
            __in Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr> & logRecords);

    private:
        Data::Utilities::IAsyncEnumerator<Data::LogRecordLib::LogRecord::SPtr>::SPtr logRecords_;
        bool isDisposed_ = false;
    };
}
