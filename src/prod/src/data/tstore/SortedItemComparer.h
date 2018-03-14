// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define ITEM_COMPARER_TAG 'moCI'

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>
      class SortedItemComparer 
       : public IComparer<KeyValuePair<TKey, TValue>>
       , public KObject<SortedItemComparer<TKey, TValue>>
       , public KShared<SortedItemComparer<TKey, TValue>>
      {
         K_FORCE_SHARED(SortedItemComparer)
         K_SHARED_INTERFACE_IMP(IComparer)

      public:
         static NTSTATUS Create(
            __in IComparer<TKey>& keyComparer,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            result = _new(ITEM_COMPARER_TAG, allocator) SortedItemComparer(keyComparer);

            if (!result)
            {
               return STATUS_INSUFFICIENT_RESOURCES;
            }

            if (!NT_SUCCESS(result->Status()))
            {
                return (SPtr(Ktl::Move(result)))->Status();
            }

            return STATUS_SUCCESS;
         }

         int Compare(
            __in const KeyValuePair<TKey, TValue>& x,
            __in const KeyValuePair<TKey, TValue>& y) const override
         {
            return keyComparerSPtr_->Compare(x.Key, y.Key);
         }

      private:
         SortedItemComparer(__in IComparer<TKey>& keyComparer);

         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
      };

      template<typename TKey, typename TValue>
      SortedItemComparer<TKey, TValue>::SortedItemComparer(
         __in IComparer<TKey>& keyComparer) :
         keyComparerSPtr_(&keyComparer)
      {
      }

      template <typename TKey, typename TValue>
      SortedItemComparer<TKey, TValue>::~SortedItemComparer()
      {
      }
   }
}
