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
        class ReadOnlySortedList<TKey, TValue>::KeysEnumerator : public IFilterableEnumerator<TKey>
        {
            K_FORCE_SHARED(KeysEnumerator)
        public:
            static NTSTATUS Create(
                __in ReadOnlySortedList<TKey, TValue> & readOnlySortedList,
                __out KSharedPtr<IFilterableEnumerator<TKey>> & result)
            {
                result = _new(READONLYSORTEDLIST_TAG, readOnlySortedList.GetThisAllocator()) KeysEnumerator(readOnlySortedList);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }


            TKey Current() override
            {
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                ASSERT_IFNOT(static_cast<ULONG>(index_) < listSPtr_->Count, "Got out of bounds index: {0}", index_);
                return (*listSPtr_->arraySPtr_)[index_].Key;
            }

            bool MoveNext() override
            {
                index_++;
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                return static_cast<ULONG>(index_) < listSPtr_->Count;
            }

            bool MoveTo(TKey const & key)
            {
                TValue value;
                KeyValuePair<TKey, TValue> searchItem(key, value);
                
                KSharedPtr<KSharedArray<Data::KeyValuePair<TKey, TValue>>> itemsSPtr = (listSPtr_->arraySPtr_.RawPtr());
                LONG32 searchIndex = Sorter<KeyValuePair<TKey, TValue>>::BinarySearch(searchItem, listSPtr_->isAscending_, *listSPtr_->keyValueComparerSPtr_, itemsSPtr);

                if (searchIndex >= 0)
                {
                    index_ = searchIndex;
                }
                else
                {
                    index_ = ~searchIndex;
                }
                
                bool hasNext = static_cast<ULONG>(index_) < itemsSPtr->Count();

                index_--; // Point to previous element, so MoveNext starts at correct index
                return hasNext;
            } 

        private:
            KeysEnumerator(__in ReadOnlySortedList<TKey, TValue> & readOnlySortedList) :
                listSPtr_(&readOnlySortedList),
                index_(-1)
            {
            }

            LONG32 index_;
            KSharedPtr<ReadOnlySortedList<TKey, TValue>> listSPtr_;
        };

        template<typename TKey, typename TValue>
        ReadOnlySortedList<TKey, TValue>::KeysEnumerator::~KeysEnumerator()
        {
        }
    }
}
