// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

#pragma once

#define CONCURRENTSKIPLIST_TAG 'tlSC'

namespace Data
{
    namespace Utilities
    {
        // This is a lock-based concurrent skip list.
        //
        // SkipList is a probabilistic logarithmic search data structure.
        // It is probabilistic because unlike other search data structures (AVL, RedBlackTree, BTree) it does not balance.
        //
        // SkipList is an ideal data structure to be concurrent because
        // 1. Low contention: Only predecessors need to be locked for writes.
        // 2. Minimized memory synchronization: Only predecessor nodes whose links are changed are synchronized.
        //
        // This ConcurrentSkipList provides
        // 1. Expected O(log n), Lock-free and wait-free ContainsKey and TryGetValue.
        // 2. Expected O(log n), TryAdd, Update and TryRemove.
        // 3. Lock-free key enumerations.
        //
        // Notes:
        // Logical delete is used to avaid double skiplist traversal for adds and removes.
        // For logical delete, isDeleted mark is used where a node is marked (under lock) to be deleted before it is phsyically removed from the skip list.
        //
        // Logical insert is used since the node needs to be linked into the list one level at a time.
        //
        // Invariants:
        // List(level1) is a super set of List(level2) iff level1 less than or equal to level2.
        // Side effect of this is that nodes are added bottom up and removed top down.
        //
        // Locks: Bottom up. (Unlock order is irrelevant).

        class MonitorLock
        {
        private:
            inline ULONGLONG Encode(DWORD tid)
            {
                return ((ULONGLONG)tid) << 31 | 0x40000000;
            }

            inline ULONGLONG GetTid(ULONGLONG Encoded)
            {
                return Encoded & ~((ULONGLONG)0x3FFFFFFF);
            }

            inline ULONGLONG GetCount(ULONGLONG Encoded)
            {
                return Encoded & ((ULONGLONG)0x3FFFFFFF);
            }

        public:
            MonitorLock() : value_(0) {}

            void Acquire()
            {
                ULONGLONG tid =  Encode(GetCurrentThreadId());
                if (GetTid(value_) != tid)
                {
                    spin_.Acquire();
                    ULONGLONG oldValue = InterlockedCompareExchange64((volatile LONGLONG*)&value_, tid + 1, 0);
                    ASSERT_IFNOT(oldValue == 0, "ConcurrentSkipList Acquire() old value is expected to be 0. It is {0}", oldValue);
                }
                else
                {
                    InterlockedIncrement64((volatile LONGLONG*)&value_);
                }
            }

            void Release()
            {
                ULONGLONG tid =  Encode(GetCurrentThreadId());
                ULONGLONG newValue = InterlockedDecrement64((volatile LONGLONG*)&value_);
                ASSERT_IFNOT(GetTid(newValue) == tid, "ConcurrentSkipList Release() tid {0} is not equal to GetTid(newValue) {1}", tid, GetTid(newValue));

                if (GetCount(newValue) == 0)
                {
                    value_ = 0;
                    spin_.Release();
                }
            }

        private:
            volatile ULONGLONG value_;
            KSpinLock spin_;
        };

