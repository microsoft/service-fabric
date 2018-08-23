// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONCURRENTHASHTABLE_TAG 'btHC'

namespace Data
{
    namespace Utilities
    {
        // Concurrent re-implementation of KHashTable, with inspiration from CLR ConcurrentDictionary implementation
        // Enumeration is NOT lock free and will prevent writes to buckets sharing the same lock that the enumeration is on
        // Active enumeration will also prevent resizes
        // Number of locks does not grow/shrink when table is resized

        template<typename TKey, typename TValue>
        class ConcurrentHashTable sealed : public IDictionary<TKey, TValue>
        {
            K_FORCE_SHARED(ConcurrentHashTable)

        private:
            class HashElement;
            class Enumerator;

            friend class Enumerator;

        public:
            typedef KDelegate<ULONG(const TKey& Key)> HashFunctionType;
            typedef KDelegate<BOOLEAN(__in const TKey& KeyOne, __in const TKey& KeyTwo)> EqualityComparisonFunctionType;

        public:
            __declspec(property(get = get_Count)) ULONG Count;
            ULONG get_Count() const override
            {
                return static_cast<ULONG>(count_);
            }

            static NTSTATUS Create(
                __in HashFunctionType hashFunction,
                __in ULONG concurrencyLevel,
                __in KAllocator& allocator,
                __out SPtr & result)
            {
                SPtr output = _new(CONCURRENTHASHTABLE_TAG, allocator) ConcurrentHashTable(hashFunction, concurrencyLevel);

                if (!output)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                result = Ktl::Move(output);
                return STATUS_SUCCESS;
            }

            bool TryGetValue(
                __in const TKey& key, 
                __out TValue & data) const override
            {
                while (true)
                {
                    HashElement** snapTable = table_;

                    ULONG locus = hashFunction_.ConstCall(key) % numBuckets_;

                    KReaderWriterSpinLock & lock = locks_[locus % numLocks_];
                    lock.AcquireShared();
                    KFinally([&] { lock.ReleaseShared();  });

                    if (snapTable != table_)
                    {
                        // Table was resized, we may be holding onto the wrong lock. Retry...
                        continue;
                    }

                    HashElement* node = table_[locus];
                    for (; node; node = node->Next)
                    {
                        if (AreEqual(node->Key, key))
                        {
                            data = node->Data;
                            return true;
                        }
                    }

                    return false;
                }
            }

            bool ContainsKey(__in const TKey& key) const override
            {
                TValue value;
                bool exists = TryGetValue(key, value);
                return exists;
            }

            void Add(__in const TKey& key, __in const TValue& value) override
            {
                bool added = TryAdd(key, value);
                if (!added)
                {
                    throw ktl::Exception(STATUS_OBJECT_NAME_COLLISION);
                }
            }

            bool Remove(__in const TKey& key) override
            {
                TValue value;
                return TryRemove(key, value);
            }

            void Clear() override
            {
                // Acquire all locks
                for (ULONG32 i = 0; i < numLocks_; i++)
                {
                    locks_[i].AcquireExclusive();
                }

                HashElement** oldTable = table_;
                LONG64 oldNumBuckets = numBuckets_;

                currentPrimeIndex_ = 0;
                numBuckets_ = Primes[currentPrimeIndex_];
                table_ = _newArray<HashElement*>(CONCURRENTHASHTABLE_TAG, this->GetThisAllocator(), numBuckets_);

                if (table_ == nullptr)
                {
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }

                count_ = 0;

                // Release all locks
                for (ULONG32 i = 0; i < numLocks_; i++)
                {
                    locks_[i].ReleaseExclusive();
                }

                if (oldTable)
                {
                    for (ULONG32 locus = 0; locus < oldNumBuckets; locus++)
                    {
                        HashElement* node = oldTable[locus];
                        while (node)
                        {
                            HashElement* temp = node->Next;
                            _delete(node);
                            node = temp;
                        }
                        oldTable[locus] = nullptr;
                    }
                }

                _deleteArray(oldTable);
            }

            // Using this enumerator is potentially dangerous since it is not lock free
            // As a worst case scenario, multiple enumerators could be created that collectively take all the locks.
            // If these enumerators are abandoned, no more writes can occur
            KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> GetEnumerator() const override
            {
                KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> result;
                KSharedPtr<ConcurrentHashTable<TKey, TValue>> hashTable = const_cast<ConcurrentHashTable<TKey, TValue> *>(this);
                NTSTATUS status = Enumerator::Create(*hashTable, this->GetThisAllocator(), result);

                if (!NT_SUCCESS(status))
                {
                    throw ktl::Exception(status);
                }

                return result;
            }

