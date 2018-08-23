// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace System.Fabric.Common.Tracing
{
    /// <summary>
    /// Trace sink types.
    /// </summary>
    internal enum TraceSinkType : byte
    {
        /// <summary>
        /// ETW sink.  This is the only sink that should be used for production environment.
        /// </summary>
        ETW = 0,

        /// <summary>
        /// Text file sink.  Used mainly in dev environment for convenience.
        /// </summary>
        TextFile = 1,

        /// <summary>
        /// Console sink.  Must be highly filtered.
        /// </summary>
        Console = 2,

        /// <summary>
        /// The number of sink types.
        /// </summary>
        Max = 3
    }
}