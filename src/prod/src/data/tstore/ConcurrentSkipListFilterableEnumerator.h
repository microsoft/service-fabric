// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONCURRENTSKIPLIST_FILTERABLEENUMERATOR_TAG 'efSC'

namespace Data
{
    namespace TStore
    {
        template<typename TKey, typename TValue>
        class ConcurrentSkipListFilterableEnumerator : public IFilterableEnumerator<TKey>
        {
            K_FORCE_SHARED(ConcurrentSkipListFilterableEnumerator)

        public:
            static NTSTATUS Create(
                __in ConcurrentSkipList<TKey, TValue> & skipList,
                __out KSharedPtr<IFilterableEnumerator<TKey>> & result)
            {
                result = _new(CONCURRENTSKIPLIST_FILTERABLEENUMERATOR_TAG, skipList.GetThisAllocator()) ConcurrentSkipListFilterableEnumerator(skipList);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

        private:
            ConcurrentSkipListFilterableEnumerator(__in ConcurrentSkipList<TKey, TValue> & skipList) : 
                skipList_(&skipList),
                current_(skipList.Head()),
                isFirstMove_(false) // defaulting to false. should be true only when MoveTo is used
            {
            }

        public:
            TKey Current()
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

            bool MoveTo(__in TKey const & key)
            {
                bool hasNext = true;

                current_ = skipList_->FindNode(key);

                // If current is tail, this must be the end of the list.
                auto tailNodeType = ConcurrentSkipList<TKey, TValue>::Node::NodeType::Tail;
                if (current_->Type == tailNodeType)
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

            bool MoveNext()
            {
                auto tailNodeType = ConcurrentSkipList<TKey, TValue>::Node::NodeType::Tail;
                if (current_ == nullptr || current_->Type == tailNodeType)
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
                    auto nextNode = current_->GetNextNode(0);
                    current_->Unlock();
                    current_ = nextNode;

                    auto headNodeType = ConcurrentSkipList<TKey, TValue>::Node::NodeType::Head;
                    ASSERT_IFNOT(current_->Type != headNodeType, "Invalid head node type encountered");

                    // If current is tail, this must be the end of the list.
                    if (current_->Type == tailNodeType)
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
            typename ConcurrentSkipList<TKey, TValue>::Node::SPtr current_;
            typename ConcurrentSkipList<TKey, TValue>::SPtr skipList_;
            bool isFirstMove_;
        };

        template<typename TKey, typename TValue>
        ConcurrentSkipListFilterableEnumerator<TKey, TValue>::~ConcurrentSkipListFilterableEnumerator()
        {
        }
    }
}
