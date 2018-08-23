// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define VERSIONEDITEM_TAG 'mtIV'

namespace Data
{
   namespace TStore
   {
      class MetadataTable;
      class FileMetadata;

      template<typename TValue>
      class VersionedItem : public KObject<VersionedItem<TValue>>, public KShared<VersionedItem<TValue>>
      {
         K_FORCE_SHARED_WITH_INHERITANCE(VersionedItem);

      public:

         virtual ULONG GetFileId() const
         {
            return fileId_;
         }

         virtual void SetFileId(__in ULONG value)
         {
            fileId_ = value;
         }

         virtual bool IsInMemory() const
         {
            return (valueOffset_ & VersionedItem<TValue>::IsInMemoryFlag) == VersionedItem<TValue>::IsInMemoryFlag;
         }

         virtual void SetIsInMemory(__in bool value)
         {
            LONGLONG offset = 0;
            LONGLONG newOffset = 0;

            do
            {
               offset = valueOffset_;
               if (value)
               {
                  newOffset = offset | VersionedItem<TValue>::IsInMemoryFlag;
               }
               else
               {
                  newOffset = offset & (~VersionedItem<TValue>::IsInMemoryFlag);
               }

            } while (::InterlockedCompareExchange64(&valueOffset_, newOffset, offset) != offset);
         }

         LONG64 GetVersionSequenceNumber() const
         {
            return versionSequenceNumber_;
         }

         void SetVersionSequenceNumber(__in LONG64 value)
         {
            versionSequenceNumber_ = value;
         }

         virtual LONG32 GetValueSize() const
         {
            return valueSize_;
         }

         virtual void SetValueSize(__in LONG32 valueSize)
         {
            valueSize_ = valueSize;
         }

         virtual ULONG64 GetValueChecksum() const
         {
            return valueChecksum_;
         }

         virtual void SetValueChecksum(__in ULONG64 checksum)
         {
            valueChecksum_ = checksum;
         }

         virtual bool GetInUse() const
         {
            return (valueOffset_ & VersionedItem<TValue>::InUseFlag) == VersionedItem<TValue>::InUseFlag;
         }

         virtual void SetInUse(__in bool value)
         {
            LONG64 offset = 0;
            LONG64 newOffset = 0;

            do
            {
               offset = valueOffset_;
               if (value)
               {
                  newOffset = offset | VersionedItem<TValue>::InUseFlag;
               }
               else
               {
                  newOffset = offset & (~VersionedItem<TValue>::InUseFlag);
               }

            } while (::InterlockedCompareExchange64(&valueOffset_, newOffset, offset) != offset);
         }

         virtual bool GetLockBit()
         {
            return (valueOffset_ & VersionedItem<TValue>::LockFlag) == VersionedItem<TValue>::LockFlag;
         }

         virtual void SetLockBit(__in bool value)
         {
            LONG64 offset = 0;
            LONG64 newOffset = 0;

            // TODO: Check if the loop below spins taking up CPU cycles on perf runs.
            while (true)
            {
               offset = valueOffset_;

               if (value)
               {
                  if ((offset & VersionedItem::LockFlag) != 0i64) // already locked
                     continue;

                  newOffset = offset | VersionedItem::LockFlag;
               }
               else
               {
                  newOffset = offset & (~VersionedItem::LockFlag);
               }

               if(InterlockedCompareExchange64(&valueOffset_, newOffset, offset) == offset)
               {
                  break;
               }
            }
         }

         virtual LONG64 GetOffset() const
         {
            return (valueOffset_ & (~VersionedItem<TValue>::MetadataMask));
         }

         virtual void SetOffset(__in LONG64 value, __in StoreTraceComponent & traceComponent)
         {
            if ((value & VersionedItem<TValue>::IsInMemoryFlag) == VersionedItem<TValue>::IsInMemoryFlag
               || (value & VersionedItem<TValue>::InUseFlag) == VersionedItem<TValue>::InUseFlag
               || (value & VersionedItem<TValue>::LockFlag) == VersionedItem<TValue>::LockFlag) // If either InUse or InMemory or LockFlagare set, then flag as error 
            {
               // Offset only supports long values up till 2^62-1.
               ASSERT_IFNOT(false, "{0}: Offset only supports long values up till 2^62-1", traceComponent.AssertTag);
            }

            LONG64 offset = 0;
            LONG64 newOffset = 0;

            do
            {
               offset = valueOffset_;
               newOffset = (offset & VersionedItem<TValue>::MetadataMask) | value; // Retain the InMemory/InUse flags from old value but replace the last 62 bits

            } while (::InterlockedCompareExchange64(&valueOffset_, newOffset, offset) != offset);
         }

         virtual void AcquireLock()
         {
            // if sweep is not enabled
            SetLockBit(true);
         }

