// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data.Collections
{
    using System.Collections.Generic;
    using System.Fabric.Store;

    /// <summary>
    /// Partitions the string input space.
    /// </summary>
    internal class StringRangePartitioner : IRangePartitioner<string>
    {
        /// <summary>
        /// Gets the partitions.
        /// </summary>
        public IEnumerable<string> Partitions
        {
            get { return new List<string>() {"g", "n", "u"}; }
        }
    }
}