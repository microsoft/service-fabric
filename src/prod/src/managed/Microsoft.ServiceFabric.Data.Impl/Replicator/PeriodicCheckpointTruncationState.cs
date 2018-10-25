// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    /// <summary>
    /// Various phases for periodic checkpointing + truncation. Provides guarantee that log does not contain items older than configured interval
    /// </summary>
    public enum PeriodicCheckpointTruncationState : int
    {
        /// <summary>
        /// Configured interval has not yet passed
        /// </summary>
        NotStarted = 0,

        /// <summary>
        /// Configured interval has passed. 
        /// Subsequent call to 'ShouldCheckpoint' return true (if no checkpoint in progress), overriding checkpointIntervalBytes configuration.
        /// </summary>
        Ready = 1,

        /// <summary>
        /// Checkpoint initiated
        /// </summary>
        CheckpointStarted = 2,

        /// <summary>
        /// Checkpoint completed. 
        /// Subsequent calls to 'ShouldTruncateHead' return true (if no truncation in progress), overriding truncation threshold and min log size configurations
        /// </summary>
        CheckpointCompleted = 3,

        /// <summary>
        /// Truncation has been initiated
        /// </summary>
        TruncationStarted = 4
    }
}