        template<typename TKey, typename TValue>
        class ConcurrentSkipList sealed :
            public KObject<ConcurrentSkipList<TKey, TValue>>,
            public KShared<ConcurrentSkipList<TKey, TValue>>
        {
            K_FORCE_SHARED(ConcurrentSkipList)

        public:
            typedef KDelegate<int(__in const TKey& KeyOne, __in const TKey& KeyTwo)> ComparisonFunctionType;
            typedef KDelegate<TValue(__in const TKey& Key, __in const TValue& Value)> UpdateFunctionType;
            typedef KDelegate<TValue(__in const TKey& Key)> ValueFactoryType;

        private:
            class SearchResult;
            class SearchResultForRead;

        public:
            class Node;
            class Enumerator;

        private:
            static const int InvalidLevel = -1;
            static const int BottomLevel = 0;
            static const int DefaultNumberOfLevels = 32;
            static constexpr double DefaultPromotionProbability = 0.5;

        private:
            int numberOfLevels_;
            int topLevel_;
            double promotionProbability_;
            ThreadSafeSPtrCache<Node> head_ = { nullptr };
            ThreadSafeSPtrCache<Node> tail_ = { nullptr };
            typename IComparer<TKey>::SPtr keyComparer_;
            RandomGenerator randomGenerator_;

        public:
            static NTSTATUS Create(
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                KSharedPtr<IComparer<TKey>> comparer;
                status = DefaultComparer<TKey>::Create(allocator, comparer);
                if (!NT_SUCCESS(status))
                {
                    return status;
                }

                return Create(comparer,
                              DefaultNumberOfLevels,
                              DefaultPromotionProbability,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in KSharedPtr<IComparer<TKey>> const & comparer,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                return Create(comparer,
                              DefaultNumberOfLevels,
                              DefaultPromotionProbability,
                              allocator,
                              result);
            }

            static NTSTATUS Create(
                __in KSharedPtr<IComparer<TKey>> const & comparer,
                __in LONG32 numberOfLevels,
                __in double promotionProbability,
                __in KAllocator & allocator,
                __out SPtr & result)
            {
                NTSTATUS status;
                SPtr output = _new(CONCURRENTSKIPLIST_TAG, allocator) ConcurrentSkipList(comparer, numberOfLevels, promotionProbability, allocator);
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

        private:
            FAILABLE ConcurrentSkipList(
                __in KSharedPtr<IComparer<TKey>> const & keyComparer,
                __in int numberOfLevels,
                __in double promotionProbability,
                __in KAllocator & allocator)
                : keyComparer_(keyComparer),
                  numberOfLevels_(numberOfLevels),
                  topLevel_(numberOfLevels - 1),
                  promotionProbability_(promotionProbability)
            {
                ASSERT_IFNOT(numberOfLevels > 0, "Invalid number of levels: {0}", numberOfLevels);
                ASSERT_IFNOT(promotionProbability > 0, "Invalid promotion probability: {0}", promotionProbability);
                ASSERT_IFNOT(promotionProbability < 1, "Invalid promotion probability: {0}", promotionProbability);

                typename Node::SPtr headNode = _new(CONCURRENTSKIPLIST_TAG, allocator) Node(Node::NodeType::Head, topLevel_);
                if (headNode == nullptr)
                {
                    this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                this->head_.Put(Ktl::Move(headNode));
                headNode = this->head_.Get();

                typename Node::SPtr tailNode = _new(CONCURRENTSKIPLIST_TAG, allocator) Node(Node::NodeType::Tail, topLevel_);
                if (tailNode == nullptr)
                {
                    this->SetConstructorStatus(STATUS_INSUFFICIENT_RESOURCES);
                }

                this->tail_.Put(Ktl::Move(tailNode));
                tailNode = this->tail_.Get();

                // Always link in bottom up.
                for (int level = 0; level <= topLevel_; level++)
                {
                    typename Node::SPtr cachedTailNode = this->tail_.Get();
                    headNode->SetNextNode(level, cachedTailNode);
                }

                headNode->IsInserted = true;
                tailNode->IsInserted = true;

            }

        public:
            __declspec(property(get = get_Count)) ULONG Count;
            ULONG ConcurrentSkipList::get_Count()
            {
                return GetCount(BottomLevel);
            }

            __declspec(property(get = get_IsEmpty)) bool IsEmpty;
            bool ConcurrentSkipList::get_IsEmpty()
            {
                return get_Count() == 0;
            }

            void Clear()
            {
                typename Node::SPtr cachedHead = head_.Get();

                for (int level = 0; level <= topLevel_; level++)
                {
                    typename Node::SPtr cachedTail = tail_.Get();
                    cachedHead->SetNextNode(level, cachedTail);
                }
            }

            bool ContainsKey(__in TKey const & key) const
            {
                auto searchResult = this->WeakSearchForRead(key);

                // If node is not found, not logically inserted or logically removed, return false.
                if (searchResult->IsFound && searchResult->SearchNode->IsInserted && searchResult->SearchNode->IsDeleted == false)
                {
                    return true;
                }

                return false;
            }
            
            KSharedPtr<Node> Head() const
            {
                return head_.Get();
            }

            // Finds the node for the key >= the given key
            KSharedPtr<Node> FindNode(__in TKey const & key) const
            {
                auto searchResult = this->WeakSearchForRead(key);
                return searchResult->GetNode();
            }

            bool TryGetValue(__in TKey const & key, __out TValue & value)
            {
                typename SearchResultForRead::SPtr searchResult = WeakSearchForRead(key);
                if (!searchResult->IsFound)
                {
                    return false;
                }

                typename Node::SPtr node = searchResult->SearchNode;
                if (!node->IsInserted || node->IsDeleted)
                {
                    return false;
                }

                node->Lock();
                KFinally([&]()
                {
                    node->Unlock();
                });
                if (node->IsDeleted)
                {
                    return false;
                }

                value = node->Value;
                return true;
            }

            template <typename ValueFactory,
                      typename UpdateValueFactory>
            TValue AddOrUpdate(
                __in TKey const & key,
                __in ValueFactory valueFactory,
                __in UpdateValueFactory updateValueFactory)
            {
                auto nodeUpdateFunc = [&] (__in typename Node::SPtr & node)
                {
                    node->Value = updateValueFactory(key, node->Value);
                };
                return GetOrAddOrUpdate(key, valueFactory, nodeUpdateFunc);
            }

            template <typename UpdateValueFactory>
            TValue AddOrUpdate(
                __in TKey const & key,
                __in TValue const & value,
                __in UpdateValueFactory updateValueFactory)
            {
                auto valueFactory = [&] (TKey const &)
                {
                    return value;
                };
                return AddOrUpdate(key, valueFactory, updateValueFactory);
            }

            template <typename ValueFactoryFunc>
            TValue GetOrAdd(
                __in TKey const & key,
                __in ValueFactoryFunc valueFactory,
                __out_opt bool * added = nullptr)
            {
                auto nodeUpdateFunc = [] (__in typename Node::SPtr &)
                {
                    return;
                };
                return GetOrAddOrUpdate(key, valueFactory, nodeUpdateFunc, added);
            }

            TValue GetOrAdd(
                __in TKey const & key,
                __in TValue const & value)
            {
                auto valueFactory = [&] (TKey const &)
                {
                    return value;
                };
                return GetOrAdd(key, valueFactory);
            }

            bool TryAdd(
                __in TKey const & key,
                __in TValue const & value)
            {
                bool added = false;
                auto valueFactory = [&] (TKey const &)
                {
                    return value;
                };
                GetOrAdd(key, valueFactory, &added);
                return added;
            }

            template <typename ValueEqualFunc>
            bool TryUpdate(
                __in TKey const & key,
                __in TValue const & newValue,
                __in TValue const & comparisonValue,
                __in ValueEqualFunc valueEqualFunc)
            {
                typename SearchResult::SPtr searchResult = this->WeakSearch(key);

                typename Node::SPtr nodeToBeUpdated = nullptr;
                if (searchResult->IsFound)
                {
                    nodeToBeUpdated = searchResult->GetNodeFound();
                }

                if (searchResult->IsFound == false || nodeToBeUpdated->IsInserted == false || nodeToBeUpdated->IsDeleted)
                {
                    return false;
                }

                nodeToBeUpdated = searchResult->GetNodeFound();
                nodeToBeUpdated->Lock();

                KFinally([&]()
                {
                    nodeToBeUpdated->Unlock();
                });
                if (nodeToBeUpdated->IsDeleted)
                {
                    return false;
                }

                if (!valueEqualFunc(nodeToBeUpdated->Value, comparisonValue)) {
                    return false;
                }

                nodeToBeUpdated->Value = newValue;
                return true;
            }

            bool TryUpdate(
                __in TKey const & key,
                __in TValue const & newValue,
                __in TValue const & comparisonValue)
            {
                auto valueEqualFunc = [] (__in TValue const & lhs, __in TValue const & rhs)
                {
                    return lhs == rhs;
                };
                return TryUpdate(key, newValue, comparisonValue, valueEqualFunc);
            }

            void Update(
                __in TKey const & key,
                __in TValue const & value)
            {
                typename SearchResult::SPtr searchResult = this->WeakSearch(key);

                typename Node::SPtr nodeToBeUpdated = nullptr;
                if (searchResult->IsFound)
                {
                    nodeToBeUpdated = searchResult->GetNodeFound();
                }

                if (searchResult->IsFound == false || nodeToBeUpdated->IsInserted == false || nodeToBeUpdated->IsDeleted)
                {
                    throw ktl::Exception(STATUS_INVALID_PARAMETER_1);
                }

                nodeToBeUpdated = searchResult->GetNodeFound();
                nodeToBeUpdated->Lock();

                KFinally([&]()
                {
                    nodeToBeUpdated->Unlock();
                });
                try
                {
                    if (nodeToBeUpdated->IsDeleted)
                    {
                        throw ktl::Exception(STATUS_INVALID_PARAMETER);
                    }

                    nodeToBeUpdated->Value = value;
                }
                catch(...)
                {
                    throw;
                }
            }

            void Update(
                __in TKey const & key,
                __in UpdateFunctionType updateFunction)
            {
                typename SearchResult::SPtr searchResult = this->WeakSearch(key);

                typename Node::SPtr nodeToBeUpdated = nullptr;
                if (searchResult->IsFound)
                {
                    nodeToBeUpdated = searchResult->GetNodeFound();
                }

                if (searchResult->IsFound == false || nodeToBeUpdated->IsInserted == false || nodeToBeUpdated->IsDeleted)
                {
                    throw ktl::Exception(STATUS_INVALID_PARAMETER);
                }

                nodeToBeUpdated = searchResult->GetNodeFound();
                nodeToBeUpdated->Lock();
                KFinally([&]()
                {
                    nodeToBeUpdated->Unlock();
                });
                try
                {
                    if (nodeToBeUpdated->IsDeleted)
                    {
                        throw ktl::Exception(STATUS_INVALID_PARAMETER);
                    }

                    auto newValue = updateFunction(key, nodeToBeUpdated->Value);
                    nodeToBeUpdated->Value = newValue;
                }
                catch(...)
                {
                    throw;
                }
            }

            bool TryRemove(
                __in TKey const & key,
                __out TValue & value)
            {
                // Node to be deleted (if it exists).
                typename Node::SPtr nodeToBeDeleted = nullptr;

                // Indicates whether the node to be deleted is already locked and logically marked as deleted.
                bool isLogicallyDeleted = false;

                // Level at which the to be deleted node was found.
                int topLevel = InvalidLevel;

                while (true)
                {
                    typename SearchResult::SPtr searchResult = this->WeakSearch(key);

                    if (searchResult->IsFound)
                    {
                        nodeToBeDeleted = searchResult->GetNodeFound();
                    }

                    // MCoskun: Why do we have a top level check?
                    // A node gets physically linked from level 0 up to its top level.
                    // Top level check ensures that we found the node at the level we were expecting it.
                    // Due to the lock-free nature of weak-search, there is a race between predecessors being physically linked to the node and node being marked logically inserted.
                    // It is possible for the weak search to snap predecessors in between the above points, then node gets marked as logically inserted and then the IsInserted below happening.
                    if (!isLogicallyDeleted && (searchResult->IsFound == false || nodeToBeDeleted->IsInserted == false || nodeToBeDeleted->TopLevel != searchResult->LevelFound || nodeToBeDeleted->IsDeleted))
                    {
                        return false;
                    }

                    // If not already logically removed, lock it and mark it.
                    if (!isLogicallyDeleted)
                    {
                        topLevel = searchResult->LevelFound;
                        nodeToBeDeleted->Lock();
                        if (nodeToBeDeleted->IsDeleted)
                        {
                            nodeToBeDeleted->Unlock();
                            return false;
                        }

                        // Linearization point: IsDeleted is volatile.
                        nodeToBeDeleted->IsDeleted = true;
                        isLogicallyDeleted = true;
                    }

                    int highestLevelLocked = InvalidLevel;
                    KFinally([&]()
                    {
                        for (int level = highestLevelLocked; level >= 0; level--)
                        {
                            searchResult->GetPredecessor(level)->Unlock();
                        }
                    });
                    try
                    {
                        bool isValid = true;
                        for (int level = 0; isValid && level <= topLevel; level++)
                        {
                            auto predecessor = searchResult->GetPredecessor(level);
                            predecessor->Lock();
                            highestLevelLocked = level;
                            isValid = predecessor->IsDeleted == false && predecessor->GetNextNode(level) == nodeToBeDeleted;
                        }

                        if (isValid == false)
                        {
                            // Using Sleep instead of YieldExeuction since the latter is an undocumented API
                            KNt::Sleep(0);
                            continue;
                        }

                        // MCoskun: To preserve the invariant that lower levels are super-set of higher levels, always unlink top to bottom.
                        // Memory Barrier could have been used to guarantee above but the node is already under lock
                        for (int level = topLevel; level >= 0; level--)
                        {
                            auto predecessor = searchResult->GetPredecessor(level);
                            auto newLink = nodeToBeDeleted->GetNextNode(level);

                            predecessor->SetNextNode(level, newLink);
                        }

                        // This requires that not between the nodeToBeDeleted.Lock and this throws.
                        nodeToBeDeleted->Unlock();

                        value = nodeToBeDeleted->Value;

#ifdef DBG
                        // This is an expensive assert that ensures that the node is physically removed from the skip list.
                        auto temp = this->WeakSearch(key);
                        ASSERT_IFNOT(temp->IsFound == false, "Node not removed from skip list");
#endif

                        return true;
                    }
                    catch(...)
                    {
                        throw;
                    }
                }
            }

        private:
            template <typename ValueFactory,
                      typename NodeUpdateFunc>
            TValue GetOrAddOrUpdate(
                __in TKey const & key,
                __in ValueFactory valueFactory,
                __in NodeUpdateFunc nodeUpdateFunc,
                __out_opt bool * added = nullptr)
            {
                int insertLevel = this->GenerateLevel();

                while (true)
                {
                    typename SearchResult::SPtr searchResult = WeakSearch(key);
                    if (searchResult->IsFound)
                    {
                        typename Node::SPtr cachedNodeToBeUpdated = searchResult->GetNodeFound();
                        if (cachedNodeToBeUpdated->IsDeleted)
                        {
                            continue;
                        }

                        // Spin until the duplicate key is logically inserted.
                        this->WaitUntilIsInserted(cachedNodeToBeUpdated);

                        cachedNodeToBeUpdated->Lock();

                        KFinally([&]()
                        {
                            cachedNodeToBeUpdated->Unlock();
                        });
                        if (cachedNodeToBeUpdated->IsDeleted)
                        {
                            continue;
                        }

                        nodeUpdateFunc(cachedNodeToBeUpdated);
                        if (added)
                        {
                            *added = false;
                        }
                        return cachedNodeToBeUpdated->Value;
                    }

                    int highestLevelLocked = InvalidLevel;
                    KFinally([&]()
                    {
                        // Unlock order is not important.
                        for (int level = highestLevelLocked; level >= 0; level--)
                        {
                            searchResult->GetPredecessor(level)->Unlock();
                        }
                    });
                    try
                    {
                        bool isValid = true;
                        for (int level = 0; isValid && level <= insertLevel; level++)
                        {
                            auto predecessor = searchResult->GetPredecessor(level);
                            auto successor = searchResult->GetSuccessor(level);

                            predecessor->Lock();
                            highestLevelLocked = level;

                            // If predecessor is locked and the predecessor is still pointing at the successor, successor cannot be deleted.
                            isValid = this->IsValidLevel(predecessor, successor, level);
                        }

                        if (isValid == false)
                        {
                            continue;
                        }

                        // Create the new node and initialize all the next pointers.
                        TValue value = valueFactory(key);
                        typename Node::SPtr newNode = _new(CONCURRENTSKIPLIST_TAG, this->GetThisAllocator()) Node(key, value, insertLevel);
                        if (newNode == nullptr)
                        {
                            throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                        }
                        for (int level = 0; level <= insertLevel; level++)
                        {
                            newNode->SetNextNode(level, searchResult->GetSuccessor(level));
                        }

                        // MCoskun: Ensure that the node is fully initialized before physical linking starts.
                        MemoryBarrier();

                        for (int level = 0; level <= insertLevel; level++)
                        {
                            // MCoskun: Note that this is required for correctness.
                            // Remove takes a dependency of the fact that if found at expected level, all the predecessors have already been correctly linked.
                            // Hence we only need to use a MemoryBarrier before linking in the top level.
                            if (level == insertLevel)
                            {
                                MemoryBarrier();
                            }

                            searchResult->GetPredecessor(level)->SetNextNode(level, newNode);
                        }

                        // Linearization point: MemoryBarrier not required since IsInserted is a volatile member (hence implicitly uses MemoryBarrier).
                        newNode->IsInserted = true;
                        if (added)
                        {
                            *added = true;
                        }
                        return value;
                    }
                    catch(...)
                    {
                        throw;
                    }
                }
            }

            int GetCount(__in int level)
            {
                int count = 0;
                typename Node::SPtr current = this->head_.Get();
                while (true)
                {
                    current = current->GetNextNode(level);

                    ASSERT_IFNOT(current->Type != Node::Head, "Corrupted internal list");

                    // If current is tail, this must be the end of the list.
                    if (current->Type == Node::Tail)
                    {
                        return count;
                    }

                    count++;
                }
            }

            void Verify()
            {
                // Verify the invariant that lower level lists are always super-set of higher-level lists.
                for (int i = BottomLevel; i < this->topLevel_; i++)
                {
                    auto lowLevelCount = this->GetCount(i);
                    auto highLevelCount = this->GetCount(i + 1);

                    ASSERT_IFNOT(
                        lowLevelCount >= highLevelCount, 
                        "Lower level lists should be superset of higher level lists - low level count: {0}, high level count: {1}",
                        lowLevelcount, highLevelCount);
                }
            }

            inline void WaitUntilIsInserted(__in typename Node::SPtr const & node)
            {
                while (node->IsInserted == false)
                {
                    // Using Sleep instead of YieldExeuction since the latter is an undocumented API
                    KNt::Sleep(0);
                }
            }

            inline int GenerateLevel()
            {
                int level = 0;
                while (level < this->topLevel_)
                {
                    double divineChoice = randomGenerator_.NextDouble();
                    if (divineChoice <= this->promotionProbability_)
                    {
                        level++;
                    }
                    else
                    {
                        break;
                    }
                }

#ifdef DBG
                ASSERT_IFNOT(level > InvalidLevel && level <= this->topLevel_, "Invalid level: {0}", level);
#endif
                return level;

            }

            typename SearchResult::SPtr WeakSearch(__in TKey const & key) const
            {
                int levelFound = InvalidLevel;

                typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr predecessorArray = nullptr;
                NTSTATUS status = ShareableArray<ThreadSafeSPtrCache<Node>>::Create(this->GetThisAllocator(), predecessorArray, this->numberOfLevels_, this->numberOfLevels_, 0);
                if (!NT_SUCCESS(status))
                {
                    throw ktl::Exception(status);
                }

                typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr successorArray = nullptr;
                status = ShareableArray<ThreadSafeSPtrCache<Node>>::Create(this->GetThisAllocator(), successorArray, this->numberOfLevels_, this->numberOfLevels_, 0);
                if (!NT_SUCCESS(status))
                {
                    throw ktl::Exception(status);
                }

                typename Node::SPtr predecessor = this->head_.Get();
                for (int level = this->topLevel_; level >= 0; level--)
                {
                    // TODO: Is MemoryBarrier required?
                    typename Node::SPtr current = predecessor->GetNextNode(level);

                    while (true)
                    {
                        int nodeComparison = this->Compare(current, key);
                        if (levelFound == InvalidLevel && nodeComparison == 0)
                        {
                            levelFound = level;
                        }

                        // current is >= searchKey, search at this level is over.
                        if (nodeComparison >= 0)
                        {
                            (*predecessorArray)[level].Put(predecessor.RawPtr());
                            (*successorArray)[level].Put(current.RawPtr());

                            break;
                        }

                        predecessor = Ktl::Move(current);
                        current = predecessor->GetNextNode(level);
                    }
                }

#ifdef DBG
                ASSERT_IFNOT(levelFound >= InvalidLevel && levelFound <= this->topLevel_, "Invalid level found: {0}", levelFound);
#endif
                typename SearchResult::SPtr result = _new(CONCURRENTSKIPLIST_TAG, this->GetThisAllocator()) SearchResult(levelFound, predecessorArray, successorArray);
                if (result == nullptr)
                {
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }
                return result;
            }

            typename SearchResultForRead::SPtr WeakSearchForRead(__in TKey const & key) const
            {
                int levelFound = InvalidLevel;
                typename Node::SPtr predecessor = this->head_.Get();
                typename Node::SPtr current = nullptr;
                for (int level = this->topLevel_; level >= 0; level--)
                {
                    // TODO: Is MemoryBarrier required?
                    current = predecessor->GetNextNode(level);

                    while (true)
                    {
                        int nodeComparison = this->Compare(current, key);
                        if (levelFound == InvalidLevel && nodeComparison == 0)
                        {
                            levelFound = level;
                        }

                        // current is >= searchKey, search at this level is over.
                        if (nodeComparison >= 0)
                        {

                            break;
                        }

                        predecessor = current;
                        current = predecessor->GetNextNode(level);
                    }
                }

#ifdef DBG
                ASSERT_IFNOT(levelFound >= InvalidLevel && levelFound <= this->topLevel_, "Invalid level found: {0}", levelFound);
#endif
                typename SearchResultForRead::SPtr result = _new(CONCURRENTSKIPLIST_TAG, this->GetThisAllocator()) SearchResultForRead(levelFound, *current);
                if (result == nullptr)
                {
                    throw ktl::Exception(STATUS_INSUFFICIENT_RESOURCES);
                }

                return result;
            }

            int Compare(typename Node::SPtr const & node, TKey const & key) const
            {
                if (node->Type == Node::NodeType::Head)
                {
                    return -1;
                }

                if (node->Type == Node::NodeType::Tail)
                {
                    return 1;
                }

                return this->keyComparer_->Compare(node->Key, key);
            }

            bool IsValidLevel(typename Node::SPtr const & predecessor, typename Node::SPtr const & successor, int level)
            {
                ASSERT_IFNOT(predecessor != nullptr, "Predecessor is null");
                ASSERT_IFNOT(successor != nullptr, "Successor is null");

                return predecessor->IsDeleted == false && successor->IsDeleted == false && predecessor->GetNextNode(level) == successor;
            }

        public:
            class Node :
                public KObject<Node>,
                public KShared<Node>
            {
                K_FORCE_SHARED(Node)

            public:
                enum NodeType : unsigned char
                {
                    Head = 0,
                    Tail = 1,
                    Data = 2
                };

                Node(__in NodeType nodeType, __in int height)
                    : nodeType_(nodeType),
                      key_(),
                      value_(),
                      isInserted_(false),
                      isDeleted_(false)
                {
                    NTSTATUS status = ShareableArray<ThreadSafeSPtrCache<Node>>::Create(this->GetThisAllocator(), nextNodeArray_, height + 1, height + 1, 0);
                    if (!NT_SUCCESS(status))
                    {
                        this->SetConstructorStatus(status);
                    }
                }

                Node(__in TKey const & key, __in TValue const & value, __in int height)
                    : nodeType_(NodeType::Data),
                      key_(key),
                      value_(value),
                      isInserted_(false),
                      isDeleted_(false)
                {
                    NTSTATUS status = ShareableArray<ThreadSafeSPtrCache<Node>>::Create(this->GetThisAllocator(), nextNodeArray_, height + 1, height + 1, 0);
                    if (!NT_SUCCESS(status))
                    {
                        this->SetConstructorStatus(status);
                    }
                }

                __declspec(property(get = get_Key)) TKey const & Key;
                TKey const & get_Key() const
                {
                    return key_;
                }

                __declspec(property(get = get_Value, put = put_Value)) TValue const & Value;
                TValue const & get_Value() const
                {
                    return value_;
                }
                void put_Value(TValue const & val)
                {
                    value_ = val;
                }

                __declspec(property(get = get_Type)) NodeType Type;
                NodeType get_Type() const
                {
                    return nodeType_;
                }

                __declspec(property(get = get_IsInserted, put = put_IsInserted)) bool IsInserted;
                bool get_IsInserted() const
                {
                    return isInserted_;
                }
                void put_IsInserted(bool val)
                {
                    isInserted_ = val;
                }

                __declspec(property(get = get_IsDeleted, put = put_IsDeleted)) bool IsDeleted;
                bool get_IsDeleted() const
                {
                    return isDeleted_;
                }
                void put_IsDeleted(bool val)
                {
                    isDeleted_ = val;
                }

                __declspec(property(get = get_TopLevel)) int TopLevel;
                int get_TopLevel() const
                {
                    return nextNodeArray_->Count() - 1;;
                }

                SPtr GetNextNode(__in int height) const
                {
                    return (*nextNodeArray_)[height].Get();
                }

                void SetNextNode(__in int height, __in SPtr const & next)
                {
                    typename Node::SPtr nextCast = const_cast<Node *>(next.RawPtr());
                    (*nextNodeArray_)[height].Put(Ktl::Move(nextCast));
                    //(*nextNodeArray_)[height] = next;
                }

                void Lock()
                {
                    nodeLock_.Acquire();
                }

                void Unlock()
                {
                    nodeLock_.Release();
                }

            private:
                mutable MonitorLock nodeLock_;
                KSharedPtr<ShareableArray<ThreadSafeSPtrCache<Node>>> nextNodeArray_;
                NodeType nodeType_;
                TKey key_;
                TValue value_;
                volatile bool isInserted_;
                volatile bool isDeleted_;
            };

        private:
            class SearchResult :
                public KObject<SearchResult>,
                public KShared<SearchResult>
            {
                K_FORCE_SHARED(SearchResult)

            private:
                int levelFound_;
                typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr predecessorArray_;
                typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr successorArray_;

            public:
                SearchResult(
                        __in int lvlFound,
                        __in typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr const & predArray,
                        __in typename ShareableArray<ThreadSafeSPtrCache<Node>>::SPtr const & succArray)
                : levelFound_(lvlFound),
                  predecessorArray_(predArray),
                  successorArray_(succArray)
                {
                }

                __declspec(property(get = get_LevelFound)) int LevelFound;
                int get_LevelFound() const
                {
                    return this->levelFound_;
                }

                __declspec(property(get = get_IsFound)) bool IsFound;
                bool get_IsFound() const
                {
                    return this->levelFound_ != InvalidLevel;
                }

                typename Node::SPtr GetPredecessor(int level)
                {
                    ASSERT_IFNOT(this->predecessorArray_ != nullptr, "Predecessor array is null");
                    return (*predecessorArray_)[level].Get();
                }

                typename Node::SPtr GetSuccessor(int level)
                {
                    ASSERT_IFNOT(this->successorArray_ != nullptr, "Successor array is null");
                    return (*successorArray_)[level].Get();
                }

                typename Node::SPtr GetNodeFound()
                {
#ifdef DBG
                    ASSERT_IFNOT(this->successorArray_ != nullptr, "Successor array is null");
                    ASSERT_IFNOT(this->IsFound, "Node not found");
#endif
                    return (*successorArray_)[levelFound_].Get();
                }
            };

            class SearchResultForRead :
                public KObject<SearchResultForRead>,
                public KShared<SearchResultForRead>
            {
                K_FORCE_SHARED(SearchResultForRead)

            private:
                int levelFound_;
                typename ThreadSafeSPtrCache<Node> searchNode_ = { nullptr };

            public:
                SearchResultForRead(
                    __in int levelFound,
                    __in Node& node)
                    : levelFound_(levelFound)
                {
                    typename Node::SPtr nodeSPtr = &node;
                    searchNode_.Put(Ktl::Move(nodeSPtr));
                }

                __declspec(property(get = get_LevelFound)) int LevelFound;
                int get_LevelFound() const
                {
                    return this->levelFound_;
                }

                __declspec(property(get = get_IsFound)) bool IsFound;
                bool get_IsFound() const
                {
                    return this->levelFound_ != InvalidLevel;
                }

                __declspec(property(get = get_SearchNode))  typename Node::SPtr SearchNode;
                typename Node::SPtr  get_SearchNode() const
                {
#ifdef DBG
                    ASSERT_IFNOT(this->IsFound, "Node not found");
#endif
                    return this->searchNode_.Get();
                }

                typename Node::SPtr GetNode() const
                {
                    return this->searchNode_.Get();
                }


            };
        };

        template<typename TKey, typename TValue>
        ConcurrentSkipList<TKey, TValue>::~ConcurrentSkipList()
        {
        }
        template<typename TKey, typename TValue>
        ConcurrentSkipList<TKey, TValue>::Node::~Node()
        {
        }

        template<typename TKey, typename TValue>
        ConcurrentSkipList<TKey, TValue>::SearchResult::~SearchResult()
        {
        }

        template<typename TKey, typename TValue>
        ConcurrentSkipList<TKey, TValue>::SearchResultForRead::~SearchResultForRead()
        {
        }
    }
}
