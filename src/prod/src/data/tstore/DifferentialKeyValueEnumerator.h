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

      //
      // Enuemrates keys and values in differential state.
      //
      class DifferentialKeyValueEnumerator : public IEnumerator<KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>>
      {
         K_FORCE_SHARED(DifferentialKeyValueEnumerator)

      public:
         static NTSTATUS Create(
            __in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> & dictionary,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTATEENUM_TAG, allocator) DifferentialKeyValueEnumerator(dictionary);

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
            KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> result(currentKey_, currentVersionedItemSPtr_);
            return result;
         }

         bool MoveNext()
         {
            bool ret = componentEnumeratorSPtr_->MoveNext();
            if (ret)
            {
               KeyValuePair<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> entry = componentEnumeratorSPtr_->Current();
               currentKey_ = entry.Key;
               auto differentialStateVersions = entry.Value;
               ASSERT_IFNOT(differentialStateVersions != nullptr, "differentialStateVersions != nullptr");

               currentVersionedItemSPtr_ = differentialStateVersions->CurrentVersionSPtr;
            }
            return ret;
         }

      private:

         DifferentialKeyValueEnumerator(__in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>& dictionary);

         KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>>> componentEnumeratorSPtr_;

         // Note: TKey should have default constructor.
         TKey currentKey_;
         KSharedPtr<VersionedItem<TValue>> currentVersionedItemSPtr_;
      };

      template <typename TKey, typename TValue>
      DifferentialKeyValueEnumerator<TKey, TValue>::DifferentialKeyValueEnumerator(__in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>& dictionary)
         : componentEnumeratorSPtr_(dictionary.GetEnumerator()),
         currentVersionedItemSPtr_(nullptr)
      {
      }

      template <typename TKey, typename TValue>
      DifferentialKeyValueEnumerator<TKey, TValue>::~DifferentialKeyValueEnumerator()
      {
      }
   }
}
