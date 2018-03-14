// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        // BackupMetadataFileProperties contains properties of BackupMetadataFile.
        // BackupMetadataFileProperties used key-value pairs.
        // Here is the properties format:
        //
        //         |--Properties Keys--|--Size--|--Properties Value--|...
        //
        // For the Size, we used VarInt, which uses msb of each byte to tell us whether it is the end of the number
        //         | 1 x x x x x x x | 1 x x x x x x x | 0 x x x x x x x |
        //
        // BackupMetadataFileProperties can only read the properties it knows, for the unknown properties, it just skips the specific size
        class BackupMetadataFileProperties final
            : public Utilities::FileProperties
        {
            K_FORCE_SHARED(BackupMetadataFileProperties)

        public: // Static functions.
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static NTSTATUS Create(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in KGuid const & parentBackupId,
                __in KGuid const & backupId,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in TxnReplicator::Epoch const & startingEpoch,
                __in FABRIC_SEQUENCE_NUMBER startingLSN,
                __in TxnReplicator::Epoch const & backupEpoch,
                __in FABRIC_SEQUENCE_NUMBER backupLSN,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public: // Properties.
            __declspec(property(get = get_BackupEpoch, put = put_BackupEpoch)) TxnReplicator::Epoch BackupEpoch;
            TxnReplicator::Epoch get_BackupEpoch() const;
            void put_BackupEpoch(__in TxnReplicator::Epoch const & epoch);

            // KGuid is 16 bytes, using copy by reference
            __declspec(property(get = get_BackupId, put = put_BackupId)) KGuid BackupId;
            KGuid get_BackupId() const;
            void put_BackupId(__in KGuid const & backupId);

            __declspec(property(get = get_BackupLSN, put = put_BackupLSN)) FABRIC_SEQUENCE_NUMBER BackupLSN;
            FABRIC_SEQUENCE_NUMBER get_BackupLSN() const;
            void put_BackupLSN(__in FABRIC_SEQUENCE_NUMBER backupLSN);

            __declspec(property(get = get_BackupOption, put = put_BackupOption)) FABRIC_BACKUP_OPTION BackupOption;
            FABRIC_BACKUP_OPTION get_BackupOption() const;
            void put_BackupOption(__in FABRIC_BACKUP_OPTION backupOption);

            __declspec(property(get = get_ParentBackupId, put = put_ParentBackupId)) KGuid ParentBackupId;
            KGuid get_ParentBackupId() const;
            void put_ParentBackupId(__in KGuid const & parentBackupId);

            __declspec(property(get = get_PartitionId, put = put_PartitionId)) KGuid PartitionId;
            KGuid get_PartitionId() const;
            void put_PartitionId(__in KGuid const & partitionId);

            __declspec(property(get = get_ReplicaId, put = put_ReplicaId)) FABRIC_REPLICA_ID ReplicaId;
            FABRIC_REPLICA_ID get_ReplicaId() const;
            void put_ReplicaId(__in FABRIC_REPLICA_ID replicaId);

            __declspec(property(get = get_StartingEpoch, put = put_StartingEpoch)) TxnReplicator::Epoch StartingEpoch;
            TxnReplicator::Epoch get_StartingEpoch() const;
            void put_StartingEpoch(__in TxnReplicator::Epoch const & epoch);

            __declspec(property(get = get_StartingLSN, put = put_StartingLSN)) FABRIC_SEQUENCE_NUMBER StartingLSN;
            FABRIC_SEQUENCE_NUMBER get_StartingLSN() const;
            void put_StartingLSN(__in FABRIC_SEQUENCE_NUMBER startingLSN);

        public: // Override FileProperties virtual functions.
            // Writes all the properties into the target writer.
            void Write(__in Utilities::BinaryWriter & writer) override;

            // Reads property if known. Unknown properties are skipped.
            void ReadProperty(
                __in Utilities::BinaryReader & reader,
                __in KStringView const & propertyName,
                __in ULONG32 valueSize) override;

        private: // Constructor
            BackupMetadataFileProperties(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in KGuid const & parentBackupId,
                __in KGuid const & backupId,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in TxnReplicator::Epoch const & startingEpoch,
                __in FABRIC_SEQUENCE_NUMBER startingLSN,
                __in TxnReplicator::Epoch const & backupEpoch,
                __in FABRIC_SEQUENCE_NUMBER backupLSN) noexcept;

        private: // Keys for the properties
            const KStringView BackupEpochPropertyName = L"epoch";
            const KStringView BackupIdPropertyName = L"backupid";
            const KStringView BackupLSNPropertyName = L"lsn";
            const KStringView BackupOptionPropertyName = L"option";
            const KStringView ParentBackupIdPropertyName = L"parentBackupId";
            const KStringView PartitionIdPropertyName = L"partitionid";
            const KStringView ReplicaIdPropertyName = L"replicaid";
            const KStringView StartingEpochPropertyName = L"startingEpoch";
            const KStringView StartLSNPropertyName = L"startingLSN";

        private:
            // Note: In the managed, BackupOption::Full is 0 and BackupOption::Incremental is 1.
            // While in the native BackupOption::INVALID is 0, BackupOption::Full is 1 and BackupOption::Incremental is 2.
            // So we use this offset to make they are compatible by writing (BackupOption - 1) and reading (BackupOption + 1) in native.
            const int backupOptionEnumOffset = 1;

        private:
            // The epoch at the time the backup was taken.
            TxnReplicator::Epoch backupEpoch_ = TxnReplicator::Epoch::InvalidEpoch();

            // The backup id.
            KGuid backupId_;

            // The logical sequence number at the time the backup was taken.
            FABRIC_SEQUENCE_NUMBER backupLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

            // The backup option.
            FABRIC_BACKUP_OPTION backupOption_ = FABRIC_BACKUP_OPTION_INVALID;

            // Parent backup id.
            KGuid parentBackupId_;

            // The partition id for the backup.
            KGuid partitionId_;

            // The replica id for the backup.
            FABRIC_REPLICA_ID replicaId_;

            // The epoch at the time the backup was taken.
            TxnReplicator::Epoch startingEpoch_ = TxnReplicator::Epoch::InvalidEpoch();

            // The logical sequence number at the time the backup was taken.
            FABRIC_SEQUENCE_NUMBER startingLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;
        };
    }
}
