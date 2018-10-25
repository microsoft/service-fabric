// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    internal class Partition<TKey, TValue> : KVList<TKey, TValue>, IPartition<TKey, TValue>
    {
        public TKey FirstKey
        {
            get
            {
                // Will throw if partition is empty.
                return this.GetKey(0);
            }
        }

        public Partition(int maxSize): base(maxSize)
        { 
        }

        public Partition(int intialSize, int maxSize) : base(intialSize, maxSize)
        {
        }
    }
}