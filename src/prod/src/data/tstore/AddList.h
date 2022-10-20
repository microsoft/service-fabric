// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define ADDLIST_TAG 'tsLA'

namespace Data
{
   using namespace Data::TStore;

   namespace TStore
   {
      template<typename TKey>
      class AddList : public KObject<AddList<TKey>>, public KShared<AddList<TKey>>
      {
         K_FORCE_SHARED(AddList)

      public:

         static NTSTATUS
            Create(
               __in IComparer<TKey>& keyComparer,
               __in KAllocator& allocator,
               __out SPtr& result)
         {
            NTSTATUS status;
            SPtr output = _new(ADDLIST_TAG, allocator) AddList(keyComparer);

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
            return STATUS_SUCCESS;
         }

         void Add(__in TKey& key)
         {
            // Item may already exist.
            bool result = addedItemsDictionarySPtr_->TryAdd(key, DefaultValue);
            if (result == false)
            {
               bool present = addedItemsDictionarySPtr_->ContainsKey(key);
               KInvariant(present);
            }
         }

         KSharedPtr<IFilterableEnumerator<TKey>> GetEnumerator()
         {
             KSharedPtr<IFilterableEnumerator<TKey>> enumeratorSPtr = nullptr;
             NTSTATUS status = ConcurrentSkipListFilterableEnumerator<TKey, byte>::Create(*addedItemsDictionarySPtr_, enumeratorSPtr);
             Diagnostics::Validate(status);
             ASSERT_IFNOT(enumeratorSPtr != nullptr, "enumeratorSPtr should not be null");
             return enumeratorSPtr;
         }

      private:
         AddList(__in IComparer<TKey>& keyComparer)
            : keyComparerSPtr_(&keyComparer)
         {
            NTSTATUS status;
            status = ConcurrentSkipList<TKey, byte>::Create(keyComparerSPtr_, this->GetThisAllocator(), addedItemsDictionarySPtr_);
            this->SetConstructorStatus(status);
         }

         KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
         KSharedPtr<Data::Utilities::ConcurrentSkipList<TKey, byte>> addedItemsDictionarySPtr_;
         const byte DefaultValue = 0;
      };

      template <typename TKey>
      AddList<TKey>::~AddList()
      {
      }
   }
}
