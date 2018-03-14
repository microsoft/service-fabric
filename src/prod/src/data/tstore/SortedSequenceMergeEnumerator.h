// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define SORTED_SEQUENCE_MERGE_TAG 'ssme'

namespace Data
{
    namespace TStore
    {
        template<typename T>
        class SortedSequenceMergeEnumerator : public IEnumerator<T>
        {
            K_FORCE_SHARED(SortedSequenceMergeEnumerator)
        public:
            typedef KDelegate<bool(const T & key)> FilterFunctionType;

            static NTSTATUS Create(
                __in KSharedArray<KSharedPtr<IEnumerator<T>>> & inputListSPtr,
                __in IComparer<T> & keyComparer,
                //__in FilterFunctionType keyFilter,
                __in bool useFirstKey,
                __in T & firstKey,
                __in bool useLastKey,
                __in T & lastKey,
                __in KAllocator & allocator,
                __out KSharedPtr<IEnumerator<T>> & result)
            {
                 
                result = _new(SORTED_SEQUENCE_MERGE_TAG, allocator) SortedSequenceMergeEnumerator(inputListSPtr, keyComparer,/* keyFilter,*/ useFirstKey, firstKey, useLastKey, lastKey);

                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                if (!NT_SUCCESS(result->Status()))
                {
                    return (KSharedPtr<IEnumerator<T>>(Ktl::Move(result)))->Status();
                }

                return STATUS_SUCCESS;
            }

            T Current() override
            {
                ASSERT_IFNOT(isCurrentSet_, "Current got called before MoveNext()");
                return current_;
            }

            bool MoveNext() override
            {
                T lastCurrent = current_;

                while (!priorityQueueSPtr_->IsEmpty())
                {
                    KSharedPtr<ComparableSortedSequenceEnumerator<T>> currentEnumerator;
                    priorityQueueSPtr_->Pop(currentEnumerator);
                    current_ = currentEnumerator->Current();

                    bool hasMoreItems = currentEnumerator->MoveNext();
                    // TODO: dispose enumerator if it is empty
                    if (hasMoreItems)
                    {
                        InsertToPriorityQueue(*currentEnumerator);
                    }

                    // If current item is larger than the "last key", merge is finished.
                    // This must be done before the key filter check
                    if (useLastKey_ && keyComparerSPtr_->Compare(current_, lastKey_) > 0)
                    {
                        return false;
                    }

                    // TODO: Check against key filter
                    if (isCurrentSet_ == false)
                    {
                        isCurrentSet_ = true;
                        return true;
                    }

                    // De-duplicate the items
                    int comparisonWithLastCurrent = keyComparerSPtr_->Compare(lastCurrent, current_);
                    ASSERT_IFNOT(comparisonWithLastCurrent <= 0, "Last item must be less than or equal to current item. ComparisonResult: {0}", comparisonWithLastCurrent);

                    if (comparisonWithLastCurrent < 0)
                    {
                        return true;
                    }
                }

                return false;
            }

            void Close()
            {
                if (priorityQueueSPtr_ == nullptr)
                {
                    return;
                }

                while (!priorityQueueSPtr_->IsEmpty())
                {
                    KSharedPtr<ComparableSortedSequenceEnumerator<T>> currentEnumerator;
                    priorityQueueSPtr_->Pop(currentEnumerator);
                    // dispose enumerator
                }
            }

        private:
            void InitializePriorityQueue(__in KSharedArray<KSharedPtr<IEnumerator<T>>> & inputList)
            {
                for (ULONG32 i = 0; i < inputList.Count(); i++)
                {
                    KSharedPtr<IEnumerator<T>> list = inputList[i];
                    KSharedPtr<ComparableSortedSequenceEnumerator<T>> enumeratorSPtr;
                    auto status = ComparableSortedSequenceEnumerator<T>::Create(*list, *keyComparerSPtr_, this->GetThisAllocator(), enumeratorSPtr);
                    Diagnostics::Validate(status);

                    bool hasItems = PrepareEnumerator(*enumeratorSPtr);
                    // TODO: Dispose enumerator if it is empty
                    if (hasItems)
                    {
                        InsertToPriorityQueue(*enumeratorSPtr);
                    }
                }
            }

            bool PrepareEnumerator(__in ComparableSortedSequenceEnumerator<T> & enumerator)
            {
                while (true)
                {
                    bool itemExists = enumerator.MoveNext();
                    if (itemExists == false)
                    {
                        return false;
                    }

                    if (useFirstKey_)
                    {
                        int firstKeyComparison = keyComparerSPtr_->Compare(firstKey_, enumerator.Current());

                        // If firstKey for the filter is greater than the current, continue moving the enumerator
                        if (firstKeyComparison > 0)
                        {
                            continue;
                        }
                    }

                    if (useLastKey_)
                    {
                        int lastKeyComparison = keyComparerSPtr_->Compare(lastKey_, enumerator.Current());

                        // If lastKey for the filter is less than the current, indicate that the enumerator is practically empty
                        if (lastKeyComparison < 0)
                        {
                            return false;
                        }
                    }

                    // TODO: Test against key filter

                    return true;
                }
            }

            void InsertToPriorityQueue(__in ComparableSortedSequenceEnumerator<T> & enumerator)
            {
                priorityQueueSPtr_->Push(&enumerator);
                // TODO: TryPush the enumerator for de-duplication
            }

            static int CompareEnumerators(
                __in KSharedPtr<ComparableSortedSequenceEnumerator<T>> const & one, 
                __in KSharedPtr<ComparableSortedSequenceEnumerator<T>> const & two)
            {
                return one->CompareTo(*two);
            }
                
            KSharedPtr<IComparer<T>> keyComparerSPtr_;
            //FilterFunctionType keyFilter_;
            bool useFirstKey_;
            T firstKey_;
            bool useLastKey_;
            T lastKey_;

            T current_;
            bool isCurrentSet_ = false;
            KSharedPtr<SharedPriorityQueue<KSharedPtr<ComparableSortedSequenceEnumerator<T>>>> priorityQueueSPtr_;
            
            SortedSequenceMergeEnumerator(
                __in KSharedArray<KSharedPtr<IEnumerator<T>>> & inputList,
                __in IComparer<T> & keyComparer,
                //__in FilterFunctionType keyFilter,
                __in bool useFirstKey,
                __in T & firstKey,
                __in bool useLastKey,
                __in T & lastKey);
        };

        template<typename T>
        SortedSequenceMergeEnumerator<T>::SortedSequenceMergeEnumerator(
            __in KSharedArray<KSharedPtr<IEnumerator<T>>> & inputList,
            __in IComparer<T> & keyComparer,
            //__in FilterFunctionType keyFilter,
            __in bool useFirstKey,
            __in T & firstKey,
            __in bool useLastKey,
            __in T & lastKey) :
            isCurrentSet_(false),
            current_(T()),
            keyComparerSPtr_(&keyComparer),
            //keyFilter_(keyFilter),
            useFirstKey_(useFirstKey),
            useLastKey_(useLastKey),
            firstKey_(firstKey),
            lastKey_(lastKey),
            priorityQueueSPtr_(nullptr)
        {
            auto status = SharedPriorityQueue<KSharedPtr<ComparableSortedSequenceEnumerator<T>>>::Create(
                CompareEnumerators,
                this->GetThisAllocator(), 
                priorityQueueSPtr_
            );
            Diagnostics::Validate(status);
            InitializePriorityQueue(inputList);
        }

        template<typename T>
        SortedSequenceMergeEnumerator<T>::~SortedSequenceMergeEnumerator()
        {
        }
    }
}
