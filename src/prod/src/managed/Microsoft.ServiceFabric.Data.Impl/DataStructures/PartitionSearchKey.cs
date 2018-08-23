// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    using System;
    using System.Collections.Generic;

    internal struct PartitionSearchKey<TKey, TValue> : IPartition<TKey, TValue>
    {
        private readonly TKey creationValue;

        public TKey FirstKey
        {
            get
            {
                return this.creationValue;
            }
        }

        public int Count
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        public IReadOnlyList<TKey> Keys
        {
            get
            {
                throw new NotImplementedException();
            }
        }

        public PartitionSearchKey(TKey value)
        {
            this.creationValue = value;
        }

        public void Add(TKey key, TValue value)
        {
            throw new NotImplementedException();
        }

        public TKey GetKey(int index)
        {
            throw new NotImplementedException();
        }

        public TValue GetValue(int index)
        {
            throw new NotImplementedException();
        }

        public void UpdateValue(int index, TValue value)
        {
            throw new NotImplementedException();
        }

        public int BinarySearch(TKey key, IComparer<TKey> comparer)
        {
            throw new NotImplementedException();
        }
    }
}