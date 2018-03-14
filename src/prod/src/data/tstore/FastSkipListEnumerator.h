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
        class FastSkipList<TKey, TValue>::Enumerator : public IEnumerator<KeyValuePair<TKey, TValue>>
        {
            K_FORCE_SHARED(Enumerator)

        public:
            static NTSTATUS Create(
                __in typename FastSkipList::SPtr const & skipList,
                __out typename IEnumerator<KeyValuePair<TKey, TValue>>::SPtr & result)
            {
                if (skipList == nullptr)
                {
                    return STATUS_INVALID_PARAMETER;
                }

                result = _new(CONCURRENTSKIPLIST_TAG, skipList->GetThisAllocator()) Enumerator(skipList->head_);
                if (!result)
                {
                    return STATUS_INSUFFICIENT_RESOURCES;
                }

                return STATUS_SUCCESS;
            }

        private:
            Enumerator(typename Node::SPtr const & head) : current_(head)
            {
            }

        public:
            KeyValuePair<TKey, TValue> Current()
            {
                if (current_ != nullptr)
                {
                    return KeyValuePair<TKey, TValue>(current_->Key, current_->Value);
                }
                else
                {
                    throw ktl::Exception(STATUS_NO_MORE_ENTRIES);
                }
            }

            bool MoveNext()
            {
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
            typename Node::SPtr current_;
        };

        template<typename TKey, typename TValue>
        FastSkipList<TKey, TValue>::Enumerator::~Enumerator()
        {
        }
    }
}
