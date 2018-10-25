// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    /// <summary>
    /// Azure table for operational channel traces.
    /// </summary>
    public enum TableEntityKind : byte
    {
        /// <summary>
        /// 
        /// </summary>
        Unknown = 0,
        /// <summary>
        /// 
        /// </summary>
        Cluster      = 1,
        /// <summary>
        /// 
        /// </summary>
        Nodes        = 2,
        /// <summary>
        /// 
        /// </summary>
        Applications = 3,
        /// <summary>
        /// 
        /// </summary>
        Partitions   = 4,
        /// <summary>
        /// 
        /// </summary>
        Replicas     = 5,

        /// <summary>
        /// Correlation Kind
        /// </summary>
        Correlation = 6
    }
}