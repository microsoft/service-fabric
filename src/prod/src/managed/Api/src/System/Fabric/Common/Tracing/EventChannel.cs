// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    /// <summary>
    /// Channel type for ETW tracing.
    /// </summary>
    internal enum EventChannel : byte
    {
        /// <summary>
        /// Admin channel.
        /// </summary>
        Admin = 0,

        /// <summary>
        /// Operational channel.
        /// </summary>
        Operational = 1,

        /// <summary>
        /// Analytic channel.
        /// </summary>
        Analytic = 2,

        /// <summary>
        /// Debug channel.
        /// </summary>
        Debug = 3
    }
}