// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Concurrent;
    using System.Collections.Generic;

    /// <summary>
    /// Version that is carried with every key/value modification.
    /// </summary>
    internal class ConcurrentVersionChainsDictionary<TKey, TValue> : ConcurrentDictionary<TKey, TValue>, IVersionChainsDictionary<TKey, TValue>
    {
        public ConcurrentVersionChainsDictionary(IEqualityComparer<TKey> comparer)
            : base(comparer)
        {
        }
    }
}