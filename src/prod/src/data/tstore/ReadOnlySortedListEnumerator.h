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
        class ReadOnlySortedList<TKey, TValue>::Enumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
        {
            K_FORCE_SHARED(Enumerator)
        public:
            static NTSTATUS Create(
                __in ReadOnlySortedList<TKey, TValue> & readOnlySortedList,
                __out KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> & result)
            {
                result = _new(READONLYSORTEDLIST_TAG, readOnlySortedList.GetThisAllocator()) Enumerator(readOnlySortedList);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }
            virtual KeyValuePair<TKey,TValue> Current() override
            {
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                ASSERT_IFNOT(static_cast<ULONG>(index_) < listSPtr_->Count, "Got out of bounds index: {0}", index_);
                return (*listSPtr_->arraySPtr_)[index_];
            }

            virtual bool MoveNext() override
            {
                index_++;
                ASSERT_IFNOT(index_ >= 0, "Got negative index: {0}", index_);
                return static_cast<ULONG>(index_) < listSPtr_->Count;
            }
        private:
            Enumerator(__in ReadOnlySortedList<TKey, TValue> & readOnlySortedList) :
                listSPtr_(&readOnlySortedList),
                index_(-1)
            {
            }

            LONG32 index_;
            KSharedPtr<ReadOnlySortedList<TKey, TValue>> listSPtr_;
        };

        template<typename TKey, typename TValue>
        ReadOnlySortedList<TKey, TValue>::Enumerator::~Enumerator()
        {
        }
    }
}
