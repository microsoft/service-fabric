// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Notifications
{
    using System;
    using System.Collections.Generic;

    /// <summary>
    /// Describes the action that caused the DictionaryChanged event.
    /// </summary>
    public enum NotifyDictionaryChangedAction : int
    {
        /// <summary>
        /// Indicates that the notification is for an add operation.
        /// </summary>
        Add = 0,

        /// <summary>
        /// Indicates that the notification is for an update operation.
        /// </summary>
        Update = 1,

        /// <summary>
        /// Indicates that the notification is for a remove operation.
        /// </summary>
        Remove = 2,

        /// <summary>
        /// Indicates that the notification is for a clear operation.
        /// </summary>
        Clear = 3,

        /// <summary>
        /// Indicates that the notification is for a rebuild operation.
        /// </summary>
        Rebuild = 4
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <remarks>
    /// DictionaryChanged notifications are synchronously fired by <cref name="IReliableDictionary"/> as part of applying the operation.
    /// Holding up the completion of these events can cause the replica to be blocked on the completion of the event.
    /// It is recommended that the events are handled as fast as possible.
    /// </remarks>
    public abstract class NotifyDictionaryChangedEventArgs<TKey, TValue> : EventArgs
    {
        private readonly NotifyDictionaryChangedAction action;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryChangedEventArgs"/>
        /// </summary>
        /// <param name="action">The type of notification.</param>
        public NotifyDictionaryChangedEventArgs(NotifyDictionaryChangedAction action)
        {
            this.action = action;
        }

        /// <summary>
        /// Gets the action that caused the event.
        /// </summary>
        /// <value>The type of notification.</value>
        public NotifyDictionaryChangedAction Action
        {
            get
            {
                return this.action;
            }
        }
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by a transactional operation.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    public abstract class NotifyDictionaryTransactionalEventArgs<TKey, TValue> : NotifyDictionaryChangedEventArgs<TKey, TValue>
    {
        private readonly ITransaction transaction;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryTransactionalEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the operation is related to.</param>
        /// <param name="action">Type of the change.</param>
        public NotifyDictionaryTransactionalEventArgs(ITransaction transaction, NotifyDictionaryChangedAction action) : base(action)
        {
            this.transaction = transaction;
        }

        /// <summary>
        /// Gets the transaction that the operation belongs to.
        /// </summary>
        /// <value>The transaction object associated with the notification.</value>
        public ITransaction Transaction
        {
            get
            {
                return this.transaction;
            }
        }
    }

    /// <summary>
    /// Provides data for the RebuildNotificationAsyncCallback event caused by a rebuild operation.
    /// Rebuild notification is fired at the end of recovery, copy or restore of reliable state.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <remarks>
    /// Note that until this operation completes, rebuild of the <cref name="IReliableDictionary"/> will not complete.
    /// This can cause the replica to be blocked waiting for the callback to complete before proceeding. 
    /// Asynchronous iteration over the state may require IO.
    /// </remarks>
    public class NotifyDictionaryRebuildEventArgs<TKey, TValue> : NotifyDictionaryChangedEventArgs<TKey, TValue>
    {
        private readonly IAsyncEnumerable<KeyValuePair<TKey, TValue>> enumerableState;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryRebuildEventArgs"/>
        /// </summary>
        /// <param name="enumerableState"><cref name="IAsyncEnumerable"/> that can be used to iterate the new state of the <cref name="IReliableDictionary"/>.</param>
        public NotifyDictionaryRebuildEventArgs(IAsyncEnumerable<KeyValuePair<TKey, TValue>> enumerableState) : base(NotifyDictionaryChangedAction.Rebuild)
        {
            this.enumerableState = enumerableState;
        }

        /// <summary>
        /// Gets an asynchronous enumerable that contains all items in the <cref name="IReliableDictionary"/>.
        /// </summary>
        /// <value>Asynchronous enumerable that contains the new state.</value>
        public IAsyncEnumerable<KeyValuePair<TKey, TValue>> State
        {
            get
            {
                return this.enumerableState;
            }
        }
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by a clear operation.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    public class NotifyDictionaryClearEventArgs<TKey, TValue> : NotifyDictionaryChangedEventArgs<TKey, TValue>
    {
        private readonly long commitSequenceNumber;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryClearEventArgs"/>
        /// </summary>
        /// <param name="commitSequenceNumber">
        /// The commit sequence number of the transaction cleared the <cref name="IReliableDictionary"/>
        /// </param>
        public NotifyDictionaryClearEventArgs(long commitSequenceNumber) : base(NotifyDictionaryChangedAction.Clear)
        {
            this.commitSequenceNumber = commitSequenceNumber;
        }

        /// <summary>
        /// Gets the commit sequence number for the operation that committed the clear.
        /// </summary>
        /// <value>
        /// Sequence number at which the Clear was committed.
        /// </value>
        public long CommitSequenceNumber
        {
            get
            {
                return this.commitSequenceNumber;
            }
        }
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by item addition.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    public class NotifyDictionaryItemAddedEventArgs<TKey, TValue> : NotifyDictionaryTransactionalEventArgs<TKey, TValue>
    {
        private readonly TKey key;
        private readonly TValue value;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryItemAddedEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the operation is related to.</param>
        /// <param name="key">Key that was added.</param>
        /// <param name="value">Value that was added.</param>
        public NotifyDictionaryItemAddedEventArgs(ITransaction transaction, TKey key, TValue value) : base(transaction, NotifyDictionaryChangedAction.Add)
        {
            this.key = key;
            this.value = value;
        }

        /// <summary>
        /// Gets the key.
        /// </summary>
        /// <value>
        /// The key.
        /// </value>
        public TKey Key
        {
            get
            {
                return this.key;
            }
        }

        /// <summary>
        /// Gets the value.
        /// </summary>
        /// <value>
        /// The value.
        /// </value>
        public TValue Value
        {
            get
            {
                return this.value;
            }
        }
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by item update.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    public class NotifyDictionaryItemUpdatedEventArgs<TKey, TValue> : NotifyDictionaryTransactionalEventArgs<TKey, TValue>
    {
        private readonly TKey key;
        private readonly TValue value;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryItemUpdatedEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the operation is related to.</param>
        /// <param name="key">Key that was updated.</param>
        /// <param name="value">The new value.</param>
        public NotifyDictionaryItemUpdatedEventArgs(ITransaction transaction, TKey key, TValue value) : base(transaction, NotifyDictionaryChangedAction.Update)
        {
            this.key = key;
            this.value = value;
        }

        /// <summary>
        /// Gets the key.
        /// </summary>
        /// <value>
        /// The key.
        /// </value>
        public TKey Key
        {
            get
            {
                return this.key;
            }
        }

        /// <summary>
        /// Gets the value.
        /// </summary>
        /// <value>
        /// The value.
        /// </value>
        public TValue Value
        {
            get
            {
                return this.value;
            }
        }
    }

    /// <summary>
    /// Provides data for the DictionaryChanged event caused by item removal.
    /// </summary>
    /// <typeparam name="TKey">The type of the keys in the <cref name="IReliableDictionary"/>.</typeparam>
    /// <typeparam name="TValue">The type of the values in the <cref name="IReliableDictionary"/>.</typeparam>
    public class NotifyDictionaryItemRemovedEventArgs<TKey, TValue> : NotifyDictionaryTransactionalEventArgs<TKey, TValue>
    {
        private readonly TKey key;

        /// <summary>
        /// Initializes a new instance of the <cref name="NotifyDictionaryItemRemovedEventArgs"/>
        /// </summary>
        /// <param name="transaction">Transaction that the operation is related to.</param>
        /// <param name="key">Key that was removed.</param>
        public NotifyDictionaryItemRemovedEventArgs(ITransaction transaction, TKey key) : base(transaction, NotifyDictionaryChangedAction.Remove)
        {
            this.key = key;
        }

        /// <summary>
        /// Gets the key.
        /// </summary>
        /// <value>
        /// The key.
        /// </value>
        public TKey Key
        {
            get
            {
                return this.key;
            }
        }
    }
}