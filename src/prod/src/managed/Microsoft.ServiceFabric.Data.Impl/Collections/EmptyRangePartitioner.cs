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
    internal class EmptyRangePartitioner<T> : IRangePartitioner<T>
    {
        /// <summary>
        /// Gets the partitions.
        /// </summary>
        IEnumerable<T> IRangePartitioner<T>.Partitions
        {
            get { return new List<T>(); }
        }
    }
}