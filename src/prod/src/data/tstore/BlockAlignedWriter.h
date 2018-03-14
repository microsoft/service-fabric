// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define BlockAlignedWriter_TAG 'twAB'

namespace Data
{
    namespace TStore
    {

        template<typename TKey, typename TValue>
        class BlockAlignedWriter : 
            public KObject<BlockAlignedWriter<TKey, TValue>>,
            public KShared<BlockAlignedWriter<TKey, TValue>>
        {
            K_FORCE_SHARED(BlockAlignedWriter)

        public:

            static NTSTATUS
                Create(
                    __in ValueCheckpointFile& valueCheckpointFile,
                    __in KeyCheckpointFile& keyCheckpointFile,
                    __in SharedBinaryWriter& valueBuffer,
                    __in SharedBinaryWriter& keyBuffer,
                    __in ktl::io::KFileStream& valueFileStream,
                    __in ktl::io::KFileStream& keyFileStream,
                    __in ULONG64 logicalTimeStamp,
                    __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                    __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(BlockAlignedWriter_TAG, allocator)
                    BlockAlignedWriter(
                        valueCheckpointFile,
                        keyCheckpointFile,
                        valueBuffer,
                        keyBuffer,
                        valueFileStream,
                        keyFileStream,
                        logicalTimeStamp,
                        keySerializer,
                        valueSerializer,
                        traceComponent);

                if (!output)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                status = output->Status();
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            static const int DefaultBlockAlignmentSize = 4 * 1024;

            //
            // Values is allowed to be null so it should pass by pointer.
            //
            ktl::Awaitable<void> BlockAlignedWriteItemAsync(
                __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> item,
                __in KSharedPtr<KBuffer> value, 
                __in bool shouldSerialize)
            {
                co_await valueBlockAlignedWriterSPtr_->BlockAlignedWriteValueAsync(item, value, shouldSerialize);
                co_await keyBlockAlignedWriterSPtr_->BlockAlignedWriteKeyAsync(item);
            }

            ktl::Awaitable<void> FlushAsync()
            {
                co_await keyBlockAlignedWriterSPtr_->FlushAsync();
                co_await valueBlockAlignedWriterSPtr_->FlushAsync();
            }

        private:

            BlockAlignedWriter(
                __in ValueCheckpointFile& valueCheckpointFile,
                __in KeyCheckpointFile& keyCheckpointFile,
                __in SharedBinaryWriter& valueBuffer,
                __in SharedBinaryWriter& keyBuffer,
                __in ktl::io::KFileStream& valueFileStream,
                __in ktl::io::KFileStream& keyFileStream,
                __in ULONG64 logicalTimeStamp,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
                __in StoreTraceComponent & traceComponent);

            KSharedPtr<ValueCheckpointFile> valueCheckpointFileSPtr_;
            KSharedPtr<KeyCheckpointFile> keyCheckpointFileSPtr_;
            KSharedPtr<SharedBinaryWriter> valueBufferSPtr_;
            KSharedPtr<SharedBinaryWriter> keyBufferSPtr_;
            KSharedPtr<ktl::io::KFileStream> valueFileStreamSPtr_;
            KSharedPtr<ktl::io::KFileStream> keyFileStreamSPtr_;
            ULONG64 logicalTimeStamp_;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_;
            KSharedPtr<KeyBlockAlignedWriter<TKey, TValue>> keyBlockAlignedWriterSPtr_;
            KSharedPtr<ValueBlockAlignedWriter<TKey, TValue>> valueBlockAlignedWriterSPtr_;
            KSharedPtr<StoreTraceComponent> traceComponent_;
        };

        template<typename TKey, typename TValue>
        BlockAlignedWriter<TKey, TValue>::BlockAlignedWriter(
            __in ValueCheckpointFile& valueCheckpointFile,
            __in KeyCheckpointFile& keyCheckpointFile,
            __in SharedBinaryWriter& valueBuffer,
            __in SharedBinaryWriter& keyBuffer,
            __in ktl::io::KFileStream& valueFileStream,
            __in ktl::io::KFileStream& keyFileStream,
            __in ULONG64 logicalTimeStamp,
            __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
            __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
            __in StoreTraceComponent & traceComponent)
            :valueCheckpointFileSPtr_(&valueCheckpointFile),
            keyCheckpointFileSPtr_(&keyCheckpointFile),
            valueBufferSPtr_(&valueBuffer),
            keyBufferSPtr_(&keyBuffer),
            valueFileStreamSPtr_(&valueFileStream),
            keyFileStreamSPtr_(&keyFileStream),
            logicalTimeStamp_(logicalTimeStamp),
            keySerializerSPtr_(&keySerializer),
            valueSerializerSPtr_(&valueSerializer),
            traceComponent_(&traceComponent)
        {
            NTSTATUS status = KeyBlockAlignedWriter<TKey, TValue>::Create(
                keyFileStream, 
                keyCheckpointFile, 
                keyBuffer, 
                keySerializer, 
                logicalTimeStamp, 
                traceComponent,
                this->GetThisAllocator(), 
                keyBlockAlignedWriterSPtr_);
            if (!NT_SUCCESS(status))
            {
                this->SetConstructorStatus(status);
                return;
            }

            status = ValueBlockAlignedWriter<TKey, TValue>::Create(
                valueFileStream,
                valueCheckpointFile,
                valueBuffer,
                valueSerializer,
                traceComponent,
                this->GetThisAllocator(),
                valueBlockAlignedWriterSPtr_);

            this->SetConstructorStatus(status);          
        }

        template<typename TKey, typename TValue>
        BlockAlignedWriter<TKey, TValue>::~BlockAlignedWriter()
        {
        }
    }
}
