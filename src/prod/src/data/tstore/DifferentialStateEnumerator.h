// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once
#pragma once

#define DIFFERENTIALSTATEENUM_TAG 'mnED'

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>
      class DifferentialStateEnumerator : 
         public IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>
      {
         K_FORCE_SHARED(DifferentialStateEnumerator)

      public:
         static NTSTATUS Create(
            __in DifferentialStoreComponent<TKey, TValue> & differentialStoreComponent,
            __in IComparer<TKey>& keyComparer,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTATEENUM_TAG, allocator) DifferentialStateEnumerator(differentialStoreComponent, keyComparer);

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

         __declspec(property(get = get_ComponentSPtr)) KSharedPtr<DifferentialStoreComponent<TKey, TValue>> ComponentSPtr;
         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> get_ComponentSPtr() const
         {
            return componentSPtr_;
         }

         int Compare(__in const DifferentialStateEnumerator<TKey, TValue>& other) const
         {
            auto currentItem = (const_cast<DifferentialStateEnumerator<TKey, TValue> *>(this))->Current();
            auto otherItem = const_cast<DifferentialStateEnumerator<TKey, TValue>&>(other).Current();

            // Compare Keys
            int compare = keyComparerSPtr_->Compare(currentItem.Key, otherItem.Key);
            if (compare != 0)
            {
               return compare;
            }

            // Compare LSNs
            LONG64 currentLSN = currentItem.Value->GetVersionSequenceNumber();
            LONG64 otherLSN = otherItem.Value->GetVersionSequenceNumber();

            if (currentLSN > otherLSN)
            {
               return -1;
            }
            else if (currentLSN < otherLSN)
            {
               return 1;
            }

            return 0;
         }

         KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> Current()
         {
            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> result(currentKey_, currentVersionedItemSPtr_);
            return result;
         }

         bool MoveNext()
         {
            bool ret = componentEnumeratorSPtr_->MoveNext();
            if (ret)
            {
               currentKey_ = componentEnumeratorSPtr_->Current().Key;
               currentVersionedItemSPtr_ = componentEnumeratorSPtr_->Current().Value;
            }

            return ret;
         }

      private:

         DifferentialStateEnumerator(__in DifferentialStoreComponent<TKey, TValue>& component, __in IComparer<TKey>& keyComparer);

         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> componentSPtr_;

         // Note: TKey should have default constructor.
         TKey currentKey_;
         KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr_;
        
         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
         KSharedPtr<DifferentialKeyValueEnumerator<TKey, TValue>> componentEnumeratorSPtr_;
      };

      template <typename TKey, typename TValue>
      DifferentialStateEnumerator<TKey, TValue>::DifferentialStateEnumerator(
         __in DifferentialStoreComponent<TKey, TValue>& component,
         __in IComparer<TKey>& keyComparer)
         : componentSPtr_(&component),
         currentVersionedItemSPtr_(nullptr),
         keyComparerSPtr_(&keyComparer)
      {
         componentEnumeratorSPtr_ = componentSPtr_->GetKeyAndValues();
      }

      template <typename TKey, typename TValue>
      DifferentialStateEnumerator<TKey, TValue>::~DifferentialStateEnumerator()
      {
      }
   }
}
