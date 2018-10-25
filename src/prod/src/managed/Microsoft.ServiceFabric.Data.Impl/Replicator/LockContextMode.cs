// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Transaction Lock Mode.
    /// </summary>
    public enum LockContextMode
    {
        /// <summary>
        /// Mode used for read operations.
        /// </summary>
        Read = 0,

        /// <summary>
        /// Mode used for write operations.
        /// </summary>
        Write = 1,
    }
}