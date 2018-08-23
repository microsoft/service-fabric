// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define READONLYSORTEDLIST_TAG 'roSL'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class ReadOnlySortedList sealed : public IDictionary<TKey, TValue>
        {
            K_FORCE_SHARED(ReadOnlySortedList)
        public:
            class Enumerator;
            class KeysEnumerator;

            static NTSTATUS Create(
                __in IDictionary<TKey, TValue> & items,
                __in IComparer<TKey> & keyComparer,
                __in bool isAscending,
                __in bool isInputSorted,
                __in KAllocator& allocator, 
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(READONLYSORTEDLIST_TAG, allocator) ReadOnlySortedList(items, keyComparer, isAscending, isInputSorted);

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

            __declspec(property(get = get_Count)) ULONG Count;
            ULONG get_Count() const override
            {
                return arraySPtr_->Count();
            }

            bool ContainsKey(__in const TKey& key) const override
            {
                TValue value;
                KeyValuePair<TKey, TValue> searchItem(key, value);
                
                KSharedPtr<KSharedArray<Data::KeyValuePair<TKey, TValue>>> itemsSPtr = *(&arraySPtr_);
                
                auto index = Sorter<KeyValuePair<TKey, TValue>>::BinarySearch(searchItem, isAscending_, *keyValueComparerSPtr_, itemsSPtr);
                return index >= 0;
            }

            bool TryGetValue(__in const TKey& key, __out TValue& value) const override
            {
                KeyValuePair<TKey, TValue> searchItem(key, value);
                KSharedPtr<KSharedArray<Data::KeyValuePair<TKey, TValue>>> itemsSPtr = *(&arraySPtr_);
                
                auto index = Sorter<KeyValuePair<TKey, TValue>>::BinarySearch(searchItem, isAscending_, *keyValueComparerSPtr_, itemsSPtr);
                
                if (index < 0)
                {
                    return false;
                }

                KeyValuePair<TKey, TValue> result = (*arraySPtr_)[index];
                value = result.Value;
                return true;
            }

            KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const override
            {
                NTSTATUS status;
                KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> enumeratorSPtr;
                auto readOnlySortedList = const_cast<ReadOnlySortedList<TKey, TValue> *>(this);
                status = ReadOnlySortedList<TKey, TValue>::Enumerator::Create(*readOnlySortedList, enumeratorSPtr);
                if (!NT_SUCCESS(status))
                {
                    return nullptr;
                }

                return enumeratorSPtr;
            }

            KSharedPtr<IFilterableEnumerator<TKey>> GetKeys() 
            {
                NTSTATUS status;
                KSharedPtr<IFilterableEnumerator<TKey>> enumeratorSPtr;
                auto readOnlySortedList = const_cast<ReadOnlySortedList<TKey, TValue> *>(this);
                status = ReadOnlySortedList<TKey, TValue>::KeysEnumerator::Create(*readOnlySortedList, enumeratorSPtr);
                if (!NT_SUCCESS(status))
                {
                    return nullptr;
                }

                return enumeratorSPtr;
            }

            void Add(__in const TKey & key, __in const TValue & value) override
            {
                UNREFERENCED_PARAMETER(key);
                UNREFERENCED_PARAMETER(value);
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            bool Remove(__in const TKey& key) override
            {
                UNREFERENCED_PARAMETER(key);
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }

            void Clear()
            {
                throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
            }


        private:
            ReadOnlySortedList(__in IDictionary<TKey, TValue> & items, __in IComparer<TKey> & keyComparer, __in bool isAscending, __in bool isInputSorted);

            KSharedPtr<KSharedArray<KeyValuePair<TKey, TValue>>> arraySPtr_;
            bool isAscending_;
            KSharedPtr<SortedItemComparer<TKey, TValue>> keyValueComparerSPtr_;
        };

        template<typename TKey, typename TValue>
        ReadOnlySortedList<TKey, TValue>::ReadOnlySortedList(
            __in IDictionary<TKey, TValue> & items,
            __in IComparer<TKey> & keyComparer,
            __in bool isAscending,
            __in bool isInputSorted) :
            isAscending_(isAscending),
            arraySPtr_(nullptr),
            keyValueComparerSPtr_(nullptr)
        {
            UNREFERENCED_PARAMETER(keyComparer);
            arraySPtr_ = _new(READONLYSORTEDLIST_TAG, this->GetThisAllocator()) KSharedArray<Data::KeyValuePair<TKey, TValue>>();

            KSharedPtr<Data::IEnumerator<Data::KeyValuePair<TKey, TValue>>> enumeratorSPtr = items.GetEnumerator();
            while (enumeratorSPtr->MoveNext())
            {
                Data::KeyValuePair<TKey, TValue> current = enumeratorSPtr->Current();
                arraySPtr_->Append(current);
            }

            NTSTATUS status = SortedItemComparer<TKey, TValue>::Create(keyComparer, this->GetThisAllocator(), keyValueComparerSPtr_);
            Diagnostics::Validate(status);

            if (!isInputSorted)
            {
                Sorter<Data::KeyValuePair<TKey, TValue>>::QuickSort(isAscending, *keyValueComparerSPtr_, arraySPtr_);
            }
        }
        
        template<typename TKey, typename TValue>
        ReadOnlySortedList<TKey, TValue>::~ReadOnlySortedList()
        {
        }
    }
}
