// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once


#define PARTITIONSORTEDLIST_TAG 'rtST'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class PartitionedSortedListFilterableEnumerator;

        //
        // Partitioned sorted list expects items to be inserted in order
        // Recovery and and consolidation get the item sin order following merge.
        //
        template <typename TKey, typename TValue>
        class PartitionedSortedList : public PartitionedList<TKey, TValue>
        {
            K_FORCE_SHARED(PartitionedSortedList)

        public:
            static NTSTATUS Create(
                __in IComparer<TKey>& keyComparer,
                __in KAllocator& allocator,
                __out SPtr& result,
                __in int maxSublistSize=8192)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITIONSORTEDLIST_TAG, allocator) PartitionedSortedList(keyComparer, maxSublistSize);

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

            TValue GetValue(__in TKey key)
            {
                Index index(-1, -1);
                bool itemExists = TryGetIndex(key, index);
                if (itemExists == false)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION);
                }

                return (*this->partitionListSPtr_)[index.PartitionIndex]->GetValue(index.ItemIndex);
            }

            void UpdateValue(__in TKey key, __in TValue value)
            {
                Index index(-1, -1);
                bool itemExists = TryGetIndex(key, index);
                if (itemExists == false)
                {
                   throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
                }

               (*this->partitionListSPtr_)[index.PartitionIndex]->UpdateValue(index.ItemIndex, value);
            }

            virtual void Add(__in TKey key, __in TValue value)
            {
                // this.ThrowIfKeyIsNull(key);

#if DBG            // todo: run in debug mode only
                if (this->Count() != 0)
                {
                    TKey lastKey = PartitionedList<TKey, TValue>::GetLastKey();
                    int comparison = keyComparerSPtr_->Compare(lastKey, key);
                    KInvariant(comparison < 0);
                }
#endif

                PartitionedList<TKey, TValue>::Add(key, value);
            }

            bool ContainsKey(__in const TKey& key)
            {
                Index index(-1, -1);
                return TryGetIndex(key, index);
            }

            bool TryGetValue(__in TKey key, __out TValue& value)
            {
                Index index(-1, -1);
                bool itemExists = TryGetIndex(key, index);

                if (itemExists == false)
                {
                    return false;
                }

                value = (*this->partitionListSPtr_)[index.PartitionIndex]->GetValue(index.ItemIndex);
                return true;
            }

            bool TryGetKeyValue(__in Index index, __out KeyValuePair<TKey, TValue> & item)
            {
                if (index.PartitionIndex < 0 || static_cast<ULONG>(index.PartitionIndex) >= this->partitionListSPtr_->Count())
                {
                    return false;
                }

                KSharedPtr<Partition<TKey, TValue>> partition = (*this->partitionListSPtr_)[index.PartitionIndex];

                if (index.ItemIndex < 0 || index.ItemIndex >= partition->Count())
                {
                    return false;
                }

                item.Key = (*this->partitionListSPtr_)[index.PartitionIndex]->GetKey(index.ItemIndex);
                item.Value = (*this->partitionListSPtr_)[index.PartitionIndex]->GetValue(index.ItemIndex);
                return true;
            }

            bool TryGetIndex(__in const TKey key, __out Index& index)
            {
                index = Index(-1, -1);
                //this.ThrowIfKeyIsNull(key);

                if (this->Count() == 0)
                {
                    return false;
                }

                PartitionSearchKey<TKey, TValue> searchKey(key);
                int partitionIndex = BinarySearch(searchKey, *partitionComparerSPtr_);

                if (partitionIndex >= 0)
                {
                    index = Index(partitionIndex, 0);
                    return true;
                }

                partitionIndex = (~partitionIndex) - 1;

                if (partitionIndex < 0)
                {
                    return false;
                }

                int itemIndex = (*this->partitionListSPtr_)[partitionIndex]->BinarySearch(key, *keyComparerSPtr_);
                if (itemIndex >= 0)
                {
                    index = Index(partitionIndex, itemIndex);
                    return true;
                }

                if (~itemIndex == this->maxSubListSize_)
                {
                    partitionIndex++;
                    itemIndex = ~0;
                }

                index = Index(~partitionIndex, itemIndex);
                return false;
            }
            
            KSharedPtr<PartitionedSortedListFilterableEnumerator<TKey, TValue>> GetEnumerator()
            {
                KSharedPtr<PartitionedSortedListFilterableEnumerator<TKey, TValue>> resultSPtr = nullptr;
                NTSTATUS status = PartitionedSortedListFilterableEnumerator<TKey, TValue>::Create(*this, this->GetThisAllocator(), resultSPtr);
                Diagnostics::Validate(status);
                return resultSPtr;
            }

        private:
            //
            // Returns the index, if value is present. If not, returns the negative result. Bitwise complement operator can be applied to the negative result to get the index 
            // of the fisrt element that is larger than the given search value.
            //
            int BinarySearch(__in const PartitionSearchKey<TKey, TValue>& searchKey, __in const PartitionComparer<TKey, TValue>& comparer)
            {
                int low = 0;
                int high = this->partitionListSPtr_->Count()-1;
                while (low <= high)
                {
                    int mid = (low + high) / 2;

                    // mid is greather than the input value
                    const KSharedPtr<Partition<TKey, TValue>>& midPartition = (*this->partitionListSPtr_)[mid];
                    
                    int compareResult = comparer.Compare(*midPartition, searchKey);
                    if (compareResult > 0)
                    {
                        high = mid - 1;
                    }
                    else if(compareResult < 0)
                    {
                        low = mid + 1;
                    }
                    else
                    {
                        return mid;
                    }
                }

                return ~low;
            }

            KSharedPtr<PartitionComparer<TKey, TValue>> partitionComparerSPtr_;
            KSharedPtr<IComparer<TKey>> keyComparerSPtr_;
            PartitionedSortedList(__in IComparer<TKey>& keyComparer, __in int maxSublistSize=8192);
        };

        template <typename TKey, typename TValue>
        PartitionedSortedList<TKey, TValue>::PartitionedSortedList(__in IComparer<TKey>& keyComparer, __in int maxSublistSize) :
            // todo: hard-coding it for now.
           PartitionedList<TKey, TValue>(maxSublistSize),
           keyComparerSPtr_(&keyComparer)
        {
           NTSTATUS status = PartitionComparer<TKey, TValue>::Create(keyComparer, this->GetThisAllocator(), partitionComparerSPtr_);
           this->SetConstructorStatus(status);
        }

        template <typename TKey, typename TValue>
        PartitionedSortedList<TKey, TValue>::~PartitionedSortedList()
        {
        }
    }
}
