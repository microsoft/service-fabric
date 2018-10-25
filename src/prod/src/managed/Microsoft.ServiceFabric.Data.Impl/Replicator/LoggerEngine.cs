// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Context for the apply.
    /// </summary>
    internal enum LoggerEngine : int
    {
        /// <summary>
        /// Invalid
        /// </summary>
        Invalid = 0,

        /// <summary>
        /// The underlying log is a file 
        /// </summary>
        File = 1,

        /// <summary>
        /// The underlying log is a memory stream
        /// </summary>
        Memory = 2,

        /// <summary>
        /// The underlying log is a ktl logger
        /// </summary>
        Ktl = 3,
    }
}