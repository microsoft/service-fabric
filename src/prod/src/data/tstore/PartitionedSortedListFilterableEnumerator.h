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
        class PartitionedSortedListFilterableEnumerator : public IFilterableEnumerator<KeyValuePair<TKey, TValue>>
        {
            K_FORCE_SHARED(PartitionedSortedListFilterableEnumerator)

        public:
            static NTSTATUS Create(
                __in PartitionedSortedList<TKey, TValue> & partitionList,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(PENUM_TAG, allocator) PartitionedSortedListFilterableEnumerator(partitionList);

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
                return current_;
            }

            bool MoveNext()
            {
                if (isFirstMove_)
                {
                    isFirstMove_ = false;
                    return listSPtr_->TryGetKeyValue(currentIndex_, current_);
                }

                currentIndex_ = Index(currentIndex_.PartitionIndex, currentIndex_.ItemIndex + 1);
                bool exists = listSPtr_->TryGetKeyValue(currentIndex_, current_);

                if (exists)
                {
                    return true;
                }

                // Not found, try first item in next partition
                currentIndex_ = Index(currentIndex_.PartitionIndex + 1, 0);
                return listSPtr_->TryGetKeyValue(currentIndex_, current_);
            }

            bool MoveTo(KeyValuePair<TKey, TValue> const & item)
            {
                TKey key = item.Key;
                bool found = listSPtr_->TryGetIndex(key, currentIndex_);

                if (!found)
                {
                    currentIndex_ = Index(~currentIndex_.PartitionIndex, ~currentIndex_.ItemIndex);
                }

                bool hasNext = listSPtr_->TryGetKeyValue(currentIndex_, current_);
                isFirstMove_ = true;
                return hasNext;
            }

        private:
            Index currentIndex_ = Index(-1, -1);
            KeyValuePair<TKey, TValue> current_;
            bool isFirstMove_;

            KSharedPtr<PartitionedSortedList<TKey, TValue>> listSPtr_;
            NOFAIL PartitionedSortedListFilterableEnumerator(__in PartitionedSortedList<TKey, TValue> & list);
        };

        template <typename TKey, typename TValue>
        // todo: initialize here.
        PartitionedSortedListFilterableEnumerator<TKey, TValue>::PartitionedSortedListFilterableEnumerator(__in PartitionedSortedList<TKey, TValue> & list)
            : listSPtr_(&list)
            , isFirstMove_(false) // defaulting to false. should be true only when MoveTo is used
        {
        }

        template <typename TKey, typename TValue>
        PartitionedSortedListFilterableEnumerator<TKey, TValue>::~PartitionedSortedListFilterableEnumerator()
        {
        }
    }
}
