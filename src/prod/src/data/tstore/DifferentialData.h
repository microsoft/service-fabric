// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
   namespace TStore
   {
      template <typename TKey, typename TValue>
      class DifferentialData : public KObject<DifferentialData<TKey, TValue>>, public KShared<DifferentialData<TKey, TValue>>
      {
         K_FORCE_SHARED(DifferentialData)

      public:
         static NTSTATUS Create(
            __in DifferentialStoreComponent<TKey, TValue> &component,
            __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>& keyValuePair,
            __in KAllocator & allocator,
            __out SPtr & result)
         {
            NTSTATUS status;
            SPtr output = _new(DIFFERENTIALSTATEENUM_TAG, allocator) DifferentialData(component, keyValuePair);

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

         __declspec(property(get = get_DifferentialStoreComponentSPtr, put = set_DifferentialStoreComponentSPtr)) KSharedPtr<DifferentialStoreComponent<TKey, TValue>> DifferentialStoreComponentSPtr;
         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> get_DifferentialStoreComponentSPtr() const
         {
            return differentialStoreComponentSPtr_;
         }

         void set_DifferentialStoreComponentSPtr(__in DifferentialStoreComponent<TKey, TValue>& diffState)
         {
            differentialStoreComponentSPtr_ = &diffState;
         }

         __declspec(property(get = get_KeyAndVersionedItemPair, put = set_KeyAndVersionedItemPair)) KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> KeyAndVersionedItemPair;
         KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> get_KeyAndVersionedItemPair() const
         {
            return keyAndVersionedItemPair_;
         }

         void set_KeyAndVersionedItemPair(__in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>>& value)
         {
            keyAndVersionedItemPair_ = value;
         }

      private:
         DifferentialData(__in DifferentialStoreComponent<TKey, TValue> &component, __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> keyValuePair);

         KSharedPtr<DifferentialStoreComponent<TKey, TValue>> differentialStoreComponentSPtr_;
         KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> keyAndVersionedItemPair_;
      };

      template <typename TKey, typename TValue>
      DifferentialData<TKey, TValue>::DifferentialData(
         __in DifferentialStoreComponent<TKey, TValue> &component,
         __in KeyValuePair<TKey, KSharedPtr<VersionedItem<TValue>>> keyValuePair)
         : differentialStoreComponentSPtr_(&component)
         {
            keyAndVersionedItemPair_ = keyValuePair;
         }

         template <typename TKey, typename TValue>
         DifferentialData<TKey, TValue>::~DifferentialData()
         {
         }
   }
}
