// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace StateManager
    {
        // StateManager checkpoint file format.
        //
        //  Here is what the file looks like:
        //         
        //              |__Metadata_1__|__Checksum__|
        //              |__Metadata_2__|__Checksum__|
        //                         ......                   
        //              |__Metadata_N__|__Checksum__|
        //              |__FileBlock___|__Checksum__|
        //              |__Properties__|__Checksum__|
        //              |__Footer______|__Checksum__|
        //  
        //  Metadata section shows stateprovider information
        //  FileBlock contains the information 1. how many metadatas 2. size of each metadata
        //  Properties is key-value pairs of checkpoint file properties
        //  Footer section has the version, properties offset and size
        //  Checksum used to check whether the data is corrupted or not
        //
        class CheckpointFile
            : public KObject<CheckpointFile>
            , public KShared<CheckpointFile>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(CheckpointFile)
            
            // Use friend class to test the private function ReadMetadata and WriteMetadata
            friend class TestCheckpointFileReadWrite;
            friend class SerializableMetadataEnumerator;

        public:
            static NTSTATUS Create(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & filePath,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static ktl::Awaitable<void> SafeFileReplaceAsync(
                __in Data::Utilities::PartitionedReplicaId const & partitionedReplicaId,
                __in KString const & checkpointFileName,
                __in KString const & temporaryCheckpointFileName,
                __in KString const & backupCheckpointFileName,
                __in ktl::CancellationToken const & cancellationToken,
                __in KAllocator & allocator);

            static OperationDataEnumerator::SPtr GetCopyData(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KArray<SerializableMetadata::CSPtr> const & stateArray,
                __in FABRIC_REPLICA_ID targetReplicaId,
                __in SerializationMode::Enum serailizationMode,
                __in KAllocator & allocator);

            static KSharedPtr<SerializableMetadataEnumerator> ReadCopyData(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in_opt Data::Utilities::OperationData const * const operationData,
                __in KAllocator & allocator);

        public:
            __declspec(property(get = get_FileSize)) LONG64 FileSize;
            LONG64 get_FileSize() const;

            __declspec(property(get = get_PropertiesSPtr)) CheckpointFileProperties::SPtr PropertiesSPtr;
            CheckpointFileProperties::SPtr get_PropertiesSPtr() { return propertiesSPtr_; }

        public:
            // Create a new CheckpointFile and write it to the given file.    
            ktl::Awaitable<void> WriteAsync(
                __in KSharedArray<SerializableMetadata::CSPtr> const & metadataArray,
                __in SerializationMode::Enum serailizationMode,
                __in bool allowPrepareCheckpointLSNToBeInvalid,
                __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
                __in ktl::CancellationToken const & cancellationToken);

            // Open a existing CheckpointFile
            ktl::Awaitable<void> ReadAsync(
                __in ktl::CancellationToken const & cancellationToken);

        public:
            CheckpointFileAsyncEnumerator::SPtr GetAsyncEnumerator() const;

        private:
            ktl::Awaitable<void> WriteMetadataAsync(
                __in SerializationMode::Enum serailizationMode,
                __in bool allowPrepareCheckpointLSNToBeInvalid,
                __in FABRIC_SEQUENCE_NUMBER prepareCheckpointLSN,
                __in ktl::CancellationToken const & cancellationToken,
                __inout ktl::io::KStream & fileStream);

            ktl::Awaitable<Utilities::BlockHandle::SPtr> WritePropertiesAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __inout ktl::io::KFileStream & fileStream);

            ktl::Awaitable<void> WriteFooterAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __in ktl::io::KFileStream & fileStream,
                __in Data::Utilities::BlockHandle & propertiesBlockHandle);

            ktl::Awaitable<void> ReadPropertiesAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<void> ReadFooterAsync(
                __in ktl::io::KFileStream & fileStream,
                __in ktl::CancellationToken const & cancellationToken);

            ktl::Awaitable<KSharedArray<ULONG32>::SPtr> ReadBlocksAsync(
                __in ktl::io::KStream & stream,
                __in ktl::CancellationToken const & cancellationToken);

        private:
            static ULONG32 WriteMetadata(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryWriter & writer, 
                __in SerializableMetadata const & metadata,
                __in SerializationMode::Enum serailizationMode);

            static void WriteManagedInitializationParameter(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryWriter & writer,
                __in Utilities::OperationData const * const initializationParameter);

            static void WriteNativeInitializationParameter(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryWriter & writer,
                __in Utilities::OperationData const * const initializationParameter);

        private: // Constructor
            CheckpointFile(
                __in Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & filePath) noexcept;

        private:
            static const ULONG32 CopyProtocolVersion;
            static const ULONG32 DesiredBlockSize;
            static const ULONG32 Version;
            static const LONGLONG BlockSizeSectionSize;
            static const LONGLONG CheckSumSectionSize;

        private:
            const KWString filePath_;

        private:
            // stateProviderMetadata stores all the state provider metadata
            KSharedArray<SerializableMetadata::CSPtr>::CSPtr stateProviderMetadata_;
            KSharedArray<ULONG32>::SPtr blockSize_;

            // propertiesSPtr is the file properties
            CheckpointFileProperties::SPtr propertiesSPtr_;

            // footerSPtr has the information about version and offset and size of properties
            Utilities::FileFooter::SPtr footerSPtr_;

            LONG64 fileSize_;
            ULONG32 rootStateProviderCount_;
            ULONG32 stateProviderCount_;
        };
    }
}
