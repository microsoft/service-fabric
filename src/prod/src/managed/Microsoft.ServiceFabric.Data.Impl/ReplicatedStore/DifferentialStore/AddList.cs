// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections;
    using System.Collections.Generic;

    using Microsoft.ServiceFabric.Data.DataStructures.Concurrent;

    internal interface IAddList<TKey>
    {
        bool TryAdd(TKey key);
    }

    /// <summary>
    /// AddList keeps track of keys that are in the differential componenet but not in the consolidated.
    /// </summary>
    /// <typeparam name="TKey">Type of the key.</typeparam>
    internal class AddList<TKey> : IAddList<TKey>, IEnumerable<TKey>
    {
        private const byte DefaultValue = Byte.MaxValue;

        private readonly ConcurrentSkipList<TKey, byte> addedItemsDictionary;

        private readonly IComparer<TKey> keyComparer;

        public AddList(IComparer<TKey> keyComparer)
        {
            this.keyComparer = keyComparer;
            this.addedItemsDictionary = new ConcurrentSkipList<TKey, byte>(this.keyComparer);
        }

        public bool TryAdd(TKey key)
        {
            // Item may already exist.
            return this.addedItemsDictionary.TryAdd(key, DefaultValue);
        }

        public IEnumerator<TKey> GetEnumerator()
        {
            return this.addedItemsDictionary.GetEnumerator();
        }

        IEnumerator IEnumerable.GetEnumerator()
        {
            return this.GetEnumerator();
        }
    }
}