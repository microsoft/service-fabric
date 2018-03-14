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
        class FastSkipList<TKey, TValue>::KeysEnumerator : public IFilterableEnumerator<TKey>
        {
            K_FORCE_SHARED(KeysEnumerator)
        public:
            static NTSTATUS Create(
                __in FastSkipList<TKey, TValue> & fastSkipList,
                __out KSharedPtr<IFilterableEnumerator<TKey>> & result)
            {
                result = _new(CONCURRENTSKIPLIST_TAG, fastSkipList.GetThisAllocator()) KeysEnumerator(fastSkipList);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

            TKey Current() override
            {
                if (current_ != nullptr)
                {
                    return current_->Key;
                }
                else
                {
                    throw ktl::Exception(STATUS_NO_MORE_ENTRIES);
                }
            }

            bool MoveTo(__in TKey const & key) override
            {
                bool hasNext = true;

                current_ = skipList_->FindNode(key);
                // If current is tail, this must be the end of the list.
                if (current_->Type == Node::NodeType::Tail)
                {
                    return false;
                }

                // Takes advantage of the fact that next is set before the node is physically linked.
                if (current_->IsInserted == false || current_->IsDeleted)
                {
                    hasNext = MoveNext();
                }

                isFirstMove_ = true;
                return hasNext;
            }

            bool MoveNext() override
            {
                if (current_ == nullptr || current_->Type == Node::NodeType::Tail)
                {
                    return false;
                }

                if (isFirstMove_)
                {
                    isFirstMove_ = false;
                    return true;
                }

                while (true)
                {
                    current_->Lock();
                    auto nextNode = current_->GetNextNode(BottomLevel);
                    current_->Unlock();
                    current_ = nextNode;

                    ASSERT_IFNOT(current_->Type != Node::NodeType::Head, "Invalid head node type encountered");

                    // If current is tail, this must be the end of the list.
                    if (current_->Type == Node::NodeType::Tail)
                    {
                        return false;
                    }

                    // Takes advantage of the fact that next is set before the node is physically linked.
                    if (current_->IsInserted == false || current_->IsDeleted)
                    {
                        continue;
                    }

                    return true;
                }
            }

        private:
            KeysEnumerator(__in FastSkipList<TKey, TValue> & skipList) : 
                skipList_(&skipList),
                current_(skipList.head_),
                isFirstMove_(false) // should be true only when MoveTo is used
            {
            }
            
            typename FastSkipList<TKey, TValue>::SPtr skipList_;
            typename Node::SPtr current_;
            bool isFirstMove_;
        };

        template<typename TKey, typename TValue>
        FastSkipList<TKey, TValue>::KeysEnumerator::~KeysEnumerator()
        {
        }
    }
}
