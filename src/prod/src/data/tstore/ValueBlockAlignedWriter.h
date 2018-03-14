// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>
      class ValueBlockAlignedWriter :
         public KObject<ValueBlockAlignedWriter<TKey, TValue>>,
         public KShared<ValueBlockAlignedWriter<TKey, TValue>>
      {
         K_FORCE_SHARED(ValueBlockAlignedWriter)

      public:
         static NTSTATUS Create(
            __in ktl::io::KFileStream& fileStream,
            __in ValueCheckpointFile& valueCheckpointFile,
            __in SharedBinaryWriter& binaryWriter,
            __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
            __in StoreTraceComponent & traceComponent,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(VERSIONEDITEM_TAG, allocator) ValueBlockAlignedWriter(fileStream, valueCheckpointFile, binaryWriter, valueSerializer, traceComponent);

            if (!output)
            {
               status = STATUS_INSUFFICIENT_RESOURCES;
               return status;
            }

            status = output->Status();
            if (!NT_SUCCESS(status))
            {
               return status;
            }

            result = Ktl::Move(output);
            return STATUS_SUCCESS;
         }

         //
         // Parameter Value can be null, so it should not be passed by reference.
         //
         ktl::Awaitable<void> BlockAlignedWriteValueAsync(
            __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> item,
            __in KSharedPtr<KBuffer> value,
            __in bool shouldSerialize)
         {
            if (item.Value->GetRecordKind() == RecordKind::DeletedVersion)
            {
               valueCheckpointFileSPtr_->WriteItem(*valueFileStreamSPtr_, *valueBufferSPtr_, *item.Value, *valueSerializerSPtr_);
               co_return;
            }

            LONG32 valueSize = item.Value->GetValueSize();
            ASSERT_IFNOT(valueSize >= 0, "valueSize is greater than zero {0}", valueSize);
            ULONG32 uValueSize = static_cast<ULONG32>(valueSize);

            // Case 1: The value size is smaller than 4k
            if (uValueSize <= blockAlignmentSize_)
            {
               // If it falls within the block
                if (currentBlockPosition_ + uValueSize <= blockAlignmentSize_)
               {
                  co_await WriteItemAsync(item, value, shouldSerialize);
               }
               else
               {
                  // Value does not fit into the current block, move position to the 4k+1 
                  ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
                  ByteAlignedReaderWriterHelper::AppendBadFood(*valueBufferSPtr_, skipSize);

                  // Reset current position.
                  currentBlockPosition_ = 0;

                  // Reset block size to default if value size is smaller, else pick the size correctly.
                  if (uValueSize <= BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize)
                  {
                     blockAlignmentSize_ = BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
                  }
                  else
                  {
                      SetBlockSize(item, uValueSize);
                  }

                  // Write key and value
                  co_await WriteItemAsync(item, value, shouldSerialize);
               }
            }
            else
            {
               // Case 2: Value size is greater than block size
               if (currentBlockPosition_ > 0)
               {
                  // Move the memory stream ahead to a 4k boundary.
                  // Value does not fit into the current block, move position to the 4k+1 

                  ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
                  ByteAlignedReaderWriterHelper::AppendBadFood(*valueBufferSPtr_, skipSize);

                  // Reset current position
                  currentBlockPosition_ = 0;
               }

               // Set block size to be a multiple of 4k bigger than the given value size.
               SetBlockSize(item, uValueSize);

               // Write key and value
               co_await WriteItemAsync(item, value, shouldSerialize);
            }

            co_return;
         }

         ktl::Awaitable<void> FlushAsync()
         {
            if (currentBlockPosition_ > 0)
            {
               ULONG32 skipSize = blockAlignmentSize_ - currentBlockPosition_;
               ByteAlignedReaderWriterHelper::AppendBadFood(*valueBufferSPtr_, skipSize);
            }

            co_await valueCheckpointFileSPtr_->FlushAsync(*valueFileStreamSPtr_, *valueBufferSPtr_);
            co_return;
         }

      private:

         void SetBlockSize(__in KeyValuePair<TKey, __in KSharedPtr<VersionedItem<TValue>>>& item, __in ULONG32 valueSize)
         {
             UNREFERENCED_PARAMETER(item);
            // set block size to be a multiple of 4k bigger than the given value size.
             if (valueSize % BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize == 0)
            {
                 blockAlignmentSize_ = valueSize;
            }
            else
            {
               blockAlignmentSize_ = BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize *
                   (valueSize / BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize + 1);
            }
         }

         //
         // Parameter Value can be null, so it should not be passed by reference.
         //
         ktl::Awaitable<void> WriteItemAsync(
            __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> item,
            __in KSharedPtr<KBuffer>& value,
            __in bool shouldSerialize)
         {
            STORE_ASSERT(currentBlockPosition_ < blockAlignmentSize_, "currentBlockPosition_={1} <= blockAlignmentSize_={2}", currentBlockPosition_, blockAlignmentSize_);

            // Write the value
            ULONG32 startPosition = static_cast<ULONG32>(valueBufferSPtr_->Position);
            ULONG32 startBlockPosition = currentBlockPosition_;
            ULONG32 originalValueSize = item.Value->GetValueSize();
            STORE_ASSERT(originalValueSize >= 0, "originalValueSize={1} <= 0", originalValueSize);

            if (shouldSerialize)
            {
               valueCheckpointFileSPtr_->WriteItem(*valueFileStreamSPtr_, *valueBufferSPtr_, *item.Value, *valueSerializerSPtr_);
            }
            else
            {
               STORE_ASSERT(value != nullptr, "value is null");
               valueCheckpointFileSPtr_->WriteItem(*valueFileStreamSPtr_, *valueBufferSPtr_, *item.Value, *value);
            }

            ULONG32 endPosition = static_cast<ULONG32>(valueBufferSPtr_->Position);

            STORE_ASSERT(endPosition >= startPosition, "endPosition {1} >= startPosition {2}", endPosition, startPosition);
            ULONG32 valueSize = endPosition - startPosition;
            currentBlockPosition_ += valueSize;

            // Validate value size
            if (valueSize > originalValueSize)
            {
               if (currentBlockPosition_ > blockAlignmentSize_)
               {
                  ULONG32 shiftOffset = blockAlignmentSize_ - startBlockPosition;
                  ULONG32 dstOffset = startPosition + shiftOffset;

                  // Binary writer will expand the underlying stream on writes so there is no need to set length.

                  // Calculate the new block.
                  currentBlockPosition_ = 0;
                  SetBlockSize(item, valueSize);


                  KBuffer::SPtr bufferSPtr = valueBufferSPtr_->GetBuffer(startPosition);

                  valueBufferSPtr_->Position = dstOffset;
                  valueBufferSPtr_->Write(bufferSPtr.RawPtr(), valueSize);
                  item.Value->SetOffset(item.Value->GetOffset() + shiftOffset, *traceComponent_);

                  currentBlockPosition_ += valueSize;
               }
            }

            // Flush the memory buffer periodically to disk.
            if (valueBufferSPtr_->Position >= ValueCheckpointFile::MemoryBufferFlushSize)
            {
               co_await valueCheckpointFileSPtr_->FlushMemoryBufferAsync(*valueFileStreamSPtr_, *valueBufferSPtr_);
            }

            // Assert that the current block position is lesser than or equal to BlockAlignmentSize.
            STORE_ASSERT(currentBlockPosition_ <= blockAlignmentSize_, "currentBlockPosition_={1} <= blockAlignmentSize_={2}", currentBlockPosition_, blockAlignmentSize_);

            if (currentBlockPosition_ == blockAlignmentSize_)
            {
               // Reset current position.
               currentBlockPosition_ = 0;

               // reset block size to default.
               blockAlignmentSize_ = BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
            }

            co_return;
         }

         ValueBlockAlignedWriter<TKey, TValue>(
            __in ktl::io::KFileStream& fileStream,
            __in ValueCheckpointFile& valueCheckpointFile,
            __in SharedBinaryWriter& binaryWriter,
            __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
            __in StoreTraceComponent & traceComponent);

         ULONG32 blockAlignmentSize_ = 0;
         ULONG32 currentBlockPosition_ = 0;
         ValueCheckpointFile::SPtr valueCheckpointFileSPtr_;
         SharedBinaryWriter::SPtr valueBufferSPtr_;
         ktl::io::KFileStream::SPtr valueFileStreamSPtr_;
         KSharedPtr<Data::StateManager::IStateSerializer<TValue>> valueSerializerSPtr_;
         StoreTraceComponent::SPtr traceComponent_;
      };

      template <typename TKey, typename TValue>
      ValueBlockAlignedWriter<TKey, TValue>::ValueBlockAlignedWriter(
          __in ktl::io::KFileStream& fileStream,
          __in ValueCheckpointFile& valueCheckpointFile,
          __in SharedBinaryWriter& binaryWriter,
          __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
          __in StoreTraceComponent & traceComponent)
          : valueCheckpointFileSPtr_(&valueCheckpointFile),
          valueBufferSPtr_(&binaryWriter),
          valueFileStreamSPtr_(&fileStream),
          valueSerializerSPtr_(&valueSerializer),
          traceComponent_(&traceComponent)
      {
         blockAlignmentSize_ = BlockAlignedWriter<TKey, TValue>::DefaultBlockAlignmentSize;
      }

      template <typename TKey, typename TValue>
      ValueBlockAlignedWriter<TKey, TValue>::~ValueBlockAlignedWriter()
      {
      }
   }
}
