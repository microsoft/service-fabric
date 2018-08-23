// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.DataStructures
{
    internal struct Index
    {
        int partitionIndex;
        int itemIndex;

        public Index(int partitionIndex, int itemIndex)
        {
            this.partitionIndex = partitionIndex;
            this.itemIndex = itemIndex;
        }

        public int PartitionIndex
        {
            get
            {
                return this.partitionIndex;
            }
        }

        public int ItemIndex
        {
            get
            {
                return this.itemIndex;
            }
        }
    }
}