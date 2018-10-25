// ------------------------------------------------------------
// Copyright (c) Microsoft Corporation.  All rights reserved.
// Licensed under the MIT License (MIT). See License.txt in the repo root for license information.
// ------------------------------------------------------------

namespace Microsoft.ServiceFabric.Replicator
{
    using System;
    using System.Threading.Tasks;

    /// <summary>
    /// Replicated log manager interface.
    /// </summary>
    internal interface IReplicatedLogManager
    {
        /// <summary>
        /// Get the Progress Vector.
        /// </summary>
        ProgressVector ProgressVector
        {
            get;
        }

        /// <summary>
        /// Gets the last completed End Checkpoint Log Record.
        /// </summary>
        EndCheckpointLogRecord LastCompletedEndCheckpointRecord
        {
            get;
        }

        void ThrowReplicationException(
            LogicalLogRecord record, 
            Exception e);

        /// <summary>
        /// Flushes the underlying log.
        /// </summary>
        /// <param name="flushInitiator">String id of the flusher.</param>
        /// <returns></returns>
        Task FlushAsync(string flushInitiator);
    }
}