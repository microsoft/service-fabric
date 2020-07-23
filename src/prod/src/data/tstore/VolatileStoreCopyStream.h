// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define VOLATILE_STORE_COPY_STREAM_TAG 'scSV'

namespace Data
{
    namespace TStore
    {
        class VolatileCopyStage
        {
        public:
            enum Enum : byte
            {
                // Indicates the copy protocol version will be sent
                Version = 0,

                // Indicates that metadata will be sent
                Metadata = 1,

                // Indicates that data will be sent
                Data = 2,

                // Indicates the copy "completed" marker will be sent
                Complete = 3,

                // Indicates no more copy data needs to be sent
                None = 4
            };
        };

        template<typename TKey, typename TValue>
        class VolatileStoreCopyStream
            : public TxnReplicator::OperationDataStream
        {
            K_FORCE_SHARED(VolatileStoreCopyStream)

        public:
            // TODO: make configurable
            static const ULONG32 CopyChunkSize = 512  * 1024; // Exposed for testing, normally 500 KB
            static const ULONG32 MetadataSizeBytes = 18;

            static NTSTATUS Create(
                __in Data::TStore::ConsolidationManager<TKey, TValue> & consolidationManager,
                __in IComparer<TKey> & keyComparer,
                __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(VOLATILE_STORE_COPY_STREAM_TAG, allocator) VolatileStoreCopyStream(
                    consolidationManager,
                    keyComparer,
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
                return status;
            }
            
            ktl::Awaitable<NTSTATUS> GetNextAsync(
                __in ktl::CancellationToken const &,
                __out OperationData::CSPtr & result) noexcept override
            {
                result = nullptr;
                NTSTATUS status = STATUS_SUCCESS;

                try
                {
                    switch (copyStage_)
                    {
                    case VolatileCopyStage::Version:
                    {
                        result = OnCopyStageVersion();
                        break;
                    }
                    case VolatileCopyStage::Metadata:
                    {
                        result = OnCopyStageMetadata();
                        break;
                    }
                    case VolatileCopyStage::Data:
                    {
                        result = OnCopyStageData();
                        break;
                    }
                    case VolatileCopyStage::Complete:
                    {
                        result = OnCopyStageComplete();
                        break;
                    }
                    }
                }
                catch (ktl::Exception const & e)
                {
                    status = e.GetStatus();
                }

                co_return status;
            }
            void Dispose() override
            {
            }

        private:
            OperationData::CSPtr OnCopyStageVersion()
            {
                // Next copy stage
                copyStage_ = VolatileCopyStage::Metadata;

                BinaryWriter writer(this->GetThisAllocator());
                
                ULONG32 CopyProtocolVersion = 2; // TODO: make constant
                byte CopyOperationVersion = VolatileStoreCopyOperation::Version;

                writer.Write(CopyProtocolVersion);
                writer.Write(CopyOperationVersion);


                OperationData::SPtr resultSPtr = OperationData::Create(this->GetThisAllocator());
                resultSPtr->Append(*writer.GetBuffer(0));

                StoreEventSource::Events->VolatileStoreCopyStreamStageVersion(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    CopyProtocolVersion,
                    writer.Position);

                OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
                return resultCSPtr;
            }

            OperationData::CSPtr OnCopyStageMetadata()
            {
                // TODO: Trace

                // Next copy stage
                bool hasNext = consolidatedStateEnumeratorSPtr_->MoveNext();
                if (hasNext)
                {
                    copyStage_ = VolatileCopyStage::Data;
                }
                else
                {
                    copyStage_ = VolatileCopyStage::Complete;
                }

                BinaryWriter writer(this->GetThisAllocator());
                
                byte CopyOperationMetadata = VolatileStoreCopyOperation::Metadata;

                writer.Write(MetadataSizeBytes);
                writer.Write(CopyOperationMetadata);

                // Workaround for Linux compiler
                ULONG32 metadataSize = MetadataSizeBytes;

                StoreEventSource::Events->VolatileStoreCopyStreamStageMetadata(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    metadataSize,
                    writer.Position);

                OperationData::SPtr resultSPtr = OperationData::Create(this->GetThisAllocator());
                resultSPtr->Append(*writer.GetBuffer(0));

                OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
                return resultCSPtr;
            }

            OperationData::CSPtr OnCopyStageData()
            {
                LONG64 chunkCount = 0;

                BinaryWriter keyMemoryBuffer(this->GetThisAllocator());
                BinaryWriter valueMemoryBuffer(this->GetThisAllocator());

                bool hasNext = true;
                ULONG totalSize = keyMemoryBuffer.Position + valueMemoryBuffer.Position;

                while (hasNext && totalSize < CopyChunkSize)
                {
                    auto item = consolidatedStateEnumeratorSPtr_->Current();

                    if (item.Value->GetRecordKind() != RecordKind::DeletedVersion)
                    {
                        this->WriteValue(valueMemoryBuffer, *item.Value);
                        this->WriteKey(keyMemoryBuffer, item);

                        chunkCount++;
                        count_++;
                    }

                    totalSize = keyMemoryBuffer.Position + valueMemoryBuffer.Position;

                    // Advance to the next unique key (i.e. skip duplicate keys with smaller LSNs)
                    while (true)
                    {
                        hasNext = consolidatedStateEnumeratorSPtr_->MoveNext();

                        if (hasNext == false)
                        {
                            break;
                        }

                        TKey nextKey = consolidatedStateEnumeratorSPtr_->Current().Key;
                        if (keyComparerSPtr_->Compare(item.Key, nextKey) != 0)
                        {
                            break;
                        }
                    }
                }

                if (hasNext == false)
                {
                    copyStage_ = VolatileCopyStage::Complete;
                }

                keyMemoryBuffer.Write(static_cast<byte>(VolatileStoreCopyOperation::Data));

                OperationData::SPtr resultSPtr = OperationData::Create(this->GetThisAllocator());
                resultSPtr->Append(*keyMemoryBuffer.GetBuffer(0));
                resultSPtr->Append(*valueMemoryBuffer.GetBuffer(0));

                StoreEventSource::Events->VolatileStoreCopyStreamStageData(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    chunkCount,
                    count_,
                    keyMemoryBuffer.Position,
                    valueMemoryBuffer.Position);

                OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
                return resultCSPtr;
            }

            OperationData::CSPtr OnCopyStageComplete()
            {
                // TODO: Trace
                copyStage_ = VolatileCopyStage::None;

                KBuffer::SPtr operationDataBufferSPtr;
                auto status = KBuffer::Create(sizeof(byte), operationDataBufferSPtr, GetThisAllocator());
                Diagnostics::Validate(status);

                byte* data = static_cast<byte *>(operationDataBufferSPtr->GetBuffer());
                *data = VolatileStoreCopyOperation::Complete;

                OperationData::SPtr resultSPtr = OperationData::Create(GetThisAllocator());
                resultSPtr->Append(*operationDataBufferSPtr);

                StoreEventSource::Events->VolatileStoreCopyStreamStageComplete(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    count_);

                OperationData::CSPtr resultCSPtr = resultSPtr.RawPtr();
                return resultCSPtr;
            }
            
            // 
            // Name                    Type        Size
            // 
            // KeySize                 ULONG32     4
            // Kind                    byte        1
            // VersionSequenceNumber   LONG64      8
            // ValueSize               int         4
            // OptionalFields          byte        1
            // 
            // Key                     TKey        KeySize
            // TTL                     LONG64      (OptionalFields & HasTTL) ? 8 : 0
            // 
            // Note: Largest Key size supported is ULONG32_MAX in bytes.
            //
            void WriteKey(
                __in BinaryWriter& memoryBuffer,
                __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> & item)
            {
                ULONG metadataStartPosition = memoryBuffer.Position;
                STORE_ASSERT(item.Value->GetRecordKind() != RecordKind::DeletedVersion, "Got a DeletedVersionedItem");
                ULONG recordPosition = memoryBuffer.Position;
                memoryBuffer.Position += sizeof(ULONG32); // Leave space for size to be filled later
                memoryBuffer.Write(static_cast<byte>(item.Value->GetRecordKind())); // RecordKind
                memoryBuffer.Write(item.Value->GetVersionSequenceNumber());
                memoryBuffer.Write(item.Value->GetValueSize());

                byte optionalFields = 0;
                memoryBuffer.Write(optionalFields);

                STORE_ASSERT(memoryBuffer.Position - metadataStartPosition == MetadataSizeBytes, "Metadata size different than expected. Expected={1} Actual={2}", MetadataSizeBytes, memoryBuffer.Position);

                ULONG keyStartPosition = memoryBuffer.Position;
                keySerializerSPtr_->Write(item.Key, memoryBuffer);
                ULONG keyEndPosition = memoryBuffer.Position;
                STORE_ASSERT(keyEndPosition >= keyStartPosition,
                    "keyEndPosition={1} >= keyStartPosition={2}",
                    keyEndPosition, keyStartPosition);

                // Go back and write key size
                memoryBuffer.Position = recordPosition;
                memoryBuffer.Write(static_cast<ULONG32>(keyEndPosition - keyStartPosition));

                // Go back to where we left off
                memoryBuffer.Position = keyEndPosition;

                // Write optional fields
                if (optionalFields & VolatileStoreCopyOptionalFlags::HasTTL)
                {
                    // TODO: Actual TTL; left here as an example
                    LONG64 ttl = 17;
                    memoryBuffer.Write(ttl);
                }
            }

            void WriteValue(
                __in BinaryWriter& memoryBuffer,
                __in VersionedItem<TValue> & item)
            {
                if (item.GetRecordKind() != RecordKind::DeletedVersion)
                {
                    // Serialize the value
                    ULONG valueStartPosition = memoryBuffer.Position;
                    valueSerializerSPtr_->Write(item.GetValue(), memoryBuffer);
                    ULONG valueEndPosition = memoryBuffer.Position;

                    STORE_ASSERT(
                        valueEndPosition >= valueStartPosition, 
                        "valueEndPosition={1} >= valueStartPosition={2}", 
                        valueEndPosition, valueStartPosition);
                    
                    // Write the checksum of the value's bytes
                    ULONG32 valueSize = static_cast<ULONG32>(valueEndPosition - valueStartPosition);
                    item.SetValueSize(valueSize);
                }
            }

            VolatileStoreCopyStream(
                __in Data::TStore::ConsolidationManager<TKey, TValue> & consolidationManager,
                __in IComparer<TKey> & keyComparer,
                __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
                __in StoreTraceComponent & traceComponent);

            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_ = nullptr;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_ = nullptr;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_ = nullptr;

            KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>>
                consolidatedStateEnumeratorSPtr_ = nullptr;

            VolatileCopyStage::Enum copyStage_;

            LONG64 count_ = 0;

            StoreTraceComponent::SPtr traceComponent_;
        };


        template<typename TKey, typename TValue>
        VolatileStoreCopyStream<TKey, TValue>::VolatileStoreCopyStream(
            __in Data::TStore::ConsolidationManager<TKey, TValue> & consolidationManager,
            __in IComparer<TKey> & keyComparer,
            __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
            __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
            __in StoreTraceComponent & traceComponent)
            : traceComponent_(&traceComponent)
            , keyComparerSPtr_(&keyComparer)
            , keySerializerSPtr_(&keySerializer)
            , valueSerializerSPtr_(&valueSerializer)
            , copyStage_(VolatileCopyStage::Version)
        {
            consolidatedStateEnumeratorSPtr_ = consolidationManager.GetAllKeysAndValuesEnumerator();
        }

        template<typename TKey, typename TValue>
        VolatileStoreCopyStream<TKey, TValue>::~VolatileStoreCopyStream()
        {
        }
    }
}