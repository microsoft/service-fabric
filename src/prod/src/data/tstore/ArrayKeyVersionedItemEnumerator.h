// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define ARRAYENUM_TAG 'mnEA'

namespace Data
{
   namespace TStore
   {

      template<typename TKey, typename TValue>
      class ArrayKeyVersionedItemEnumerator : public IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>
      {
         K_FORCE_SHARED(ArrayKeyVersionedItemEnumerator)

      public:
         static NTSTATUS Create(
            __in KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> const & items,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(ARRAYENUM_TAG, allocator) ArrayKeyVersionedItemEnumerator(items);

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

      public:
         KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> Current()
         {
            ASSERT_IFNOT(index_ >= 0, "index_ >= 0");
            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> sortedItemData = (*sortedItemSPtr_)[index_];
            return sortedItemData;
         }

         bool MoveNext()
         {
            if (sortedItemSPtr_->Count() > static_cast<ULONG>(index_ + 1))
            {
               ++index_;
               return true;
            }

            return false;
         }

      private:

         ArrayKeyVersionedItemEnumerator(__in KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> const & sortedItem);

         KSharedPtr<KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> const> sortedItemSPtr_;
         LONG32 index_;

         // Note: TKey should have default constructor.
         TKey currentKey_;
         KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr_;
      };

      template <typename TKey, typename TValue>
      ArrayKeyVersionedItemEnumerator<TKey, TValue>::ArrayKeyVersionedItemEnumerator(__in KSharedArray<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>> const & sortedItem)
         : sortedItemSPtr_(&sortedItem),
         index_(-1)
      {
      }

      template <typename TKey, typename TValue>
      ArrayKeyVersionedItemEnumerator<TKey, TValue>::~ArrayKeyVersionedItemEnumerator()
      {
      }
   }
}
