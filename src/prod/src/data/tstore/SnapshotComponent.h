// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SNAPSHOT_COMPONENT_TAG 'snCT'

namespace Data
{
    namespace TStore
    {
        // Forward declaring to break circular dependency
        template<typename TKey, typename TValue>
        class SnapshotContainer; 

        template<typename TKey, typename TValue>
        class SnapshotComponent
            : public KObject<SnapshotComponent<TKey, TValue>>
            , public KShared<SnapshotComponent<TKey, TValue>>
        {
            K_FORCE_SHARED(SnapshotComponent)
        public:
            static NTSTATUS Create(
                __in SnapshotContainer<TKey, TValue> & containerSPtr,
                __in bool isValueAReferenceType,
                __in IComparer<TKey> & keyComparer,
                __in StoreTraceComponent & traceComponent,
                __in KAllocator & allocator,
                __out SPtr& result)
            {
                NTSTATUS status;

                SPtr output = _new(SNAPSHOT_COMPONENT_TAG, allocator) SnapshotComponent(containerSPtr, isValueAReferenceType, keyComparer, traceComponent);

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

            __declspec(property(get = get_Size)) LONG64 Size;
            LONG64 get_Size() const
            {
                return size_;
            }

            __declspec(property(get = get_Count)) ULONG32 Count;
            ULONG32 get_Count() const
            {
                return static_cast<ULONG32>(componentSPtr_->Count);
            }

            void Add(__in TKey & key, __in VersionedItem<TValue> & value)
            {
                KSharedPtr<VersionedItem<TValue>> valueSPtr(&value);

                // TODO: Determine whether this is actually needed
                auto updateFunc = [&](__in TKey, __in KSharedPtr<VersionedItem<TValue>> currentValue)
                {
                    bool shouldUpdate = currentValue->GetVersionSequenceNumber() <= valueSPtr->GetVersionSequenceNumber();
                    if (shouldUpdate)
                    {
                        if (currentValue->IsInMemory())
                        {
                            InterlockedAdd64(&size_, -currentValue->GetValueSize());
                            STORE_ASSERT(size_ >= 0, "Size {1} should not be negative", size_);
                        }

                        return valueSPtr;
                    }

                    return currentValue;
                };

                componentSPtr_->AddOrUpdate(key, valueSPtr, updateFunc);

                if (valueSPtr->IsInMemory())
                {
                    InterlockedAdd64(&size_, valueSPtr->GetValueSize());
                }
            }

            KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key, __in LONG64 visibilityLSN) const
            {
                KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;
                bool result = componentSPtr_->TryGetValue(key, versionedItemSPtr);
                if (result)
                {
                    if (versionedItemSPtr->GetVersionSequenceNumber() <= visibilityLSN)
                    {
                        return versionedItemSPtr;
                    }
                }

                return nullptr;
            }

            ktl::Awaitable<KSharedPtr<StoreComponentReadResult<TValue>>> ReadAsync(__in TKey & key, __in LONG64 visibilityLSN, __in ReadMode readMode)
            {
                KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;
                bool result = componentSPtr_->TryGetValue(key, versionedItemSPtr);

                if (result)
                {
                    TValue value = TValue();
                    bool shouldLoadFromDisk = false;

                    {
                        versionedItemSPtr->AcquireLock();
                        KFinally([&] { versionedItemSPtr->ReleaseLock(*traceComponent_); });

                        if (versionedItemSPtr->IsInMemory())
                        {
                            value = versionedItemSPtr->GetValue();
                        }
                        else
                        {
                            shouldLoadFromDisk = true;
                        }
                    }

                    if (shouldLoadFromDisk)
                    {
                        // Get file metadata table.
                        ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr fileMetadataTableSPtr = nullptr;

                        ConcurrentDictionary<LONG64, ConcurrentDictionary<ULONG32, FileMetadata::SPtr>::SPtr>::SPtr fileMetadataTableLsnMapSPtr;
                        fileMetadataTableLsnMapSPtr = containerSPtr_->FileMetadataTableLsnMap;
                        
                        fileMetadataTableLsnMapSPtr->TryGetValue(visibilityLSN, fileMetadataTableSPtr);

                        STORE_ASSERT(fileMetadataTableSPtr != nullptr, "File metadata table is null.");
                        STORE_ASSERT(versionedItemSPtr->GetFileId() > 0, "FileId must be positive.");

                        FileMetadata::SPtr fileMetadataSPtr = nullptr;
                        bool foundMetadata = fileMetadataTableSPtr->TryGetValue(versionedItemSPtr->GetFileId(), fileMetadataSPtr);
                        STORE_ASSERT(foundMetadata, "Failed to find file id");

                        if (versionedItemSPtr->GetRecordKind() != RecordKind::DeletedVersion && readMode == ReadMode::CacheResult)
                        {
                            versionedItemSPtr->SetInUse(true);

                            // Increment size - concurrent loads may cause overcounting
                            InterlockedAdd64(&size_, versionedItemSPtr->GetValueSize());
                        }

                        // Load the value into memory.
                        value = co_await versionedItemSPtr->GetValueAsync(*fileMetadataSPtr, *containerSPtr_->ValueSerializer, readMode, *traceComponent_, ktl::CancellationToken::None);
                    }

                    if (versionedItemSPtr->GetVersionSequenceNumber() <= visibilityLSN)
                    {
                        KSharedPtr<StoreComponentReadResult<TValue>> readResultSPtr = nullptr;
                        StoreComponentReadResult<TValue>::Create(versionedItemSPtr, value, this->GetThisAllocator(), readResultSPtr);
                        co_return readResultSPtr;
                    }
                }

                versionedItemSPtr = nullptr;
                KSharedPtr<StoreComponentReadResult<TValue>> readResultSPtr = nullptr;
                TValue defaultValue = TValue();
                StoreComponentReadResult<TValue>::Create(versionedItemSPtr, defaultValue, this->GetThisAllocator(), readResultSPtr);
                co_return readResultSPtr;
            }

            KSharedPtr<IFilterableEnumerator<TKey>> GetEnumerable()
            {
                KSharedPtr<IFilterableEnumerator<TKey>> enumeratorSPtr;
                auto status = ConcurrentSkipListFilterableEnumerator<TKey, KSharedPtr<VersionedItem<TValue>>>::Create(*componentSPtr_, enumeratorSPtr);
                Diagnostics::Validate(status);
                return enumeratorSPtr;
            }

        private:
           SnapshotComponent(
              __in SnapshotContainer<TKey, TValue> & containerSPtr,
              __in bool isValueAReferenceType,
              __in IComparer<TKey> & keyComparer,
              __in StoreTraceComponent & traceComponent)
              : isValueAReferenceType_(isValueAReferenceType),
              containerSPtr_(&containerSPtr),
              componentSPtr_(nullptr),
              size_(0),
              traceComponent_(&traceComponent)
           {
              KSharedPtr<IComparer<TKey>> keyComparerSPtr = &keyComparer;
              NTSTATUS status = ConcurrentSkipList<TKey, KSharedPtr<VersionedItem<TValue>>>::Create(keyComparerSPtr, this->GetThisAllocator(), componentSPtr_);
              Diagnostics::Validate(status);
           }
            
            KSharedPtr<SnapshotContainer<TKey, TValue>> containerSPtr_;
            KSharedPtr<ConcurrentSkipList<TKey, KSharedPtr<VersionedItem<TValue>>>> componentSPtr_;
            bool isValueAReferenceType_;
            LONG64 size_;

            StoreTraceComponent::SPtr traceComponent_;
        };
        
        template <typename TKey, typename TValue>
        SnapshotComponent<TKey, TValue>::~SnapshotComponent()
        {
            containerSPtr_ = nullptr;
            componentSPtr_ = nullptr;
        }
    }
}