         virtual void ReleaseLock(StoreTraceComponent & traceComponent)
         {
            if ((valueOffset_ & VersionedItem::LockFlag) == 0i64) // already unlocked
            {
               ASSERT_IFNOT(false, "{0}: It is not valid to unset the lock bit when it is already unset.", traceComponent.AssertTag);
            }

            SetLockBit(false);
         }

         virtual ktl::Awaitable<TValue> GetValueAsync(
            __in MetadataTable& metadataTable,
            __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
            __in ReadMode readMode,
            __in StoreTraceComponent & traceComponent,
            __in ktl::CancellationToken const & cancellationToken)
         {
            UNREFERENCED_PARAMETER(cancellationToken);
            MetadataTable::SPtr metadataTableSPtr = &metadataTable;
            bool loadValueFromDisk = false;

            StoreTraceComponent::SPtr traceComponentSPtr = &traceComponent;

            if (readMode == ReadMode::Off)
            {
                // Since we don't care about values for ReadMode::Off, it is OK to not take a lock
                co_return this->GetValue();
            }

            // Check if value needs to be loaded from the disk.
            {
               this->AcquireLock();

               KFinally([&]
               {
                  this->ReleaseLock(*traceComponentSPtr);
               });

               if (this->IsInMemory() == false)
               {
                  loadValueFromDisk = true;
               }
               else
               {
                  this->SetInUse(true);

                  // Always call set and get within the lock - if TValue is a shared ptr its access should be protected.
                  co_return this->GetValue();
               }
            }

            // load it from disk
            TValue value = co_await MetadataManager::ReadValueAsync<TValue>(*metadataTableSPtr, *this, valueSerializer);

            // Acquire lock before setting the flags

            if (readMode == ReadMode::CacheResult)
            {
                this->AcquireLock();

                KFinally([&]
                {
                    this->ReleaseLock(*traceComponentSPtr);
                });

                // Always call set and get within the lock - if TValue is a shared ptr its access should be protected.
                this->SetValue(value);

                // Update in memory after setting value
                this->SetIsInMemory(true);

                // Set in use only after updating the value
                this->SetInUse(true);
            }

            co_return value;
         }

         virtual ktl::Awaitable<TValue> GetValueAsync(
            __in FileMetadata& fileMetadata,
            __in Data::StateManager::IStateSerializer<TValue>& valueSerializer,
            __in ReadMode readMode,
            __in StoreTraceComponent & traceComponent,
            __in ktl::CancellationToken const & cancellationToken)
         {
            UNREFERENCED_PARAMETER(cancellationToken);
            FileMetadata::SPtr fileMetadataSPtr = &fileMetadata;
            bool loadValueFromDisk = false;

            StoreTraceComponent::SPtr traceComponentSPtr = &traceComponent;

            if (readMode == ReadMode::Off)
            {
                // Since we don't care about values for ReadMode::Off, it is OK to not take a lock
                co_return this->GetValue();
            }

            // Check if value needs to be loaded.
            {
               this->AcquireLock();

               KFinally([&]
               {
                  this->ReleaseLock(*traceComponentSPtr);
               });

               if (this->IsInMemory() == false)
               {
                  loadValueFromDisk = true;
               }
               else
               {
                  // Always call set and get within the lock - if TValue is a shared ptr its access should be protected.
                  co_return this->GetValue();
               }
            }

            // load it from disk
            TValue value = co_await MetadataManager::ReadValueAsync<TValue>(*fileMetadataSPtr, *this, valueSerializer);

            if (readMode == ReadMode::CacheResult)
            {
                // Acquire lock before setting the flags
                this->AcquireLock();

                KFinally([&]
                {
                    this->ReleaseLock(*traceComponentSPtr);
                });

                // Always call set and get within the lock - if TValue is a shared ptr its access should be protected.
                this->SetValue(value);

                // Update in memory after setting value
                this->SetIsInMemory(true);

                // Set in use only after updating the value
                this->SetInUse(true);
            }

            co_return value;
         }

         virtual RecordKind GetRecordKind() const = 0;
         virtual TValue GetValue() const = 0;
         virtual void SetValue(__in const TValue& value) = 0;
         virtual void UnSetValue() = 0;

      protected:
         volatile LONG64 valueOffset_ = 0;
         LONG64          versionSequenceNumber_ = 0;

         ULONG32     fileId_ = 0;
         LONG32      valueSize_ = -1;
         ULONG64    valueChecksum_ = 0;

      private:
         static const LONG64 IsInMemoryFlag = 1LL << 63;  // Most significant bit
         static const LONG64 InUseFlag = 1LL << 62;     // Most significant second bit
         static const LONG64 LockFlag = 1LL << 61; // Most significant third bit
         static const LONG64 MetadataMask = VersionedItem<TValue>::IsInMemoryFlag | VersionedItem<TValue>::InUseFlag | VersionedItem<TValue>::LockFlag;
      };

      template <typename TValue>
      VersionedItem<TValue>::VersionedItem()
      {
      }

      template <typename TValue>
      VersionedItem<TValue>::~VersionedItem()
      {
      }
   }
}
