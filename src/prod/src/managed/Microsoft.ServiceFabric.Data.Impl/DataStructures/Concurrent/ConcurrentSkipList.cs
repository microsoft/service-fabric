// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures.Concurrent
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Threading;

    using Microsoft.ServiceFabric.Replicator;

    /// <summary>
    /// This is a lock-based concurrent skip list.
    /// 
    /// SkipList is a probabilistic logarithmic search data structure. 
    /// It is probabilistic because unlike other search data structures (AVL, RedBlackTree, BTree) it does not balance.
    /// 
    /// SkipList is an ideal data structure to be concurrent because
    /// 1. Low contention: Only predecessors need to be locked for writes.
    /// 2. Minimized memory synchronization: Only predecessor nodes whose links are changed are synchronized.
    /// 
    /// This ConcurrentSkipList provides
    /// 1. Expected O(log n), Lock-free and wait-free ContainsKey and TryGetValue.
    /// 2. Expected O(log n), TryAdd, Update and TryRemove.
    /// 3. Lock-free key enumerations.
    /// </summary>
    /// <typeparam name="TKey">Type of the key.</typeparam>
    /// <typeparam name="TValue">Type if the value.</typeparam>
    /// <devnote>
    /// Notes:
    /// Logical delete is used to avaid double skiplist traversal for adds and removes.
    /// For logical delete, isDeleted mark is used where a node is marked (under lock) to be deleted before it is phsyically removed from the skip list.
    /// 
    /// Logical insert is used since the node needs to be linked into the list one level at a time.
    /// 
    /// Invariants:
    /// List(level1) is a super set of List(level2) iff level1 less than or equal to level2. 
    /// Side effect of this is that nodes are added bottom up and removed top down.
    /// 
    /// Locks: Bottom up. (Unlock order is irrelevant).
    /// </devnote>
    internal class ConcurrentSkipList<TKey, TValue> : ISortedList<TKey, TValue>
    {
        /// <summary>
        /// Invalid level.
        /// </summary>
        internal const int InvalidLevel = -1;

        /// <summary>
        /// Bottom level.
        /// </summary>
        internal const int BottomLevel = 0;

        /// <summary>
        /// Default number of levels
        /// </summary>
        internal const int DefaultNumberOfLevels = 32;

        /// <summary>
        /// Default promotion chance for each level. [0, 1).
        /// </summary>
        private const double DefaultPromotionProbability = 0.5;

        /// <summary>
        /// Number of levels in the skip list.
        /// </summary>
        private readonly int numberOfLevels;

        /// <summary>
        /// Heighest level allowed in the skip list,
        /// </summary>
        private readonly int topLevel;

        /// <summary>
        /// The promotion chance for each level. [0, 1).
        /// </summary>
        private readonly double promotionProbability;

        /// <summary>
        /// Head of the skip list.
        /// </summary>
        private readonly Node head;

        /// <summary>
        /// Key comparer used to order the keys.
        /// </summary>
        private readonly IComparer<TKey> keyComparer;

        /// <summary>
        /// Initializes a new instance of the ConcurrentSkipList class.
        /// </summary>
        /// <param name="keyComparer">The key comparer for the chosen key type.</param>
        public ConcurrentSkipList(IComparer<TKey> keyComparer) : this(keyComparer, DefaultNumberOfLevels, DefaultPromotionProbability)
        {
        }

        /// <summary>
        /// Initializes a new instance of the ConcurrentSkipList class.
        /// </summary>
        /// <param name="keyComparer">The key comparer for the chosen key type.</param>
        /// <param name="numberOfLevels">The number of levels in the skip list.</param>
        /// <param name="promotionProbability">The promotion probability.</param>
        public ConcurrentSkipList(IComparer<TKey> keyComparer, int numberOfLevels, double promotionProbability)
        {
            Utility.Assert(keyComparer != null, "keyComparer cannot be null");
            Utility.Assert(numberOfLevels > 0, "Number of levels cannot be less than or equal to zero.");
            Utility.Assert(promotionProbability > 0, "Promotion probability cannot be less than or equal to zero.");
            Utility.Assert(promotionProbability < 1, "Promotion probability cannot be greater than or equal to one.");

            this.keyComparer = keyComparer;
            this.numberOfLevels = numberOfLevels;
            this.topLevel = numberOfLevels - 1;
            this.promotionProbability = promotionProbability;

            this.head = new Node(Node.NodeType.Head, this.topLevel);
            var tail = new Node(Node.NodeType.Tail, this.topLevel);

            // Always link in bottom up.
            for (int level = 0; level <= this.topLevel; level++)
            {
                this.head.SetNextNode(level, tail);
            }

            this.head.IsInserted = true;
            tail.IsInserted = true;
        }


        /// <summary>
        /// Lock free enumeration. Note that it is READ COMMITTED semantics.
        /// </summary>
        /// <returns>Enumerator that represents the items in the ConcurrentSkipList.</returns>
        public IEnumerator<TKey> GetEnumerator()
        {
            Node current = this.head;
            while (true)
            {
                current = current.GetNextNode(BottomLevel);

                Utility.Assert(current.Type != Node.NodeType.Head, "Current type cannot be head.");

                // If current is tail, this must be the end of the list.
                if (current.Type == Node.NodeType.Tail)
                {
                    yield break;
                }

                // Takes advantage of the fact that next is set before the node is physically linked.
                if (current.IsInserted == false || current.IsDeleted)
                {
                    continue;
                }

                yield return current.Key;
            }
        }

        /// <summary>
        /// Lock free enumeration. Note that it is READ COMMITTED semantics.
        /// </summary>
        /// <returns>Enumerator that represents the items in the ConcurrentSkipList.</returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }

        /// <summary>
        /// Determines whether the ConcurrentSkipList contains the specified key.
        /// </summary>
        /// <param name="key">The key to locate.</param>
        /// <returns>
        /// true if the key exists. Otherwise false.
        /// </returns>
        public bool Contains(TKey key)
        {
            this.ThrowIfKeyIsNull(key);

            var searchResult = this.WeakSearch(key);

            // If node is not found, not logically inserted or logically removed, return false.
            if (searchResult.IsFound && searchResult.GetNodeFound().IsInserted && searchResult.GetNodeFound().IsDeleted == false)
            {
                return true;
            }

            return false;
        }

        /// <summary>
        /// Attempts to get the value associated with the specified key from the ConcurrentSkipList.
        /// </summary>
        /// <param name="key">The key to locate.</param>
        /// <param name="value">The output value.</param>
        /// <returns>
        /// true if the key exists. Otherwise false.
        /// </returns>
        public bool TryGetValue(TKey key, out TValue value)
        {
            this.ThrowIfKeyIsNull(key);

            var searchResult = this.WeakSearch(key);
            if (searchResult.IsFound && searchResult.GetNodeFound().IsInserted && searchResult.GetNodeFound().IsDeleted == false)
            {
                value = searchResult.GetNodeFound().Value;
                return true;
            }

            value = default(TValue);
            return false;
        }

        /// <summary>
        /// Attempts to add the specified key and value to the ConcurrentSkipList.
        /// </summary>
        /// <param name="key">Type of the key.</param>
        /// <param name="value">Type of the value.</param>
        /// <returns>
        /// true if row was added. false if it already exists.
        /// </returns>
        public bool TryAdd(TKey key, TValue value)
        {
            this.ThrowIfKeyIsNull(key);

            int insertLevel = this.GenerateLevel();

            while (true)
            {
                SearchResult searchResult = this.WeakSearch(key);
                if (searchResult.IsFound)
                {
                    if (searchResult.GetNodeFound().IsDeleted)
                    {
                        continue;
                    }

                    // Spin until the duplicate key is logically inserted.
                    this.WaitUntilIsInserted(searchResult.GetNodeFound());
                    return false;
                }

                int highestLevelLocked = InvalidLevel;
                try
                {
                    bool isValid = true;
                    for (int level = 0; isValid && level <= insertLevel; level++)
                    {
                        var predecessor = searchResult.GetPredecessor(level);
                        var successor = searchResult.GetSuccessor(level);

                        predecessor.Lock();
                        highestLevelLocked = level;

                        // If predecessor is locked and the predecessor is still pointing at the successor, successor cannot be deleted.
                        isValid = this.IsValidLevel(predecessor, successor, level);
                    }

                    if (isValid == false)
                    {
                        continue;
                    }

                    // Create the new node and initialize all the next pointers.
                    var newNode = new Node(key, value, insertLevel);
                    for (int level = 0; level <= insertLevel; level++)
                    {
                        newNode.SetNextNode(level, searchResult.GetSuccessor(level));
                    }

                    // MCoskun: Ensure that the node is fully initialized before physical linking starts.
                    Thread.MemoryBarrier();

                    for (int level = 0; level <= insertLevel; level++)
                    {
                        // MCoskun: Note that this is required for correctness.
                        // Remove takes a dependency of the fact that if found at expected level, all the predecessors have already been correctly linked.
                        // Hence we only need to use a MemoryBarrier before linking in the top level. 
                        if (level == insertLevel)
                        {
                            Thread.MemoryBarrier();
                        }

                        searchResult.GetPredecessor(level).SetNextNode(level, newNode);
                    }

                    // Linearization point: MemoryBarrier not required since IsInserted is a volatile member (hence implicitly uses MemoryBarrier). 
                    newNode.IsInserted = true;
                    return true;
                }
                finally
                {
                    // Unlock order is not important.
                    for (int level = highestLevelLocked; level >= 0; level--)
                    {
                        searchResult.GetPredecessor(level).Unlock();
                    }
                }
            }
        }

        /// <summary>
        /// Update the specified key with specified value.
        /// </summary>
        /// <param name="key">Type of the key.</param>
        /// <param name="value">Type of the value.</param>
        /// <devnote>
        /// 1. Lock-free search to find the node.
        /// 2. If the node is not found, not completely inserted or being deleted throw.
        /// 3. Lock the node
        /// 4. If being deleted, Unlock and throw.
        /// 5. Update the node and Unlock.
        /// </devnote>
        public void Update(TKey key, TValue value)
        {
            this.ThrowIfKeyIsNull(key);

            SearchResult searchResult = this.WeakSearch(key);

            Node nodeToBeUpdated = null;
            if (searchResult.IsFound)
            {
                nodeToBeUpdated = searchResult.GetNodeFound();
            }

            if (searchResult.IsFound == false || nodeToBeUpdated.IsInserted == false || nodeToBeUpdated.IsDeleted)
            {
                throw new ArgumentException("key does not exist", "key");
            }

            nodeToBeUpdated = searchResult.GetNodeFound();
            nodeToBeUpdated.Lock();
            try
            {
                if (nodeToBeUpdated.IsDeleted)
                {
                    throw new ArgumentException("key does not exist", "key");
                }

                nodeToBeUpdated.Value = value;
            }
            finally
            {
                nodeToBeUpdated.Unlock();
            }
        }

        /// <summary>
        /// Updates the value for the specified key.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <param name="updateFunction">Update function used to generate the new value.</param>
        /// <remarks>
        /// Unlike Concurrent Dictionary, this operation is atomic and thread-safe: function is executed under the lock. 
        /// </remarks>
        /// <devnote>
        /// 1. Lock-free search to find the node.
        /// 2. If the node is not found, not completely inserted or being deleted throw.
        /// 3. Lock the node
        /// 4. If being deleted, Unlock and throw.
        /// 5. Execute the function and get the new value.
        /// 6. Update the node and Unlock.
        /// </devnote>
        public void Update(TKey key, Func<TKey, TValue, TValue> updateFunction)
        {
            this.ThrowIfKeyIsNull(key);

            if (updateFunction == null)
            {
                throw new ArgumentNullException("null", "updateFunction");
            }

            SearchResult searchResult = this.WeakSearch(key);

            Node nodeToBeUpdated = null;
            if (searchResult.IsFound)
            {
                nodeToBeUpdated = searchResult.GetNodeFound();
            }

            if (searchResult.IsFound == false || nodeToBeUpdated.IsInserted == false || nodeToBeUpdated.IsDeleted)
            {
                throw new ArgumentException("does not exist.", "key");
            }

            nodeToBeUpdated = searchResult.GetNodeFound();
            nodeToBeUpdated.Lock();
            try
            {
                if (nodeToBeUpdated.IsDeleted)
                {
                    throw new ArgumentException("does not exist.", "key");
                }

               var newValue = updateFunction.Invoke(key, nodeToBeUpdated.Value);
               nodeToBeUpdated.Value = newValue;
            }
            finally
            {
                nodeToBeUpdated.Unlock();
            }
        }

        /// <summary>
        /// Attempts to remove the specified key and value to the ConcurrentSkipList.
        /// </summary>
        /// <param name="key">The key.</param>
        /// <returns>
        /// True if the row was removed. 
        /// False if it did not already exist.
        /// </returns>
        /// <devnote>
        /// Phases:
        /// 1. WeakSearch the node
        /// 2. If not found, return false
        /// 3. If not already marked, Lock the node and mark is as IsDeleted
        /// 4. Lock the predecessors and verify the weak search is stable. If not goto 1.
        /// 5. Physically remove the node.
        /// </devnote>
        public bool TryRemove(TKey key)
        {
            this.ThrowIfKeyIsNull(key);

            // Node to be deleted (if it exists).
            Node nodeToBeDeleted = null;
            
            // Indicates whether the node to be deleted is already locked and logically marked as deleted.
            bool isLogicallyDeleted = false;

            // Level at which the to be deleted node was found.
            int topLevel = InvalidLevel;

            while (true)
            {
                SearchResult searchResult = this.WeakSearch(key);

                if (searchResult.IsFound)
                {
                    nodeToBeDeleted = searchResult.GetNodeFound();
                }

                // MCoskun: Why do we have a top level check?
                // A node gets physically linked from level 0 up to its top level.
                // Top level check ensures that we found the node at the level we were expecting it.
                // Due to the lock-free nature of weak-search, there is a race between predecessors being physically linked to the node and node being marked logically inserted.
                // It is possible for the weak search to snap predecessors in between the above points, then node gets marked as logically inserted and then the IsInserted below happening.
                if (!isLogicallyDeleted && (searchResult.IsFound == false || nodeToBeDeleted.IsInserted == false || nodeToBeDeleted.TopLevel != searchResult.LevelFound || nodeToBeDeleted.IsDeleted))
                {
                    return false;
                }

                // If not already logically removed, lock it and mark it.
                if (!isLogicallyDeleted)
                {
                    topLevel = searchResult.LevelFound;
                    nodeToBeDeleted.Lock();
                    if (nodeToBeDeleted.IsDeleted)
                    {
                        nodeToBeDeleted.Unlock();
                        return false;
                    }

                    // Linearization point: IsDeleted is volatile.
                    nodeToBeDeleted.IsDeleted = true;
                    isLogicallyDeleted = true;
                }

                int highestLevelLocked = InvalidLevel;
                try
                {
                    bool isValid = true;
                    for (int level = 0; isValid && level <= topLevel; level++)
                    {
                        var predecessor = searchResult.GetPredecessor(level);
                        predecessor.Lock();
                        highestLevelLocked = level;
                        isValid = predecessor.IsDeleted == false && predecessor.GetNextNode(level) == nodeToBeDeleted;
                    }

                    if (isValid == false)
                    {
                        continue;
                    }

                    // MCoskun: To preserve the invariant that lower levels are super-set of higher levels, always unlink top to bottom.
                    // Memory Barrier could have been used to guarantee above but the node is already under lock
                    for (int level = topLevel; level >= 0; level--)
                    {
                        var predecessor = searchResult.GetPredecessor(level);
                        var newLink = nodeToBeDeleted.GetNextNode(level);

                        predecessor.SetNextNode(level, newLink);
                    }

                    // This requires that not between the nodeToBeDeleted.Lock and this throws.
                    nodeToBeDeleted.Unlock();

#if DEBUG
                    // This is an expensive assert that ensures that the node is physically removed from the skip list.
                    var temp = this.WeakSearch(key);
                    Utility.Assert(temp.IsFound == false, "LevelFound: {0} Key: {1}", temp.LevelFound, key.ToString());
#endif

                    return true;
                }
                finally
                {
                    for (int level = highestLevelLocked; level >= 0; level--)
                    {
                        searchResult.GetPredecessor(level).Unlock();
                    }
                }
            }
        }

        /// <summary>
        /// To be used for testing purposes only.
        /// </summary>
        /// <returns></returns>
        internal int GetCount(int level)
        {
            int count = 0;
            Node current = this.head;
            while (true)
            {
                current = current.GetNextNode(level);

                Utility.Assert(current.Type != Node.NodeType.Head, "Current type cannot be head.");

                // If current is tail, this must be the end of the list.
                if (current.Type == Node.NodeType.Tail)
                {
                    return count;
                }

                count++;
            }
        }

        /// <summary>
        /// To be used for testing purposes only.
        /// </summary>
        /// <returns></returns>
        internal void Verify()
        {
            // Verify the invariant that lower level lists are always super-set of higher-level lists.
            for (int i = BottomLevel; i < this.topLevel; i++)
            {
                var lowLevelCount = this.GetCount(i);
                var highLevelCount = this.GetCount(i + 1);

                Utility.Assert(lowLevelCount >= highLevelCount, "Verify the invariant that lower level lists are always super-set of higher-level lists. LLC: {0} HLC: {1}", lowLevelCount, highLevelCount);
            }
        }

        private void ThrowIfKeyIsNull(TKey key)
        {
            if (key == null)
            {
                throw new ArgumentNullException("null", "key");
            }
        }

        /// <summary>
        /// Spins until the node is marked as logically inserted: physically inserted at every level.
        /// </summary>
        /// <param name="node">Node to ensure logical insertion.</param>
        private void WaitUntilIsInserted(Node node)
        {
            while (node.IsInserted == false)
            {
            }
        }

        /// <summary>
        /// Probability of picking a given level: P(L) = p ^ -(L + 1) where p = PromotionChance.
        /// </summary>
        /// <returns>The level.</returns>
        private int GenerateLevel()
        {
            int level = 0;
            while (level < this.topLevel)
            {
                double divineChoice = RandomGenerator.NextDouble();
                if (divineChoice <= this.promotionProbability)
                {
                    level++;
                }
                else
                {
                    break;
                }
            }

#if DEBUG
            Utility.Assert(level > InvalidLevel && level <= this.topLevel, "Level generated is out of range. Level: {0} InvalidLevel: {1} MaxLevel: {2}", level, InvalidLevel, this.topLevel);
#endif
            return level;
        }

        /// <summary>
        /// Lock-free search for the Node with the specific key.
        /// </summary>
        /// <param name="key">The search key.</param>
        /// <returns>
        /// SearchResult.IsFound indicates whether the node was found.
        /// If so, predecessors and successors are set accordingly.
        /// 
        /// If not, predecessors are the nodes at each layer that would have been its predeccessor if the node existed.
        /// Same is true for the successors.
        /// </returns>
        private SearchResult WeakSearch(TKey key)
        {
            int levelFound = InvalidLevel;
            Node[] predecessorArray = new Node[this.numberOfLevels];
            Node[] successorArray = new Node[this.numberOfLevels];

            Node predecessor = this.head;
            for (int level = this.topLevel; level >= 0; level--)
            {
                // TODO: Is MemoryBarrier required?
                Node current = predecessor.GetNextNode(level);

                while (true)
                {
                    int nodeComparison = this.Compare(current, key);
                    if (levelFound == InvalidLevel && nodeComparison == 0)
                    {
                        levelFound = level;
                    }

                    // current is >= searchKey, search at this level is over.
                    if (nodeComparison >= 0)
                    {
                        predecessorArray[level] = predecessor;
                        successorArray[level] = current;

                        break;
                    }

                    predecessor = current;
                    current = predecessor.GetNextNode(level);
                }
            }

#if DEBUG
            Utility.Assert(levelFound >= InvalidLevel && levelFound <= this.topLevel, "level found is out of range. LevelFound: {0} InvalidLevel: {1} MaxLevel: {2}", levelFound, InvalidLevel, this.topLevel);
#endif
            return new SearchResult(levelFound, predecessorArray, successorArray);
        }

        private int Compare(Node node, TKey key)
        {
            if (node.Type == Node.NodeType.Head)
            {
                return -1;
            }

            if (node.Type == Node.NodeType.Tail)
            {
                return 1;
            }

            return this.keyComparer.Compare(node.Key, key);
        }

        private bool IsValidLevel(Node predecessor, Node successor, int level)
        {
            Utility.Assert(predecessor != null, "Predecessor cannot be null");
            Utility.Assert(successor != null, "Successor cannot be null");

            return predecessor.IsDeleted == false && successor.IsDeleted == false && predecessor.GetNextNode(level) == successor;
        }

        /// <summary>
        /// SkipList node.
        /// </summary>
        private class Node
        {
            public enum NodeType : byte
            {
                Head = 0,
                Tail = 1,
                Data = 2
            }

            // Monitor is used as a re-entrant spin-lock.
            // Spin-lock is prefered since these are latches: held for a short duration.
            private readonly object nodeLock;

            private readonly Node[] nextNodeArray;

            private readonly NodeType nodeType;

            /// <summary>
            /// The key for the node.
            /// </summary>
            private readonly TKey key;

            // The value for the node.
            private TValue value;

            /// <summary>
            /// Indicates whether the node has been logically inserted: has been physically inserted at each level.
            /// </summary>
            /// <devnote>This must be volatile or Thread.MemoryBarrier should be used.</devnote>
            private volatile bool isInserted;

            /// <summary>
            /// Indicates whether the node is logically removed.
            /// </summary>
            /// <devnote>This must be volatile or Thread.MemoryBarrier should be used.</devnote>
            private volatile bool isDeleted;

            /// <summary>
            /// Initializes a new instance of the Node class.
            /// </summary>
            /// <param name="nodeType">Type of the node.</param>
            /// <param name="height">Level of the node.</param>
            public Node(NodeType nodeType, int height)
            {
                this.nodeType = nodeType;

                this.key = default(TKey);
                this.value = default(TValue);

                this.nodeLock = new object();

                this.isInserted = false;
                this.isDeleted = false;
                this.nextNodeArray = new Node[height + 1];
            }

            public Node(TKey key, TValue value, int height)
            {
                this.nodeType = NodeType.Data;

                this.key = key;
                this.value = value;

                this.nodeLock = new object();

                this.isInserted = false;
                this.isDeleted = false;
                this.nextNodeArray = new Node[height + 1];
            }

            public TKey Key
            {
                get
                {
#if DEBUG
                    Utility.Assert(this.nodeType == NodeType.Data, "Head and tail nodes do not have keys. NodeType: {0}", (byte)this.nodeType);
#endif 
                    return this.key;
                }
            }

            public TValue Value
            {
                get
                {
                    Utility.Assert(this.nodeType == NodeType.Data, "Head and tail nodes do not have values. NodeType: {0}", (byte)this.nodeType);
                    return this.value;
                }
                set
                {
                    Utility.Assert(this.nodeType == NodeType.Data, "Head and tail nodes do not have values. NodeType: {0}", (byte)this.nodeType);
                    Utility.Assert(Monitor.IsEntered(this.nodeLock), "Cannot mark the node as deleted, if the lock is not held.");
                    this.value = value;
                }
            }

            public NodeType Type
            {
                get
                {
                    return this.nodeType;
                }
            }

            /// <summary>
            /// Indicates whether the node is logically inserted: inserted physically at each level.
            /// </summary>
            public bool IsInserted
            {
                get
                {
                    return this.isInserted;
                }
                set
                {
#if DEBUG
                    if (this.nodeType == NodeType.Data)
                    {
                        Utility.Assert(value, "IsInserted can only be set to true. Key: {0}", this.key.GetHashCode());
                        Utility.Assert(this.isInserted == false, "A node cannot be inserted twice. Key: {0}", this.key.GetHashCode());
                    }
                    else
                    {
                        Utility.Assert(value, "IsInserted can only be set to true. Type: {0}", this.nodeType.ToString());
                        Utility.Assert(this.isInserted == false, "A node cannot be inserted twice. Type: {0}", this.nodeType.ToString());
                    }
#endif

                    this.isInserted = value;
                }
            }

            /// <summary>
            /// Indicates whether the node is logically deleted: may or may not have been physically removed at each level.
            /// </summary>
            /// <devnote>
            /// For set, CALLER HOLDS LOCK!
            /// </devnote>
            public bool IsDeleted
            {
                get
                {
                    return this.isDeleted;
                }
                set
                {
#if DEBUG
                    Utility.Assert(Monitor.IsEntered(this.nodeLock), "Cannot mark the node as deleted, if the lock is not held.");
                    Utility.Assert(this.Type == NodeType.Data, "Only data type nodes can be deleted.", (byte)this.nodeType);
                    Utility.Assert(this.isInserted, "A node that is not inserted, cannot be removed. Key: {0}", this.key.GetHashCode());
                    Utility.Assert(this.isDeleted == false, "Node has already been deleted. Key: {0}", this.key.GetHashCode());
#endif
                    this.isDeleted = value;
                }
            }

            public int TopLevel
            {
                get
                {
#if DEBUG
                    Utility.Assert(this.nextNodeArray != null, "this.nextNodeArray cannot be null");
#endif

                    return this.nextNodeArray.Length - 1;
                }
            }

            public Node GetNextNode(int height)
            {
                return this.nextNodeArray[height];
            }

            public void SetNextNode(int height, Node next)
            {
                this.nextNodeArray[height] = next;
            }

            public void Lock()
            {
                Monitor.Enter(this.nodeLock);
            }

            public void Unlock()
            {
                Monitor.Exit(this.nodeLock);
            }
        }

        private class SearchResult
        {
            private readonly int levelFound;
            private readonly Node[] predecessorArray;
            private readonly Node[] successorArray;

            public SearchResult(int levelFound, Node[] predecessorArray, Node[] successorArray)
            {
#if DEBUG
                Utility.Assert(predecessorArray != null, "predecessor array cannot be null");
                Utility.Assert(successorArray != null, "successorArray array cannot be null");
#endif

                this.levelFound = levelFound;
                this.predecessorArray = predecessorArray;
                this.successorArray = successorArray;
            }

            public int LevelFound
            {
                get
                {
                    return this.levelFound;
                }
            }

            public bool IsFound
            {
                get
                {
                    return this.levelFound != InvalidLevel;
                }
            }

            public Node GetPredecessor(int level)
            {
                Utility.Assert(this.predecessorArray != null, "predecessor array cannot be null");

                return this.predecessorArray[level];
            }

            public Node GetSuccessor(int level)
            {
                Utility.Assert(this.successorArray != null, "predecessor array cannot be null");

                return this.successorArray[level];
            }

            public Node GetNodeFound()
            {
#if DEBUG
                Utility.Assert(this.successorArray != null, "predecessor array cannot be null");
                Utility.Assert(this.IsFound, "item must have been found.");
#endif

                return this.successorArray[this.levelFound];
            }
        }
    }
}