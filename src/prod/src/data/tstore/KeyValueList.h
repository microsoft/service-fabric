// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define KEYVALUELIST_TAG 'tlVK'

namespace Data
{
    namespace TStore
    {
        template <typename TKey, typename TValue>
        class KeyValueList : public KObject<KeyValueList<TKey, TValue>>, public KShared<KeyValueList<TKey, TValue>>
        {
            K_FORCE_SHARED_WITH_INHERITANCE(KeyValueList)

        public:
            static NTSTATUS Create(
                    __in int maxSize,
                    __in KAllocator& allocator,
                    __out SPtr& result)
            {
                NTSTATUS status;
                SPtr output = _new(KEYVALUELIST_TAG, allocator) KeyValueList(maxSize);
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
                SPtr output = _new(KEYVALUELIST_TAG, allocator) KeyValueList(initialSize, maxSize);

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

            LONG Count() const
            {
                return keyList_.Count();
            }

            void Add(__in const TKey& key, __in const TValue& value)
            {
                if (Count() == maxSize_)
                {
                    throw ktl::Exception(K_STATUS_OBJECT_NOT_ATTACHED); //TODO: Out of capacity. KStatus doesn't have this we shall create one in KTL?
                }

                keyList_.Append(key);
                valueList_.Append(value);
            }

            const TKey& GetKey(__in int index) const
            {
                ThrowArgumentOutOfRangeIfNecessary(index);
                return keyList_[index];
            }

            const TValue& GetValue(__in int index) const
            {
                ThrowArgumentOutOfRangeIfNecessary(index);
                return valueList_[index];
            }

            void UpdateValue(__in int index, __in const TValue& value)
            {
                ThrowArgumentOutOfRangeIfNecessary(index);

                valueList_[index] = value;
            }

            int BinarySearch(__in const TKey& key, __in const IComparer<TKey>& comparer) const
            {
                return InternalBinarySearch(key, comparer);
            }

        private:
            //
            // Returns the index, if value is present. If not, returns the negative result. Bitwise complement operator can be applied to the negative result to get the index 
            // of the fisrt element taht is larger tahn the given search value.
            //
            int InternalBinarySearch(__in const TKey& key, __in const IComparer<TKey>& comparer) const
            {
                int low = 0;
                int high = keyList_.Count()-1;
                while (low <= high)
                {
                    int mid = (low + high) / 2;

                    int compareResult = comparer.Compare(keyList_[mid], key);
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

            void ThrowArgumentOutOfRangeIfNecessary(__in int& index) const
            {
                if (index >= Count())
                {
                    throw ktl::Exception(K_STATUS_OUT_OF_BOUNDS);
                }
            }

            KArray<TKey> keyList_;
            KArray<TValue> valueList_;
            int maxSize_;
            KeyValueList(__in int maxSize);
            KeyValueList(__in int initialSize, __in int maxSize);
        };

        template <typename TKey, typename TValue>
        KeyValueList<TKey, TValue>::KeyValueList(__in int maxSize) :
            // todo: hard-coding it for now.
            keyList_(this->GetThisAllocator(), maxSize, 50),
            valueList_(this->GetThisAllocator(), maxSize, 50),
            maxSize_(maxSize)
        {
            KInvariant(maxSize > 0);
        }

        template <typename TKey, typename TValue>
        KeyValueList<TKey, TValue>::KeyValueList(__in int initialSize, __in int maxSize) :
            // todo: hard-coding it for now.
            keyList_(this->GetThisAllocator(), maxSize, 50),
            valueList_(this->GetThisAllocator(), maxSize, 50),
            maxSize_(maxSize)
        {
            KInvariant(maxSize > 0);
            KInvariant(initialSize <= maxSize);
        }

        template <typename TKey, typename TValue>
        KeyValueList<TKey, TValue>::~KeyValueList()
        {
        }
    }
}