        public:
            bool TryAdd(__in const TKey& key, __in const TValue& value)
            {
                bool added;
                AddOrUpdateInternal(key, value, /* forceUpdate: */ false, added);
                return added;
            }

            void AddOrUpdate(__in const TKey& key, __in const TValue& value, __out bool & added)
            {
                AddOrUpdateInternal(key, value, /* forceUpdate: */ true, added);
            }

            bool TryRemove(__in const TKey& key, __out TValue& value)
            {
                bool removed = false;
                bool resizeNeeded = false;

                HashElement** snapTable = nullptr;

                while (true)
                {
                    snapTable = table_;

                    ULONG locus = hashFunction_.ConstCall(key) % numBuckets_;
                    KReaderWriterSpinLock & lock = locks_[locus % numLocks_];

                    {
                        lock.AcquireExclusive();
                        KFinally([&] { lock.ReleaseExclusive(); });

                        if (snapTable != table_)
                        {
                            // Table was re-sized so it is possible that wrong lock is taken. Retry...
                            continue;
                        }

                        HashElement* node = table_[locus];
                        HashElement* previous = nullptr;

                        for (; node != nullptr; previous = node, node = node->Next)
                        {
                            if (AreEqual(node->Key, key))
                            {
                                if (previous)
                                {
                                    previous->Next = node->Next;
                                }
                                else
                                {
                                    table_[locus] = node->Next;
                                }

                                value = node->Data;

                                _delete(node);
                                LONG64 newCount = InterlockedDecrement64(&count_);
                                resizeNeeded = IsUnderSaturated(newCount);

                                removed = true;
                                break;
                            }
                        }
                    }

                    // This is done outside of lock so that resize does not deadlock

                    if (resizeNeeded)
                    {
                        ASSERT_IFNOT(snapTable != nullptr, "Snapped table should not be null before resize");
                        Resize(snapTable, false);
                    }

                    return removed;
                }
            }

        private:
            void AddOrUpdateInternal(
                __in const TKey& key, 
                __in const TValue& value,
                __in bool forceUpdate,
                __out bool & keyAdded)
            {
                HashElement** snapTable = nullptr;;
                bool resizeNeeded = false;

                while (true)
                {
                    snapTable = table_;
                    ULONG hashValue = hashFunction_(key);
                    ULONG locus = hashValue % numBuckets_;

                    KReaderWriterSpinLock & lock = locks_[locus % numLocks_];

                    {
                        lock.AcquireExclusive();
                        KFinally([&] { lock.ReleaseExclusive(); });

                        if (snapTable != table_)
                        {
                            // Resize occurred and so it is possible wrong lock was held. Retry...
                            continue;
                        }

                        HashElement* node = table_[locus];
                        HashElement* previous = nullptr;

                        for (; node != nullptr; previous = node, node = node->Next)
                        {
                            if (AreEqual(node->Key, key))
                            {
                                if (forceUpdate)
                                { 
                                    node->Data = value;
                                }

                                keyAdded = false;
                                return;
                            }
                        }

                        // Didn't find pre-existing key, so add a new one
                        HashElement* newNode = _new(CONCURRENTHASHTABLE_TAG, this->GetThisAllocator()) HashElement(key, value, hashValue);

                        if (newNode == nullptr)
                        {
                            throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                        }

                        if (previous)
                        {
                            previous->Next = newNode;
                        }
                        else
                        {
                            table_[locus] = newNode;
                        }

                        LONG64 newCount = InterlockedIncrement64(&count_);
                        resizeNeeded = IsOverSaturated(newCount);
                    }
                    
                    // This is done outside the lock so that resize does not deadlock

                    if (resizeNeeded)
                    {
                        ASSERT_IFNOT(snapTable != nullptr, "snapTable should not be null");
                        Resize(snapTable, true);
                    }

                    keyAdded = true;
                    return;
                }
            }

