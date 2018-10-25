// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Represents different calls to IStateManager.PerformCheckpoint
    /// </summary>
    public enum PerformCheckpointMode : int
    {
        /// <summary>
        /// Regular call to PerformCheckpoint, checkpointed files consolidated as per state provider policy
        /// </summary>
        Default = 0,

        /// <summary>
        /// Checkpoint requested due to user configuration for 'LogTruncationIntervalInSeconds'. Expects consolidation to be initiated by state provider.
        /// </summary>
        Periodic = 1
    }
}