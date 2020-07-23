// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections;
    using System.Collections.Generic;

    /// <summary>
    /// Version that is carried with every key/value modification.
    /// </summary>
    internal interface IVersionChainsDictionary<TKey, TValue> : IDictionary<TKey, TValue>,
        ICollection<KeyValuePair<TKey, TValue>>,
        IEnumerable<KeyValuePair<TKey, TValue>>,
        IDictionary,
        ICollection,
        IEnumerable
    {
        TValue GetOrAdd(TKey key, Func<TKey, TValue> valueFactory);


        bool TryRemove(TKey key, out TValue value);
    }
}