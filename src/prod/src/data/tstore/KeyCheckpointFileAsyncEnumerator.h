// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEYCHECKPOINTASYNCENUMERATOR_TAG 'eaCK'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class KeyCheckpointFileAsyncEnumerator : public KObject<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>,
            public KShared<KeyCheckpointFileAsyncEnumerator<TKey, TValue>>,
            public IAsyncEnumerator<KSharedPtr<KeyData<TKey, TValue>>>
        {
            K_SHARED_INTERFACE_IMP(IDisposable)
            K_SHARED_INTERFACE_IMP(IAsyncEnumerator)
            K_FORCE_SHARED(KeyCheckpointFileAsyncEnumerator)

        public:

            static NTSTATUS
                Create(
                    __in KeyCheckpointFile& keyCheckpointFile,
                    __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                    __in ULONG64 startOffset,
                    __in ULONG64 endOffset,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(KEYCHECKPOINTASYNCENUMERATOR_TAG, allocator)
                    KeyCheckpointFileAsyncEnumerator(keyCheckpointFile, keySerializer, startOffset, endOffset, traceComponent);

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

            KSharedPtr<KeyData<TKey, TValue>> GetCurrent() override
            {
                return current_;
            }

            __declspec(property(get = get_KeyComparer, put = set_KeyComparer)) KSharedPtr<IComparer<TKey>> KeyComparerSPtr;
            KSharedPtr<IComparer<TKey>> get_KeyComparer() const
            {
                return keyComparerSPtr_;
            }
            void set_KeyComparer(__in IComparer<TKey>& value)
            {
                keyComparerSPtr_ = &value;
            }

            ktl::Awaitable<void> CloseAsync()
            {
                if (fileStreamSPtr_ != nullptr && fileStreamSPtr_->IsOpen())
                {
                    co_await keyCheckpointFileSPtr_->StreamPoolSPtr->ReleaseStreamAsync(*fileStreamSPtr_);
                    fileStreamSPtr_ = nullptr;
                }
            }

            void Dispose()
            {

            }

            void Reset() override
            {
            }

            //todo: fix the interface to change int to ULONG
            int Compare(__in KeyCheckpointFileAsyncEnumerator<TKey, TValue>& other) const
            {
                // Compare Keys
                // todo: keyComparer_ may not be initialized. Is this a good design? shall we check keyComparer_ is not null here?
                STORE_ASSERT(keyComparerSPtr_ != nullptr, "keyComparer should not be null");
                int compare = keyComparerSPtr_->Compare(current_->Key, other.GetCurrent()->Key);
                if (compare != 0)
                {
                    return compare;
                }

                // Compare LSNs
                LONG64 currentLsn = current_->Value->GetVersionSequenceNumber();
                LONG64 otherLsn = other.GetCurrent()->Value->GetVersionSequenceNumber();

                if (currentLsn > otherLsn)
                {
                    return -1;
                }
                else if (currentLsn < otherLsn)
                {
                    return 1;
                }

                // If they are both deleted items, compare timestamps.
                VersionedItem<TValue>& currentItem = *(current_->Value);
                VersionedItem<TValue>& otherItem = *(other.GetCurrent()->Value);

                if (currentItem.GetRecordKind() == RecordKind::DeletedVersion && otherItem.GetRecordKind() == RecordKind::DeletedVersion)
                {
                    STORE_ASSERT(currentItem.GetFileId() != otherItem.GetFileId(), "current item and other item file ids should not match. fileId={1}", currentItem.GetFileId());
                    if (current_->LogicalTimeStamp > other.GetCurrent()->LogicalTimeStamp)
                    {
                        return -1;
                    }
                    else if (current_->LogicalTimeStamp < other.GetCurrent()->LogicalTimeStamp)
                    {
                        return 1;
                    }

                    return 0;
                }

                return 0;
            }

            static int CompareEnumerators(__in KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> const & one, __in KSharedPtr<KeyCheckpointFileAsyncEnumerator<TKey, TValue>> const & two)
            {
                return one->Compare(*two);
            }

            ktl::Awaitable<bool> MoveNextAsync(__in ktl::CancellationToken const & cancellationToken) override
            {
                // Starting from state zero.
                if (stateZero_)
                {
                    // Assert that startOffset - endOffset is a multiple of 4k
                    stateZero_ = false;
                    itemsBufferSPtr_ = _new(KEYCHECKPOINTASYNCENUMERATOR_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>();
                    STORE_ASSERT(itemsBufferSPtr_ != nullptr, "itemsBufferSPtr_ should not be null");
                    index_ = 0;

                    // Call read keys and populate list.
                    NTSTATUS status = KBuffer::Create(ChunkSize, memoryStreamSPtr_, this->GetThisAllocator());
                    Diagnostics::Validate(status);

                    // Assert file stream is null;
                    STORE_ASSERT(fileStreamSPtr_ == nullptr, "fileStreamSPtr_ == nullptr");

                    fileStreamSPtr_ = co_await keyCheckpointFileSPtr_->StreamPoolSPtr->AcquireStreamAsync();
                    STORE_ASSERT(fileStreamSPtr_ != nullptr, "fileStreamSPtr_ != nullptr");

                    fileStreamSPtr_->Position = startOffset_;
                    bool result = co_await ReadChunkAsync();

                    if (result)
                    {
                        current_ = (*itemsBufferSPtr_)[index_];
                        co_return true;
                    }
                    else
                    {
                        STORE_ASSERT(keyCount_ == keyCheckpointFileSPtr_->PropertiesSPtr->KeyCount, "Key Counts differ. actual={1} expected={2}", keyCount_, keyCheckpointFileSPtr_->PropertiesSPtr->KeyCount);
                        co_return false;
                    }
                }
                else
                {
                    index_++;

                    // Check if it is in the buffer.
                    if (static_cast<ULONG32>(index_) < itemsBufferSPtr_->Count())
                    {
                        current_ = (*itemsBufferSPtr_)[index_];
                        co_return true;
                    }
                    else
                    {
                        // read next block
                        bool result = co_await ReadChunkAsync();
                        if (result)
                        {
                            current_ = (*itemsBufferSPtr_)[index_];
                            co_return true;
                        }
                        else
                        {
                            STORE_ASSERT(keyCount_ == keyCheckpointFileSPtr_->PropertiesSPtr->KeyCount, "Key counts differ. actual={1} expected={2}", keyCount_, keyCheckpointFileSPtr_->PropertiesSPtr->KeyCount);
                            co_return false;
                        }
                    }
                }
            }

        private:

            ktl::Awaitable<bool> ReadChunkAsync()
            {
                itemsBufferSPtr_->Clear();
                index_ = 0;

                // Pick a chunk size that is a multiple of 4k lesser than the end offset.
                ULONG chunkSize = static_cast<ULONG>(GetChunkSize());
                if (chunkSize == 0)
                {
                    co_return false;
                }

                // Read the entire chunk (plus the checksum and next chunk size) into memory.
                NTSTATUS status = KBuffer::Create(chunkSize, memoryStreamSPtr_, this->GetThisAllocator());
                Diagnostics::Validate(status);

                ULONG startPosition = 0;
                ULONG bytesRead = 0;
    
                status = co_await fileStreamSPtr_->ReadAsync(*memoryStreamSPtr_, bytesRead, startPosition, chunkSize);
                STORE_ASSERT(NT_SUCCESS(status), "Failed to read from filestream. status={1}", status);
                STORE_ASSERT(bytesRead == chunkSize, "bytesRead={1} != chunkSize={2}", bytesRead, chunkSize);

                //need sharedreader because the while loop may adjust the br buffer which requires reader to be recreated.
                KSharedPtr<SharedBinaryReader> brSPtr = nullptr;
                status = SharedBinaryReader::Create(this->GetThisAllocator(), *memoryStreamSPtr_, brSPtr);
                Diagnostics::Validate(status);
            
                while (true)
                {
                    ULONG32 alignedStartBlockOffset = brSPtr->Position;
                    KeyChunkMetadata blockMetadata = KeyChunkMetadata::Read(*brSPtr);
                    ULONG32 currentBlockSize = blockMetadata.BlockSize;
                    ULONG32 alignedBlockSize = GetBlockSize(currentBlockSize);

                    // Check if the next block was only partially read.  If so, read the remainder of it into memory.
                    if ((alignedStartBlockOffset + alignedBlockSize) > chunkSize)
                    {
                        ULONG32 remainingBlockSize = (alignedStartBlockOffset + alignedBlockSize) - chunkSize;
                        ULONG32 remainder = remainingBlockSize %  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
                        STORE_ASSERT(remainder == 0, "remainder={1} should be 0", remainder);

                        memoryStreamSPtr_->SetSize(chunkSize + remainingBlockSize, true);
                        bytesRead = 0;
                        status = co_await fileStreamSPtr_->ReadAsync(*memoryStreamSPtr_, bytesRead, chunkSize, remainingBlockSize);
                        STORE_ASSERT(NT_SUCCESS(status), "Failed to read from filestream. status={1}", status);
                        STORE_ASSERT(bytesRead == remainingBlockSize, "bytesRead={1} != remainingBlockSize={2}", bytesRead, remainingBlockSize);
                        chunkSize = chunkSize + remainingBlockSize;

                        //create the binary reader again to update its base stream and keep the current position.
                        //this differs from managed since br in native made a copy of stream
                        ULONG currentPosition = brSPtr->Position;
                        status = SharedBinaryReader::Create(this->GetThisAllocator(), *memoryStreamSPtr_, brSPtr);
                        Diagnostics::Validate(status);
                        brSPtr->Position = currentPosition;
                    }

                    KSharedPtr<KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>> keysFromBlockSPtr = ReadBlock(currentBlockSize, *brSPtr);

                    for (ULONG i = 0; i < keysFromBlockSPtr->Count(); i++)
                    {
                        itemsBufferSPtr_->Append((*keysFromBlockSPtr)[i]);
                    }

                    // Move the reader ahead to the next block, if possible, else reset and break.
                    brSPtr->Position = alignedStartBlockOffset + alignedBlockSize;
                    if (alignedStartBlockOffset + alignedBlockSize >= chunkSize)
                    {
                        STORE_ASSERT(alignedStartBlockOffset + alignedBlockSize == chunkSize, "offset+size {1} != chunkSize {2}", alignedStartBlockOffset + alignedBlockSize, chunkSize);
                        break;
                    }
                }

                // Track the number of keys returned.
                keyCount_ += itemsBufferSPtr_->Count();

                STORE_ASSERT(itemsBufferSPtr_->Count() > 0, "items buffer count={1} should be 0", itemsBufferSPtr_->Count());
                co_return true;
            }

            KSharedPtr<KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>> ReadBlock(
                __in ULONG32 blockSize,
                __in BinaryReader& reader)
            {
                ULONG32 blockStartPosition = reader.Position;
                ULONG32 alignedBlockStartPosition = blockStartPosition - KeyChunkMetadata::Size;

                reader.Position = alignedBlockStartPosition + blockSize - sizeof(ULONG64);
                ULONG64 expectedChecksum;
                reader.Read(expectedChecksum);
                reader.Position = blockStartPosition;

                // Verify checksum.
                ULONG64 actualChecksum = CRC64::ToCRC64(*memoryStreamSPtr_, alignedBlockStartPosition, blockSize - sizeof(ULONG64));
                if (actualChecksum != expectedChecksum)
                {
                    //todo: throw invalid data exception.
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                KSharedPtr<KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>> keysDataSPtr = _new(KEYCHECKPOINTASYNCENUMERATOR_TAG, this->GetThisAllocator()) KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>();
                STORE_ASSERT(keysDataSPtr != nullptr, "keysDataSPtr should not be null");

                while(reader.Position < (alignedBlockStartPosition + blockSize - sizeof(ULONG64)))
                {
                    KSharedPtr<KeyData<TKey, TValue>> keyDataSPtr = keyCheckpointFileSPtr_->ReadKey<TKey, TValue>(reader, *keySerializerSPtr_);
                    keysDataSPtr->Append(keyDataSPtr);
                }

                STORE_ASSERT(reader.Position == (alignedBlockStartPosition + blockSize - sizeof(ULONG64)), "reader.Position={1} != expected position={2}", reader.Position, alignedBlockStartPosition + blockSize - sizeof(ULONG64));
                return keysDataSPtr;
            }

            ULONG64 GetChunkSize()
            {
                // Get chunk size of 64k if available, else remaining size.
                if (static_cast<ULONG64>(fileStreamSPtr_->Position) < endOffset_)
                {
                    ULONG64 remainingSize = endOffset_ - fileStreamSPtr_->Position;

                    if (remainingSize < ReadChunkSize)
                    {
                        return remainingSize;
                    }
                    else
                    {
                        return ReadChunkSize;
                    }
                }

                STORE_ASSERT(static_cast<ULONG64>(fileStreamSPtr_->Position) == endOffset_, "file stream position={1} != endOffset_={2}", static_cast<ULONG64>(fileStreamSPtr_->Position), endOffset_);
                return 0;
            }

            ULONG32 GetBlockSize(__in ULONG32 chunkSize)
            {
                // set block size to be a multiple of 4k bigger than the given value size.
                if (chunkSize %  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize == 0)
                {
                    return chunkSize;
                }
                else
                {
                    return  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize*(chunkSize /  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize + 1);
                }
            }

            KeyCheckpointFileAsyncEnumerator(
                __in KeyCheckpointFile& keyCheckpointFile,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in ULONG64 startOffset,
                __in ULONG64 endOffset,
                __in StoreTraceComponent & traceComponent);

            static const ULONG32 ChunkSize = 64 * 1024;
            static const ULONG32 ReadChunkSize = 32 * 1024;

            int index_;
            ULONG64 keyCount_;
            bool stateZero_;
            KSharedPtr<KeyCheckpointFile> keyCheckpointFileSPtr_;
            ULONG64 startOffset_;
            ULONG64 endOffset_;
            KSharedPtr<KeyData<TKey, TValue>> current_;
            KSharedPtr<KSharedArray<KSharedPtr<KeyData<TKey, TValue>>>> itemsBufferSPtr_;
            KSharedPtr<KBuffer> memoryStreamSPtr_;
            KSharedPtr<ktl::io::KFileStream> fileStreamSPtr_;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_;

            StoreTraceComponent::SPtr traceComponent_;

        };

        template<typename TKey, typename TValue>
        KeyCheckpointFileAsyncEnumerator<TKey, TValue>::KeyCheckpointFileAsyncEnumerator(
            __in KeyCheckpointFile& keyCheckpointFile,
            __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
            __in ULONG64 startOffset,
            __in ULONG64 endOffset,
            __in StoreTraceComponent & traceComponent)
            :keyCheckpointFileSPtr_(&keyCheckpointFile),
            keySerializerSPtr_(&keySerializer),
            startOffset_(startOffset),
            endOffset_(endOffset),
            traceComponent_(&traceComponent),
            index_(-1),
            keyCount_(0),
            stateZero_(true),
            itemsBufferSPtr_(nullptr),
            fileStreamSPtr_(nullptr),
            keyComparerSPtr_(nullptr),
            memoryStreamSPtr_(nullptr)
        {
        }

        template<typename TKey, typename TValue>
        KeyCheckpointFileAsyncEnumerator<TKey, TValue>::~KeyCheckpointFileAsyncEnumerator()
        {
        }
    };
}
