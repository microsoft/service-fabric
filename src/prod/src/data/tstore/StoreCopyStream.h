// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define STORE_COPY_STREAM_TAG 'ssCP'

namespace Data
{
    namespace TStore
    {
        class StoreCopyStream
            : public TxnReplicator::OperationDataStream
        {
            K_FORCE_SHARED(StoreCopyStream)

        public:
            static ULONG32 CopyChunkSize; // Exposed for testing, normally 500KB

            static NTSTATUS Create(
                __in IStoreCopyProvider & copyProvider,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __in StorePerformanceCountersSPtr & perfCounters,
                __out SPtr & result);


            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const & cancellationToken,
                __out OperationData::CSPtr & result) noexcept override;
            ktl::Awaitable<void> CloseAsync();
            void Dispose() override;
        private:
            class CopyStage
            {
            public:
                enum Enum : byte
                {
                    // Indicates the copy protocol version will be sent
                    Version = 0,

                    // Indicates the metadata table bytes will be sent
                    MetadataTable = 1,

                    // Indicates key file
                    KeyFile = 2,

                    // Indicates value file
                    ValueFile = 3,

                    // Indicates the copy "completed" marker will be sent
                    Complete = 4,

                    // Indicates no more copy data needs to be sent
                    None = 5
                };
            };


        private:
            ktl::Awaitable<OperationData::CSPtr> OnCopyStageVersionAsync();
            ktl::Awaitable<OperationData::CSPtr> OnCopyStageMetadataTableAsync();
            ktl::Awaitable<OperationData::CSPtr> OnCopyStageKeyFileAsync();
            ktl::Awaitable<OperationData::CSPtr> OnCopyStageValueFileAsync();
            ktl::Awaitable<OperationData::CSPtr> OnCopyStageCompleteAsync();
            
            KString::SPtr CombineWithWorkingDirectoryPath(__in KStringView & filename);
            KString::SPtr GetKeyCheckpointFilePath(__in KStringView & filename);
            KString::SPtr GetValueCheckpointFilePath(__in KStringView & filename);

            ktl::Awaitable<KBlockFile::SPtr> OpenFileAsync(__in KStringView & filename);
            ktl::Awaitable<OperationData::CSPtr> CreateCheckpointFileChunkOperationData(
                __in KStringView & filename, 
                __in byte startMarker, 
                __in byte writeMarker, 
                __in byte endMarker,
                __out bool & completed);

            void TraceException(__in KStringView const & methodName, __in ktl::Exception const & exception);

            StoreCopyStream(
                __in IStoreCopyProvider & copyProvider,
                __in StoreTraceComponent & traceComponent,
                __in StorePerformanceCountersSPtr & perfCounters);

            IStoreCopyProvider::SPtr copyProviderSPtr_;
            CopyStage::Enum copyStage_;
            MetadataTable::SPtr snapshotOfMetadataTableSPtr_;
            IEnumerator<KeyValuePair<ULONG32, FileMetadata::SPtr>>::SPtr snapshotOfMetadataTableEnumeratorSPtr_;
            ktl::io::KFileStream::SPtr currentFileStreamSPtr_;
            KBlockFile::SPtr currentFileSPtr_;
            KBuffer::SPtr copyDataBufferSPtr_;
            bool isClosed_;

            StoreTraceComponent::SPtr traceComponent_;
            CopyPerformanceCounterWriter perfCounterWriter_;
        };
    }
}
