// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------



namespace Microsoft.ServiceFabric.Data.DataStructures.Concurrent
{
    using System;
    using System.Collections.Generic;

    internal interface ISortedList<TKey, TValue> : IEnumerable<TKey>
    {
        bool Contains(TKey key);

        bool TryAdd(TKey key, TValue value);

        void Update(TKey key, TValue value);

        void Update(TKey key, Func<TKey, TValue, TValue> updateFunction);

        bool TryRemove(TKey key);

        bool TryGetValue(TKey key, out TValue value);
    }
}