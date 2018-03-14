// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class BackupLogFileProperties final
            : public Utilities::FileProperties
        {
            K_FORCE_SHARED(BackupLogFileProperties)

        public: // Static functions.
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public: // Properties.
            __declspec(property(get = get_Count, put = put_Count)) ULONG32 Count;
            ULONG32 BackupLogFileProperties::get_Count() const;
            void BackupLogFileProperties::put_Count(__in ULONG32 count);

            __declspec(property(get = get_IndexingRecordEpoch, put = put_IndexingRecordEpoch)) TxnReplicator::Epoch IndexingRecordEpoch;
            TxnReplicator::Epoch BackupLogFileProperties::get_IndexingRecordEpoch() const;
            void BackupLogFileProperties::put_IndexingRecordEpoch(__in TxnReplicator::Epoch epoch);

            __declspec(property(get = get_IndexingRecordLSN, put = put_IndexingRecordLSN)) FABRIC_SEQUENCE_NUMBER IndexingRecordLSN;
            FABRIC_SEQUENCE_NUMBER BackupLogFileProperties::get_IndexingRecordLSN() const;
            void BackupLogFileProperties::put_IndexingRecordLSN(__in FABRIC_SEQUENCE_NUMBER LSN);

            __declspec(property(get = get_LastBackedUpEpoch, put = put_LastBackedUpEpoch)) TxnReplicator::Epoch LastBackedUpEpoch;
            TxnReplicator::Epoch BackupLogFileProperties::get_LastBackedUpEpoch() const;
            void BackupLogFileProperties::put_LastBackedUpEpoch(__in TxnReplicator::Epoch epoch);

            __declspec(property(get = get_LastBackedUpLSN, put = put_LastBackedUpLSN)) FABRIC_SEQUENCE_NUMBER LastBackedUpLSN;
            FABRIC_SEQUENCE_NUMBER BackupLogFileProperties::get_LastBackedUpLSN() const;
            void BackupLogFileProperties::put_LastBackedUpLSN(__in FABRIC_SEQUENCE_NUMBER LSN);

            __declspec(property(get = get_RecordsHandle, put = put_RecordsHandle)) Utilities::BlockHandle::SPtr RecordsHandle;
            Utilities::BlockHandle::SPtr BackupLogFileProperties::get_RecordsHandle() const;
            void BackupLogFileProperties::put_RecordsHandle(__in Utilities::BlockHandle & blocksHandle);

        public: // Override FileProperties virtual functions.
            // Writes all the properties into the target writer.
            void Write(__in Utilities::BinaryWriter & writer) override;

            // Reads property if known. Unknown properties are skipped.
            void ReadProperty(
                __in Utilities::BinaryReader & reader,
                __in KStringView const & property,
                __in ULONG32 valueSize) override;

        private: // Keys for the properties
            const KStringView CountPropertyName = L"count";
            const KStringView IndexingRecordEpochPropertyName = L"indexepoch";
            const KStringView IndexingRecordLsnPropertyName = L"indexlsn";
            const KStringView LastBackedUpEpochPropertyName = L"backupepoch";
            const KStringView LastBackedUpLsnPropertyName = L"backuplsn";
            const KStringView RecordsHandlePropertyName = L"records";

        private:
            // The number of log records.
            ULONG32 count_ = 0;

            // The logical sequence number of the first record in the backup log.
            TxnReplicator::Epoch indexingRecordEpoch_ = TxnReplicator::Epoch::InvalidEpoch();

            // The Epoch of the last record in the backup log.
            FABRIC_SEQUENCE_NUMBER indexingRecordLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

            // The Epoch of the last record in the backup log.
            TxnReplicator::Epoch lastBackedUpEpoch_ = TxnReplicator::Epoch::InvalidEpoch();

            // The logical sequence number of the last record in the backup log.
            FABRIC_SEQUENCE_NUMBER lastBackedUpLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

            // Values for the properties
            Utilities::ThreadSafeSPtrCache<Utilities::BlockHandle> recordsHandle_;
        };
    }
}
