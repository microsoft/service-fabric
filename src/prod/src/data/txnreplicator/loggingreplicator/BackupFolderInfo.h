// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace LoggingReplicator
    {
        class BackupFolderInfo final
            : public KObject<BackupFolderInfo>
            , public KShared<BackupFolderInfo>
            , public TxnReplicator::IBackupFolderInfo
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::LR>
        {
            K_FORCE_SHARED(BackupFolderInfo);
            K_SHARED_INTERFACE_IMP(IBackupFolderInfo);

        public:
            static SPtr Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KGuid const & id,
                __in KString const & backupFolder,
                __in KAllocator & allocator);

        public:
            __declspec(property(get = get_HighestBackedupEpoch)) TxnReplicator::Epoch HighestBackedupEpoch;
            TxnReplicator::Epoch get_HighestBackedupEpoch() const;

            __declspec(property(get = get_HighestBackedupLSN)) FABRIC_SEQUENCE_NUMBER HighestBackedupLSN;
            FABRIC_SEQUENCE_NUMBER get_HighestBackedupLSN() const;

            __declspec(property(get = get_BackupMetadataArray)) KArray<BackupMetadataFile::CSPtr> const & BackupMetadataArray;
            KArray<BackupMetadataFile::CSPtr> const & get_BackupMetadataArray() const;

            __declspec(property(get = get_StateManagerBackupFolderPath)) KString::CSPtr StateManagerBackupFolderPath;
            KString::CSPtr get_StateManagerBackupFolderPath() const;

            __declspec(property(get = get_LogPathList)) KArray<KString::CSPtr> const & LogPathList;
            KArray<KString::CSPtr> const & get_LogPathList() const override;

        public:
            ktl::Awaitable<void> AnalyzeAsync(
                __in ktl::CancellationToken const & cancellationToken);

        private:
            BackupFolderInfo(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KGuid const & id,
                __in KString const & backupFolder);

        private:
            ktl::Awaitable<void> AddFolderAsync(
                __in FABRIC_BACKUP_OPTION backupOption,
                __in wstring const & backupDirectoryPath,
                __in wstring const & backupMetadataFilePath,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> VerifyAsync(
                __in ktl::CancellationToken const & cancellationToken);

            void VerifyMetadataList();

            ktl::Awaitable<void> VerifyLogFilesAsync(
                __in ktl::CancellationToken const & cancellationToken);

        private:
            void TraceException(
                __in Common::WStringLiteral const & message,
                __in ktl::Exception & exception) const;

            void ThrowIfNecessary(
                __in NTSTATUS status,
                __in std::wstring const & message) const;

        private:
            KGuid const id_;
            KString::CSPtr const backupFolderCSPtr_;

        private:
            KArray<TxnReplicator::BackupVersion> backupVersionArray_;
            KArray<BackupMetadataFile::CSPtr> metadataArray_;
            KArray<KString::CSPtr> logFilePathArray_;
        
        private:
            KString::SPtr fullBackupFolderPath_;
            KString::CSPtr stateManagerBackupFolderPath_;
        };
    }
}
