// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System.Collections.Generic;

    internal class PartitionComparer<TKey, TValue> : IComparer<IPartition<TKey, TValue>>
    {
        private readonly IComparer<TKey> comparer;

        public PartitionComparer(IComparer<TKey> comparer)
        {
            this.comparer = comparer;
        }

        public int Compare(IPartition<TKey, TValue> x, IPartition<TKey, TValue> y)
        {
            return this.comparer.Compare(x.FirstKey, y.FirstKey);
        }
    }
}