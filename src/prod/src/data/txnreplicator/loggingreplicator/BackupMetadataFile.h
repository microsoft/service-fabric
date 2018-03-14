// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        //  Here is what the file looks like:
        //         
        //              |__Properties__|__Checksum__|
        //              |__Footer______|__Checksum__|
        //  
        //  Properties is key-value pairs of BackupMetadataFile properties
        //  Footer section has the version, properties offset and size
        //  Checksum used to check whether the data is corrupted or not
        //
        class BackupMetadataFile final
            : public KObject<BackupMetadataFile>
            , public KShared<BackupMetadataFile>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(BackupMetadataFile)

        public: // Static functions.
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & filePath,
                __in KAllocator & allocator,
                __out SPtr & result);

        public: // Properties.
            __declspec(property(get = get_FileName)) KWString const FileName;
            KWString const get_FileName() const;

            __declspec(property(get = get_BackupEpoch)) TxnReplicator::Epoch BackupEpoch;
            TxnReplicator::Epoch get_BackupEpoch() const;

            __declspec(property(get = get_BackupId)) KGuid BackupId;
            KGuid get_BackupId() const;

            __declspec(property(get = get_BackupLSN)) FABRIC_SEQUENCE_NUMBER BackupLSN;
            FABRIC_SEQUENCE_NUMBER get_BackupLSN() const;

            __declspec(property(get = get_BackupOption)) FABRIC_BACKUP_OPTION BackupOption;
            FABRIC_BACKUP_OPTION get_BackupOption() const;

            __declspec(property(get = get_ParentBackupId)) KGuid ParentBackupId;
            KGuid get_ParentBackupId() const;

            __declspec(property(get = get_PropertiesPartitionId)) KGuid PropertiesPartitionId;
            KGuid get_PropertiesPartitionId() const;

            __declspec(property(get = get_PropertiesReplicaId)) FABRIC_REPLICA_ID PropertiesReplicaId;
            FABRIC_REPLICA_ID get_PropertiesReplicaId() const;

            __declspec(property(get = get_StartingEpoch)) TxnReplicator::Epoch StartingEpoch;
            TxnReplicator::Epoch get_StartingEpoch() const;

            __declspec(property(get = get_StartingLSN)) FABRIC_SEQUENCE_NUMBER StartingLSN;
            FABRIC_SEQUENCE_NUMBER get_StartingLSN() const;

        public:
            // Create a new BackupMetadataFile and write it to the given file.    
            ktl::Awaitable<NTSTATUS> WriteAsync(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in KGuid const & parentBackupId,
                __in KGuid const & backupId,
                __in KGuid const & partitionId,
                __in FABRIC_REPLICA_ID replicaId,
                __in TxnReplicator::Epoch const & startingEpoch,
                __in FABRIC_SEQUENCE_NUMBER startingLSN,
                __in TxnReplicator::Epoch const & backupEpoch,
                __in FABRIC_SEQUENCE_NUMBER backupLSN,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            // Open a existing BackupMetadataFile
            ktl::Awaitable<NTSTATUS> ReadAsync(
                __in KGuid const & readId,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        public:
            bool Equals(__in BackupMetadataFile const & other) const noexcept;

        private:
            ktl::Awaitable<NTSTATUS> ReadFooterAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

            ktl::Awaitable<NTSTATUS> ReadPropertiesAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken) noexcept;

        private: // Constructor
            BackupMetadataFile::BackupMetadataFile(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & filePath,
                __in BackupMetadataFileProperties & properties) noexcept;

        private:
            static const ULONG32 Version = 1;
            static const LONGLONG CheckSumSectionSize = sizeof(ULONG64);

        private: // Initializer list initialized.
            KWString filePath_;

        private: // Set later.
                 // propertiesSPtr is the file properties
            BackupMetadataFileProperties::SPtr propertiesSPtr_;

            // footerSPtr has the information about version and offset and size of properties
            Utilities::FileFooter::SPtr footerSPtr_;
        };
    }
}
