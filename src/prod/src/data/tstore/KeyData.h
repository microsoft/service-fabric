// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEYDATA_TAG 'tdYK'

namespace Data
{
    using namespace Data::TStore;

    namespace TStore
    {
        template<typename TKey, typename TValue>
        struct KeyData : public KObject<KeyData<TKey, TValue>>, public KShared<KeyData<TKey, TValue>>
        {
            K_FORCE_SHARED(KeyData)
        

        public:

            static NTSTATUS
                Create(
                    __in TKey& key, 
                    __in VersionedItem<TValue>& versionedItem, 
                    __in ULONG64 logicalTimeStamp,
                    __in LONG64 keySize,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(KEYDATA_TAG, allocator) KeyData(key, versionedItem, logicalTimeStamp, keySize);

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

            __declspec(property(get = get_Key)) TKey Key;
            TKey get_Key() const
            {
                return key_;
            }


            __declspec(property(get = get_Value)) KSharedPtr<VersionedItem<TValue>> Value;
            KSharedPtr<VersionedItem<TValue>> get_Value() const
            {
                return versionedItem_;
            }


            __declspec(property(get = get_LogicalTimeStamp)) ULONG64 LogicalTimeStamp;
            ULONG64 get_LogicalTimeStamp() const
            {
                return logicalTimeStamp_;
            }

            __declspec(property(get = get_KeySize)) LONG64 KeySize;
            LONG64 get_KeySize() const
            {
                return keySize_;
            }

        private:
            TKey key_;
            KSharedPtr<VersionedItem<TValue>> versionedItem_;
            ULONG64 logicalTimeStamp_;
            LONG64 keySize_;

            KeyData(
                __in TKey& key,
                __in VersionedItem<TValue>& versionedItem,
                __in ULONG64 logicalTimeStamp,
                __in LONG64 keySize)
                : key_(key),
                versionedItem_(&versionedItem),
                logicalTimeStamp_(logicalTimeStamp),
                keySize_(keySize)
            {
            }

        };

        template <typename TKey, typename TValue>
        KeyData<TKey, TValue>::KeyData()
        {
        }

        template <typename TKey, typename TValue>
        KeyData<TKey, TValue>::~KeyData()
        {
        }
    }
}
