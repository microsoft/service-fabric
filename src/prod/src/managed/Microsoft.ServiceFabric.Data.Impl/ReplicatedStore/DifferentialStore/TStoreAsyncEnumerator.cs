// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Implements an async enumerator.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    internal class TStoreAsyncEnumerator<TKey, TValue> : IAsyncEnumerator<KeyValuePair<TKey, TValue>>
    {
        #region Instance Members

        /// <summary>
        /// Inner enumerator.
        /// </summary>
        private IEnumerator<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> enumerator;

        /// <summary>
        /// Current position in the enumerator.
        /// </summary>
        private KeyValuePair<TKey, TValue> current = new KeyValuePair<TKey, TValue>(default(TKey), default(TValue));

        /// <summary>
        /// Indicates that the enumerator has completed.
        /// </summary>
        private bool isDone;

        #endregion

        /// <summary>
        /// Constructor.
        /// </summary>
        /// <param name="enumerable">Store enumerable.</param>
        public TStoreAsyncEnumerator(IEnumerable<Task<KeyValuePair<TKey, ConditionalValue<TValue>>>> enumerable)
        {
            this.enumerator = enumerable.GetEnumerator();
        }

        #region IEnumerator Members

        /// <summary>
        /// Gets the current element in the collection.
        /// </summary>
        public KeyValuePair<TKey, TValue> Current
        {
            get
            {
                if (this.isDone)
                {
                    throw new InvalidOperationException();
                }

                return this.current;
            }
        }

        #endregion

        #region IAsyncEnumerator Members

        /// <summary>
        /// Advances the enumerator to the next element of the collection asynchronously.
        /// </summary>
        /// <param name="cancellationToken">Propagates notification that operations should be canceled.</param>
        /// <returns></returns>
        public async Task<bool> MoveNextAsync(CancellationToken cancellationToken)
        {
            if (this.isDone)
            {
                return false;
            }

            while (this.enumerator.MoveNext())
            {
                var currentTask = await this.enumerator.Current.ConfigureAwait(false);
                if (currentTask.Value.HasValue)
                {
                    this.current = new KeyValuePair<TKey, TValue>(currentTask.Key, currentTask.Value.Value);
                    return true;
                }

                cancellationToken.ThrowIfCancellationRequested();
            }

            this.isDone = true;
            return false;
        }

        #endregion

        #region IEnumerator Members

        /// <summary>
        /// Performs application-defined tasks associated with freeing, releasing, or resetting unmanaged resources.
        /// </summary>
        public void Dispose()
        {
            this.isDone = true;
            this.enumerator.Dispose();
        }

        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element in the collection.
        /// </summary>
        public void Reset()
        {
            this.isDone = false;
            this.enumerator.Reset();
        }

        #endregion
    }
}