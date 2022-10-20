// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Wave
{
    using System;
    using System.Collections.Generic;
    using System.Fabric.Store;
    using System.Linq;
    using System.Text;
    using System.Threading.Tasks;

    /// <summary>
    /// Provides partitioning for the wave store.
    /// </summary>
    internal class StringRangePartitioner : IRangePartitioner<string>
    {
        public IEnumerable<string> Partitions
        {
            get
            {
                return new List<string>() { "2", "5", "8" };
            }
        }
    }
}