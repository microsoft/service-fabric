// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define PARTITIONEDLIST_TAG 'tsLP'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class PartitionedList : public KObject<PartitionedList<TKey, TValue>>, public KShared<PartitionedList<TKey, TValue>>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(PartitionedList)

        public:
            static NTSTATUS Create(
                __in int maxSize,
                __in KAllocator& allocator,
                __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITIONEDLIST_TAG, allocator) PartitionedList(maxSize);

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
                __in int initialSize,
                __in int maxSize,
                __in KAllocator& allocator,
                __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(PARTITIONEDLIST_TAG, allocator) PartitionedList(initialSize, maxSize);

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

            int Count() const
            {
                // todo: Interlocked read?
                return count_;
            }

            virtual void Add(__in TKey key, __in TValue value)
            {
                GrowIfNecessary();

                currentPartitionSPtr_->Add(key, value);
                InterlockedIncrement(&count_);
            }

            TKey GetLastKey() const
            {
                int currentPartitionCount = currentPartitionSPtr_->Count();
                if (currentPartitionCount == 0)
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION); //Empty Partition List;
                }

                return currentPartitionSPtr_->GetKey(currentPartitionCount - 1);
            }

            KSharedPtr<PartitionedListEnumerator<TKey, TValue>> GetKeys()
            {
                KSharedPtr<PartitionedListEnumerator<TKey, TValue>> partitionedListEnumerator;
                NTSTATUS status = PartitionedListEnumerator<TKey, TValue>::Create(*partitionListSPtr_, this->GetThisAllocator(), partitionedListEnumerator);
                Diagnostics::Validate(status);
                return partitionedListEnumerator;
            }

            KSharedPtr<IEnumerator<TKey>> EnumerateKeys()
            {
                KSharedPtr<IEnumerator<TKey>> keyEnumeratorSPtr;
                PartitionedListKeysEnumerator<TKey, TValue>::Create(*GetKeys(), this->GetThisAllocator(), keyEnumeratorSPtr);
                return keyEnumeratorSPtr;
            }

        protected: 
            KSharedPtr<KSharedArray<KSharedPtr<Partition<TKey, TValue>>>> partitionListSPtr_;
            int maxSubListSize_;
            PartitionedList(__in int maxSubListSize);

        private:
            void GrowIfNecessary()
            {
                if (currentPartitionSPtr_== nullptr || currentPartitionSPtr_->Count() == maxSubListSize_)
                {
                    AddNewPartition();
                }
            }
            
            void AddNewPartition(__in_opt bool isFirstPartition = false)
            {
                if (partitionListSPtr_->Count() == static_cast<ULONG>(MaxNumberOfSubLists_))
                {
                    throw ktl::Exception(SF_STATUS_INVALID_OPERATION); 
                }

                KSharedPtr<Partition<TKey, TValue>> newPartitionSPtr = nullptr;
                if (isFirstPartition)
                {
                    NTSTATUS status = Partition<TKey, TValue>::Create(maxSubListSize_, this->GetThisAllocator(), newPartitionSPtr);
                    Diagnostics::Validate(status);
                }
                else
                {
                    NTSTATUS status = Partition<TKey, TValue>::Create(maxSubListSize_, maxSubListSize_, this->GetThisAllocator(), newPartitionSPtr);
                    Diagnostics::Validate(status);
                }

                partitionListSPtr_->Append(newPartitionSPtr);
                currentPartitionSPtr_ = newPartitionSPtr;
            }

            volatile LONG count_;
            const int MaxNumberOfSubLists_ = MAXINT;
            KSharedPtr<Partition<TKey, TValue>> currentPartitionSPtr_;
        };

        template <typename TKey, typename TValue>
        PartitionedList<TKey, TValue>::PartitionedList(__in int maxSubListSize) :
           count_(0),
           maxSubListSize_(maxSubListSize)
        {
           NTSTATUS status;
           partitionListSPtr_ = _new(PARTITIONEDLIST_TAG, this->GetThisAllocator())KSharedArray<KSharedPtr<Partition<TKey, TValue>>>();

           if (!partitionListSPtr_)
           {
              this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
              return;
           }
           status = partitionListSPtr_->Status();
           this->SetConstructorStatus(status);
        }

        template <typename TKey, typename TValue>
        PartitionedList<TKey, TValue>::~PartitionedList()
        {
        }
    }
}
