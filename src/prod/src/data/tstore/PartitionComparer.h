// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define PARTITIONCOMPARER_TAG 'rpCP'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class PartitionComparer 
        : public IComparer<IPartition<TKey, TValue>>
        , public KObject<PartitionComparer<TKey, TValue>>
        , public KShared<PartitionComparer<TKey, TValue>>
        {
            K_FORCE_SHARED(PartitionComparer)
            K_SHARED_INTERFACE_IMP(IComparer)

        public:
            static NTSTATUS Create(
                    __in IComparer<TKey>& comparer,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITIONCOMPARER_TAG, allocator) PartitionComparer(comparer);

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

            int Compare(__in const IPartition<TKey, TValue>& x, __in const IPartition<TKey, TValue>& y) const
            {
                const TKey& xKey = x.GetFirstKey();
                const TKey& yKey = y.GetFirstKey();
                return comparerSPtr_->Compare(xKey, yKey);
            }

        private:
            KSharedPtr<IComparer<TKey>> comparerSPtr_;
            PartitionComparer(__in IComparer<TKey>& comparer);
        };

        template <typename TKey, typename TValue>
        PartitionComparer<TKey, TValue>::PartitionComparer(__in IComparer<TKey>& comparer)
            : comparerSPtr_(&comparer)
        {
        }

        template <typename TKey, typename TValue>
        PartitionComparer<TKey, TValue>::~PartitionComparer()
        {
        }
    }
}


