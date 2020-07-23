// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace NativeReplicatorHTTPClient
{
    /// <summary>
    /// service partition types
    /// </summary>
    public enum ServicePartitionType
    {
        /// <summary>
        /// no partition type selected
        /// </summary>
        None,

        /// <summary>
        /// singleton type
        /// </summary>
        Singleton,

        /// <summary>
        /// named type
        /// </summary>
        Named,

        /// <summary>
        /// uniform type
        /// </summary>
        Uniform
    }
}