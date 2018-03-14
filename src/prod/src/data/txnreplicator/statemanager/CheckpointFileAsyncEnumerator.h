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
            , public Utilities::IAsyncEnumerator<SerializableMetadata::CSPtr>
            , public Utilities::PartitionedReplicaTraceComponent<Common::TraceTaskCodes::SM>
        {
            K_FORCE_SHARED(CheckpointFileAsyncEnumerator);
            K_SHARED_INTERFACE_IMP(IDisposable);
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator);

            // Use friend class to test the private function ReadMetadata and WriteMetadata
            friend class TestCheckpointFileReadWrite;
            friend class SerializableMetadataEnumerator;

        public: // Statics
            static SPtr Create(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & fileName,
                __in KSharedArray<ULONG32> const & blockSize,
                __in CheckpointFileProperties const & properties,
                __in KAllocator & allocator);

            static SerializableMetadata::CSPtr ReadMetadata(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in Utilities::BinaryReader & reader,
                __in KAllocator & allocator);

            static Data::Utilities::OperationData::CSPtr DeSerialize(
                __in Data::Utilities::BinaryReader & binaryReader,
                __in KAllocator & allocator);

        public: // IAsyncEnumerator
            SerializableMetadata::CSPtr GetCurrent();

            ktl::Awaitable<bool> MoveNextAsync(
                __in ktl::CancellationToken const & cancellationToken);

            void Reset();

        public: // IDisposable
            void Dispose();

        public:
            ktl::Awaitable<void> CloseAsync();

        private:
            ktl::Awaitable<bool> ReadBlockAsync();

        private: // Constructor
            CheckpointFileAsyncEnumerator(
                __in Data::Utilities::PartitionedReplicaId const & traceId,
                __in KWString const & fileName,
                __in KSharedArray<ULONG32> const & blockSize,
                __in CheckpointFileProperties const & properties) noexcept;

        private: // Static constants
            static const LONGLONG BlockSizeSectionSize = sizeof(LONG32);
            static const LONGLONG CheckSumSectionSize = sizeof(ULONG64);
            static const ULONG32 DesiredBlockSize = 32 * 1024;

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
