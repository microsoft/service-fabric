// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// Exposes an <see cref="Microsoft.ServiceFabric.Data.IAsyncEnumerator{T}"/> 
    /// which supports an asynchronous iteration over a collection 
    /// of a specified type.
    /// </summary>
    /// <typeparam name="T">The type of objects to enumerate.</typeparam>
    public interface IAsyncEnumerable<out T>
    {
        /// <summary>
        /// Returns an <see cref="Microsoft.ServiceFabric.Data.IAsyncEnumerator{T}"/> 
        /// that asynchronously iterates through the collection.
        /// </summary>
        /// <returns>An <see cref="Microsoft.ServiceFabric.Data.IAsyncEnumerator{T}"/> 
        /// that can be used to asynchronously iterate through the collection.</returns>
        IAsyncEnumerator<T> GetAsyncEnumerator();
    }
}