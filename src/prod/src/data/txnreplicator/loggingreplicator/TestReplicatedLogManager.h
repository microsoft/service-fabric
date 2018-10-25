// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace LoggingReplicatorTests
{
    class TestReplicatedLogManager
        : public KObject<TestReplicatedLogManager>
        , public KShared<TestReplicatedLogManager>
        , public Data::LoggingReplicator::IReplicatedLogManager
    {
        K_FORCE_SHARED(TestReplicatedLogManager);
        K_SHARED_INTERFACE_IMP(IReplicatedLogManager);

    public: // Statics
        static SPtr Create(
            __in KAllocator& allocator,
            __in_opt Data::LogRecordLib::IndexingLogRecord const * const indexingLogRecord = nullptr) noexcept;

    public: // Test APIs
        void SetProgressVector(
            __in Data::LogRecordLib::ProgressVector::SPtr && entry);

    public: // IReplicatedLogManager APIs
        __declspec(property(get = get_CurrentLogTailLsn)) FABRIC_SEQUENCE_NUMBER CurrentLogTailLsn;
        FABRIC_SEQUENCE_NUMBER get_CurrentLogTailLsn() const override;

        __declspec(property(get = get_CurrentLogTailEpoch)) TxnReplicator::Epoch CurrentLogTailEpoch;
        TxnReplicator::Epoch get_CurrentLogTailEpoch() const override;

        __declspec(property(get = get_LastInformationRecord)) Data::LogRecordLib::InformationLogRecord::SPtr LastInformationRecord;
        Data::LogRecordLib::InformationLogRecord::SPtr get_LastInformationRecord() const override;

        __declspec(property(get = get_LastCompletedEndCheckpointRecord)) Data::LogRecordLib::EndCheckpointLogRecord::SPtr LastCompletedEndCheckpointRecord;
        Data::LogRecordLib::EndCheckpointLogRecord::SPtr get_LastCompletedEndCheckpointRecord() const override;

        __declspec(property(get = get_ProgressVectorValue)) Data::LogRecordLib::ProgressVector::SPtr ProgressVectorValue;
        Data::LogRecordLib::ProgressVector::SPtr get_ProgressVectorValue() const override;

        TxnReplicator::Epoch GetEpoch(
            __in FABRIC_SEQUENCE_NUMBER LSN) const override;

        Data::LogRecordLib::IndexingLogRecord::CSPtr TestReplicatedLogManager::GetIndexingLogRecordForBackup() const override;

        NTSTATUS ReplicateAndLog(
            __in Data::LogRecordLib::LogicalLogRecord & record,
            __out LONG64 & bufferedBytes,
            __in_opt Data::LoggingReplicator::TransactionManager * const transactionManager) override;

        void Information(
            __in Data::LogRecordLib::InformationEvent::Enum type) override;

    private:
        TestReplicatedLogManager(
            __in Data::LogRecordLib::IndexingLogRecord const * const indexingLogRecord);

    private:
        Data::LogRecordLib::ProgressVector::SPtr progressVectorSPtr_;
        Data::LogRecordLib::IndexingLogRecord::CSPtr indexingLogRecordCSPtr_;

        FABRIC_SEQUENCE_NUMBER tailLSN_;
    };
}
