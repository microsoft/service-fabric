// ---------------------------------------------------------------------------- -
// Copyright (c) Microsoft Corporation.  All rights reserved.
//-----------------------------------------------------------------------------
#pragma once

#define RCQ_TAG 'rtCQ'

namespace Data
{
    namespace Collections
    {
        //
        // A lock-free array-based circular queue that can be linked together to form a bigger queue
        //
        template <typename TValue>
        class ConcurrentQueueSegment           
            : public KObject<ConcurrentQueueSegment<TValue>>
            , public KShared<ConcurrentQueueSegment<TValue>>
        {
            K_FORCE_SHARED(ConcurrentQueueSegment)

            FAILABLE ConcurrentQueueSegment(__in size_t size);

        public :           
            static NTSTATUS Create(
                __in KAllocator& allocator,
                __in LONG64 size,
                __out SPtr& result)
            {
                ConcurrentQueueSegment<TValue>* pointer = _new(RCQ_TAG, allocator) ConcurrentQueueSegment<TValue>(size);
                if (!pointer)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                result = pointer;
                return STATUS_SUCCESS;
            }

        public :
            bool TryEnqueue(__in TValue value)
            {
                ASSERT_IFNOT(value != TValue(), "Empty value is reserved");

                while (true)
                {                    
                    LONG64 tail = tail_;
                    LONG64 index = tail & mask_;
                    LONG64 diff = slots_[index].Sequence - tail;
                    if (diff == 0)
                    {
                        // the item is not occupied
                        if (InterlockedCompareExchange64(&tail_, tail + 1, tail) == tail)
                        {
                            // we win the race and incremented the tail
                            slots_[index].Value = value;

                            // mark the item as occupied / enqueued
                            InterlockedExchange64(&slots_[index].Sequence, (tail + 1));

                            return true;
                        }

                        // otherwise we lost - try again
                    }
                    else if (diff < 0)
                    {
                        // either we are catching up with dequeued data or this item has been enqueued
                        // either way the queue is now full
                        return false;
                    }
                    else
                    {
                        // We lost a race to another enqueue operation - do nothing and try again
                    }
                }
            }

            //
            // Remove the first occurrence of specific value in the queue and null it out with default value
            // This is used when dequeue specific item in secondary where dequeue could come in arbitary order 
            //
            bool TryRemove(__in TValue value)
            {
                ASSERT_IFNOT(value != TValue(), "Empty value is reserved");

                LONG64 head = head_;
                while (true)
                {
                    LONG64 index = head & mask_;

                    LONG64 diff = slots_[index].Sequence - (head + 1);

                    if (diff == 0)
                    {
                        // the item has been enqueued earlier
                        if (slots_[index].Value == value)
                        {
                            // it's a match - "null-out" the entry
                            slots_[index].Value = TValue();
                            return true;
                        }

                        head++;
                    }
                    else if (diff < 0)
                    {
                        // Empty queue - might be transient but it's ok to treat it as empty
                        return false;
                    }
                    else
                    {
                        // this has just been dequeued - in theory this should never happen in secondary as
                        // secondary only does remove and never dequeue
                    }
                }
            }

            bool TryDequeue(__in TValue &value)
            {
                while (true)
                {
                    LONG64 head = head_;
                    LONG64 index = head & mask_;

                    LONG64 diff = slots_[index].Sequence - (head + 1);

                    if (diff == 0)
                    {
                        // the item has been enqueued earlier
                        // try to reserve the item
                        if (InterlockedCompareExchange64(&head_, (head + 1), head) == head)
                        {
                            value = slots_[index].Value;
                            
                            slots_[index].Value = TValue();

                            // Mark sequence number as head + segment size for two reasons:
                            // 1. Another dequeuer will know this item has already been dequeued, and will retry
                            // 2. When dequeue come around and see this item again, it will treat this as empty
                            InterlockedExchange64(&slots_[index].Sequence, (head + mask_ + 1));

                            if (value == TValue())
                            {
                                // This item has been removed from secondary earlier and represents a gap
                                // We should skip this one and move to the next item
                                // This won't ever race as secondary would only do Remove and primary would
                                // only dequeue
                                continue;
                            }

                            return true;
                        }
                    }
                    else if (diff < 0)
                    {
                        // Empty queue - might be transient but it's ok to treat it as empty
                        return false;
                    }
                    else
                    {
                        // we are racing with a dequeue and lost - try again
                    }
                }
            }

            bool IsEmpty()
            {
                LONG64 head = head_;
                LONG64 index = head & mask_;

                LONG64 diff = slots_[index].Sequence - (head + 1);

                return (diff < 0);
            }

            SPtr GetNext()
            {
                return next_.Get();
            }

            bool SetNextIfNull(__in SPtr &&sptr)
            {
                return next_.PutIf(Ktl::Move(sptr), nullptr);
            }

            size_t GetSize()
            {
                return mask_ + 1;
            }

        private :
            struct ConcurrentQueueSegmentItem
            {
                LONG64 Sequence;        // Sequence number
                TValue Value;           // value
            };

        private :
            LONG64 head_;               // index for dequeue 
            LONG64 tail_;               // index for enqueue
            LONG64 mask_;               // masking index into (0, size-1). size is always power of 2

            ConcurrentQueueSegmentItem *slots_;                             // all data
            ThreadSafeSPtrCache<ConcurrentQueueSegment<TValue>> next_;      // next segment
        };

