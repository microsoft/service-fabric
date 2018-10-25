// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Store
{
    using System.Collections.Generic;

    /// <summary>
    /// Provides a list of key range partitions for a given key type.
    /// </summary>
    /// <typeparam name="T">Key range type.</typeparam>
    internal interface IRangePartitioner<T>
    {
        /// <summary>
        /// Gets the list of partitions (static).
        /// </summary>
        IEnumerable<T> Partitions { get; }
    }
}