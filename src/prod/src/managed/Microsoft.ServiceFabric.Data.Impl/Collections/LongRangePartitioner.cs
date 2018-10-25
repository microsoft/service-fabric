// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Collections.Generic;
    using System.Fabric.Store;

    /// <summary>
    /// Partitions long input space .
    /// </summary>
    internal class LongRangePartitioner : IRangePartitioner<long>
    {
        /// <summary>
        /// Gets the partitions.
        /// </summary>
        public IEnumerable<long> Partitions
        {
            get
            {
                // Default set of partions for a key range 0 to long.MaxValue.
                // Since partitioning without knowing the workload is hard, following assumptions are being made
                //     1) 2/3 of users will use key numbers 0 to 33554432. 
                //     2) It is more likely for customers to use low number for their keys than high numbers.
                return new List<long>()
                {
                    8192,
                    16384,
                    32768,
                    65536,
                    131072,
                    262144,
                    524288,
                    1048576,
                    2097152,
                    4194304,
                    8388608,
                    16777216,
                    576460752303423488,
                    1152921504606846976,
                    2305843009213693952,
                    4611686018427387904
                };
            }
        }
    }
}