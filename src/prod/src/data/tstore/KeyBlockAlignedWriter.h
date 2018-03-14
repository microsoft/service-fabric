// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEYBLOCKALIGNEDWRITER_TAG 'waBK'

namespace Data
{
    namespace TStore
    {

        template<typename TKey, typename TValue>
        class KeyBlockAlignedWriter : 
            public KObject<KeyBlockAlignedWriter<TKey, TValue>>,
            public KShared<KeyBlockAlignedWriter<TKey, TValue>>
        {
            K_FORCE_SHARED(KeyBlockAlignedWriter)

        public:

            static NTSTATUS
                Create(
                    __in ktl::io::KFileStream& keyFileStream,
                    __in KeyCheckpointFile& keyCheckpointFile,
                    __in SharedBinaryWriter& keyBuffer,
                    __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                    __in ULONG64 logicTimestamp,
                    __in StoreTraceComponent & traceComponent,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(KEYBLOCKALIGNEDWRITER_TAG, allocator) 
                    KeyBlockAlignedWriter(keyFileStream, keyCheckpointFile, keyBuffer, keySerializer, logicTimestamp, traceComponent);

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

            static const ULONG32 ChecksumSize = sizeof(ULONG64);


            //
            // Batches given keys and writes them on disk in 4K aligned batches.
            // The data is written is 8 bytes aligned.
            // 
            // Name                    Type        Size
            // 
            // BlockMetadata
            // BlockSize               int         4
            // RESERVED                            4
            // 
            // Block                   byte[]      N
            // PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
            //
            // Checksum                ulong       8
            // 
            // RESERVED: Fixed padding that is usable to add fields in future.
            // PADDING:  Due to dynamic size, cannot be used for adding fields.
            // 
            // Note: Larges Block supported is 2GB
            // 
            ktl::Awaitable<void> BlockAlignedWriteKeyAsync(__in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> item)
            {
                STORE_ASSERT(tempKeyBufferSPtr_->Position == 0, "position={1} should be 0", tempKeyBufferSPtr_->Position);
                keyCheckpointFileSPtr_->WriteItem<TKey, TValue>(*tempKeyBufferSPtr_, item, *keySerializerSPtr_, logicalTimeStamp_);

                // Temp key buffer always gets reset after every key copy, so it is safe to assume position as record size.
                ULONG32 keyRecordsize = tempKeyBufferSPtr_->Position;

                // Case1: when record size is lesser than or equal to 4k.
                if (keyRecordsize + ChecksumSize + KeyChunkMetadata::Size <= blockAlignmentSize_)
                {
                    // Check if the current record can fit into this block (just account for checksum).
                    if (currentBlockPosition_ + keyRecordsize + ChecksumSize <=  blockAlignmentSize_)
                    {
                        // Do a mem copy.
                        co_await CopyStreamAsync();
                    }
                    else
                    {
                        STORE_ASSERT(currentBlockPosition_ > 0, "currentBlockPosition={1} should be > 0", currentBlockPosition_);

                        // write checksum since we are done with this 4k block.
                        WriteEndOfBlock();

                        // Skip to next 4k boundary.
                        // Value does not fit into the current block, move position to the 4k+1 
                        //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                        ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
                        ByteAlignedReaderWriterHelper::AppendBadFood(*keyBufferSPtr_, skipSize);

                        co_await ResetCurrentBlockPositionAndFlushIfNecessaryAsync();

                        // Reset block size to default if value size is smaller, else pick the size correctly.
                        if (keyRecordsize + ChecksumSize + KeyChunkMetadata::Size <=  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize)
                        {
                            blockAlignmentSize_ =  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
                        }
                        else
                        {
                            GetBlockSize(keyRecordsize + ChecksumSize + KeyChunkMetadata::Size);
                        }

                        // Do a mem copy.
                        co_await CopyStreamAsync();
                    }
                }
                else
                {
                    // Case 2: Value size is greater than block size
                    if (currentBlockPosition_ > 0)
                    {
                        // write checksum since we are done with this 4k block.
                        WriteEndOfBlock();

                        // Move the memory stream ahead to a 4k boundary.
                        // Value does not fit into the current block, move position to the 4k+1 
                        //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                        ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
                        ByteAlignedReaderWriterHelper::AppendBadFood(*keyBufferSPtr_, skipSize);

                        // Reset current position.
                        co_await ResetCurrentBlockPositionAndFlushIfNecessaryAsync();
                    }

                    // set block size to be a multiple of 4k bigger than the given value size.
                    GetBlockSize(keyRecordsize + ChecksumSize + KeyChunkMetadata::Size);

                    // Do a mem copy.
                    co_await CopyStreamAsync();
                }       
            }

            ktl::Awaitable<void> FlushAsync()
            {
                if (currentBlockPosition_ > 0)
                {
                    WriteEndOfBlock();

                    // Move the memory stream ahead to a 4k boundary.
                    //this.keyBuffer.BaseStream.Position += (this.blockAlignmentSize - this.currentBlockPosition);
                    ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
                    ByteAlignedReaderWriterHelper::AppendBadFood(*keyBufferSPtr_, skipSize);
                }

                // Flush the memory buffer periodically to disk.
                if (keyBufferSPtr_->Position > 0)
                {
                    co_await keyCheckpointFileSPtr_->FlushMemoryBufferAsync(*keyFileStreamSPtr_, *keyBufferSPtr_);
                }

                ULONG32 remainder = keyFileStreamSPtr_->Position % BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
                STORE_ASSERT(remainder == 0, "remainder={1} should be 0", remainder);

                co_await keyCheckpointFileSPtr_->FlushAsync(*keyFileStreamSPtr_, *keyBufferSPtr_);
            }
  

        private:

            ktl::Awaitable<void> CopyStreamAsync()
            {
                if (currentBlockPosition_ == 0)
                {
                    ReserveSpaceForKeyBlockMetadata();
                }

                // TODO: account for key metadata correctly.
                // 32 is the metadata size of DeletedVersion which has the smallest metadata.
                STORE_ASSERT(tempKeyBufferSPtr_->Position >= 32, "position={1} >= 32", tempKeyBufferSPtr_->Position);
                ULONG32 totalRecordSize = tempKeyBufferSPtr_->Position;
                keyBufferSPtr_->Write(*tempKeyBufferSPtr_->GetBuffer(0, totalRecordSize));

                // Reset temporary stream.
                tempKeyBufferSPtr_->Position = 0;

                STORE_ASSERT(currentBlockPosition_ + totalRecordSize <= blockAlignmentSize_, "position={1} + offset={2} <= blockAlignmentSize_={3}", currentBlockPosition_, totalRecordSize, blockAlignmentSize_);

                // Update current block position.
                currentBlockPosition_ += totalRecordSize;

                if (currentBlockPosition_ + ChecksumSize == blockAlignmentSize_)
                {
                    // write checksum since we are done with this 4k block.
                    WriteEndOfBlock();

                    co_await ResetCurrentBlockPositionAndFlushIfNecessaryAsync();

                    // reset block size to default.
                    blockAlignmentSize_ =  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
                }
            }

            void GetBlockSize(__in ULONG32 recordSize)
            {
                // set block size to be a multiple of 4k bigger than the given value size.
                if (recordSize% BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize == 0)
                {
                    blockAlignmentSize_ = recordSize;
                }
                else
                {
                    blockAlignmentSize_ = ( BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize*
                        (recordSize /  BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize + 1));
                }
            }

            // <summary>
            // This function not only writes the end of the block (checksum) but also 
            // writes the metadata about the block (size and reserved fields in the beginning.
            // </summary>
            // <remarks>
            // The data is written is 8 bytes aligned.
            // 
            // Name                    Type        Size
            // 
            // Metadata
            // BlockSize               int         4
            // RESERVED                            4
            // 
            // Block                   byte[]      N
            // PADDING                             (N % 8 ==0) ? 0 : 8 - (N % 8)
            //
            // Checksum                ulong       8
            // 
            // RESERVED: Fixed padding that is usable to add fields in future.
            // PADDING:  Due to dynamic size, cannot be used for adding fields.
            // 
            // Note: Larges Block supported is 2GB
            // </remarks>
            void WriteEndOfBlock()
            {
                STORE_ASSERT(currentBlockPosition_ > 0, "position={1} > 0", currentBlockPosition_);
                STORE_ASSERT(currentBlockPosition_ < blockAlignmentSize_, "position={1} < size={2}", currentBlockPosition_, blockAlignmentSize_);

                ByteAlignedReaderWriterHelper::AssertIfNotAligned(keyBufferSPtr_->Position);

                // Write blocksize and checksum.
                ULONG32 blockEndPosition = keyBufferSPtr_->Position;
                ULONG32 blockStartPosition = blockEndPosition - currentBlockPosition_;

                // Move buffer to start position.
                keyBufferSPtr_->Position = blockStartPosition;

                // Write block size - current block position + checksum will account for block size.
                ULONG32 blockSize = currentBlockPosition_ + ChecksumSize;
                STORE_ASSERT(blockSize > 0, "blockSize={1} > 0", blockSize);
                KeyChunkMetadata keyChunkMeta(blockSize);
                keyChunkMeta.Write(*keyBufferSPtr_);

                ByteAlignedReaderWriterHelper::AssertIfNotAligned(keyBufferSPtr_->Position);

                // Move to end  of stream to write checksum.
                keyBufferSPtr_->Position = blockEndPosition;
                ULONG64 checksum = CRC64::ToCRC64(*keyBufferSPtr_->GetBuffer(0), blockStartPosition, currentBlockPosition_);
                keyBufferSPtr_->Write(checksum);

                ByteAlignedReaderWriterHelper::AssertIfNotAligned(keyBufferSPtr_->Position);

                currentBlockPosition_ += ChecksumSize;
            }

            ktl::Awaitable<void> ResetCurrentBlockPositionAndFlushIfNecessaryAsync()
            {
                // Reset current block position.
                currentBlockPosition_ = 0;

                // Flush the memory buffer periodically to disk.
                if (keyBufferSPtr_->Position >= KeyCheckpointFile::MemoryBufferFlushSize )
                {
                    co_await keyCheckpointFileSPtr_->FlushMemoryBufferAsync(*keyFileStreamSPtr_, *keyBufferSPtr_);
                }
            }

            //
            // Reserves enough space for the KeyBlockMetadata.
            //
            void ReserveSpaceForKeyBlockMetadata()
            {
                STORE_ASSERT(currentBlockPosition_ == 0, "currentBlockPosition_={1} == 0", currentBlockPosition_);

                // Write size at the start of every flush block.
                // Leave space for an int 'size' at the front of the memory stream.
                keyBufferSPtr_->Position += KeyChunkMetadata::Size;

                // Move current position to size + Reserved.
                currentBlockPosition_ += KeyChunkMetadata::Size;
            }

            KeyBlockAlignedWriter(
                __in ktl::io::KFileStream& keyFileStream,
                __in KeyCheckpointFile& keyCheckpointFile,
                __in SharedBinaryWriter& keyBuffer,
                __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
                __in ULONG64 logicTimestamp,
                __in StoreTraceComponent & traceComponent);

            ULONG32 blockAlignmentSize_;
            ULONG32 currentBlockPosition_;
            KSharedPtr<KeyCheckpointFile> keyCheckpointFileSPtr_;
            KSharedPtr<SharedBinaryWriter> keyBufferSPtr_;
            KSharedPtr<ktl::io::KFileStream> keyFileStreamSPtr_;
            KSharedPtr<Data::StateManager::IStateSerializer<TKey>> keySerializerSPtr_;
            ULONG64 logicalTimeStamp_;
            KSharedPtr<SharedBinaryWriter> tempKeyBufferSPtr_;

            StoreTraceComponent::SPtr traceComponent_;
        };

        template<typename TKey, typename TValue>
        KeyBlockAlignedWriter<TKey, TValue>::KeyBlockAlignedWriter(
            __in ktl::io::KFileStream& keyFileStream,
            __in KeyCheckpointFile& keyCheckpointFile,
            __in SharedBinaryWriter& keyBuffer,
            __in Data::StateManager::IStateSerializer<TKey>& keySerializer,
            __in ULONG64 logicTimestamp,
            __in StoreTraceComponent & traceComponent)
            :keyCheckpointFileSPtr_(&keyCheckpointFile),
            keyFileStreamSPtr_(&keyFileStream),
            keyBufferSPtr_(&keyBuffer),
            keySerializerSPtr_(&keySerializer),
            blockAlignmentSize_( BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize),
            currentBlockPosition_(0),
            logicalTimeStamp_(logicTimestamp),
            traceComponent_(&traceComponent)
        {
            NTSTATUS status = SharedBinaryWriter::Create(this->GetThisAllocator(), tempKeyBufferSPtr_);

            if (!NT_SUCCESS(status))
            {
                this->SetConstructorStatus(status);
                return;
            }
        }

        template<typename TKey, typename TValue>
        KeyBlockAlignedWriter<TKey, TValue>::~KeyBlockAlignedWriter()
        {
        }
    }
}
