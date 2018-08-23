// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System.Collections.Generic;

    internal interface IPartition<TKey, TValue>
    {
        TKey FirstKey
        {
            get;
        }

        int Count { get; }

        /// <summary>
        /// Gets the current list of keys.
        /// </summary>
        IReadOnlyList<TKey> Keys { get; }

        void Add(TKey key, TValue value);

        TKey GetKey(int index);

        TValue GetValue(int index);

        void UpdateValue(int index, TValue value);

        int BinarySearch(TKey key, IComparer<TKey> comparer);
    }
}