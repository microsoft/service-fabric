// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections.ReliableConcurrentQueue
{
    using System;
    using System.Collections.Concurrent;
    using System.Collections.Generic;
    using System.Fabric;
    using System.Fabric.Common.Tracing;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Replicator;
    using Transaction = Microsoft.ServiceFabric.Replicator.Transaction;

    internal enum DequeuerState : short
    {
        /// <summary>
        /// Unknown state, Never became Primary
        /// </summary>
        None = 0,

        /// <summary>
        /// Currently on the primary. The dequeue list should be used only it is in this state
        /// </summary>
        OnPrimary = 1,

        /// <summary>
        /// Used by <see cref="IStateProvider2.ChangeRoleAsync"/> when we move away from Primary
        /// </summary>
        ChangeRoleFromPrimary = 2,

        /// <summary>
        /// Used by <see cref="IStateProvider2.CloseAsync"/>
        /// </summary>
        Closing = 3,

        /// <summary>
        /// Used by <see cref="IStateProvider2.Abort"/>
        /// </summary>
        Aborting = 4
    }

    /// <summary>
    /// Contains the Dictionary{id, ListElement} for storing and looking up list elements, and manages the linked list of list
    /// elements (storing head/tail, offering link/unlink operations on the list).
    /// </summary> 
    internal class DataStore<T>
    {
        private readonly object updateLatch;

        private ListElement head;

        private ListElement tail;

        private ConcurrentQueue<TaskCompletionSource<ConditionalValue<IListElement<T>>>> dequeuerQueue;

        private DequeuerState dequeuersState;

        private readonly TimeSpan DefaultTransactionStatePollingTimeout = TimeSpan.FromSeconds(4);

        private readonly string traceType;

        private readonly RCQMode queueMode;

        public IListElement<T> Head
        {
            get
            {
                return this.head;
            }
        }

        public IListElement<T> Tail
        {
            get
            {
                return this.tail;
            }
        }

        public int LinkedCount { get; private set; }

        // Iterates through the listElements to find the highest Id
        public long HighestId
        {
            get
            {
                if (this.ListElements.IsEmpty)
                {
                    return long.MinValue;
                }

                var max = long.MinValue;
                foreach (var listElement in this.ListElements.Values)
                {
                    if (((IListElement<T>)listElement).Id > max)
                    {
                        max = ((IListElement<T>)listElement).Id;
                    }
                }
                return max;
            }
        }

        public IEnumerable<IListElement<T>> Items
        {
            get
            {
                return this.ListElements.Values;
            }
        }

        public int ItemsCount
        {
            get
            {
                return this.ListElements.Count;
            }
        }

        private ConcurrentDictionary<long, ListElement> ListElements { get; set; }

        public DataStore(RCQMode rcqMode, string traceType)
            : this(new List<IListElement<T>>(), rcqMode, traceType)
        {
        }

        public DataStore(List<IListElement<T>> listElements, RCQMode rcqMode, string traceType)
        {
            this.queueMode = rcqMode;
            this.traceType = traceType;
            this.dequeuerQueue = new ConcurrentQueue<TaskCompletionSource<ConditionalValue<IListElement<T>>>>();
            this.dequeuersState = DequeuerState.None;
            this.LinkedCount = 0;
            this.updateLatch = new object();
            
            this.ListElements = new ConcurrentDictionary<long, ListElement>();
            this.head = null;
            this.tail = null;

            // descending order
            listElements.Sort((a, b) => b.Id.CompareTo(a.Id));
            foreach (var listElement in listElements)
            {
                this.AddListElement(listElement);
                this.AssertUnlinked(listElement);
                this.LinkHeadUnsafe(listElement);
            }
        }

        /// <summary>
        /// Try and dequeue from the DataStore.
        /// If there are dequeuers waiting or there is no element to dequeue, this dequeueur would be added to the waiting queue.
        /// Else, the result would be set for the <paramref name="tcs"/>
        /// Caller must await on tcs.Task
        /// </summary>
        public void TryDequeue(
            Transaction txn,
            TaskCompletionSource<ConditionalValue<IListElement<T>>> tcs,
            TimeSpan timeout,
            CancellationToken cancellationToken)
        {
            lock (this.updateLatch)
            {
                switch (this.dequeuersState)
                {
                    case DequeuerState.Aborting:
                    case DequeuerState.Closing:
                        tcs.TrySetException(
                            new FabricObjectClosedException(
                                string.Format("Queue is closed. Please dispose the current transaction and retry the operation with a new transaction, Cause : {0}", this.dequeuersState)));
                        return;

                    case DequeuerState.None:
                    case DequeuerState.ChangeRoleFromPrimary:
                        tcs.TrySetException(
                            new FabricNotPrimaryException(
                                string.Format("Current role not Primary. Please dispose the current transaction and retry the operation with a new transaction, Cause : {0}", this.dequeuersState)));
                        return;

                    case DequeuerState.OnPrimary:
                        // In the expected state, continue DataStore.TryDequeue
                        break;

                    default:
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "TryDequeue",
                            "DataStore.TryDequeue : Trying to add a dequeuer in an invalid state. Current DequeuerState = {0}",
                            this.dequeuersState);
                        break; // unreachable
                }

                var res = this.TryUnlinkHead();

                if (res.HasValue)
                {
                    // There is something to unlink and no waiting dequeuer
                    if (!tcs.TrySetResult(res))
                    {
                        TestableAssertHelper.FailInvalidOperation(
                            this.traceType,
                            "TryDequeue",
                            "Could not set the tcs result after unlinking from the DataStore. TCS {0}",
                            res);
                    }
                }
                else
                {
                    this.AddDequeuer(txn, tcs, timeout, cancellationToken);
                }
            }
        }

        public void Close()
        {
            // noop the call for a NonBlocking Queue
            if (this.queueMode == RCQMode.Blocking)
            {
                this.DrainDequeuers(DequeuerState.Closing);
            }
        }

        public void Abort()
        {
            // noop the call for a NonBlocking Queue
            if (this.queueMode == RCQMode.Blocking)
            {
                this.DrainDequeuers(DequeuerState.Aborting);
            }
        }

        public void ChangeRole(ReplicaRole currentRole, ReplicaRole newRole)
        {
            // noop the call for a NonBlocking Queue
            if (this.queueMode == RCQMode.Blocking)
            {
                if (currentRole == ReplicaRole.Primary)
                {
                    this.DrainDequeuers(DequeuerState.ChangeRoleFromPrimary);
                }

                if (newRole == ReplicaRole.Primary)
                {
                    this.AssertNoDequeuer();
                    this.dequeuersState = DequeuerState.OnPrimary;
                }
            }
        }

        /// <summary>
        /// Drain all the dequeuers.
        /// Would be called during ChangeRole (to S, None), Abort, Close.
        /// </summary>
        /// <param name="cause">The DrainCause</param>
        private void DrainDequeuers(DequeuerState cause)
        {
            TaskCompletionSource<ConditionalValue<IListElement<T>>> tcs;
            Exception exc = null;

            switch (cause)
            {
                case DequeuerState.Aborting:
                case DequeuerState.Closing:
                    exc = new FabricObjectClosedException(string.Format("Dequeue was canceled as the queue is {0}. Please dispose the current transaction and retry the operation with a new transaction.", cause));
                    break;

                case DequeuerState.ChangeRoleFromPrimary:
                    exc = new FabricNotPrimaryException("Dequeue was canceled as the role is no longer primary. Please dispose the current transaction and retry the operation with a new transaction.");
                    break;

                default:
                    TestableAssertHelper.FailInvalidData(
                        this.traceType,
                        "DataStore.DrainDequeuers",
                        "DequeueScheduler.DrainDequeuers : Invalid DrainQueue cause : {0}",
                        cause);
                    break; // unreachable
            }

            lock (this.updateLatch)
            {
                this.dequeuersState = cause;
            }

            while (this.dequeuerQueue.TryDequeue(out tcs))
            {
                tcs.TrySetException(exc);
            }
        }

        /// <summary>
        /// Assert that there are no dequeuers waiting currently
        /// </summary>
        private void AssertNoDequeuer()
        {
            if (!this.dequeuerQueue.IsEmpty)
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "DataStore.AssertNoDequeuer",
                    "DataStore.AssertNoDequeuers : Waiting Dequeuer Queue is not empty. Count = {0}",
                    this.dequeuerQueue.Count);
            }
        }

        public ConditionalValue<IListElement<T>> TryUnlinkHead()
        {
            lock (this.updateLatch)
            {
                if (this.Head == null)
                {
                    return new ConditionalValue<IListElement<T>>();
                }

                var oldHead = this.Head;
                this.UnlinkUnsafe(oldHead);
                return new ConditionalValue<IListElement<T>>(true, oldHead);
            }
        }

        public void Unlink(IListElement<T> listElement)
        {
            lock (this.updateLatch)
            {
                this.UnlinkUnsafe(listElement);
            }
        }

        public void LinkHead(IListElement<T> listElement)
        {
            this.AssertUnlinked(listElement);

            lock (this.updateLatch)
            {
                switch (this.queueMode)
                {
                    // IReliableConcurrentQueue
                    case RCQMode.NonBlocking:
                    {
                        this.LinkHeadUnsafe(listElement);
                        break;
                    }

                    // IReliableConcurrentBlockingQueue
                    case RCQMode.Blocking:
                    {
                        bool waitingDeq = false;

                        if (this.dequeuersState == DequeuerState.OnPrimary)
                        {
                            waitingDeq = this.TryNotifyDequeuer(listElement);
                        }
                        else
                        {
                            this.AssertNoDequeuer();
                        }

                        if (!waitingDeq)
                        {
                            // If there was no dequeuer waiting, link the element
                            this.LinkHeadUnsafe(listElement);
                        }
                        break;
                    }

                    default:
                        TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkHead",
                            "Invalid QueueMode : {0}", this.queueMode);
                        break;
                }
            }
        }

        public void LinkTail(IListElement<T> listElement)
        {
            this.AssertUnlinked(listElement);

            lock (this.updateLatch)
            {
                switch (this.queueMode)
                {
                    // IReliableConcurrentQueue
                    case RCQMode.NonBlocking:
                    {
                        this.LinkTailUnsafe(listElement);
                        break;
                    }

                    // IReliableConcurrentBlockingQueue
                    case RCQMode.Blocking:
                    {
                        bool waitingDeq = false;

                        if (this.dequeuersState == DequeuerState.OnPrimary)
                        {
                            waitingDeq = this.TryNotifyDequeuer(listElement);
                        }
                        else
                        {
                            this.AssertNoDequeuer();
                        }

                        if (!waitingDeq)
                        {
                            // If there was no dequeuer waiting, link the element
                            this.LinkTailUnsafe(listElement);
                        }
                        break;
                    }

                    default:
                        TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkTail",
                            "Invalid QueueMode : {0}", this.queueMode);
                        break;
                }
            }
        }

        public void RemoveListElement(IListElement<T> listElement)
        {
            this.AssertUnlinked(listElement);

            ListElement removed;
            var res = this.ListElements.TryRemove(listElement.Id, out removed);
            if (!res)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.RemoveListElement", "res");
            }
            if (removed != (ListElement)listElement)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.RemoveListElement", "removed == (ReliableQueueListElement)listElement");
            }
        }

        public bool HasListElement(long listElementId)
        {
            return this.ListElements.ContainsKey(listElementId);
        }

        public bool HasListElement(IListElement<T> listElement)
        {
            return this.HasListElement(listElement.Id);
        }

        public void AddListElement(IListElement<T> listElement)
        {
            if (this.HasListElement(listElement))
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.AddListElement", "!this.HasListElement(listElement)");
            }
            this.ListElements[listElement.Id] = (ListElement)listElement;
        }

        public IListElement<T> GetListElement(long listElementId)
        {
            if (!this.HasListElement(listElementId))
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.GetListElement", "this.HasListElement(listElementId)");
            }
            return this.ListElements[listElementId];
        }

        private bool IsHead(IListElement<T> listElement)
        {
            if (this.Head != listElement)
            {
                return false;
            }

            if (listElement.Previous != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsHead", "Head must have null Previous pointer");
            }

            if ((listElement == this.Tail) != (listElement.Next == null))
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsHead", "If the listElement is also the tail, it must not have a valid next pointer.  Otherwise, it must have a valid next pointer.");
            }

            return true;
        }

        private bool IsTail(IListElement<T> listElement)
        {
            if (this.Tail != listElement)
            {
                return false;
            }

            if (listElement.Next != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsTail", "Tail must have null Next pointer");
            }

            if ((this.Head == listElement) != (listElement.Previous == null))
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsTail", "If the listElement is also the head, it must not have a valid previous pointer.  Otherwise, it must have a valid previous pointer.");
            }

            return true;
        }

        public bool IsLinked(IListElement<T> listElement)
        {
            lock (this.updateLatch)
            {
                return this.IsLinkedUnsafe(listElement);
            }
        }

        private bool IsLinkedUnsafe(IListElement<T> listElement)
        {
            // The order of the following checks is significant

            // is this listElement in the dictionary at all
            if (!this.HasListElement(listElement))
            {
                if (listElement.Next != null)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsLinkedUnsafe", "sanity: If this listElement is not in the dictionary, it must not have a valid next pointer.");
                }
                if (listElement.Previous != null)
                {
                    TestableAssertHelper.FailInvalidOperation(
                        this.traceType,
                        "DataStore.IsLinkedUnsafe",
                        "sanity: If this listElement is not in the dictionary, it must not have a valid previous pointer.");
                }
                return false;
            }

            // is it linked at the head or the tail
            if (this.IsHead(listElement) || this.IsTail(listElement))
            {
                return true;
            }

            // is it not linked at all
            if (listElement.Next == null && listElement.Previous == null)
            {
                return false;
            }

            // is it linked in the middle
            if (listElement.Next != null && listElement.Previous != null)
            {
                if (!(this.HasListElement(listElement.Next)))
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsLinkedUnsafe", "sanity: listElement.Next must be in the dictionary.");
                }
                if (listElement.Next.Previous != listElement)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsLinkedUnsafe", "sanity: listElement.Next.Previous must be listElement.");
                }
                if (!(this.HasListElement(listElement.Previous)))
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsLinkedUnsafe", "sanity: listElement.Previous must be in the dictionary.");
                }
                if (listElement.Previous.Next != listElement)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.IsLinkedUnsafe", "sanity: listElement.Previous.Next must be listElement.");
                }
                return true;
            }

            // it is partially linked and thus incorrect
            TestableAssertHelper.FailInvalidOperation(
                this.traceType,
                "DataStore.IsLinkedUnsafe",
                "Invalid listElement state: <id: {0}, next: {1}, previous: {2}, state: {3}>",
                listElement.Id,
                listElement.Next != null ? listElement.Next.Id : ListElement.ForbiddenId,
                listElement.Previous != null ? listElement.Previous.Id : ListElement.ForbiddenId,
                listElement.State);

            throw new InvalidOperationException("Invalid listElement state: " + listElement); // unreachable
        }

        public void AssertUnlinked(IListElement<T> listElement)
        {
            if (listElement == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "DataStore.AssertUnlinked", "listElement");
            }

            if (this.IsLinked(listElement))
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "DataStore.AssertUnlinked",
                    "listElement <id: {0}, next: {1}, previous: {2}, state: {3}> is linked.",
                    listElement.Id,
                    listElement.Next != null ? listElement.Next.Id : ListElement.ForbiddenId,
                    listElement.Previous != null ? listElement.Previous.Id : ListElement.ForbiddenId,
                    listElement.State);
            }
        }

        public void AssertLinked(IListElement<T> listElement)
        {
            if (listElement == null)
            {
                TestableAssertHelper.FailArgumentNull(this.traceType, "DataStore.AssertLinked", "listElement");
            }

            if (!this.IsLinked(listElement))
            {
                TestableAssertHelper.FailInvalidOperation(
                    this.traceType,
                    "DataStore.AssertLinked",
                    "listElement <id: {0}, next: {1}, previous: {2}, state: {3}> is not linked.",
                    listElement.Id,
                    listElement.Next != null ? listElement.Next.Id : ListElement.ForbiddenId,
                    listElement.Previous != null ? listElement.Previous.Id : ListElement.ForbiddenId,
                    listElement.State);
            }
        }

        public static IListElement<T> CreateQueueListElement(long id, T value, string traceType, ListElementState state = ListElementState.Invalid)
        {
            return new ListElement(id, value, traceType, state);
        }

        /// <summary>
        /// Unlink a listElement from the list.  Must be called under the update lock.
        /// </summary>
        /// <param name="listElement"></param>
        private void UnlinkUnsafe(IListElement<T> listElement)
        {
            if (listElement == null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "listElement != null");
            }
            if (this.LinkedCount == 0)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "this.LinkedCount != 0");
            }
            if (this.Head == null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "this.Head != null");
            }
            if (this.Head.Previous != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "this.Head.Previous == null");
            }
            if (this.Tail == null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "this.Tail != null");
            }
            if (this.Tail.Next != null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "this.Tail.Next == null");
            }
            if (this.LinkedCount != 1 && listElement.Next == null && listElement.Previous == null)
            {
                TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "!(this.LinkedCount != 1 && listElement.Next == null && listElement.Previous == null)");
            }

            var queueListElement = (ListElement)listElement;

            if (listElement.Previous == null)
            {
                if (listElement != this.Head)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "listElement == this.Head");
                }
                this.head = this.head.Next;
            }
            else
            {
                queueListElement.Previous.Next = queueListElement.Next;
            }

            if (listElement.Next == null)
            {
                if (listElement != this.Tail)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.UnlinkUnsafe", "listElement == this.Tail");
                }
                this.tail = queueListElement.Previous;
            }
            else
            {
                queueListElement.Next.Previous = queueListElement.Previous;
            }

            queueListElement.Previous = null;
            queueListElement.Next = null;

            this.LinkedCount--;
        }

        /// <summary>
        /// <paramref name="listElement"/> must be unlinked, Caller must Assert on this
        /// </summary>
        private void LinkHeadUnsafe(IListElement<T> listElement)
        {
            var queueListElement = (ListElement)listElement;

            if (this.Head == null)
            {
                if (this.Tail != null)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkHeadUnsafe", "this.Tail == null");
                }
                if (this.LinkedCount != 0)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkHeadUnsafe", "this.LinkedCount == 0");
                }

                this.head = queueListElement;
                this.tail = queueListElement;
            }
            else
            {
                if (this.Tail == null)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkHeadUnsafe", "this.Tail != null");
                }
                if (this.LinkedCount == 0)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkHeadUnsafe", "this.LinkedCount != 0");
                }

                this.head.Previous = queueListElement;
                queueListElement.Next = this.head;
                this.head = queueListElement;
            }

            this.LinkedCount++;
        }

        /// <summary>
        /// Should be called under the update latch
        /// <paramref name="listElement"/> must be unlinked, Caller must Assert on this
        /// </summary>
        /// <param name="listElement"></param>
        private void LinkTailUnsafe(IListElement<T> listElement)
        {
            var queueListElement = (ListElement)listElement;

            if (this.Tail == null)
            {
                if (this.Head != null)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkTailUnsafe", "this.Head == null");
                }

                if (this.LinkedCount != 0)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkTailUnsafe", "this.LinkedCount == 0");
                }

                this.head = queueListElement;
                this.tail = queueListElement;
            }
            else
            {
                if (this.Head == null)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkTailUnsafe", "this.Head != null");
                }
                if (this.LinkedCount == 0)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.LinkTailUnsafe", "this.LinkedCount != 0");
                }

                this.tail.Next = queueListElement;
                queueListElement.Previous = this.tail;
                this.tail = queueListElement;
            }

            this.LinkedCount++;
        }

        /// <summary>
        /// Start a timer and add the <paramref name="tcs"/> to the dequeuerQueue
        /// Should be called under the updateLatch
        /// </summary>
        private void AddDequeuer(
            Transaction txn,
            TaskCompletionSource<ConditionalValue<IListElement<T>>> tcs,
            TimeSpan timeSpan,
            CancellationToken cancellationToken)
        {
            this.dequeuerQueue.Enqueue(tcs);

            // Fire the timer task
            var t1 = this.StartDequeueTimerAsync(tcs, timeSpan, cancellationToken);
            var t2 = this.PollTransactionStateAsync(txn, tcs, cancellationToken).IgnoreException();
        }

        /// <summary>
        /// Start the timer for dequeue. It is not expected for this task to be awaited.
        /// If the timer expires, set the result for <paramref name="tcs"/> as false.
        /// </summary>
        /// <param name="tcs">The TaskCompletionSource for which the timer is being tracked</param>
        /// <param name="timeSpan">Timespan for the timer</param>
        /// <param name="cancellationToken">RunAsync CancellationToken</param>
        /// <returns>
        /// Task representing the timer.
        /// </returns>
        private async Task StartDequeueTimerAsync(
            TaskCompletionSource<ConditionalValue<IListElement<T>>> tcs,
            TimeSpan timeSpan,
            CancellationToken cancellationToken)
        {
            try
            {
                await Task.Delay(timeSpan, cancellationToken).ConfigureAwait(false);
            }
            catch (Exception e)
            {
                tcs.TrySetException(e);
                return;
            }

            // If the timer expires, set the conditional value to false.
            tcs.TrySetResult(new ConditionalValue<IListElement<T>>(false, null));
        }

        /// <summary>
        /// Parallel task that checks if the transaction has been aborted internally
        /// If so, cause the waiting dequeuer to throw InvalidOperationException
        /// </summary>
        private async Task PollTransactionStateAsync(
            Transaction txn,
            TaskCompletionSource<ConditionalValue<IListElement<T>>> tcs,
            CancellationToken cancellationToken)
        {
            while (true)
            {
                // Ignore the exception thrown by this call
                // As this polls every 4 secs, The cancellationTokenSource could have been disposed and set to null 
                //    causing Task.Delay to throw ObjectDisposedException.
                // The token would be honoured by the StartTimerAsync task and the tcs would be set to the TaskCanceledException.
                await Task.Delay(this.DefaultTransactionStatePollingTimeout, cancellationToken).ConfigureAwait(false);

                if (tcs.Task.IsCompleted)
                {
                    // If the tcs was already set, no need to wait check the transaction state.
                    // End transaction polling task
                    return;
                }

                switch (txn.State)
                {
                    case TransactionState.Active:
                    case TransactionState.Reading:
                        // Do nothing, the transaction is still active, go back to sleep
                        break;

                    case TransactionState.Aborted:
                    case TransactionState.Aborting:
                    case TransactionState.Faulted:
                        tcs.TrySetException(
                            new InvalidOperationException(
                                string.Format("DataStore.PollTransactionStateAsync : The transaction was aborted or faulted, dispose the transaction and try again, Txn : {0}", txn)));
                        return;

                    case TransactionState.Committed:
                    case TransactionState.Committing:
                        var exc =
                            new InvalidOperationException(
                                string.Format(
                                    "DataStore.PollTransactionStateAsync : The transaction was commited but DequeueAsync did not finish. Make sure DequeueAsync was awaited. Txn : {0}",
                                    txn));

                        if (tcs.TrySetException(exc))
                        {
                            // If we were able to set the result, then DequeueAsync was not awaited.
                            // Do not Assert as this is something that the user is doing incorrectly.
                            // Do not throw as the exception would be thrown by the tcs.
                            FabricEvents.Events.ReliableConcurrentQueue_ExceptionWarning(this.traceType, exc.ToString());
                        }

                        return;

                    default:
                        // Unknown transaction state : Assert
                        TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.PollTransactionStateAsync", "Unknown transaction state");
                        return;
                }
            }
        }

        /// <summary>
        /// Whenever an element is linked
        /// Set the result of the first waiting dequeuer.
        /// Should be called under the update latch
        /// </summary>
        /// <returns>True if there was a dequeuer waiting and we could set its result, False otherwise</returns>
        private bool TryNotifyDequeuer(IListElement<T> listElement)
        {
            TaskCompletionSource<ConditionalValue<IListElement<T>>> res;

            // While we have a waiting dequeuer
            while (this.dequeuerQueue.TryDequeue(out res))
            {
                // If the dequeuers result can be set, then we are done, return true
                if (res.TrySetResult(new ConditionalValue<IListElement<T>>(true, listElement)))
                {
                    return true;
                }
            }

            // There was no waiting dequeuer
            return false;
        }

        private class ListElement : IListElement<T>
        {
            private readonly string traceType;

            public const long ForbiddenId = long.MinValue;

            private readonly long id;

            private readonly T value;

            private readonly object stateMachineLock;

            public ListElement Next { get; set; }

            public ListElement Previous { get; set; }

            private ListElementState State { get; set; }

            //public long? DequeueCommitLsn { get; private set; }
            public ITransaction DequeueCommittingTransaction { get; private set; }

            public override string ToString()
            {
                return string.Format(
                    "Id: {0}, State: {1}, Next: {2}, Previous: {3}, DequeueCommittingTransactionId: {4}",
                    this.id,
                    this.State,
                    this.Next != null ? this.Next.id.ToString() : "null",
                    this.Previous != null ? this.Previous.id.ToString() : "null",
                    this.DequeueCommittingTransaction);
            }

            public ListElement(long id, T value, string traceType, ListElementState state = ListElementState.Invalid)
            {
                this.traceType = traceType;

                if (id == ForbiddenId)
                {
                    TestableAssertHelper.FailInvalidOperation(this.traceType, "DataStore.ListElement", "id == ForbiddenId: {0}", ForbiddenId);
                }

                this.id = id;
                this.value = value;
                this.State = state;
                this.Next = null;
                this.Previous = null;
                this.DequeueCommittingTransaction = null;
                this.stateMachineLock = new object();
            }

            void IListElement<T>.StartEnqueue()
            {
                this.TransitionToState(ListElementState.EnqueueInFlight, ListElementState.Invalid);
            }

            void IListElement<T>.AbortEnqueue()
            {
                this.TransitionToState(ListElementState.Invalid, ListElementState.EnqueueInFlight);
            }

            void IListElement<T>.ApplyEnqueue()
            {
                this.TransitionToState(ListElementState.EnqueueApplied, ListElementState.EnqueueInFlight);
            }

            void IListElement<T>.UndoEnqueue()
            {
                this.TransitionToState(ListElementState.EnqueueUndone, ListElementState.EnqueueApplied);
            }

            void IListElement<T>.StartDequeue()
            {
                this.TransitionToState(ListElementState.DequeueInFlight, ListElementState.EnqueueApplied);
            }

            void IListElement<T>.AbortDequeue()
            {
                this.TransitionToState(ListElementState.EnqueueApplied, ListElementState.DequeueInFlight);
            }

            void IListElement<T>.ApplyDequeue(ITransaction commitingTx)
            {
                if (this.DequeueCommittingTransaction != null)
                {
                    throw new InvalidOperationException("DequeueCommitLsn != null");
                }

                this.DequeueCommittingTransaction = commitingTx;
                this.TransitionToState(ListElementState.DequeueApplied, ListElementState.DequeueInFlight);
            }

            void IListElement<T>.UndoDequeue()
            {
                this.DequeueCommittingTransaction = null;
                this.TransitionToState(ListElementState.EnqueueApplied, ListElementState.DequeueApplied);
            }

            long IListElement<T>.Id
            {
                get
                {
                    return this.id;
                }
            }

            T IListElement<T>.Value
            {
                get
                {
                    return this.value;
                }
            }

            IListElement<T> IListElement<T>.Next
            {
                get
                {
                    return this.Next;
                }
            }

            IListElement<T> IListElement<T>.Previous
            {
                get
                {
                    return this.Previous;
                }
            }

            ListElementState IListElement<T>.State
            {
                get
                {
                    return this.State;
                }
            }

            private void TransitionToState(ListElementState targetState, ListElementState allowedCurrentState)
            {
                lock (this.stateMachineLock)
                {
                    if (allowedCurrentState != this.State)
                    {
                        throw new InvalidOperationException(
                            string.Format(
                                "Invalid listElement state transition. \n\tCurrent state: {0}\n\tTarget state: {1}\n\tAllowed current state: {2}",
                                this.State,
                                targetState,
                                allowedCurrentState));
                    }

                    this.State = targetState;
                }
            }
        }
    }
}