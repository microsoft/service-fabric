// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define PARTITION_TAG 'ntRP'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class Partition : public KObject<Partition<TKey, TValue>>, public KShared<Partition<TKey, TValue>>, public IPartition<TKey, TValue>
        {
            K_FORCE_SHARED(Partition)

        public:
            static NTSTATUS Create(
                __in int maxsize,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITION_TAG, allocator) Partition(maxsize);

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

            static NTSTATUS Create(
                __in int maxSize,
                __in int initialSize,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITION_TAG, allocator) Partition(maxSize, initialSize);

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

            const TKey& GetFirstKey() const
            {
                return GetKey(0);
            }

            int Count()
            {
                return keyValueListSPtr_->Count();
            }

            void Add(__in const TKey& key, __in const TValue& value)
            {
                return keyValueListSPtr_->Add(key, value);
            }

            const TKey& GetKey(__in int index) const
            {
                return keyValueListSPtr_->GetKey(index);
            }

            const TValue& GetValue(__in int index) const
            {
                return keyValueListSPtr_->GetValue(index);
            }

            void UpdateValue(__in int index, __in const TValue& value)
            {
                return keyValueListSPtr_->UpdateValue(index, value);
            }

            int BinarySearch(__in const TKey& key, __in const IComparer<TKey>& comparer)
            {
                return keyValueListSPtr_->BinarySearch(key, comparer);
            }

        private:
            Partition(__in int maxSize);
            Partition(__in int initialSize, __in int maxSize);
            KSharedPtr<KeyValueList<TKey, TValue>> keyValueListSPtr_;
        };

        template <typename TKey, typename TValue>
        Partition<TKey, TValue>::Partition(__in int maxSize)
        {
           NTSTATUS status = KeyValueList<TKey, TValue>::Create(maxSize, this->GetThisAllocator(), keyValueListSPtr_);
           this->SetConstructorStatus(status);
        }

        template <typename TKey, typename TValue>
        Partition<TKey, TValue>::Partition(__in int initialSize, __in int maxSize)
        {
           NTSTATUS status = KeyValueList<TKey, TValue>::Create(initialSize, maxSize, this->GetThisAllocator(), keyValueListSPtr_);
           this->SetConstructorStatus(status);
        }

        template <typename TKey, typename TValue>
        Partition<TKey, TValue>::~Partition()
        {
        }
    }
}
