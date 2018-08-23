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
        // An abstraction for an iterator over SerializableMetadata read from checkpoint file
        //
        class CheckpointFileAsyncEnumerator final
            : public KObject<CheckpointFileAsyncEnumerator>
            , public KShared<CheckpointFileAsyncEnumerator>
            , public Utilities::IAsyncEnumeratorNoExcept<SerializableMetadata::CSPtr>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(CheckpointFileAsyncEnumerator);
            K_SHARED_INTERFACE_IMP(IDisposable);
            K_SHARED_INTERFACE_IMP(IAsyncEnumeratorNoExcept);

            // Use friend class to test the private function ReadMetadata and WriteMetadata
            friend class TestCheckpointFileReadWrite;
            friend class SerializableMetadataEnumerator;

        public: // Statics
            static NTSTATUS Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & fileName,
                __in KSharedArray<ULONG32> const & blockSize,
                __in CheckpointFileProperties const & properties,
                __in KAllocator & allocator,
                __out SPtr & result) noexcept;

            static NTSTATUS ReadMetadata(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator,
                __out SerializableMetadata::CSPtr & result) noexcept;

            static NTSTATUS CheckpointFileAsyncEnumerator::DeSerialize(
                __in Data::Utilities::BinaryReader & binaryReader,
                __in KAllocator & allocator,
                __out Data::Utilities::OperationData::CSPtr & result) noexcept;

        public: // IAsyncEnumerator
            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out SerializableMetadata::CSPtr & result) noexcept;

            NTSTATUS Reset() noexcept;

        public: // IDisposable
            void Dispose();

        public:
            ktl::Awaitable<void> CloseAsync();

        private:
            ktl::Awaitable<NTSTATUS> ReadBlockAsync() noexcept;

        private: // Constructor
            CheckpointFileAsyncEnumerator(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & fileName,
                __in KSharedArray<ULONG32> const & blockSize,
                __in CheckpointFileProperties const & properties) noexcept;

        private: // Static constants
            static const LONGLONG BlockSizeSectionSize;
            static const LONGLONG CheckSumSectionSize;
            static const ULONG32 DesiredBlockSize;

        private: // Initializer list initialized constants.
            const KWString fileName_;
            const CheckpointFileProperties::CSPtr propertiesCSPtr_;

        private: // Default initialized values
            LONG32 currentIndex_ = -1;
            
            // BlockIndex used to track metadata blocks.
            ULONG blockIndex_ = 0;

            bool isDisposed_ = false;

        private: // Constructor initialized
            KSharedArray<SerializableMetadata::CSPtr>::SPtr currentSerializableMetadataCSPtr_;
            KSharedArray<ULONG32>::CSPtr blockSize_;

        private:
            KBlockFile::SPtr fileSPtr_;
            ktl::io::KFileStream::SPtr fileStreamSPtr_;
        };
    }
}
