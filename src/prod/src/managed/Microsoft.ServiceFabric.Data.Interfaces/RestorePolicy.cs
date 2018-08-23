// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Data
{
    /// <summary>
    /// Policy for restore operation.
    /// </summary>
    public enum RestorePolicy : int
    {
        /// <summary>
        /// Ensures that the backed up state being restored is ahead of the current state.
        /// </summary>
        Safe = 0,

        /// <summary>
        /// Does not check whether backed up state being restored is ahead of the current state.
        /// </summary>
        Force = 1,
    }
}