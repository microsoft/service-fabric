// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define PENUM_TAG 'reLP'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class PartitionedListEnumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
        {
            K_FORCE_SHARED(PartitionedListEnumerator)

        public:
            static NTSTATUS Create(
                __in KSharedArray<KSharedPtr<Partition<TKey, TValue>>> & partitionList,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(PENUM_TAG, allocator) PartitionedListEnumerator(partitionList);

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
            KeyValuePair<TKey, TValue> Current()
            {
                KSharedPtr<Partition<TKey, TValue>> partitionSPtr = (*partitionListSPtr_)[partitionIndex_];

                TKey key = partitionSPtr->GetKey(index_);
                TValue value = partitionSPtr->GetValue(index_);

                KeyValuePair<TKey, TValue> result(key, value);
                return result;
            }

            bool MoveNext()
            {
                if (partitionListSPtr_->Count() == 0)
                {
                    return false;
                }

                ++index_;
                KSharedPtr<Partition<TKey, TValue>> partitionSPtr = (*partitionListSPtr_)[partitionIndex_];

                if (partitionIndex_ == 0 && partitionSPtr == nullptr)
                {
                    return false;
                }

                KInvariant(partitionSPtr != nullptr);

                // Get count for zero based index.
                int count = partitionSPtr->Count() - 1;
                if (index_ > count)
                {
                    count = partitionListSPtr_->Count() - 1;
                    ++partitionIndex_;

                    if (partitionIndex_ > count)
                    {
                        return false;
                    }

                    // reset index for the next partition.
                    index_ = 0;
                }

                return true;
            }

        private:
            int index_;
            int partitionIndex_;
            const int DefaultMaxSubListSize = 1024;
            KSharedPtr<KSharedArray<KSharedPtr<Partition<TKey, TValue>>>> partitionListSPtr_;
            NOFAIL PartitionedListEnumerator(__in KSharedArray<KSharedPtr<Partition<TKey, TValue>>>& partitionList);
        };

        template <typename TKey, typename TValue>
        // todo: initialize here.
        PartitionedListEnumerator<TKey, TValue>::PartitionedListEnumerator(__in KSharedArray<KSharedPtr<Partition<TKey, TValue>>> & partitionList)
            : index_(-1),
            partitionIndex_(0),
            partitionListSPtr_(&partitionList)
        {
        }

        template <typename TKey, typename TValue>
        PartitionedListEnumerator<TKey, TValue>::~PartitionedListEnumerator()
        {
        }
    }
}
