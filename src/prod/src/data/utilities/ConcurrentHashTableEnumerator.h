// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

namespace Data
{
    namespace Utilities
    {
        // TODO: Lock-free enumeration
        // Using this enumerator is potentially dangerous since it is not lock free
        // As a worst case scenario, multiple enumerators could be created that collectively take all the locks.
        // If these enumerators are abandoned, no more writes can occur
        template<typename TKey, typename TValue>
        class ConcurrentHashTable<TKey, TValue>::Enumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
        {
            K_FORCE_SHARED(Enumerator)

        public:
            static NTSTATUS Create(
                __in ConcurrentHashTable<TKey, TValue> & hashTable,
                __in KAllocator & allocator,
                __out KSharedPtr<IEnumerator<KeyValuePair<TKey, TValue>>> & result)
            {
                result = _new(CONCURRENTHASHTABLE_TAG, allocator) Enumerator(hashTable);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            KeyValuePair<TKey, TValue> Current()
            {
                return KeyValuePair<TKey, TValue>(currentNode_->Key, currentNode_->Data);
            }

            bool MoveNext()
            {
                if (currentNode_ != nullptr)
                {
                    currentNode_ = currentNode_->Next;

                    if (currentNode_ != nullptr)
                    {
                        return true;
                    }
                }

                while (currentBucketNumber_ < hashTableSPtr_->numBuckets_ - 1)
                {
                    // Order is important - this prevents a potential resize changing the table underneath
                    AcquireNextLock();
                    ReleaseCurrentLock();

                    currentBucketNumber_++;
                    currentNode_ = hashTableSPtr_->table_[currentBucketNumber_];

                    if (currentNode_ != nullptr)
                    {
                        return true;
                    }
                }

                return false;
            }

        private:
            void AcquireNextLock()
            {
                if (currentBucketNumber_ >= -1 && currentBucketNumber_ < hashTableSPtr_->numBuckets_ - 1)
                {
                    KReaderWriterSpinLock & currentLock = hashTableSPtr_->locks_[(currentBucketNumber_ + 1) % hashTableSPtr_->numLocks_];
                    currentLock.AcquireShared();
                }
            }

            void ReleaseCurrentLock()
            {
                if (currentBucketNumber_ > -1 && currentBucketNumber_ < hashTableSPtr_->numBuckets_)
                {
                    KReaderWriterSpinLock & currentLock = hashTableSPtr_->locks_[currentBucketNumber_ % hashTableSPtr_->numLocks_];
                    currentLock.ReleaseShared();
                }
            }

            Enumerator(
                __in ConcurrentHashTable<TKey, TValue> & hashTable)
                : hashTableSPtr_(&hashTable)
                , currentBucketNumber_(-1)
                , currentNode_(nullptr)
            {
            }

            KSharedPtr<ConcurrentHashTable<TKey, TValue>> hashTableSPtr_ = nullptr;

            LONG64 currentBucketNumber_;
            HashElement* currentNode_;
        };

        template<typename TKey, typename TValue>
        ConcurrentHashTable<TKey, TValue>::Enumerator::~Enumerator()
        {
            ReleaseCurrentLock();
        }

    }
}
