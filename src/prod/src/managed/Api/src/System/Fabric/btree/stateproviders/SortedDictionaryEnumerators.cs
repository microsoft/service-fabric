// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Data.StateProviders
{
    using System;
    using System.Collections;
    using System.Collections.Generic;
    using System.Linq;
    using System.Threading;
    using System.Threading.Tasks;
    using System.Diagnostics;
    using System.Diagnostics.CodeAnalysis;
    using System.Globalization;
    using System.Fabric;
    using System.Fabric.Common;
    using System.Fabric.Data.Common;
    using System.Fabric.Data.Btree;
    using System.Fabric.Data.LockManager;
    using System.Fabric.Data.TransactionManager;

    /// <summary>
    /// Provides an enumerator over the elements in the dictionary. The isolation level provided is serializable.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    /// <typeparam name="KC">Key comparator.</typeparam>
    /// <typeparam name="KBC">Key serializer.</typeparam>
    /// <typeparam name="VBC">Value serializer.</typeparam>
    sealed class SerializableSortedDictionaryEnumerator<K, V, KC, KBC, VBC> :
        IEnumerator<KeyValuePair<K, V>>,
        IDictionaryEnumerator
        where KC : IKeyComparable, new()
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="btreeScan">Btree scan used for enumeration.</param>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="rotx">Read only transaction.</param>
        /// <param name="lockTimeout">Timeout used for lock acquisition.</param>
        public SerializableSortedDictionaryEnumerator(
            IBtreeScan btreeScan, 
            IBtreeKey keyStart, 
            IBtreeKey keyEnd, 
            IReadOnlyTransaction rotx,
            int lockTimeout)
        {
            this.btreeScan = btreeScan;
            this.keyStart = keyStart;
            this.keyEnd = keyEnd;
            this.rotx = rotx;
            this.lockTimeout = lockTimeout;

            AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.SerializableSortedDictionaryEnumerator", "{0}", this.GetHashCode());
        }

        #region Instance Members

        /// <summary>
        /// Btree scan to use.
        /// </summary>
        IBtreeScan btreeScan;
        /// <summary>
        /// Range key start.
        /// </summary>
        IBtreeKey keyStart;
        /// <summary>
        /// Range key end.
        /// </summary>
        IBtreeKey keyEnd;
        /// <summary>
        /// Current btree scan position.
        /// </summary>
        IBtreeScanPosition scanPosition;
        /// <summary>
        /// Timeout used in lock acquisition.
        /// </summary>
        int lockTimeout;
        /// <summary>
        /// Status of the scan.
        /// </summary>
        bool isOpen;
        /// <summary>
        /// State of the scan.
        /// </summary>
        bool isDone;
        /// <summary>
        /// Used to convert the key from and to byte array.
        /// </summary>
        KBC keyBitConverter = new KBC();
        /// <summary>
        /// Read only transaction associted with the scan.
        /// </summary>
        IReadOnlyTransaction rotx;

        #endregion

        #region IEnumerator<KeyValuePair<K, V>> Members

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        public KeyValuePair<K, V> Current
        {
            get
            {
                if (!this.isOpen || this.isDone)
                {
                    AppTrace.TraceSource.WriteError("Data.SerializableSortedDictionaryEnumerator.Current", "{0} is not open", this.GetHashCode());
                    throw new InvalidOperationException();
                }
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.Current", "{0} return current position", this.GetHashCode());
                byte[] valueBytes = (null != this.scanPosition.Value) ? this.scanPosition.Value.Bytes : null;
                BtreeValue<V, VBC> itemValue = new BtreeValue<V, VBC>(valueBytes);
                byte[] keyBytes = (null != this.scanPosition.Key) ? this.scanPosition.Key.Bytes : null;
                BtreeKey<K, KBC> itemKey = new BtreeKey<K, KBC>(keyBytes);
                return new KeyValuePair<K, V>(itemKey.Key, itemValue.Value);
            }
        }
        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            if (null != this.btreeScan)
            {
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.Dispose", "{0}", this.GetHashCode());
                //
                // Release all acquired locks.
                //
                this.rotx.Terminate();
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.Dispose", "{0} close scan", this.GetHashCode());
                    //
                    // Attempt to close the btree scan.
                    //
                    this.btreeScan.CloseAsync(CancellationToken.None).Wait();
                }
                catch { }
                //
                // Reset internal state.
                //
                this.btreeScan = null;
                //
                // Supress finalizer.
                //
                GC.SuppressFinalize(this);
            }
        }
        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        object IEnumerator.Current
        {
            get
            {
                return this.Current;
            }
        }
        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns></returns>
        public bool MoveNext()
        {
            Exception exp = null;
            if (this.isDone)
            {
                AppTrace.TraceSource.WriteError("Data.SerializableSortedDictionaryEnumerator.MoveNext", "{0} is not open", this.GetHashCode());
                //
                // Cannot continue.
                //
                throw new InvalidOperationException();
            }
            if (!this.isOpen)
            {
                try
                {
                    //
                    // Acquire lock on the btree store.
                    //
                    this.rotx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.Shared, this.lockTimeout).Wait();
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumerator.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is TimeoutException || e is OutOfMemoryException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.MoveNext", "{0} open cursor", this.GetHashCode());
                    //
                    // Open cursor. This puts the scan position on the key before the start key.
                    //
                    IBtreeKey prevKey = this.btreeScan.OpenAsync(this.keyStart, this.keyEnd, null, true, CancellationToken.None).Result;
                    ReplicatedStateProvider.Assert(null != prevKey, "unexpected null previous key");

                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.MoveNext", "{0} move to first position in the scan", this.GetHashCode());
                    //
                    // Move cursor on the start key or the closest highest key to start key.
                    //
                    this.scanPosition = this.btreeScan.MoveNextAsync(CancellationToken.None).Result;
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumerator.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                //
                // Open was performed.
                //
                this.isOpen = true;
            }
            else
            {
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.MoveNext", "{0} move to next position in the scan", this.GetHashCode());
                    //
                    // Move the btree scan to the next position.
                    //
                    this.scanPosition = this.btreeScan.MoveNextAsync(CancellationToken.None).Result;
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumerator.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
            }
            //
            // Check to see if the enumerator got to the end.
            //
            this.isDone = (null != this.scanPosition.Key);
            return (null != this.scanPosition.Key);
        }
        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        public void Reset()
        {
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.Reset", "{0} reset the scan", this.GetHashCode());
                //
                // Reset and close the btree scan. 
                //
                this.btreeScan.ResetAsync(CancellationToken.None).Wait();
                this.btreeScan.CloseAsync(CancellationToken.None).Wait();
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumerator.Reset", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is FabricObjectClosedException || e is OutOfMemoryException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is FabricObjectClosedException)
                {
                    throw new InvalidOperationException();
                }
                else
                {
                    throw;
                }
            }
            //
            // Reset internal state.
            //
            this.isOpen = false;
            this.isDone = false;
        }

        #endregion

        #region IDictionaryEnumerator Members

        /// <summary>
        /// 
        /// </summary>
        public DictionaryEntry Entry
        {
            get
            {
                return new DictionaryEntry(this.Current.Key, this.Current.Value);
            }
        }
        /// <summary>
        /// 
        /// </summary>
        public object Key
        {
            get
            {
                return this.Current.Key;
            }
        }
        /// <summary>
        /// 
        /// </summary>
        public object Value
        {
            get
            {
                return this.Current.Value;
            }
        }

        #endregion
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    sealed class SerializableSortedDictionaryKeyEnumerator<K, V> : IEnumerator<K>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="enumerator"></param>
        public SerializableSortedDictionaryKeyEnumerator(IEnumerator<KeyValuePair<K, V>> enumerator)
        {
            this.enumerator = enumerator;
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        IEnumerator<KeyValuePair<K, V>> enumerator;

        #endregion

        #region IEnumerator<V> Members

        /// <summary>
        /// 
        /// </summary>
        public K Current
        {
            get { return this.enumerator.Current.Key; }
        }
        /// <summary>
        /// 
        /// </summary>
        public void Dispose()
        {
            if (null != this.enumerator)
            {
                this.enumerator.Dispose();
                this.enumerator = null;
                //
                // Supress finalizer.
                //
                GC.SuppressFinalize(this);
            }
        }
        /// <summary>
        /// 
        /// </summary>
        object IEnumerator.Current
        {
            get { return this.enumerator.Current.Key; }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public bool MoveNext()
        {
            return this.enumerator.MoveNext();
        }
        /// <summary>
        /// 
        /// </summary>
        public void Reset()
        {
            this.enumerator.Reset();
        }

        #endregion
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    sealed class SerializableSortedDictionaryKeyEnumerable<K, V> : Disposable, IEnumerable<K>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="enumerator"></param>
        public SerializableSortedDictionaryKeyEnumerable(IEnumerator<KeyValuePair<K, V>> enumerator)
        {
            this.enumerator = new SerializableSortedDictionaryKeyEnumerator<K, V>(enumerator);
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        SerializableSortedDictionaryKeyEnumerator<K, V> enumerator;

        #endregion

        #region IEnumerable<K> Members

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public IEnumerator<K> GetEnumerator()
        {
            return this.enumerator;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.enumerator;
        }

        #endregion

        #region IDisposable Overrides

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (null != this.enumerator)
                    {
                        this.enumerator.Dispose();
                        this.enumerator = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    sealed class SerializableSortedDictionaryValueEnumerator<K, V> : IEnumerator<V>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="enumerator"></param>
        public SerializableSortedDictionaryValueEnumerator(IEnumerator<KeyValuePair<K, V>> enumerator)
        {
            this.enumerator = enumerator;
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        IEnumerator<KeyValuePair<K, V>> enumerator;

        #endregion

        #region IEnumerator<V> Members

        /// <summary>
        /// 
        /// </summary>
        public V Current
        {
            get { return this.enumerator.Current.Value; }
        }
        /// <summary>
        /// 
        /// </summary>
        public void Dispose()
        {
            if (null != this.enumerator)
            {
                this.enumerator.Dispose();
                this.enumerator = null;
                //
                // Supress finalizer.
                //
                GC.SuppressFinalize(this);
            }
        }
        /// <summary>
        /// 
        /// </summary>
        object IEnumerator.Current
        {
            get { return this.enumerator.Current.Value; }
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public bool MoveNext()
        {
            return this.enumerator.MoveNext();
        }
        /// <summary>
        /// 
        /// </summary>
        public void Reset()
        {
            this.enumerator.Reset();
        }

        #endregion
    }
    /// <summary>
    /// 
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    sealed class SerializableSortedDictionaryValueEnumerable<K, V> : Disposable, IEnumerable<V>
    {
        /// <summary>
        /// 
        /// </summary>
        /// <param name="enumerator"></param>
        public SerializableSortedDictionaryValueEnumerable(IEnumerator<KeyValuePair<K, V>> enumerator)
        {
            this.enumerator = new SerializableSortedDictionaryValueEnumerator<K, V>(enumerator);
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        SerializableSortedDictionaryValueEnumerator<K, V> enumerator;

        #endregion

        #region IEnumerable<V> Members

        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        public IEnumerator<V> GetEnumerator()
        {
            return this.enumerator;
        }
        /// <summary>
        /// 
        /// </summary>
        /// <returns></returns>
        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.enumerator;
        }

        #endregion

        #region IDisposable Overrides

        /// <summary>
        /// 
        /// </summary>
        /// <param name="disposing"></param>
        protected override void Dispose(bool disposing)
        {
            if (!this.disposed)
            {
                if (disposing)
                {
                    if (null != this.enumerator)
                    {
                        this.enumerator.Dispose();
                        this.enumerator = null;
                    }
                }
                this.disposed = true;
            }
        }

        #endregion
    }
    /// <summary>
    /// Provides an async enumerator over the elements in the dictionary. The isolation level provided is serializable.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    /// <typeparam name="KC">Key comparator.</typeparam>
    /// <typeparam name="KBC">Key serializer.</typeparam>
    /// <typeparam name="VBC">Value serializer.</typeparam>
    sealed class SerializableSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC> :
        IEnumeratorAsync<KeyValuePair<K, V>>
        where KC : IKeyComparable, new()
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="btreeScan">Btree scan used for enumeration.</param>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyPrefixMatch">Prefix key used for matching.</param>
        /// <param name="rotx">Read only transaction.</param>
        /// <param name="lockTimeout">Timeout used for lock acquisition.</param>
        public SerializableSortedDictionaryEnumeratorAsync(
            IBtreeScan btreeScan, 
            IBtreeKey keyStart, 
            IBtreeKey keyEnd, 
            IBtreeKey keyPrefixMatch, 
            IReadOnlyTransaction rotx,
            int lockTimeout)
        {
            this.btreeScan = btreeScan;
            this.keyStart = keyStart;
            this.keyEnd = keyEnd;
            this.keyPrefixMatch = keyPrefixMatch;
            this.rotx = rotx;
            this.lockTimeout = lockTimeout;

            AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.SerializableSortedDictionaryEnumeratorAsync", "{0}", this.GetHashCode());
        }

        #region Instance Members

        /// <summary>
        /// Btree scan to use.
        /// </summary>
        IBtreeScan btreeScan;
        /// <summary>
        /// Range key start.
        /// </summary>
        IBtreeKey keyStart;
        /// <summary>
        /// Range key end.
        /// </summary>
        IBtreeKey keyEnd;
        /// <summary>
        /// Prefix key match.
        /// </summary>
        IBtreeKey keyPrefixMatch;
        /// <summary>
        /// Current btree scan position.
        /// </summary>
        IBtreeScanPosition scanPosition;
        /// <summary>
        /// Timeout used in lock acquisition.
        /// </summary>
        int lockTimeout;
        /// <summary>
        /// Status of the scan.
        /// </summary>
        bool isOpen;
        /// <summary>
        /// State of the scan.
        /// </summary>
        bool isDone;
        /// <summary>
        /// Used to convert the key from and to byte array.
        /// </summary>
        KBC keyBitConverter = new KBC();
        /// <summary>
        /// Read only transaction associted with the scan.
        /// </summary>
        IReadOnlyTransaction rotx;

        #endregion

        #region IEnumeratorAsync<KeyValuePair<K, V>> Members

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        public KeyValuePair<K, V> Current
        {
            get
            {
                if (!this.isOpen || this.isDone)
                {
                    AppTrace.TraceSource.WriteError("Data.SerializableSortedDictionaryEnumeratorAsync.Current", "{0} is not open", this.GetHashCode());
                    throw new InvalidOperationException();
                }
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.Current", "{0}", this.GetHashCode());
                byte[] valueBytes = (null != this.scanPosition.Value) ? this.scanPosition.Value.Bytes : null;
                BtreeValue<V, VBC> itemValue = new BtreeValue<V, VBC>(valueBytes);
                byte[] keyBytes = (null != this.scanPosition.Key) ? this.scanPosition.Key.Bytes : null;
                BtreeKey<K, KBC> itemKey = new BtreeKey<K, KBC>(keyBytes);
                return new KeyValuePair<K, V>(itemKey.Key, itemValue.Value);
            }
        }
        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns></returns>
        public async Task<bool> MoveNextAsync()
        {
            Exception exp = null;
            if (this.isDone)
            {
                AppTrace.TraceSource.WriteError("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", "{0} is not open", this.GetHashCode());
                //
                // Cannot continue.
                //
                throw new InvalidOperationException();
            }
            if (!this.isOpen)
            {
                try
                {
                    //
                    // Acquire lock on the btree store.
                    //
                    await this.rotx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.Shared, this.lockTimeout);
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is TimeoutException || e is OutOfMemoryException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", "{0} open cursor", this.GetHashCode());
                    //
                    // Open cursor. This puts the scan position on the key before the start key.
                    //
                    IBtreeKey prevKey = await this.btreeScan.OpenAsync(this.keyStart, this.keyEnd, this.keyPrefixMatch, true, CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != prevKey, "unexpected null previous key");

                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", "{0} move to first position in the scan", this.GetHashCode());
                    //
                    // Move cursor on the start key or the closest highest key to start key.
                    //
                    this.scanPosition = await this.btreeScan.MoveNextAsync(CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                //
                // Open was performed.
                //
                this.isOpen = true;
            }
            else
            {
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", "{0} move to next position in the scan", this.GetHashCode());
                    //
                    // Move cursor on the start key or the closest highest key to start key.
                    //
                    this.scanPosition = await this.btreeScan.MoveNextAsync(CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
            }
            //
            // Check to see if the enumerator got to the end.
            //
            this.isDone = (null != this.scanPosition.Key);
            return (null != this.scanPosition.Key);
        }
        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        /// <returns></returns>
        public async Task ResetAsync()
        {
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumerator.Reset", "{0} reset the scan", this.GetHashCode());
                //
                // Reset and close the btree scan.
                //
                await this.btreeScan.ResetAsync(CancellationToken.None);
                await this.btreeScan.CloseAsync(CancellationToken.None);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.SerializableSortedDictionaryEnumeratorAsync.Reset", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is FabricObjectClosedException || e is OutOfMemoryException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is FabricObjectClosedException)
                {
                    throw new InvalidOperationException();
                }
                else
                {
                    throw;
                }
            }
            //
            // Reset internal state.
            //
            this.isOpen = false;
            this.isDone = false;
        }
        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            if (null != this.btreeScan)
            {
                AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.Dispose", "{0}", this.GetHashCode());
                //
                // Release all acquired locks.
                //
                this.rotx.Terminate();
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.SerializableSortedDictionaryEnumeratorAsync.Dispose", "{0} close scan", this.GetHashCode());
                    //
                    // Attempt to close the btree scan.
                    //
                    this.btreeScan.CloseAsync(CancellationToken.None).Wait();
                }
                catch { }
                //
                // Reset internal state.
                //
                this.btreeScan = null;
                //
                // Supress finalizer.
                //
                GC.SuppressFinalize(this);
            }
        }

        #endregion
    }
    /// <summary>
    /// Supported transaction isolation levels.
    /// </summary>
    internal enum ReadIsolationLevel
    {
        Serializable,
        Repeatable,
        Committed,
        Uncommitted,
        Unprotected
    }
    /// <summary>
    /// Provides an async enumerator over the elements in the dictionary. Different isolation levels are supported.
    /// </summary>
    /// <typeparam name="K">The type of keys in the dictionary.</typeparam>
    /// <typeparam name="V">The type of values in the dictionary.</typeparam>
    /// <typeparam name="KC">Key comparator.</typeparam>
    /// <typeparam name="KBC">Key serializer.</typeparam>
    /// <typeparam name="VBC">Value serializer.</typeparam>
    sealed class ReadIsolatedSortedDictionaryEnumeratorAsync<K, V, KC, KBC, VBC> :
        IEnumeratorAsync<KeyValuePair<K, V>>
        where KC : IKeyComparable, new()
        where KBC : IKeyBitConverter<K>, new()
        where VBC : IValueBitConverter<V>, new()
    {
        [SuppressMessage("Microsoft.Usage", "CA2214:DoNotCallOverridableMethodsInConstructors")]
        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="btreeScan">Btree scan used for enumeration.</param>
        /// <param name="keyStart">Range start key.</param>
        /// <param name="keyEnd">Range end key.</param>
        /// <param name="keyPrefixMatch">Prefix key used for matching.</param>
        /// <param name="rotx">Read only transaction.</param>
        /// <param name="isolationLevel">Isolation used for the read only transaction.</param>
        /// <param name="lockTimeout">Timeout used for lock acquisition.</param>
        public ReadIsolatedSortedDictionaryEnumeratorAsync(
            IBtree btreeStore, 
            IBtreeScan btreeScan, 
            IBtreeKey keyStart, 
            IBtreeKey keyEnd, 
            IBtreeKey keyPrefixMatch, 
            IReadOnlyTransaction rotx, 
            ReadIsolationLevel isolationLevel,
            int lockTimeout)
        {
            this.btreeStore = btreeStore;
            this.btreeScan = btreeScan;
            this.keyStart = keyStart;
            this.keyEnd = keyEnd;
            this.keyPrefixMatch = keyPrefixMatch;
            this.rotx = rotx;
            this.isolationLevel = isolationLevel;
            this.lockTimeout = lockTimeout;

            AppTrace.TraceSource.WriteNoise(
                "ReadIsolatedSortedDictionaryEnumeratorAsync.ReadIsolatedSortedDictionaryEnumeratorAsync", 
                "{0} using isolation level {1}", 
                this.GetHashCode(),
                this.isolationLevel);
        }

        #region Instance Members

        /// <summary>
        /// 
        /// </summary>
        IBtree btreeStore;
        /// <summary>
        /// Btree scan to use.
        /// </summary>
        IBtreeScan btreeScan;
        /// <summary>
        /// Range key start.
        /// </summary>
        IBtreeKey keyStart;
        /// <summary>
        /// Range key end.
        /// </summary>
        IBtreeKey keyEnd;
        /// <summary>
        /// Prefix key match.
        /// </summary>
        IBtreeKey keyPrefixMatch;
        /// <summary>
        /// Current btree scan position.
        /// </summary>
        IBtreeScanPosition scanPosition;
        /// <summary>
        /// Current key in scan.
        /// </summary>
        IBtreeKey currenyKeyInScan;
        /// <summary>
        /// Current value in scan.
        /// </summary>
        IBtreeValue currentValueInScan;
        /// <summary>
        /// Read isolation level.
        /// </summary>
        ReadIsolationLevel isolationLevel;
        /// <summary>
        /// Timeout used in lock acquisition.
        /// </summary>
        int lockTimeout;
        /// <summary>
        /// Status of the scan.
        /// </summary>
        bool isOpen;
        /// <summary>
        /// State of the scan.
        /// </summary>
        bool isDone;
        /// <summary>
        /// Used to convert the key from and to byte array.
        /// </summary>
        KBC keyBitConverter = new KBC();
        /// <summary>
        /// Read only transaction associted with the scan.
        /// </summary>
        IReadOnlyTransaction rotx;

        #endregion

        #region IEnumeratorAsync<KeyValuePair<K, V>> Members

        /// <summary>
        /// Gets the element in the collection at the current position of the enumerator.
        /// </summary>
        public KeyValuePair<K, V> Current
        {
            get
            {
                if (!this.isOpen || this.isDone)
                {
                    AppTrace.TraceSource.WriteError("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Current", "{0} is not open", this.GetHashCode());
                    throw new InvalidOperationException();
                }
                AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Current", "{0}", this.GetHashCode());
                byte[] valueBytes = (null != this.currentValueInScan) ? this.currentValueInScan.Bytes : null;
                BtreeValue<V, VBC> itemValue = new BtreeValue<V, VBC>(valueBytes);
                byte[] keyBytes = (null != this.currenyKeyInScan) ? this.currenyKeyInScan.Bytes : null;
                BtreeKey<K, KBC> itemKey = new BtreeKey<K, KBC>(keyBytes);
                return new KeyValuePair<K, V>(itemKey.Key, itemValue.Value);
            }
        }
        /// <summary>
        /// Advances the enumerator to the next element of the collection.
        /// </summary>
        /// <returns></returns>
        public async Task<bool> MoveNextAsync()
        {
            Exception exp = null;
            if (this.isDone)
            {
                AppTrace.TraceSource.WriteError("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", "{0} is not open", this.GetHashCode());
                //
                // Cannot continue.
                //
                throw new InvalidOperationException();
            }
            if (!this.isOpen)
            {
                try
                {
                    if (ReadIsolationLevel.Unprotected != this.isolationLevel)
                    {
                        //
                        // Acquire lock on the btree store.
                        //
                        await this.rotx.LockAsync(SortedDictionaryStateProvider<K, V, KC, KBC, VBC>.btreeLock, LockMode.IntentShared, this.lockTimeout);
                    }
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is TimeoutException || e is OutOfMemoryException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", "{0} open cursor", this.GetHashCode());
                    //
                    // Open cursor. This puts the scan position on the key before the start key.
                    //
                    IBtreeKey prevKey = await this.btreeScan.OpenAsync(this.keyStart, this.keyEnd, this.keyPrefixMatch, false, CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != prevKey, "unexpected null previous key");

                    AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", "{0} move to first position in the scan", this.GetHashCode());
                    //
                    // Move cursor on the start key or the closest highest key to start key.
                    //
                    this.scanPosition = await this.btreeScan.MoveNextAsync(CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                //
                // Read over the scan.
                //
                await this.ReadIsolatedAsync();
                //
                // Open was performed.
                //
                this.isOpen = true;
            }
            else
            {
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", "{0} move to next position in the scan", this.GetHashCode());
                    //
                    // Reset current position in enumerator.
                    //
                    this.currenyKeyInScan = null;
                    this.currentValueInScan = null;
                    //
                    // Move cursor on the start key or the closest highest key to start key.
                    //
                    this.scanPosition = await this.btreeScan.MoveNextAsync(CancellationToken.None);
                    ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                }
                catch (Exception e)
                {
                    e = ReplicatedStateProvider.ProcessException("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.MoveNext", e, "{0}", this.ToString());
                    ReplicatedStateProvider.Assert(
                        e is OutOfMemoryException || e is TimeoutException || e is FabricObjectClosedException,
                        string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                    //
                    // Store original exception.
                    //
                    exp = e;
                    //
                    // Mark the enumerator as completed.
                    //
                    this.isDone = true;
                }
                if (null != exp)
                {
                    //
                    // Cannot continue.
                    //
                    throw exp;
                }
                //
                // Read over the scan.
                //
                await this.ReadIsolatedAsync();
            }
            //
            // Check to see if the enumerator got to the end.
            //
            this.isDone = (null != this.currenyKeyInScan);
            return (null != this.currenyKeyInScan);
        }
        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        /// <returns></returns>
        public async Task ResetAsync()
        {
            try
            {
                AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Reset", "{0} reset the scan", this.GetHashCode());
                //
                // Reset and close the btree scan.
                //
                await this.btreeScan.ResetAsync(CancellationToken.None);
                await this.btreeScan.CloseAsync(CancellationToken.None);
            }
            catch (Exception e)
            {
                e = ReplicatedStateProvider.ProcessException("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Reset", e, "{0}", this.ToString());
                ReplicatedStateProvider.Assert(
                    e is OutOfMemoryException || e is FabricObjectClosedException,
                    string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));

                if (e is FabricObjectClosedException)
                {
                    throw new InvalidOperationException();
                }
                else
                {
                    throw;
                }
            }
            //
            // Reset internal state.
            //
            this.isOpen = false;
            this.isDone = false;
        }
        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            if (null != this.btreeScan)
            {
                AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Dispose", "{0}", this.GetHashCode());
                //
                // Release all acquired locks.
                //
                this.rotx.Terminate();
                try
                {
                    AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.Dispose", "{0} close scan", this.GetHashCode());
                    //
                    // Attempt to close the btree scan.
                    //
                    this.btreeScan.CloseAsync(CancellationToken.None).Wait();
                }
                catch { }
                //
                // Reset internal state.
                //
                this.btreeScan = null;
                this.btreeStore = null;
                //
                // Supress finalizer.
                //
                GC.SuppressFinalize(this);
            }
        }

        #endregion

        /// <summary>
        /// Perform isolated reads on the scan.
        /// </summary>
        async Task ReadIsolatedAsync()
        {
            Exception exp = null;
            IBtreeValue btreeValue = null;
            if (ReadIsolationLevel.Uncommitted != this.isolationLevel && ReadIsolationLevel.Unprotected != this.isolationLevel)
            {
                //
                // If the read is uncommitted/unprotected, then no certification visit is required.
                //
                return;
            }
            //
            // Check to see if there is anything in the scan.
            //
            if (null != this.scanPosition.Key)
            {
                while (true)
                {
                    if (null == this.scanPosition.Key)
                    {
                        //
                        // Reached the end of the scan.
                        //
                        break;
                    }
                    //
                    // Lock the key from the scan position.
                    //
                    try
                    {
                        AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.ReadIsolated", "{0} certify key", this.GetHashCode());
                        //
                        // Acquire lock on the key.
                        //
                        string keyLockResourceName = CRC64.ToCRC64String(this.scanPosition.Key.Bytes);
                        ILock keyLock = await this.rotx.LockAsync(keyLockResourceName, LockMode.Shared, this.lockTimeout);
                        //
                        // Seek the key after the lock acquisition.
                        //
                        btreeValue = await this.btreeStore.SeekAsync(this.scanPosition.Key, CancellationToken.None);
                        if (null != btreeValue)
                        {
                            AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.ReadIsolated", "{0} current position is stable", this.GetHashCode());
                            //
                            // The key is stable. We read its value.
                            //
                            this.currenyKeyInScan = this.scanPosition.Key;
                            this.currentValueInScan = btreeValue;
                            if (this.isolationLevel == ReadIsolationLevel.Committed)
                            {
                                //
                                // Release lock on the key.
                                //
                                this.rotx.Unlock(keyLock);
                            }
                            else
                            {
                                //
                                // Keep long duration shared lock until the end of read transaction.
                                //
                            }
                            break;
                        }
                        else
                        {
                            AppTrace.TraceSource.WriteNoise("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.ReadIsolated", "{0} move to next position in the scan", this.GetHashCode());
                            //
                            // Release key lock since the key disappeared after the scan returned it.
                            //
                            this.rotx.Unlock(keyLock);
                            //
                            // Advance the btree scan.
                            //
                            this.scanPosition = await this.btreeScan.MoveNextAsync(CancellationToken.None);
                            ReplicatedStateProvider.Assert(null != this.scanPosition, "unexpected null scan position");
                        }
                    }
                    catch (Exception e)
                    {
                        e = ReplicatedStateProvider.ProcessException("Data.ReadIsolatedSortedDictionaryEnumeratorAsync.ReadIsolated", e, "{0}", this.ToString());
                        ReplicatedStateProvider.Assert(
                            e is TimeoutException || e is OutOfMemoryException || e is FabricObjectClosedException,
                            string.Format(CultureInfo.InvariantCulture, "unexpected exception {0}", e.GetType()));
                        //
                        // Store original exception.
                        //
                        exp = e;
                        //
                        // Mark the enumerator as completed.
                        //
                        this.isDone = true;
                    }
                    if (null != exp)
                    {
                        //
                        // Cannot continue.
                        //
                        throw exp;
                    }
                }
            }
        }
    }
}