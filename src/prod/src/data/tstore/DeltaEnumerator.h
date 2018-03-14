// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define DELATAENUM_TAG 'meID'

namespace Data
{
   namespace TStore
   {
      template<typename TKey, typename TValue>

      //
      // Enumerates delta states from the delta list in the reverse order of index.
      //
      class DeltaEnumerator : public IEnumerator<KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>
      {
         K_FORCE_SHARED(DeltaEnumerator)

      public:
         static NTSTATUS Create(
            __in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>& deltaList,
            __in ULONG32 highestIndex,
            __in KAllocator & allocator,
            __out KSharedPtr<DeltaEnumerator> & result)
         {
            NTSTATUS status;
            SPtr output = _new(DELATAENUM_TAG, allocator) DeltaEnumerator(deltaList, highestIndex);

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

         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> Current()
         {
            return currentComponent_;
         }

         bool MoveNext()
         {
            if (index_ <= 0)
            {
               return false;
            }

            componentSPtr_->TryGetValue(index_--, currentComponent_);
            ASSERT_IFNOT(currentComponent_ != nullptr, "current component cannot be null");
            return true;
         }

      private:
         NOFAIL DeltaEnumerator(__in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>& deltaList, __in ULONG32 highestIndex);

         KSharedPtr<ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>> componentSPtr_;
         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> currentComponent_;
         ULONG32 index_;

      };

      template <typename TKey, typename TValue>
      DeltaEnumerator<TKey, TValue>::DeltaEnumerator(__in ConcurrentDictionary<LONG64, KSharedPtr<DifferentialStoreComponent<TKey, TValue>>>& deltaList, __in ULONG32 highestIndex)
         : componentSPtr_(&deltaList),
         index_(highestIndex)
      {
      }

      template <typename TKey, typename TValue>
      DeltaEnumerator<TKey, TValue>::~DeltaEnumerator()
      {
      }
   }
}
