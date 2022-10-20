// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace EventStore.Service.Cache
{
    using System;
    using System.Collections.Generic;
    using System.Threading;
    using System.Threading.Tasks;

    /// <summary>
    /// The Store interface
    /// </summary>
    /// <typeparam name="TKey"></typeparam>
    /// <typeparam name="TValue"></typeparam>
    internal interface ISortedStore<TKey, TValue> where TKey : IComparable, ITimeComparable where TValue : IComparable
    {
        Task StoreAsync(IDictionary<TKey, TValue> valueMap, CancellationToken token);

        Task StoreAsync(TKey key, TValue value, CancellationToken token);

        /// <summary>
        /// Earliest (by time) key in the store
        /// </summary>
        Task<TKey> GetEarliestKeyAsync();

        /// <summary>
        /// Latest (by time) key in the store.
        /// </summary>
        Task<TKey> GetLatestKeyAsync();

        Task<bool> ContainsKeyAsync(TKey key, CancellationToken token);

        Task<TValue> GetValueAsync(TKey key, CancellationToken token);

        Task<bool> RemoveAsync(TKey key, CancellationToken token);

        Task<int> GetItemCountAsync(CancellationToken token);

        Task<IEnumerable<TValue>> GetValuesInRangeAsync(DateTimeOffset startTime, DateTimeOffset endTime, CancellationToken token);

        Task<int> RemoveOldKeysAsync(DateTimeOffset earliestAllowedTime, CancellationToken token);

        Task<int> RemoveFirstXKeysAsync(int keyCountToRemove, CancellationToken token);

        Task<int> RemoveSelectedItemsAsync(Func<TKey, TValue, bool> condition, CancellationToken token);

        Task<bool> ClearAsync(CancellationToken token);
    }
}