// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    internal interface ICache<TKey, TValue> where TKey : IEquatable<TKey>
    {
        /// <summary>
        /// Add or Update a key value pair.
        /// </summary>
        /// <param name="key">The key</param>
        /// <param name="value">The value</param>
        /// <param name="itemLife">Life span for this pair</param>
        /// <param name="onExpireCallback">Specify the callback to be invoked when this entry is removed from cache due to expiry</param>
        /// <returns>Count of elements in the cache after this update</returns>
        bool TryAddOrUpdate(TKey key, TValue value, TimeSpan itemLife, Action<TValue> onExpireCallback);

        /// <summary>
        /// Remove a Key if present
        /// </summary>
        /// <param name="key"></param>
        void RemoveIfPresent(TKey key);

        /// <summary>
        /// Get the count of Items in the DefaultInstance.
        /// </summary>
        /// <returns></returns>
        long Count { get; }

        /// <summary>
        /// Clear out the Cache.
        /// </summary>
        void Clear();
    }
}