// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class PartitionedListKeysEnumerator : public IEnumerator<TKey>
        {
            K_FORCE_SHARED(PartitionedListKeysEnumerator)

        public:
            static NTSTATUS Create(
                __in PartitionedListEnumerator<TKey, TValue> & keyValueEnumerator,
                __in KAllocator & allocator,
                __out KSharedPtr<IEnumerator<TKey>> & result)
            {
                NTSTATUS status;
                KSharedPtr<IEnumerator<TKey>> output = _new(PENUM_TAG, allocator) PartitionedListKeysEnumerator(keyValueEnumerator);

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

            TKey Current() override
            {
                KeyValuePair<TKey, TValue> current = keyValueEnumeratorSPtr_->Current();
                return current.Key;
            }

            bool MoveNext()
            {
                return keyValueEnumeratorSPtr_->MoveNext();
            }

        private:
            PartitionedListKeysEnumerator(__in PartitionedListEnumerator<TKey, TValue> & keyValueEnumerator);
            KSharedPtr<PartitionedListEnumerator<TKey, TValue>> keyValueEnumeratorSPtr_;
        };
        
        template<typename TKey, typename TValue>
        PartitionedListKeysEnumerator<TKey, TValue>::PartitionedListKeysEnumerator(__in PartitionedListEnumerator<TKey, TValue> & keyValueEnumerator)
            : keyValueEnumeratorSPtr_(&keyValueEnumerator)
        {
        }

        template<typename TKey, typename TValue>
        PartitionedListKeysEnumerator<TKey, TValue>::~PartitionedListKeysEnumerator()
        {
        }
    }
}
