// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define VOLATILE_COPY_MANAGER_TAG 'gmCV'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class VolatileCopyManager 
            : public KObject<VolatileCopyManager<TKey, TValue>>
            , public KShared<VolatileCopyManager<TKey, TValue>>
            , public ICopyManager
        {
            K_FORCE_SHARED(VolatileCopyManager)
            K_SHARED_INTERFACE_IMP(ICopyManager)

        public:
            static const ULONG32 VolatileCopyVersion = 2;
            
            __declspec(property(get = get_IsCopyCompleted)) bool IsCopyCompleted;
            bool get_IsCopyCompleted() override
            {
                return copyCompleted_;
            }

            __declspec(property(get = get_Count)) LONG64 Count;
            LONG64 get_Count()
            {
                return count_;
            }

            static NTSTATUS Create(
                __in ConsolidationManager<TKey, TValue> & consolidationManager,
                __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer, 
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;

                SPtr output = _new(VOLATILE_COPY_MANAGER_TAG, allocator) VolatileCopyManager(
                    consolidationManager, 
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

            ktl::Awaitable<void> AddCopyDataAsync(__in OperationData const & data) override
            {
                LONG64 receivedBytes = OperationData::GetOperationSize(data);

                StoreEventSource::Events->VolatileCopyManagerAddCopyDataAsync(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    receivedBytes);
                
                // First buffer contains the operation type 
                STORE_ASSERT(data.BufferCount > 0, "Empty operation data");

                auto operationType = GetOperationType(*data[0]);

                switch (operationType)
                {
                case VolatileStoreCopyOperation::Version: 
                    ProcessVersionCopyOperation(data);
                    break;
                case VolatileStoreCopyOperation::Metadata:
                    ProcessMetadataCopyOperation(data);
                    break;
                case VolatileStoreCopyOperation::Data: 
                    ProcessDataCopyOperation(data);
                    break;
                case VolatileStoreCopyOperation::Complete: 
                    ProcessCompleteCopyOperation(data);
                    break;
                default:
                    STORE_ASSERT(false, "Invalid copy operation {1}", static_cast<int>(operationType));
                }

                co_return;
            }

            ktl::Awaitable<void> CloseAsync() override 
            {
                consolidationManagerSPtr_ = nullptr;
                co_return;
            }
        
        private:
            VolatileStoreCopyOperation::Enum GetOperationType(KBuffer const & buffer)
            {
                const byte* bufferBytes = static_cast<const byte *>(buffer.GetBuffer());

                // Last byte of buffer has the operation type
                VolatileStoreCopyOperation::Enum operation = static_cast<VolatileStoreCopyOperation::Enum>(bufferBytes[buffer.QuerySize() - 1]);
                return operation;
            }

            void ProcessVersionCopyOperation(__in OperationData const & data)
            {
                STORE_ASSERT(copyProtocolVersion_ == CopyManager::InvalidCopyProtocolVersion, "unexpected copy operation: Version received multiple times");
                STORE_ASSERT(data.BufferCount == 1, "unexpected copy operation: unexpected number of buffers={1}", data.BufferCount);
                STORE_ASSERT(data[0]->QuerySize() == sizeof(ULONG32) + 1, "unexpected copy operation: version operation data has an unexpected size: {1}", data[0]->QuerySize());

                ULONG32 copyVersion = *(static_cast<const ULONG32 *>(data[0]->GetBuffer()));
                if (copyVersion != VolatileCopyVersion) 
                {
                    StoreEventSource::Events->VolatileCopyManagerProcessVersionCopyOperationMsg(
                        traceComponent_->PartitionId, traceComponent_->TraceTag,
                        copyVersion);

                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                copyProtocolVersion_ = copyVersion;
                
                StoreEventSource::Events->VolatileCopyManagerProcessVersionCopyOperationData(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    copyProtocolVersion_);
            }

            void ProcessMetadataCopyOperation(__in OperationData const & data)
            {
                STORE_ASSERT(copyProtocolVersion_ == VolatileCopyVersion, "unexpected copy operation: Version received multiple times");
                STORE_ASSERT(data.BufferCount == 1, "unexpected copy operation: unexpected number of buffers={1}", data.BufferCount);
                STORE_ASSERT(data[0]->QuerySize() == sizeof(ULONG32) + 1, "unexpected copy operation: version operation data has an unexpected size: {1}", data[0]->QuerySize());

                ULONG32 metadataSizeBytes = *(static_cast<const ULONG32 *>(data[0]->GetBuffer()));
                NTSTATUS status = KBuffer::Create(metadataSizeBytes, metadataBuffer_, this->GetThisAllocator(), VOLATILE_COPY_MANAGER_TAG);
                Diagnostics::Validate(status);
                
                StoreEventSource::Events->VolatileCopyManagerProcessMetadataCopyOperation(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    metadataSizeBytes);
            }

            void ProcessDataCopyOperation(__in OperationData const & data)
            {
                LONG64 keysInChunk = 0;

                STORE_ASSERT(copyProtocolVersion_ == VolatileCopyVersion, "unexpected copy operation: wrong copy version = {1}", copyProtocolVersion_);
                STORE_ASSERT(metadataBuffer_ != nullptr && metadataBuffer_->QuerySize() > 0, "did not receive metadata size");
                STORE_ASSERT(data.BufferCount == 2, "unexpected copy operation: unexpected number of buffers={1}", data.BufferCount);

                BinaryReader keyMemoryBuffer(*data[0], this->GetThisAllocator());
                BinaryReader valueMemoryBuffer(*data[1], this->GetThisAllocator());

                while (valueMemoryBuffer.Position != valueMemoryBuffer.Length)
                {
                    KSharedPtr<KeyData<TKey, TValue>> keyData = ReadKey(keyMemoryBuffer);
                    ReadValue(valueMemoryBuffer, *keyData->Value);
                    consolidationManagerSPtr_->Add(keyData->Key, *keyData->Value);

                    // Sender (i.e. Primary) de-dupes keys, so every key we receive is unique
                    count_++;
                    keysInChunk++;
                }

                STORE_ASSERT(keyMemoryBuffer.Position == keyMemoryBuffer.Length - 1, "Didn't read all keys. position={1} length={2}", keyMemoryBuffer.Position, keyMemoryBuffer.Length - 1);

                StoreEventSource::Events->VolatileCopyManagerProcessDataCopyOperation(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    keysInChunk,
                    count_,
                    keyMemoryBuffer.Position,
                    valueMemoryBuffer.Position);
            }

            void ProcessCompleteCopyOperation(__in OperationData const & data)
            {
                STORE_ASSERT(copyProtocolVersion_ == VolatileCopyVersion, "unexpected copy operation: Version received multiple times");
                STORE_ASSERT(metadataBuffer_ != nullptr && metadataBuffer_->QuerySize() > 0, "did not receive metadata size");
                STORE_ASSERT(data.BufferCount == 1, "unexpected copy operation: unexpected number of buffers={1}", data.BufferCount);

                copyCompleted_ = true;
                copyProtocolVersion_ = CopyManager::InvalidCopyProtocolVersion;

                StoreEventSource::Events->VolatileCopyManagerProcessCompleteCopyOperation(
                    traceComponent_->PartitionId, traceComponent_->TraceTag,
                    count_);
            }

        private:
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
            KSharedPtr<KeyData<TKey, TValue>> ReadKey(__in BinaryReader & memoryBuffer)
            {
                // Read all the metadata from the input buffer at once
                memoryBuffer.Read(metadataBuffer_->QuerySize(), metadataBuffer_);
                
                // From the metadata, read what we know of
                BinaryReader metadataReader(*metadataBuffer_, this->GetThisAllocator());

                ULONG32 keySize = 0;
                metadataReader.Read(keySize);

                byte kindByte = 0; 
                metadataReader.Read(kindByte);
                RecordKind kind = static_cast<RecordKind>(kindByte);

                STORE_ASSERT(kind != RecordKind::DeletedVersion, "Received a Deleted Item");

                LONG64 lsn = 0;
                metadataReader.Read(lsn);

                LONG32 valueSize = 0;
                metadataReader.Read(valueSize);

                byte optionalFields = 0;
                metadataReader.Read(optionalFields);

                // Protection in case the user's key serializer doesn't leave the stream at the correct end point
                ULONG32 keyPosition = memoryBuffer.Position;
                TKey key = keySerializerSPtr_->Read(memoryBuffer);
                memoryBuffer.Position = keyPosition + keySize;

                KSharedPtr<VersionedItem<TValue>> value = nullptr;

                KSharedPtr<InsertedVersionedItem<TValue>> insertedItem = nullptr;
                KSharedPtr<UpdatedVersionedItem<TValue>> updatedItem = nullptr;

                NTSTATUS status = STATUS_SUCCESS;

                switch (kind)
                {
                case RecordKind::InsertedVersion:
                    status = InsertedVersionedItem<TValue>::Create(this->GetThisAllocator(), insertedItem);
                    Diagnostics::Validate(status);
                    insertedItem->InitializeOnRecovery(lsn, 1, /* offset: */0, valueSize, /*checksum: */0);
                    value = &(*insertedItem);
                    break;
                case RecordKind::UpdatedVersion:
                    status = UpdatedVersionedItem<TValue>::Create(this->GetThisAllocator(), updatedItem);
                    Diagnostics::Validate(status);
                    updatedItem->InitializeOnRecovery(lsn, 1, /*offset: */0, valueSize, /*checkum: */0);
                    value = &(*updatedItem);
                    break;
                default:
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                LONG64 ttl = 0;
                if (optionalFields & VolatileStoreCopyOptionalFlags::HasTTL)
                {
                    memoryBuffer.Read(ttl);
                }

                // TODO: Use TTL; left as an example

                KSharedPtr<KeyData<TKey, TValue>> keyData = nullptr;
                status = KeyData<TKey, TValue>::Create(key, *value, 0, keySize, this->GetThisAllocator(), keyData);
                Diagnostics::Validate(status);
                return keyData;
            }

            void ReadValue(
                __in BinaryReader & memoryBuffer,
                __in VersionedItem<TValue> & versionedItem)
            {
                // Assumes that the memoryBuffer is at the correct position already

                KBuffer::SPtr valueBytes = nullptr;
                NTSTATUS status = KBuffer::Create(versionedItem.GetValueSize(), valueBytes, this->GetThisAllocator());
                Diagnostics::Validate(status);

                ULONG startPosition = memoryBuffer.Position;
                TValue value = valueSerializerSPtr_->Read(memoryBuffer);
                memoryBuffer.Position = startPosition + versionedItem.GetValueSize();

                versionedItem.SetValue(value);
            }

        private:
            VolatileCopyManager(
                __in ConsolidationManager<TKey, TValue> & consolidationManager,
                __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
                __in Data::StateManager::IStateSerializer<TValue> & valueSerializer, 
                __in StoreTraceComponent & traceComponent);

            bool copyCompleted_;
            ULONG32 copyProtocolVersion_;

            // Reusable buffer for per-key metadata
            KBuffer::SPtr metadataBuffer_ = nullptr;

            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_ = nullptr;
            KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_ = nullptr;

            KSharedPtr<ConsolidationManager<TKey, TValue>> consolidationManagerSPtr_;
            LONG64 count_;

            StoreTraceComponent::SPtr traceComponent_;
        };

        template<typename TKey, typename TValue>
        VolatileCopyManager<TKey, TValue>::VolatileCopyManager(
            __in ConsolidationManager<TKey, TValue> & consolidationManager,
            __in Data::StateManager::IStateSerializer<TKey> & keySerializer,
            __in Data::StateManager::IStateSerializer<TValue> & valueSerializer,
            __in StoreTraceComponent & traceComponent)
            : consolidationManagerSPtr_(&consolidationManager)
            , keySerializerSPtr_(&keySerializer)
            , valueSerializerSPtr_(&valueSerializer)
            , traceComponent_(&traceComponent)
            , copyProtocolVersion_(CopyManager::InvalidCopyProtocolVersion)
            , copyCompleted_(false)
            , count_(0)
        {
        }

        template<typename TKey, typename TValue>
        VolatileCopyManager<TKey, TValue>::~VolatileCopyManager()
        {
        }
    }
}
