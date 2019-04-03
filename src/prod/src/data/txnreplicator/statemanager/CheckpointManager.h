// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        //
        // This component is in charge SM (local) checkpointing.
        //
        class CheckpointManager final :
            public KObject<CheckpointManager>,
            public KShared<CheckpointManager>,
            public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(CheckpointManager)

        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceType,
                __in ApiDispatcher & apiDispatcher,
                __in KStringView const & workFolderPath,
                __in KStringView const & replicaFolderPath,
                __in SerializationMode::Enum serializationMode,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

        public: // Properties
            __declspec(property(get = get_PrepareCheckpointLSN)) FABRIC_SEQUENCE_NUMBER PrepareCheckpointLSN;
            FABRIC_SEQUENCE_NUMBER get_PrepareCheckpointLSN() const;

        public: // Checkpoint APIs.
            void PrepareCheckpoint(
                __in MetadataManager const & metadataManager,
                __in FABRIC_SEQUENCE_NUMBER checkpointLSN);

            ktl::Awaitable<void> PerformCheckpointAsync(
                __in MetadataManager const & metadataManager,
                __in ktl::CancellationToken const & cancellationToken,
                __in bool hasPersistedState = true);

            ktl::Awaitable<void> CompleteCheckpointAsync(
                __in MetadataManager const & metadataManager,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<KSharedArray<Metadata::SPtr>::SPtr> RecoverCheckpointAsync(
                __in MetadataManager & metadataManager,
                __in ktl::CancellationToken const & cancellationToken);

            TxnReplicator::OperationDataStream::SPtr GetCurrentState(
                __in FABRIC_REPLICA_ID targetReplicaId);

            void RemoveStateProvider(__in FABRIC_STATE_PROVIDER_ID stateProviderId);

        public:// Backup and Restore APIs.
            ktl::Awaitable<void> BackupCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> RestoreCheckpointAsync(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> BackupActiveStateProviders(
                __in KString const & backupDirectory,
                __in ktl::CancellationToken const & cancellationToken);

        public: // Read APIs.
            KArray<Metadata::CSPtr> GetPreparedActiveOrDeletedList(
                __in MetadataMode::Enum filterMode);

            KArray<Metadata::CSPtr> GetCopyOrCheckpointActiveList();

        public:
            void Clean();

        private:
            ktl::Awaitable<void> CloseEnumeratorAndThrowIfOnFailureAsync(
                __in NTSTATUS status,
                __in CheckpointFileAsyncEnumerator & enumerator);

            KString::CSPtr GetCheckpointFilePath(__in KStringView const & fileName);
            KSharedArray<Metadata::CSPtr>::SPtr GetOrderedMetadataArray();

            // Summary: Moves files in managed path into native path.
            __checkReturn NTSTATUS MoveManagedFileIfNecessary(
                __in KStringView const & fileName) noexcept;

            void PopulateSnapshotAndMetadataCollection(
                __in KSharedArray<SerializableMetadata::CSPtr> & metadataArray,
                __in MetadataMode::Enum filterMode);

        private:
            FAILABLE CheckpointManager(
                __in Utilities::PartitionedReplicaId const & partitionedReplicId,
                __in ApiDispatcher & apiDispatcher,
                __in KStringView const & workFolderPath,
                __in KStringView const & replicaFolderPath,
                __in SerializationMode::Enum serializationMode);

        private: // Constants
            KStringView const CurrentCheckpointPrefix = L"StateManager.cpt";
            KStringView const TempCheckpointPrefix = L"StateManager.tmp";
            KStringView const BackupCheckpointPrefix = L"StateManager.bkp";

            // CRC64 of "StateManager"
            std::wstring const ManagedCurrentCheckpointSuffix = L"C8FE46D09D0660D7";

            // CRC64 of "StateManager_tmp"
            std::wstring const ManagedTmpCheckpointSuffix = L"33016AD7CC038B6F";

            // CRC64 of "StateManager_bkp"
            std::wstring const ManagedBackupCheckpointSuffix = L"AB9A0E2FFBE60B9";

        private: // Initialize in the constructor.
            SerializationMode::Enum const serializationMode_;

            FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN_ = FABRIC_INVALID_SEQUENCE_NUMBER;

        private: // Initialized in constructor initialization list.
            const ApiDispatcher::SPtr apiDispatcherSPtr_;

            // Lock used to protect copyOrCheckpointMetadataSnapshot_.
            KReaderWriterSpinLock checkpointStateLock_;

            // Note: The benefit of seperating the PrepareCheckpointMetadataSnapshot is to reserve the size of KArray 
            // to avoid multiple growth when we call GetPreparedActiveList.
            std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> prepareCheckpointActiveMetadataSnapshot_;
            std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> prepareCheckpointDeletedMetadataSnapshot_;

            // Table of state providers that should be copied as part of the copy.
            std::unordered_map<FABRIC_STATE_PROVIDER_ID, Metadata::CSPtr> copyOrCheckpointMetadataSnapshot_;

        private:
            KString::CSPtr workFolder_;
            KString::CSPtr tempCheckpointFilePath_;
            KString::CSPtr currentCheckpointFilePath_;
            KString::CSPtr backupCheckpointFilePath_;
        };
    }
}
