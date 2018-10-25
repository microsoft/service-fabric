// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;
    using Microsoft.ServiceFabric.Data;

    /// <summary>
    /// Encapsulates a readable store component functionality.
    /// </summary>
    /// <typeparam name="TKey">Key type.</typeparam>
    /// <typeparam name="TValue">Value type.</typeparam>
    internal interface IReadableStoreComponent<TKey, TValue>
    {
        /// <summary>
        /// Determines whether the store component contains an element with the specified key.
        /// </summary>
        /// <param name="key">Key to locate.</param>
        /// <returns>True if the store component contains an element with the key; otherwise, false.</returns>
        bool ContainsKey(TKey key);

        /// <summary>
        /// Reads the value for a given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>Value if the key exists.</returns>
        TValue Read(TKey key);

        /// <summary>
        /// Reads the value for a given key that is lesser taht the given visbility lsn.
        /// </summary>
        /// <param name="key"></param>
        /// <param name="visbilityLsn"></param>
        /// <returns></returns>
        TValue Read(TKey key, long visbilityLsn);

        /// <summary>
        /// Reads the value for a key greater than the given key.
        /// </summary>
        /// <param name="key">Key to read.</param>
        /// <returns>Value if the key exists.</returns>
        ConditionalValue<TKey> ReadNext(TKey key);

        /// <summary>
        /// Returns an enumerable that is used to iterate through the collection.
        /// </summary>
        /// <returns></returns>
        IEnumerable<TKey> EnumerateKeys();

        /// <summary>
        /// Returns number of keys found in the consolidated state.
        /// </summary>
        long Count();
    }
}