        template <typename TValue>
        FAILABLE ConcurrentQueueSegment<TValue>::ConcurrentQueueSegment(__in size_t size)
        {
            mask_ = size - 1;

            slots_ = _new(RCQ_TAG, this->GetThisAllocator()) ConcurrentQueueSegmentItem[size];
            if (slots_ == nullptr)
            {
                this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
                return;
            }

            for (int i = 0; i < size; ++i)
            {
                slots_[i].Sequence = i;
            }

            head_ = 0;
            tail_ = 0;
        }

        template <typename TValue>
        ConcurrentQueueSegment<TValue>::~ConcurrentQueueSegment()
        {
            _delete(slots_);
            slots_ = nullptr;
        }

        //
        // In-memory ConcurrentQueue consists of a link list of ConcurrentQueueSegments
        //
        template <typename TValue>
        class ConcurrentQueue
            : public KObject<ConcurrentQueue<TValue>>
            , public KShared<ConcurrentQueue<TValue>>
        {
            K_FORCE_SHARED(ConcurrentQueue)

        private :
            ConcurrentQueue(__in size_t startSegmentSize, __in size_t maxSegmentSize);

        public :            
            static NTSTATUS Create(
                __in KAllocator& allocator,
                __in size_t startSegmentSize,
                __in size_t maxSegmentSize,
                __out SPtr& result)
            {
                ConcurrentQueue<TValue>* pointer = _new(RCQ_TAG, allocator) ConcurrentQueue<TValue>(startSegmentSize, maxSegmentSize);
                if (!pointer)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                result = pointer;
                return STATUS_SUCCESS;
            }

            NTSTATUS TryDequeue(TValue &value, bool &succeeded)
            {
                succeeded = false;

                while (true)
                {
                    //
                    // Given that we may need to access the head while another thread is doing a dequeue, we need
                    // to protect ourselves from accessing reclaimed memory - unfortunately ref-count along won't
                    // protect us. This needs to be achieved using a R/W lock protecting both pointer access and
                    // ref-count increment. Once we "checkout" the pointer it is then safe to access the head
                    // as we already hold a ref count, and ref-counting works wonderfully from there on.
                    // Usually if we are not modifying head there will be no contention - only when we are modifying
                    // head there will be a brief writer lock which is acceptable.
                    //
                    auto head = head_.Get();

                    //
                    // Try dequeue within head 
                    //
                    if (head->TryDequeue(value))
                    {        
                        succeeded = true;
                        return STATUS_SUCCESS;
                    }

                    //
                    // Otherwise try crossing the segment and Change the head
                    // If we lost the race we should try again
                    //
                    auto newHead = head->GetNext();

                    //
                    // No more segments available
                    // NOTE: When we are removing segments from head, we don't null out its next pointer 
                    // because a dequeuer that try to change the head at the same time could see the null
                    //
                    if (newHead == nullptr)
                    {
                        return STATUS_SUCCESS;
                    }

                    //
                    // Set the head - try again if we lose the race
                    // This takes a R/W lock to avoid ABA problem
                    //
                    if (!head_.PutIf(Ktl::Move(newHead), head))
                    {
                        continue;
                    }
                }
            }

            NTSTATUS Remove(__in LONG64 value)
            {
                typename ConcurrentQueueSegment<TValue>::SPtr head;

                //
                // In order to avoid unbound memory growth in secondary - we need to free head segments
                // when we apply dequeue(key) if the head segment is already empty
                //
                while (true)
                {
                    head = head_.Get();

                    if (!head->IsEmpty())
                    {
                        break;
                    }

                    //
                    // If head is empty, repeatedly try to free the empty block
                    //
                    auto newHead = head->GetNext();

                    //
                    // This is the last segment
                    //
                    if (newHead == nullptr)
                    {
                        break;
                    }

                    //
                    // Unlink head - try again if we lost the race
                    //
                    if (!head_.PutIf(Ktl::Move(newHead), head))
                    {
                        continue;
                    }

                    head = head_.Get();
                }

                while (head != nullptr)
                {
                    //
                    // Try dequeue within head 
                    //
                    if (head->TryRemove(value))
                    {
                        return STATUS_SUCCESS;
                    }

                    //
                    // Otherwise traverse to the next segment
                    // This should be rare as dequeue always happens in the first segment in primary
                    // but in secondary it is possible the element happens to be in the next segment
                    // due to secondary receiving the enqueue in arbitary order
                    //
                    head = head->GetNext();
                }

                return STATUS_NOT_FOUND;
            }

            NTSTATUS Enqueue(__in TValue value)
            {
                typename ConcurrentQueueSegment<TValue>::SPtr newSegment;

                while (true)
                {                    
                    //
                    // Intuitively you might think it is safe to just get tail because we never deallocate 
                    // tail. However this is not true when:
                    // 1. There is only one segment that both head and tail points to
                    // 2. A dequeuer A aggressively dequeues every thing in the segment, and dereference
                    // the head segment
                    // 3. A enqueuer B ses the old head segment that is being destroyed...
                    //
                    auto tail = tail_.Get();

                    //
                    // If we can enqueue in tail, the world is a better place
                    //
                    if (tail->TryEnqueue(value))
                    {
                        return STATUS_SUCCESS;
                    }

                    // 
                    // Otherwise create a new segment (if we haven't created one already in last attempt)
                    //
                    if (!newSegment)
                    {
                        size_t newSize = tail->GetSize() * 2;
                        if (newSize > maxSegmentSize_)
                            newSize = maxSegmentSize_;

                        NTSTATUS status = ConcurrentQueueSegment<TValue>::Create(this->GetThisAllocator(), newSize, newSegment);
                        if (!NT_SUCCESS(status))
                        {
                            return status;
                        }
                    }

                    //
                    // Try change tail.next first - this way the linked segments are always consistent
                    // And if another enqueuer comes in, it'll also try to set tail.next and fail, until
                    // until tail is successfully set
                    //
                    if (!tail->SetNextIfNull(typename ConcurrentQueueSegment<TValue>::SPtr(newSegment)))
                    {
                        continue;
                    }

                    tail_.Put(Ktl::Move(newSegment));
                }
            }

        private :
            ThreadSafeSPtrCache<ConcurrentQueueSegment<TValue>> head_;
            ThreadSafeSPtrCache<ConcurrentQueueSegment<TValue>> tail_;

            size_t maxSegmentSize_;
        };

        template <typename TValue>
        FAILABLE ConcurrentQueue<TValue>::ConcurrentQueue(__in size_t startSegmentSize, __in size_t maxSegmentSize)
        {
            typename ConcurrentQueueSegment<TValue>::SPtr newSegment;
            NTSTATUS status = ConcurrentQueueSegment<TValue>::Create(this->GetThisAllocator(), startSegmentSize, newSegment);
            if (!NT_SUCCESS(status))
            {
                this->SetConstructorStatus(status);
                return;
            }

            head_.Put(Ktl::Move(newSegment));
            tail_.Put(head_.Get());

            maxSegmentSize_ = maxSegmentSize;
        }

        template <typename TValue>
        ConcurrentQueue<TValue>::~ConcurrentQueue() {}  
    }
}
