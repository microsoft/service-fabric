// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define STORE_COMPONENT_READ_RESULT_TAG 'scRR'

namespace Data
{
    namespace TStore
    {
        template<typename TValue>
        struct StoreComponentReadResult
            : public KObject<StoreComponentReadResult<TValue>>
            , public KShared<StoreComponentReadResult<TValue>>
        {
            K_FORCE_SHARED(StoreComponentReadResult)
                
        public:
            static NTSTATUS
                Create(
                    __in KSharedPtr<Data::TStore::VersionedItem<TValue>> versionedItemSPtr,
                    __in TValue & value,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(STORE_COMPONENT_READ_RESULT_TAG, allocator) StoreComponentReadResult(versionedItemSPtr, value);

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

            bool HasValue()
            {
                return versionedItemSPtr_ != nullptr;
            }
            
            __declspec(property(get = get_VersionedItem)) KSharedPtr<Data::TStore::VersionedItem<TValue>> VersionedItem;
            KSharedPtr<Data::TStore::VersionedItem<TValue>> get_VersionedItem() const
            {
                return versionedItemSPtr_;
            }

            __declspec(property(get = get_Value)) TValue Value;
            TValue get_Value() const
            {
                return value_;
            }

        private:
            StoreComponentReadResult(
                __in KSharedPtr<Data::TStore::VersionedItem<TValue>> versionedItemSPtr,
                __in TValue & value)
                : versionedItemSPtr_(versionedItemSPtr),
                value_(value)
            {
            }

            KSharedPtr<Data::TStore::VersionedItem<TValue>> versionedItemSPtr_;
            TValue value_;
        };

        template<typename TValue>
        StoreComponentReadResult<TValue>::~StoreComponentReadResult()
        {

        }
    }
}
