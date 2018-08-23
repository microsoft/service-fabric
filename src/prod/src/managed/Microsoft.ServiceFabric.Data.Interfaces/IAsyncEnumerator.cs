// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    using System;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// Asynchronous enumerator.
    /// </summary>
    /// <typeparam name="T">The type of objects to enumerate.</typeparam>
    public interface IAsyncEnumerator<out T> : IDisposable
    {
        /// <summary>
        /// Gets the current element in the enumerator.
        /// </summary>
        /// <value>
        /// Current element in the enumerator.
        /// </value>
        /// <remark>
        /// Calling after <see cref="Microsoft.ServiceFabric.Data.IAsyncEnumerator{T}.MoveNextAsync(CancellationToken)"/> 
        /// has passed the end of the collection is considered
        /// undefined behavior.
        /// </remark>
        T Current { get; }

        /// <summary>
        /// Advances the enumerator to the next element of the enumerator.
        /// </summary>
        /// <param name="cancellationToken">The token to monitor for cancellation requests.</param>
        /// <returns>
        /// true if the enumerator was successfully advanced to the next element; false
        ///  if the enumerator has passed the end of the collection.
        /// </returns>
        /// <exception cref="InvalidOperationException">The collection was modified after the enumerator was created.</exception>
        Task<bool> MoveNextAsync(CancellationToken cancellationToken);

        /// <summary>
        /// Sets the enumerator to its initial position, which is before the first element
        /// in the collection.
        /// </summary>
        /// <exception cref="InvalidOperationException">The collection was modified after the enumerator was created.</exception>
        void Reset();
    }
}