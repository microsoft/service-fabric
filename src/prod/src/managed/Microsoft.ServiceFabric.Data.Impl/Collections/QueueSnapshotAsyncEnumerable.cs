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

    internal sealed class QueueSnapshotAsyncEnumerable<T> : IAsyncEnumerable<T>
    {
        private readonly bool isDeepCopyEnabled;

        private readonly IStoreReadWriteTransaction readWriteTxn;

        private readonly IAsyncEnumerable<KeyValuePair<long, T>> asyncEnumerable;

        public QueueSnapshotAsyncEnumerable(
            bool isDeepCopyEnabled, 
            IStoreReadWriteTransaction readWriteTxn, 
            IAsyncEnumerable<KeyValuePair<long, T>> asyncEnumerable)
        {
            this.isDeepCopyEnabled = isDeepCopyEnabled;
            this.readWriteTxn = readWriteTxn;
            this.asyncEnumerable = asyncEnumerable;
        }

        public IAsyncEnumerator<T> GetAsyncEnumerator()
        {
            return new QueueSnapshotAsyncEnumerator<T>(this.isDeepCopyEnabled, this.readWriteTxn, this.asyncEnumerable.GetAsyncEnumerator());
        }
    }

    internal sealed class QueueSnapshotAsyncEnumerator<T> : IAsyncEnumerator<T>
    {
        private static string ClassName = "SnapshotAsyncEnumerator";

        private readonly bool isDeepCopyEnabled;

        private readonly IStoreReadWriteTransaction readWriteTxn;

        private readonly IAsyncEnumerator<KeyValuePair<long, T>> asyncEnumerator;

        public QueueSnapshotAsyncEnumerator(
            bool isDeepCopyEnabled, 
            IStoreReadWriteTransaction readWriteTxn, 
            IAsyncEnumerator<KeyValuePair<long, T>> asyncEnumerator)
        {
            this.isDeepCopyEnabled = isDeepCopyEnabled;
            this.readWriteTxn = readWriteTxn;
            this.asyncEnumerator = asyncEnumerator;
        }

        public T Current
        {
            get
            {
                if (this.isDeepCopyEnabled)
                {
                    var tmpValueBeforeCopy = this.asyncEnumerator.Current.Value;
                    var tmpValue = this.CloneValueIfNecessary(ref tmpValueBeforeCopy);

                    return tmpValue;
                }

                return this.asyncEnumerator.Current.Value;
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
            Diagnostics.Assert(this.asyncEnumerator != null, ClassName, "async enumerator cannot be null");
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

        /// <summary>
        /// Clone the input value if necessary.
        /// </summary>
        /// <param name="input">The input value.</param>
        /// <returns>Cloned value if necessary.</returns>
        private T CloneValueIfNecessary(ref T input)
        {
            if (null == input)
            {
                // default(T) must return null.
                return default(T);
            }

            var cloneable = input as ICloneable;

            Diagnostics.Assert(null != cloneable, ClassName, "input must be cloneable.");

            return (T)cloneable.Clone();
        }
    }
}