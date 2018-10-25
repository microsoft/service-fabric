// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.ReplicatedStore.DifferentialStore
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Data.Common;
    using System.Threading;
    using System.Threading.Tasks;

    internal class KeyValueAsyncEnumerable<TKey, TValue> : IAsyncEnumerable<KeyValuePair<TKey, TValue>>
    {
        private readonly IEnumerable<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> sourceEnumerable;

        private readonly string traceType;

        public KeyValueAsyncEnumerable(IEnumerable<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> sourceEnumerable, string traceType)
        {
            this.sourceEnumerable = sourceEnumerable;
            this.traceType = traceType;
        }

        public IAsyncEnumerator<KeyValuePair<TKey, TValue>> GetAsyncEnumerator()
        {
            return new KeyValueAsyncEnumerator<TKey, TValue>(this.sourceEnumerable.GetEnumerator(), this.traceType);
        }
    }

    internal class KeyValueAsyncEnumerator<TKey, TValue> : IAsyncEnumerator<KeyValuePair<TKey, TValue>>
    {
        private readonly IEnumerator<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> sourceEnumerator;
        private KeyValuePair<TKey, TValue> currentItem;
        private readonly string traceType;

        public KeyValueAsyncEnumerator(IEnumerator<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> sourceEnumerator, string traceType)
        {
            this.sourceEnumerator = sourceEnumerator;
            this.traceType = traceType;
        }

        public KeyValuePair<TKey, TValue> Current
        {
            get { return this.currentItem; }
        }

        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            while (this.sourceEnumerator.MoveNext())
            {
                var item = await sourceEnumerator.Current.ConfigureAwait(false);

                if (item.Value.HasValue)
                {
                    this.currentItem = new KeyValuePair<TKey, TValue>(item.Key, item.Value.Value);
                    return true;
                }
            }

            return false;
        }

        public void Reset()
        {
            this.sourceEnumerator.Reset();
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
                Diagnostics.Assert(sourceEnumerator != null, this.traceType, "async enumerator cannot be null");
                this.sourceEnumerator.Dispose();
            }
        }
    }
}