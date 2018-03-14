// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DIFFERENTIALSTATEENUM_TAG 'mnED'

namespace Data
{
   namespace TStore
   {
      //
      // Enuemrates only keys.
      //

      template<typename TKey, typename TValue>
      class DifferentialKeysEnumerator : public IEnumerator<TKey>
      {
         K_FORCE_SHARED(DifferentialKeysEnumerator)

      public:
         static NTSTATUS Create(
            __in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> & dictionary,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTATEENUM_TAG, allocator) DifferentialKeysEnumerator(dictionary);

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
         TKey Current()
         {
            return currentKey_;
         }

         bool MoveNext()
         {
            bool ret = componentEnumeratorSPtr_->MoveNext();
            if (ret)
            {
               KeyValuePair<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> entry = componentEnumeratorSPtr_->Current();
               currentKey_ = entry.Key;
            }

            return ret;
         }

      private:
         KSharedPtr<IEnumerator<KeyValuePair<TKey, KSharedPtr<DifferentialStateVersions<TValue>>>>> componentEnumeratorSPtr_;

         // Note: TKey should have default constructor.
         TKey currentKey_;

         DifferentialKeysEnumerator(__in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> & dictionary);
      };

      template <typename TKey, typename TValue>
      DifferentialKeysEnumerator<TKey, TValue>::DifferentialKeysEnumerator(__in IDictionary<TKey, KSharedPtr<DifferentialStateVersions<TValue>>> & dictionary)
         : componentEnumeratorSPtr_(dictionary.GetEnumerator())
      {
      }

      template <typename TKey, typename TValue>
      DifferentialKeysEnumerator<TKey, TValue>::~DifferentialKeysEnumerator()
      {
      }
   }
}
