// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Fabric.Store;
    using System.Threading;
    using System.Threading.Tasks;

    internal sealed class SnapshotAsyncEnumerable<TKey, TValue> : IAsyncEnumerable<KeyValuePair<TKey, TValue>>, IAsyncEnumerable<TKey>
    {
        private readonly bool isDeepCopyEnabled;
        private readonly IStoreReadWriteTransaction readWriteTxn;
        private readonly IAsyncEnumerable<KeyValuePair<TKey, TValue>> asyncEnumerable;

        public SnapshotAsyncEnumerable(
            bool isDeepCopyEnabled,
            IStoreReadWriteTransaction readWriteTxn,
            IAsyncEnumerable<KeyValuePair<TKey, TValue>> asyncEnumerable)
        {
            this.isDeepCopyEnabled = isDeepCopyEnabled;
            this.readWriteTxn = readWriteTxn;
            this.asyncEnumerable = asyncEnumerable;
        }

        public IAsyncEnumerator<KeyValuePair<TKey, TValue>> GetAsyncEnumerator()
        {
            return new SnapshotAsyncEnumerator<TKey, TValue>(this.isDeepCopyEnabled, this.readWriteTxn, this.asyncEnumerable.GetAsyncEnumerator());
        }

        IAsyncEnumerator<TKey> IAsyncEnumerable<TKey>.GetAsyncEnumerator()
        {
            return new SnapshotAsyncEnumerator<TKey, TValue>(this.isDeepCopyEnabled, this.readWriteTxn, this.asyncEnumerable.GetAsyncEnumerator());
        }
    }

    internal sealed class SnapshotAsyncEnumerator<TKey, TValue> : IAsyncEnumerator<KeyValuePair<TKey, TValue>>, IAsyncEnumerator<TKey>
    {
        private static string ClassName = "SnapshotAsyncEnumerator";

        private readonly bool isDeepCopyEnabled;
        private readonly IStoreReadWriteTransaction readWriteTxn;
        private readonly IAsyncEnumerator<KeyValuePair<TKey, TValue>> asyncEnumerator;

        public SnapshotAsyncEnumerator(
            bool isDeepCopyEnabled,
            IStoreReadWriteTransaction readWriteTxn,
            IAsyncEnumerator<KeyValuePair<TKey, TValue>> asyncEnumerator)
        {
            this.isDeepCopyEnabled = isDeepCopyEnabled;
            this.readWriteTxn = readWriteTxn;
            this.asyncEnumerator = asyncEnumerator;
        }

        public KeyValuePair<TKey, TValue> Current
        {
            get
            {
                if (this.isDeepCopyEnabled)
                {
                    var tmpKeyBeforeCopy = this.asyncEnumerator.Current.Key;
                    var tmpValueBeforeCopy = this.asyncEnumerator.Current.Value;

                    var tmpKey = this.CloneIfNecessary(ref tmpKeyBeforeCopy);
                    var tmpValue = this.CloneIfNecessary(ref tmpValueBeforeCopy);

                    return new KeyValuePair<TKey, TValue>(tmpKey, tmpValue);
                }

                return this.asyncEnumerator.Current;
            }
        }

        TKey IAsyncEnumerator<TKey>.Current
        {
            get
            {
                if (this.isDeepCopyEnabled)
                {
                    var tmpKeyBeforeCopy = this.asyncEnumerator.Current.Key;
                    var tmpKey = this.CloneIfNecessary(ref tmpKeyBeforeCopy);

                    return tmpKey;
                }

                return this.asyncEnumerator.Current.Key;
            }
        }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            // enumerator.MoveNext performs the read, and needs to be under snapshot isolation
            var originalIsolation = this.readWriteTxn.Isolation;
            this.readWriteTxn.Isolation = StoreTransactionReadIsolationLevel.Snapshot;

            var result = await this.asyncEnumerator.MoveNextAsync(cancellationToken).ConfigureAwait(false);

            this.readWriteTxn.Isolation = originalIsolation;

            return result;
        }

        public void Reset()
        {
            Diagnostics.Assert(asyncEnumerator != null, ClassName, "async enumerator cannot be null");
            this.asyncEnumerator.Reset();
        }

        public void Dispose()
        {
            this.Dispose(true);
            GC.SuppressFinalize(this);
        }

        private void Dispose(bool isDisposing)
        {
            if (isDisposing)
            {
                Diagnostics.Assert(this.asyncEnumerator != null, ClassName, "async enumerator cannot be null");
                this.asyncEnumerator.Dispose();
            }
        }

        private T CloneIfNecessary<T>(ref T input)
        {
            if (null == input)
            {
                return default(T);
            }

            var cloneable = input as ICloneable;

            Diagnostics.Assert(null != cloneable, ClassName, "input must be cloneable.");

            return (T)cloneable.Clone();
        }
    }
}