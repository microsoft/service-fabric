// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONSOLIDATEDSTORECOMPONENT_TAG 'pmCC'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class ConsolidatedStoreComponent : public KObject<ConsolidatedStoreComponent<TKey, TValue>>, public KShared<ConsolidatedStoreComponent<TKey, TValue>>
        {
            K_FORCE_SHARED(ConsolidatedStoreComponent)

        public:
            static NTSTATUS Create(
                __in IComparer<TKey>& keyComparer,
                __in KAllocator& allocator,
                __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(CONSOLIDATEDSTORECOMPONENT_TAG, allocator) ConsolidatedStoreComponent(keyComparer);

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

            // Best effort estimate of the size in memory for the component
            __declspec(property(get = get_Size)) LONG64 Size;
            LONG64 get_Size() const
            {
                return size_;
            }

            void Add(__in TKey& key, __in VersionedItem<TValue>& value)
            {
                KInvariant(value.GetRecordKind() != RecordKind::DeletedVersion);
                KSharedPtr<VersionedItem<TValue>> valueSPtr = &value;

                // There are no concurrent checkpoint, so it is okay to do it without a lock.
                auto existingValue = Read(key);
                KInvariant(existingValue == nullptr);

                componentSPtr_->Add(key, valueSPtr);

                if (value.IsInMemory() == true)
                {
                   InterlockedAdd64(&size_, value.GetValueSize());
                }
            }

            void Update(__in TKey& key, VersionedItem<TValue>& value)
            {
                auto existingValue = Read(key);
                ASSERT_IFNOT(existingValue != nullptr, "Existing value should not be null");

                auto existingSequenceNumber = existingValue->GetVersionSequenceNumber();
                auto updateSequenceNumber = value.GetVersionSequenceNumber();
                ASSERT_IFNOT(existingSequenceNumber == updateSequenceNumber, "Sequence numbers should match: Existing {0} != Update {1}", existingSequenceNumber, updateSequenceNumber);

                componentSPtr_->UpdateValue(key, &value);

                // Increase size by new value and decrease by existing value

                if (value.IsInMemory() == true)
                {
                   // Existing value might or might be in memory
                   if (existingValue->IsInMemory() == true)
                   {
                      InterlockedAdd64(&size_, value.GetValueSize() - existingValue->GetValueSize());
                   }
                   else
                   {
                      // Just add the new size
                      InterlockedAdd64(&size_, value.GetValueSize());
                   }
                }
                else
                {
                   if (existingValue->IsInMemory() == true)
                   {
                      // Subtract the existing value
                      InterlockedAdd64(&size_, -existingValue->GetValueSize());
                   }
                   else
                   {
                      // Do nothing here
                   }
                }

                // Preethas: It is safe to assert here since sweep cannot race when consolidation is in progress.
                ASSERT_IFNOT(size_ >= 0, "Size {0} should not be negative", size_);
            }

            KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key) const
            {
                KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = nullptr;
                componentSPtr_->TryGetValue(key, versionedItemSPtr);
                return versionedItemSPtr;
            }

            KSharedPtr<VersionedItem<TValue>> Read(__in TKey& key, __in LONG64 visibilitySequenceNumber) const
            {
               KSharedPtr<VersionedItem<TValue>> versionedItemSPtr = Read(key);

               if (versionedItemSPtr != nullptr && versionedItemSPtr->GetVersionSequenceNumber() <= visibilitySequenceNumber)
               {
                  return versionedItemSPtr;
               }

               return nullptr;
            }

            bool ContainsKey(__in TKey& key) const
            {
                return componentSPtr_->ContainsKey(key);
            }

            LONG64 Count() const
            {
                return componentSPtr_->Count();
            }

            KSharedPtr<PartitionedListEnumerator<TKey, KSharedPtr<VersionedItem<TValue>>>> EnumerateEntries() const
            {
                return componentSPtr_->GetKeys();
            }

            KSharedPtr<IFilterableEnumerator<TKey>> EnumerateKeys() const
            {
                auto keyValueEnumerator = componentSPtr_->GetEnumerator();
                KSharedPtr<PartitionedSortedListKeysFilterableEnumerator<TKey, KSharedPtr<VersionedItem<TValue>>>> enumerator = nullptr;
                PartitionedSortedListKeysFilterableEnumerator<TKey, KSharedPtr<VersionedItem<TValue>>>::Create(*keyValueEnumerator, this->GetThisAllocator(), enumerator);

                return enumerator.RawPtr();
            }

            void DecrementSize(__in LONG64 size)
            {
               // Decrement value size on sweep.
               InterlockedAdd64(&size_, -size);

               ASSERT_IFNOT(size_ >= 0, "Size {0} should not be negative", size_);
            }

            void AddToMemorySize(__in LONG64 size)
            {
               // Preethas : These elements could be present in one of the delta states but the size of consolidated state is incremented
               // It is okay to be approximate here.
               InterlockedAdd64(&size_, size);
               ASSERT_IFNOT(size_ >= 0, "Size {0} should not be negative", size_);
            }

        private:
            KSharedPtr<PartitionedSortedList<TKey, KSharedPtr<VersionedItem<TValue>>>> componentSPtr_;
            ConsolidatedStoreComponent(__in IComparer<TKey>& keyComparer);
            LONG64 size_;
        };

        template <typename TKey, typename TValue>
        ConsolidatedStoreComponent<TKey, TValue>::ConsolidatedStoreComponent(__in IComparer<TKey>& keyComparer) : size_(0)
        {
           NTSTATUS status = PartitionedSortedList<TKey, KSharedPtr<VersionedItem<TValue>>>::Create(keyComparer, this->GetThisAllocator(), componentSPtr_);
           this->SetConstructorStatus(status);
        }

        template <typename TKey, typename TValue>
        ConsolidatedStoreComponent<TKey, TValue>::~ConsolidatedStoreComponent()
        {
        }
    }
}