            // Resize should be called sparingly
            // Since Resize takes all the locks, there can potentially be a pause in the system while all threads wait for the resize to complete
            void Resize(__in HashElement** snappedTable, __in bool grow)
            {

                // First thread to get first lock gets to do the resizing
                locks_[0].AcquireExclusive();
                KFinally([&] { locks_[0].ReleaseExclusive(); });

                if (snappedTable != table_)
                {
                    // Some other thread resized, nothing needs to be done
                    return;
                }

                // Get rest of locks
                for (ULONG32 i = 1; i < numLocks_; i++)
                {
                    locks_[i].AcquireExclusive();
                }

                KFinally([&] {
                    for (ULONG32 i = 1; i < numLocks_; i++)
                    {
                        locks_[i].ReleaseExclusive();
                    }
                });

                if (table_ == nullptr)
                {
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }

                LONG64 oldIndex = currentPrimeIndex_;

                if (grow)
                {
                    currentPrimeIndex_ = __min(PrimesSize, currentPrimeIndex_ + 1);
                }
                else
                {
                    currentPrimeIndex_ = __max(0, currentPrimeIndex_ - 1);
                }

                if (currentPrimeIndex_ == oldIndex)
                {
                    // Unable to resize
                    return;
                }

                ULONG newSize = Primes[currentPrimeIndex_];

                // Create the new hash bucket table
                HashElement** tempTable = _newArray<HashElement*>(CONCURRENTHASHTABLE_TAG, this->GetThisAllocator(), newSize);

                if (tempTable == nullptr)
                {
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }

                RtlZeroMemory(tempTable, sizeof(HashElement*) * newSize);

                // TODO: Grow locks array?

                // Redistribute all elements to the new hash bucket table
                for (ULONG32 bucket = 0; bucket < numBuckets_; bucket++)
                {
                    HashElement* nextNode = table_[bucket];
                    while (nextNode != nullptr)
                    {
                        HashElement* currentNode = nextNode;
                        nextNode = currentNode->Next;

                        ULONG newLocus = currentNode->HashValue % newSize;
                        currentNode->Next = tempTable[newLocus];
                        tempTable[newLocus] = currentNode;
                    }
                }

                _deleteArray(table_);
                numBuckets_ = newSize;
                table_ = tempTable;
            }

            __forceinline bool IsUnderSaturated(__in LONG64 count)
            {
                // The undersaturation percentage is really low because we only want to shrink the table when there is a great benefit
                // Since Resize takes all the locks, there can potentially be a pause in the system while all threads wait for the resize to complete

                LONG64 saturationPercentage = (count * 100) / static_cast<LONG64>(numBuckets_);
                return saturationPercentage < 10; 
            }

            __forceinline bool IsOverSaturated(__in LONG64 count)
            {
                LONG64 saturationPercentage = (count * 100) / static_cast<LONG64>(numBuckets_);
                return saturationPercentage > 60;
            }

            __forceinline bool AreEqual(__in TKey const & keyOne, __in TKey const & keyTwo) const
            {
                if (!equalityFunctionSet_)
                {
                    return keyOne == keyTwo;
                }

                return equalityFunction_.ConstCall(keyOne, keyTwo);
            }

            ConcurrentHashTable(
                __in HashFunctionType hashFunction,
                __in ULONG concurrencyLevel)
                : hashFunction_(hashFunction)
                , numLocks_(concurrencyLevel)
                , currentPrimeIndex_(0)
            {
                numBuckets_ = Primes[currentPrimeIndex_];
                table_ = _newArray<HashElement*>(CONCURRENTHASHTABLE_TAG, this->GetThisAllocator(), numBuckets_);
                if (table_ == nullptr)
                {
                    this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
                    return;
                }

                RtlZeroMemory(table_, sizeof(HashElement *) * numBuckets_);

                locks_ = _newArray<KReaderWriterSpinLock>(CONCURRENTHASHTABLE_TAG, this->GetThisAllocator(), numLocks_);
                if (locks_ == nullptr)
                {
                    this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
                    return;
                }
            }

        private:
            class HashElement
            {
            public:
                const TKey    Key;
                TValue        Data;
                HashElement*  Next = nullptr;
                const ULONG   HashValue;

                HashElement(const TKey&  key, const TValue& value, ULONG hashValue) :
                    Key(key),
                    Data(value),
                    HashValue(hashValue)
                {
                }
            };

        private:
            HashElement** table_;
            ULONG numBuckets_;

            HashFunctionType hashFunction_;

            KReaderWriterSpinLock* locks_;
            ULONG numLocks_;

            EqualityComparisonFunctionType equalityFunction_;
            bool equalityFunctionSet_ = false;

            LONG64 count_ = 0;

            LONG currentPrimeIndex_;
            static ULONG const      Primes[];
            static const LONG      PrimesSize;
        };

        template<typename TKey, typename TValue>
        ConcurrentHashTable<TKey, TValue>::~ConcurrentHashTable()
        {
            Clear();
            _deleteArray(table_);
            _deleteArray(locks_);
        }

        template <typename TKey, typename TValue>
        ULONG const ConcurrentHashTable<TKey, TValue>::Primes[] =
        {
            1,          3,          7,          17,         37,         79,         163,        331,        673,        
            1361,       2729,       5471,       10949,      21911,      43853,      87719,      175447,     350899,     
            701819,     1403641,    2807303,    5614657,    11229331,   22458671,   44917381,   89834777,   179669557,  
            359339171,  718678369,  1437356741, 2874713497
        };

        template <typename TKey, typename TValue>
        const LONG ConcurrentHashTable<TKey, TValue>::PrimesSize = sizeof(Primes) / sizeof(ULONG);
    }
}
